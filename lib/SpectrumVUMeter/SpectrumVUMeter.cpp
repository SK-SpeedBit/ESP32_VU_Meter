#include "SpectrumVUMeter.h"



SpectrumVUMeter::SpectrumVUMeter() {
    ;
}



SpectrumVUMeter::~SpectrumVUMeter() {
    if (screenBackupBuffer != nullptr) delete(screenBackupBuffer);
}



// Color mixing function (ALPHA/transparency effect)
uint16_t SpectrumVUMeter::mixColors(uint16_t baseColor, uint16_t bkgColor, float alfa) {
    if (alfa >= 1.0) return baseColor;
    if (alfa <= 0.0) return bkgColor;
    uint8_t r1 = (baseColor >> 11) & 0x1F;
    uint8_t g1 = (baseColor >>  5) & 0x3F;
    uint8_t b1 =  baseColor & 0x1F;
    uint8_t r2 = (bkgColor  >> 11) & 0x1F;
    uint8_t g2 = (bkgColor  >>  5) & 0x3F;
    uint8_t b2 =  bkgColor  & 0x1F;
    uint8_t r  = (r1 * alfa) + (r2 * (1.0 - alfa));
    uint8_t g  = (g1 * alfa) + (g2 * (1.0 - alfa));
    uint8_t b  = (b1 * alfa) + (b2 * (1.0 - alfa));
    return (r << 11) | (g << 5) | b;
}



void SpectrumVUMeter::calculateGeometry() {
    int availableWidth = DISPLAY_WIDTH - margin_left - margin_right;

    if (autoSizeWidth) {
        int totalGapsX = gap_X * (NUM_BANDS - 1);
        
        if (availableWidth >= totalGapsX + NUM_BANDS) {
            segWidth = (availableWidth - totalGapsX) / NUM_BANDS;
            
            int usedWidth = (segWidth * NUM_BANDS) + totalGapsX;
            int remainingPixels = availableWidth - usedWidth;
            
            // Centering with margins
            if (remainingPixels > 0 && autoCenterWidth) {
                margin_left  += remainingPixels / 2;
                margin_right += (remainingPixels + 1) / 2;
                availableWidth = DISPLAY_WIDTH - margin_left - margin_right; 
            }
        }
    } else {
        int totalGapsX = gap_X * (NUM_BANDS - 1);
        segWidth       = (availableWidth - totalGapsX) / NUM_BANDS;

        int usedWidth = (segWidth * NUM_BANDS) + totalGapsX;
        int remainingPixels = availableWidth - usedWidth;

        // Centering by margins for manual mode
        if (remainingPixels > 0 && autoCenterWidth) {
            margin_left  += remainingPixels / 2;
            margin_right += (remainingPixels + 1) / 2;
        }
    }

    // Vertical geometry
    float availableHeight = DISPLAY_HEIGHT - margin_top - margin_bottom;
    int   totalGapsY      = gap_Y * (segments_per_band - 1);
    segHeight = (availableHeight - (float)totalGapsY) / (float)segments_per_band;
}




void SpectrumVUMeter::init_adc_continuous() {

    // Stop other activity ADC
    if (adc_handle != nullptr) { 
        adc_continuous_stop(adc_handle); 
        adc_continuous_deinit(adc_handle);
        adc_handle = nullptr;
    }

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = BUFFER_SIZE * 4,
        .conv_frame_size    = BUFFER_SIZE,
    };
    adc_continuous_new_handle(&adc_config, &adc_handle);

    adc_continuous_config_t config = {
        .pattern_num        = 1,
        .sample_freq_hz     = SAMPLE_FREQ,
        .conv_mode          = ADC_CONV_SINGLE_UNIT_1,
        .format             = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };

    adc_digi_pattern_config_t adc_pattern = {
        .atten              = ADC_ATTEN_DB_12,
        .channel            = ADC_CHANNEL,
        .unit               = ADC_UNIT,
        .bit_width          = ADC_BITWIDTH_9,
    };
    config.adc_pattern = &adc_pattern;
    adc_continuous_config(adc_handle, &config);
    adc_continuous_start (adc_handle);

    adc_owner = ADC_OWNER_SPECTRUM;
}



void SpectrumVUMeter::begin(TFT_eSPI &tftRef) {
    tft = &tftRef;

    if (!tft_initialized) {
        tft->init();
        tft->setRotation(1); 
        tft->fillScreen(TFT_BLACK);
        tft_initialized = true;
    }

    tft->setRotation(1); // Horizontal
    tft->fillScreen(background_color);

    // Clear variables
    for (int i=0; i < NUM_BANDS; i++) { 
        bandPeaks[i].lastMaxSegment =   0; 
        bandPeaks[i].peakTime       =   0; 
        smoothedBands      [i]      = 0.0; 
        lastDrawnSegments  [i]      =   0;  
        lastMaxDrawnSegment[i]      =   0;  
    }
    lastDrawnVUSegments  =   0;
    max_pixel_width      =   0;
    currentVUPixelWidth  =   0;
    interpolatedVolume   = 0.0;
    lastDrawVUPixelWidth =   0;
    lastTftWriteTime     =   0;
    // Clear end.

    calculateGeometry();
    init_adc_continuous();

    // Draw the entire frame grid
    for (int band = 0; band < NUM_BANDS; band++) {
        int posX = margin_left + band * (segWidth + gap_X);
        for (int seg = 0; seg < segments_per_band; seg++) {
            int posY = DISPLAY_HEIGHT - margin_bottom - segHeight - (seg * (segHeight + gap_Y));
            if (drawtileFrame)
                tft->drawRect(posX, posY, segWidth, segHeight, frame_color);

            uint16_t color = base_color_normal;
            if ( showHighZoneTiles && (seg >= threshold_red) )         color = base_color_highZone;
            else 
              if ( showWarningZoneTiles && (seg >= threshold_yellow) ) color = base_color_warningZone;

            float alfa = 0.10; 
            uint16_t colorOff = mixColors(color, background_color, alfa);
            tft->fillRect(posX + drawtileFrame, posY + drawtileFrame, segWidth - 2 * drawtileFrame, segHeight - 2 * drawtileFrame, colorOff);

        }
    }

    if (drawDescriptions) {
        // Switch to vertical mode
        tft->setRotation(0); 
        tft->setTextColor( text_color, background_color);
        tft->setTextSize(1);
        
        // Set the alignment to RIGHT-CENTER
        tft->setTextDatum(MR_DATUM); 

        for (int band = 0; band < NUM_BANDS; band++) {
            // Calculate the physical center of the column
            int posY_rotated_0 = margin_left + band * (segWidth + gap_X) + (segWidth / 2);
            
            // Text breakpoint (physical bottom of bars minus 3 pixels of padding)
            int posX_rotated_0 = margin_bottom - 3;

            // Frequency calculation
            int start_bin  = bandCutoffTable[band];
            int end_bin    = bandCutoffTable[band + 1];
            int center_bin = start_bin + ((end_bin - start_bin) / 2);
            float freq_hz  = (float)center_bin * ((float)SAMPLE_FREQ / (float)SAMPLES);

            String s = bandLabels[band];
            
            if (descriptionsAsFreq) { //if you prefer precise frequencies as descriptions:
                if (freq_hz < 1000.0) { s = String((int)freq_hz); }
                else                  { s = String((float)freq_hz / 1000.0, 1) + "k"; }
            }

            // Draw vertical text
            tft->drawString(s, posX_rotated_0, posY_rotated_0);
        }

        // Restore horizontal rotation
        tft->setRotation(1);
    }

    // last loop time
    oldLoopTime = millis();
    enabled = true;
}



void SpectrumVUMeter::saveBackground() {
    if (screenBackupBuffer == nullptr) {
        screenBackupBuffer = (uint16_t*)   malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));
        
        if (screenBackupBuffer == nullptr) {
            tft->fillScreen(TFT_RED);
            tft->setTextSize(1);                               
            tft->setTextDatum(MC_DATUM);                       
            tft->setTextColor(TFT_WHITE, TFT_BLACK);
            tft->drawString("SpectrumVUMeter: Insufficient RAM for buffer!", DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2); 
            while(1); // Stop the program in case of problems
        }
    }

    if (screenBackupBuffer != nullptr) {
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                uint16_t xx =  tft->readPixel(x, y) ;
                screenBackupBuffer[y * DISPLAY_WIDTH + x] = (xx>>8) | (xx<<8);
            }
        }
    }
    //DEBUG_PRINTLN("A copy of the screen is saved in RAM.");
}




void SpectrumVUMeter::restoreBackground() {
    if (screenBackupBuffer != nullptr) {
        tft->pushImage(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, screenBackupBuffer);
    }
}



void SpectrumVUMeter::loop() {
    if (!enabled) return;
    if ((adc_owner != ADC_OWNER_SPECTRUM) || (adc_handle == nullptr)) return;

    uint8_t result[BUFFER_SIZE] = {0};
    uint32_t ret_num = 0;
    unsigned long nowTime = millis();

    if ( nowTime - oldLoopTime < 1000) Serial.printf(">SVUM loop time: %ld\n", nowTime - oldLoopTime);
    oldLoopTime = nowTime;


    // Read from DMA
    esp_err_t ret = adc_continuous_read(adc_handle, result, BUFFER_SIZE, &ret_num, 0);
    
    if (ret == ESP_OK && ret_num == BUFFER_SIZE) {
        int sample_idx = 0;
        for (int i = 0; i < ret_num; i += 2) {
            adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
            vReal[sample_idx] = (float)(p->type1.data) * 8; // ADC: 9bit: *8 / 10bit *4 / 11bit *2 / 12 bit * 1 
            vImag[sample_idx] = 0.0;
            sample_idx++;
        }

        FFT.dcRemoval();
        FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
        FFT.compute(FFTDirection::Forward);
        FFT.complexToMagnitude();

        // Band Logic and Ballistics
        for (int band = 0; band < NUM_BANDS; band++) {
            float band_energy = 0;
            int start_bin = bandCutoffTable[band];
            int end_bin   = bandCutoffTable[band + 1];
            int bins_in_band = end_bin - start_bin;

            // RMS
            for (int b = start_bin; b < end_bin; b++) { band_energy += vReal[b] * vReal[b]; }
            if (bins_in_band > 0) { band_energy = sqrt(band_energy / bins_in_band); }            

            float currentVU = band_energy / bandGainTable[band];

            // NOISE GATE:
            float noiseLevel = 0.1;
            if (currentVU < noiseLevel) currentVU = 0.0;
            else {
                // Rescale to make movement above a higher threshold smooth
                currentVU = (currentVU - noiseLevel) / (1.0 - noiseLevel);
            }

            if (currentVU > 1.0)  currentVU = 1.0;

            // smoothing...
            if (currentVU > smoothedBands[band]) {
                smoothedBands[band] = (smoothedBands[band] * 0.15) + (currentVU * 0.85); 
            } else {
                smoothedBands[band] = (smoothedBands[band] * 0.85) + (currentVU * 0.15); 
            }

            // Peak Hold
            if (showPeaks) {
                int activeSegments = smoothedBands[band] * segments_per_band;
                if (activeSegments >= bandPeaks[band].lastMaxSegment) {
                    bandPeaks[band].lastMaxSegment = activeSegments;
                    bandPeaks[band].peakTime = nowTime;
                } else {
                    if (nowTime - bandPeaks[band].peakTime > peaksHoldTime) { 
                        bandPeaks[band].lastMaxSegment--;        
                        if (bandPeaks[band].lastMaxSegment < 0) bandPeaks[band].lastMaxSegment = 0;
                        bandPeaks[band].peakTime = nowTime - peaksFallTime;  
                    }
                }
            }
        }
        
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        // --- VU BAR ---
        if (showLineVUMeter) {
            float overallVolume = 0;

            // CALCULATION OF SIGNAL ENERGY (RMS)
            float sumOfSquares = 0;
            for (int band = 0; band < NUM_BANDS; band++) {
                // Square it louder signals have more weight
                sumOfSquares += (smoothedBands[band] * smoothedBands[band]); 
            }

            // Take the average and the square root (classic RMS formula)
            float rmsEnergy = sqrt(sumOfSquares / (float)NUM_BANDS);

            // CALIBRATION MULTIPLIER: Analog meters are calibrated so that 
            // the average music signal dynamically reaches approximately 70-80% of the scale (0 dB)
            float analogKick = 1.6; // If the bar is too short, increase it to 1.8. If it's too long, decrease it to 1.4.
            overallVolume = rmsEnergy * analogKick;

            // Over 100% exit protection
            if (overallVolume > 1.0) overallVolume = 1.0;

            // Bar ballistics (fast jump, slow return)
            if (overallVolume > interpolatedVolume) {
                interpolatedVolume = (interpolatedVolume * 0.1) + (overallVolume * 0.9);
            } else {
                interpolatedVolume = (interpolatedVolume * 0.93) + (overallVolume * 0.07);
            }

            // Calculate the width in pixels
            if (lineVUWidthAsSpectrum) {
                lineVULeftMargin  = margin_left;
                lineVURightMargin = margin_right;
            }
            max_pixel_width = DISPLAY_WIDTH - lineVULeftMargin - lineVURightMargin;
            currentVUPixelWidth = interpolatedVolume * max_pixel_width;
            if (currentVUPixelWidth > max_pixel_width) currentVUPixelWidth = max_pixel_width;
            if (currentVUPixelWidth < 0) currentVUPixelWidth = 0;
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        // Draw results
        if (nowTime - lastTftWriteTime >= MIN_REDRAW_TIME) {

            tft->startWrite();

            // --- DRAWING SPECTRUM  ---            
            for (int band = 0; band < NUM_BANDS; band++) {
                int activeSegments = smoothedBands[band] * segments_per_band;
                int peakSegment = bandPeaks[band].lastMaxSegment;
                int posX = margin_left + band * (segWidth + gap_X);

                // Check if anything has changed in this bar since the last frame.
                // If the audio and Peak bar states are identical -> SKIP the entire bar!
                if (activeSegments == lastDrawnSegments[band] && peakSegment == lastMaxDrawnSegment[band]) {
                    continue; 
                }

                for (int seg = 0; seg < segments_per_band; seg++) {
                    // DETERMINING IF THIS BRICK NEEDS TO BE REFRESHED
                    // Checking if the status of this specific brick has changed
                    bool wasActive = (seg < lastDrawnSegments[band]) || (seg == (lastMaxDrawnSegment[band] - 1) && lastMaxDrawnSegment[band] > 0);
                    bool isActive  = (seg < activeSegments) || (seg == (peakSegment - 1) && peakSegment > 0);
                    
                    // If the state of the brick (its brightness) has not changed, do not waste time sending via SPI!
                    if (wasActive == isActive && (lastDrawnSegments[band] != 0) && (seg == (peakSegment - 1) && peakSegment > 0) ) { // added && (seg == (peakSegment - 1) && peakSegment > 0)
                        // Exception: If the background/active transition point has moved, is needed to redraw the boundary
                        if ((seg < activeSegments && seg < lastDrawnSegments[band]) || (seg > activeSegments && seg > lastDrawnSegments[band] && seg != (peakSegment-1) && seg != (lastMaxDrawnSegment[band]-1))) {
                            continue;
                        }
                    }

                    int posY = DISPLAY_HEIGHT - margin_bottom - segHeight - (seg * (segHeight + gap_Y));

                    uint16_t baseColor = base_color_normal;
                    if      ( showHighZoneTiles    && (seg >= threshold_red   ) ) baseColor = base_color_highZone;
                    else if ( showWarningZoneTiles && (seg >= threshold_yellow) ) baseColor = base_color_warningZone;

                    float alfa = 0.10; 
                    if (seg < activeSegments) { alfa = 1.0;  } 
                    else 
                        if (seg == (peakSegment - 1) && peakSegment > 0) { alfa = 0.9; }

                    uint16_t finalColor = mixColors(baseColor, background_color, alfa);
                    if (peaksHeight > segHeight) peaksHeight = segHeight - (2 * drawtileFrame);
                    if (peaksHeight < 1 ) peaksHeight = 1;
                    // Draw a rectangle one frame smaller so as not to erase the frame
                    if (seg == (peakSegment - 1) && peakSegment > 0) { //  peak tile
                        tft->fillRect(posX + drawtileFrame, 
                                        posY + drawtileFrame + segHeight - peaksHeight - (2 * drawtileFrame), 
                                        segWidth - (2 * drawtileFrame), 
                                        peaksHeight, 
                                        finalColor);
                    }
                    else {
                        tft->fillRect(posX + drawtileFrame, 
                                        posY + drawtileFrame, 
                                        segWidth  - (2 * drawtileFrame), 
                                        segHeight - (2 * drawtileFrame), 
                                        finalColor);
                    }

                }

                // Save the current state as historical for the next frame
                lastDrawnSegments  [band] = activeSegments;
                lastMaxDrawnSegment[band] = peakSegment;
            }
                
            //////////////////////////////////////////////////////////////////////////////////////////////////////////

            // --- DRAWING VU BAR  ---
            if (!lineVUMeterAsBricks) { // linear...
                if (showLineVUMeter && currentVUPixelWidth != lastDrawVUPixelWidth) {
                    
                    int warning_pixel = max_pixel_width * lineVUWarningZoneLevel;
                    int high_pixel    = max_pixel_width * lineVUHighZoneLevel;

                    // Situation A: The bar has grown -> only draw a new fragment to the right
                    if (currentVUPixelWidth > lastDrawVUPixelWidth) {
                        for (int x = lastDrawVUPixelWidth; x < currentVUPixelWidth; x++) {
                            uint16_t pixelColor = lineVUColor;
                            if      ( lineShowHighZoneTiles    && (x >= high_pixel   ) ) pixelColor = lineVUColorHighZone;
                            else if ( lineShowWarningZoneTiles && (x >= warning_pixel) ) pixelColor = lineVUColorWarningZone;
                            
                            // Draw a vertical line with the thickness of lineVUWidth for a given X
                            tft->drawFastVLine(lineVULeftMargin + x, lineVUPos_Y, lineVUWidth, pixelColor);
                        }
                    } 
                    // Situation B: The bar has become smaller -> only erase the missing fragment on the right side with the background
                    else if (currentVUPixelWidth < lastDrawVUPixelWidth) {
                        int eraseFrom = lineVULeftMargin + currentVUPixelWidth;
                        int erasingWidth = lastDrawVUPixelWidth - currentVUPixelWidth;
                        tft->fillRect(eraseFrom, lineVUPos_Y, erasingWidth, lineVUWidth, background_color);
                    }

                    // Remember the state for the next frame
                    lastDrawVUPixelWidth = currentVUPixelWidth;
                }
            }
            else { // brics...
                //--- DRAWING A VU BAR (brics) ---
                if (showLineVUMeter) {
                    int activeVUSegments = interpolatedVolume * lineVU_segments;
                    if (activeVUSegments > lineVU_segments) activeVUSegments = lineVU_segments;
                    if (activeVUSegments < 0)           activeVUSegments = 0;

                    int total_vu_width = DISPLAY_WIDTH - lineVULeftMargin - lineVURightMargin;
                    int vu_gap_X = 2; 
                    int vu_segWidth = (total_vu_width - (vu_gap_X * (lineVU_segments - 1))) / lineVU_segments;
                    float lineVUPos_Y = (float)DISPLAY_HEIGHT - (float)margin_bottom + ((float)lineVUWidth / 2.0); 

                    if (activeVUSegments != lastDrawnVUSegments) {
                        
                        for (int seg = 0; seg < lineVU_segments; seg++) {
                            
                            bool wasActive = (seg < lastDrawnVUSegments);
                            bool isActive  = (seg < activeVUSegments);
                            
                            if (wasActive == isActive && lastDrawnVUSegments != 0) {
                                continue;
                            }

                            int posX = lineVULeftMargin + seg * (vu_segWidth + vu_gap_X);
                            uint16_t finalColor;

                            if (seg < activeVUSegments) {
                                // ACTIVE BLOCK - system colors without any conversion
                                if      (lineShowHighZoneTiles    && (seg >= lineVU_threshold_red   ) ) finalColor = lineVUColorHighZone;    // TFT_RED
                                else if (lineShowWarningZoneTiles && (seg >= lineVU_threshold_yellow) ) finalColor = lineVUColorWarningZone; // TFT_YELLOW
                                else                                 finalColor = lineVUColor;            
                            } else {
                                // INACTIVE BLOCK - already set color or pure background color (complete blanking) 
                                finalColor = lineVUInactiveBricksColor;
                            }
                            
                            // Drawing a horizontal block
                            tft->fillRect(posX, lineVUPos_Y, vu_segWidth, lineVUWidth, finalColor);
                        }

                        lastDrawnVUSegments = activeVUSegments;
                    }
                }
            }
        

            //////////////////////////////////////////////////////////////////////////////////////////////////////////
            
            tft->endWrite();
            lastTftWriteTime = nowTime;

        }
    }
}





void SpectrumVUMeter::drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor)  { drawString(string, poX, poY, txtColor, TFT_TRANSPARENT); }
void SpectrumVUMeter::drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor, uint16_t bkgColor) {
    tft->loadFont(text_Font);           
    tft->setTextColor(txtColor, bkgColor);
    tft->drawString(string, poX, poY);
    tft->unloadFont(); // Free up RAM after drawing the shield background
}



void SpectrumVUMeter::setVUTextFont(uint8_t size) {
    switch (size) {
        case  8: { text_Font = Final_Frontier8 ; break; }
        case  9: { text_Font = Final_Frontier9 ; break; }
        case 10: { text_Font = Final_Frontier10; break; }
        case 11: { text_Font = Final_Frontier11; break; }
        case 12: { text_Font = Final_Frontier12; break; }
        case 13: { text_Font = Final_Frontier13; break; }
        case 14: { text_Font = Final_Frontier14; break; }
        case 15: { text_Font = Final_Frontier15; break; }
        case 16: { text_Font = Final_Frontier16; break; }
        default: text_Font = Final_Frontier10;
    }
}



// empty string - top margin empty, otherwise it prints the text in the set font size
void SpectrumVUMeter::drawTopMarginText(String text, uint8_t size, uint16_t color) {
    if (text.length() > 0) {
        tft->setTextDatum(MC_DATUM);
        setVUTextFont(size);
        drawString(text.c_str(), DISPLAY_WIDTH/2, margin_top/2, color); 
    }
}






