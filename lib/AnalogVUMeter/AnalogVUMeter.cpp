#include <pgmspace.h>
#include <algorithm>

#include "AnalogVUMeter.h"
#include "image\background_Image.h"

#include "..\..\include\config.h"


AnalogVUMeter::AnalogVUMeter()
    : enabled(true)
{
   
}

AnalogVUMeter::~AnalogVUMeter() {
    if (imgBuffer != nullptr) delete(imgBuffer);
}



void AnalogVUMeter::init_adc_continuous() {

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

    adc_owner = ADC_OWNER_ANALOG;

}



void AnalogVUMeter::begin(TFT_eSPI &tftRef) {
    tft = &tftRef;

    if (!tft_initialized) {
        tft->init();
        tft->setRotation(1); 
        tft->fillScreen(TFT_BLACK);
        tft_initialized = true;
    
        imgBuffer = new TFT_eSprite(tft);
    }


    // Clear variables
    for (int i=0; i < MAX_TILES; i++) { 
        oldTileX[i] = 0;
        oldTileY[i] = 0;
    }
    oldTileCount     =     0;
    smoothProgress   =  0.0f; 
    lastNeedleAngle  = -1.0f; 
    for (int i=0; i < TILE_SIZE * TILE_SIZE; i++) { tileRamBuffer[i] =     0; }
    for (int i=0; i < NUM_BANDS; i++)             { smoothedBands[i] = {0.0}; }
    // Clear end.
    
    init_adc_continuous();

    // last loop time
    oldLoopTime = millis();
}


void AnalogVUMeter::saveBackground() {
    for (int i = 0; i < MAX_TILES; i++) {
      oldTileX[i] = 0x0;
      oldTileY[i] = 0x0;
    }
    oldTileCount = 0;   // Indicate that on the first move there is nothing to write yet

    //DEBUG_PRINTLN("Memory allocation for screen shadow...");
    screenBackupBuffer = (uint16_t*)malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t));

    if (screenBackupBuffer == nullptr) {
        tft->fillScreen(TFT_RED);
        //DEBUG_PRINTLN("ERROR: Insufficient RAM for buffer!");
        while(1); // Stop the program in case of problems
    }
    else {
        //DEBUG_PRINTLN("Screen Mirroring (Screen Reading)...");
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                uint16_t xx =  tft->readPixel(x, y) ;
            screenBackupBuffer[y * DISPLAY_WIDTH + x] = (xx>>8) | (xx<<8);
            }
        }
    }
    //DEBUG_PRINTLN("A copy of the screen is saved in RAM.");
}



void AnalogVUMeter::restoreBackground() {
    if (screenBackupBuffer != nullptr) {
        //DEBUG_PRINTLN("Screen Mirroring Restore (Screen Writing)...");
        for (int y = 0; y < DISPLAY_HEIGHT; y++) {
            for (int x = 0; x < DISPLAY_WIDTH; x++) {
                uint16_t xx =  screenBackupBuffer[y * DISPLAY_WIDTH + x];
                tft->drawPixel(x, y, (xx>>8) | (xx<<8));
            }
        }
    }
}



void AnalogVUMeter::fillScreen()                 { tft->fillScreen(backgroundColor); }
void AnalogVUMeter::fillScreen(uint16_t bgColor) { tft->fillScreen(bgColor); }



void AnalogVUMeter::drawBackground()                                   { drawBackground(backgroundColor, foregroundColor, backgroundDither); }
void AnalogVUMeter::drawBackground(uint16_t bgColor)                   { drawBackground(bgColor, foregroundColor, backgroundDither); }
void AnalogVUMeter::drawBackground(uint16_t bgColor, uint16_t fgColor) { drawBackground(bgColor, fgColor, backgroundDither); }
void AnalogVUMeter::drawBackground(uint16_t bgColor, uint16_t fgColor, uint8_t dither) {
    backgroundColor  = bgColor;
    foregroundColor  = fgColor; 
    backgroundDither =  dither;
    
    imgBuffer->setColorDepth(16);
    imgBuffer->createSprite(DISPLAY_WIDTH, 1); 
    int pixelIndex = 0;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            
            // Read a 16-bit pixel (RGB565) from an array
            uint8_t byte1 = pgm_read_byte(&background_Image[pixelIndex++]);
            uint8_t byte2 = pgm_read_byte(&background_Image[pixelIndex++]);
            uint16_t imageColor = (byte1 << 8) | byte2; 

            // Accurate breakdown of the original gray components (scale 0-255)
            uint8_t imgR = ((imageColor >> 11) & 0x1F) * 255 / 31;
            uint8_t imgG = ((imageColor >> 5)  & 0x3F) * 255 / 63;
            uint8_t imgB = ( imageColor        & 0x1F) * 255 / 31;

            // We are looking for the highest brightness (prevents black areas)
            uint8_t brightness = max(imgR, max(imgG, imgB));

            // Negative inversion
            // White background (255) gives alpha = 0 (i.e. it will let the background in 100%)
            // Black background (0) will give alpha = 255 (i.e. draw a pure black background)
            uint8_t alpha = 255 - brightness;

            // Call a library function with built-in micro-dithering (default value 4)
            // This function will break up the stripes in darker areas on the fly
            uint16_t finalColor = tft->alphaBlend(alpha, fgColor, bgColor, dither);

            // Save the finished, smoothed pixel in the line buffer
            imgBuffer->drawPixel(x, 0, finalColor);
        }
        // Push the line to the screen 
        imgBuffer->pushSprite(0, y);
    }

}



void AnalogVUMeter::drawRotateCircle() {
    // circle at the point of rotation of the needle axis
    if ( needleRotateCircle && ( ((float)needle_PosAxisY + (((float)needle_PosAxisY-(float)hideNeedleBelowY)/2) ) < DISPLAY_HEIGHT ) ) {
      tft->fillCircle((float)scale_PosAxisX, (float)needle_PosAxisY, (((float)needle_PosAxisY-(float)hideNeedleBelowY)/2) + 6, 0x18c3);
      tft->fillCircle((float)scale_PosAxisX, (float)needle_PosAxisY, (((float)needle_PosAxisY-(float)hideNeedleBelowY)/2) + 2, 0x2945);
    }

}



void AnalogVUMeter::calculateSafeAnglesPx(int scale_PosAxisY, int needle_PosAxisY, int scale_hLevel, int scale_MajorLen, int scale_MarginPx, int &scale_AngleStart, int &scale_AngleStop) {
    float posX = (float)scale_PosAxisX; 
    float limitLeft   = ((float)0.0) + (float)scale_MarginPx + 10.0f; // Additional small buffer for the text font width
    float limitRight  = (float)DISPLAY_WIDTH - ((float)scale_MarginPx + 10.0f);
    float limitBottom = (float)DISPLAY_HEIGHT - (float)scale_MarginPx;

    float deltaY = (float)scale_PosAxisY - (float)needle_PosAxisY; 
    float straightLineY = (float)scale_PosAxisY - (float)scale_hLevel;

    float testAngle = 150.0f; 
    float finalSafeStart = 150.0f;

    while (testAngle >= 90.0f) {
        float rad = (testAngle - 270.0f) * PI / 180.0f;
        float cos_a = cos(rad);
        float sin_a = sin(rad);
        
        float current_x = 0.0f;
        float current_y = 0.0f;

        if (scale_linear && scale_linearTicks) {
            // Test the X/Y position exactly where the text will land above the flat line
            float yText = straightLineY - (float)scale_MajorLen - 12.0f;
            if (abs(sin_a) > 0.001f) {
                current_x = (float)scale_PosAxisX + (yText - (float)needle_PosAxisY) * (cos_a / sin_a);
            } else {
                current_x = (float)scale_PosAxisX;
            }
            current_y = yText;
        } else {
            // Test for the classic scale arc
            float b = deltaY * sin_a;
            float R_needle = b + sqrt(b * b + ((float)scale_hLevel * (float)scale_hLevel) - (deltaY * deltaY));
            float r_total = R_needle + (float)scale_MajorLen + 15.0f;
            current_x = (float)scale_PosAxisX  + (r_total * cos_a);
            current_y = (float)needle_PosAxisY + (r_total * sin_a);
        }

        // Check for left, right or bottom border violations, taking into account the margin
        if (current_x <= limitLeft || current_x >= limitRight || current_y >= limitBottom) {
            finalSafeStart = testAngle;// + 1.0f; 
            break;
        }
        testAngle -= 0.5f;
    }

    scale_AngleStart = (int)ceil(finalSafeStart);
    scale_AngleStop = 180 + (180 - scale_AngleStart);
}



void AnalogVUMeter::drawScale() {

    calculateSafeAnglesPx(scale_PosAxisY, needle_PosAxisY, scale_hLevel, scale_MajorLen, scale_MarginPx, scale_AngleStart, scale_AngleStop);
    
    float deltaY_base   = (float)scale_PosAxisY - (float)needle_PosAxisY;
    float straightLineY = (float)scale_PosAxisY - (float)scale_hLevel;
    
    float edgeX0, edgeY0;
    float radStart = ((float)scale_AngleStart - 270.0f) * PI / 180.0f;

    if (scale_linear) {
        edgeY0 = straightLineY;
        if (abs(sin(radStart)) > 0.001f) {
            edgeX0 = (float)scale_PosAxisX + (edgeY0 - (float)needle_PosAxisY) * (cos(radStart) / sin(radStart));
        } else {
            edgeX0 = (float)scale_PosAxisX;
        }
    } else {
        float R_start = (deltaY_base * sin(radStart)) + sqrt((deltaY_base * sin(radStart)) * (deltaY_base * sin(radStart)) + ((float)scale_hLevel * (float)scale_hLevel) - (deltaY_base * deltaY_base));
        edgeX0 = (float)scale_PosAxisX + (R_start * cos(radStart));
        edgeY0 = (float)needle_PosAxisY + (R_start * sin(radStart));
    }
    
    float arcStartAngle = (int)(atan2(edgeY0 - (float)scale_PosAxisY, edgeX0 - (float)scale_PosAxisX) * 180.0f / PI) - 90;
    if (arcStartAngle < 0) arcStartAngle += 360;
    float arcStopAngle  = 180 + (180 - arcStartAngle);

    // for arc base...
    // Subtract from the start and add to the end (angles in drawArc increase clockwise)
    float arcStartAngleExtended = arcStartAngle - scale_ArcExtensionDeg;
    float arcStopAngleExtended  = arcStopAngle  + scale_ArcExtensionDeg;

    // for linear base...
    // Extending a simple base beyond the extreme points of the markers
    float edgeX1 = (float)scale_PosAxisX + ((float)scale_PosAxisX - edgeX0);
    float extendedX0 = edgeX0 - (float)scale_LineExtensionPx;
    float extendedX1 = edgeX1 + (float)scale_LineExtensionPx;
    

    tft->setTextSize(1);                               
    tft->setTextDatum(MC_DATUM);                       
    tft->loadFont(scale_Font);                    

    bool  colorChanged = false;
    float arcZeroAngle = 0; 
    float zeroX = 0; 

    uint32_t current_scale_color = scale_Color;

    for (int val = scale_ValMin; val <= scale_ValMax; val += scale_MinorStep) {
    
        float progress = (float)(val - scale_ValMin) / (float)(scale_ValMax - scale_ValMin);
        float alfa = (float)scale_AngleStart + (progress * (float)(scale_AngleStop - scale_AngleStart));
        float rad = (alfa - 270.0f) * PI / 180.0f; 
        
        float cos_a = cos(rad);
        float sin_a = sin(rad);
        
        float x0_f, y0_f;
        float R_needle = 0.0f;

        if (scale_linear) {
            y0_f = straightLineY;
            if (abs(sin_a) > 0.001f) {
                x0_f = (float)scale_PosAxisX + (y0_f - (float)needle_PosAxisY) * (cos_a / sin_a);
            } else {
                x0_f = (float)scale_PosAxisX;
            }
            float dx = x0_f - (float)scale_PosAxisX;
            float dy = y0_f - (float)needle_PosAxisY;
            R_needle = sqrt(dx * dx + dy * dy);
        } else {
            R_needle = (deltaY_base * sin_a) + sqrt((deltaY_base * sin_a) * (deltaY_base * sin_a) + ((float)scale_hLevel * (float)scale_hLevel) - (deltaY_base * deltaY_base));
            x0_f = (float)scale_PosAxisX  + (R_needle * cos_a);
            y0_f = (float)needle_PosAxisY + (R_needle * sin_a);
        }
        
        float x1_f, y1_f;
        float currentWidth;
        
        bool isMajor = ( ((val % scale_MajorStep) == 0) || ( val == scale_ValMax ) );
        if ( (val > 0) && (val % 2 == 0) ) isMajor = true;

        if (isMajor) {
            currentWidth = (float)scale_MajorWidth;
            
            // Flattening vertices for long tags
            if (scale_linear && scale_linearTicks) {
                y1_f = straightLineY - (float)scale_MajorLen; // The top point lies on a perfect horizontal line
                if (abs(sin_a) > 0.001f) {
                    x1_f = (float)scale_PosAxisX + (y1_f - (float)needle_PosAxisY) * (cos_a / sin_a);
                } else {
                    x1_f = (float)scale_PosAxisX;
                }
            } else {
                // Classic variant (constant arc length)
                float R_end = R_needle + (float)scale_MajorLen;
                x1_f = (float)scale_PosAxisX  + (R_end * cos_a);
                y1_f = (float)needle_PosAxisY + (R_end * sin_a);
            }

            // Description text positioning
            String textToDisplay = String(val); 
            float xText, yText;
            
            if (scale_linear && scale_linearTicks) {
                // For flat tops, the text should also run in a perfect horizontal line above the markers
                yText = straightLineY - (float)scale_MajorLen - 12.0f;
                if (abs(sin_a) > 0.001f) {
                    xText = (float)scale_PosAxisX + (yText - (float)needle_PosAxisY) * (cos_a / sin_a);
                } else {
                    xText = (float)scale_PosAxisX;
                }
            } else {
                float textDist = (scale_linear ? sqrt((x1_f - scale_PosAxisX)*(x1_f - scale_PosAxisX) + (y1_f - needle_PosAxisY)*(y1_f - needle_PosAxisY)) : R_needle + (float)scale_MajorLen) + 12.0f;
                xText = (float)scale_PosAxisX  + (textDist * cos_a);
                yText = (float)needle_PosAxisY + (textDist * sin_a);
            }

            tft->setTextColor(current_scale_color, TFT_TRANSPARENT);
            //tft->drawString(textToDisplay, xText, yText);
            drawString(textToDisplay.c_str(), xText, yText, scale_TextColor, scale_TextBkgColor);
        }
        else {
            currentWidth = (float)scale_MinorWidth;
            
            // Flattening vertices for short tags
            if (scale_linear && scale_linearTicks) {
                y1_f = straightLineY - (float)scale_MinorLen;
                if (abs(sin_a) > 0.001f) {
                    x1_f = (float)scale_PosAxisX + (y1_f - (float)needle_PosAxisY) * (cos_a / sin_a);
                } else {
                    x1_f = (float)scale_PosAxisX;
                }
            } else {
                float R_end = R_needle + (float)scale_MinorLen;
                x1_f = (float)scale_PosAxisX  + (R_end * cos_a);
                y1_f = (float)needle_PosAxisY + (R_end * sin_a);
            }
        }

        tft->drawWideLine(x0_f, y0_f, x1_f, y1_f, currentWidth, current_scale_color, TFT_TRANSPARENT);  
        
        if (scale_HighLevelZone && (!colorChanged) && (val >= 0)) {
            current_scale_color = scale_ColorHLevel;          
            tft->setTextColor(current_scale_color, TFT_TRANSPARENT);
            if (scale_linear) {
                zeroX = x0_f; 
            } else {
                arcZeroAngle = (int)(atan2(y0_f - (float)scale_PosAxisY, x0_f - (float)scale_PosAxisX) * 180.0f / PI) - 90;
                if (arcZeroAngle < 0) arcZeroAngle += 360;
            }
            colorChanged = true;
        }
    }

    if (scale_DrawBaseLine) {

        if (scale_linear) {
            // Draw the baseline          
            tft->drawWideLine(extendedX0, straightLineY, extendedX1, straightLineY, (float)scale_BaseArcWidth, scale_Color, TFT_TRANSPARENT);        
        }
        else {
            //tft->drawArc(scale_PosAxisX, scale_PosAxisY, scale_hLevel, scale_hLevel - scale_BaseArcWidth, arcStartAngleExtended, arcStopAngleExtended, scale_BaseLineColor, TFT_TRANSPARENT, true);        
            tft->drawSmoothArc(scale_PosAxisX, scale_PosAxisY, scale_hLevel, scale_hLevel - scale_BaseArcWidth, arcStartAngleExtended, arcStopAngleExtended, scale_BaseLineColor, TFT_TRANSPARENT, true);        
        }
        // HighLevel Coloring
        if (scale_HighLevelZone && colorChanged) {
            if (scale_linear) {
                //float edgeX1 = (float)scale_PosAxisX + ((float)scale_PosAxisX - edgeX0);
                tft->drawWideLine(zeroX, straightLineY, extendedX1, straightLineY, (float)scale_BaseArcWidth, current_scale_color, TFT_TRANSPARENT);
            } else {
                //tft->drawArc(scale_PosAxisX, scale_PosAxisY, scale_hLevel, scale_hLevel - scale_BaseArcWidth, arcZeroAngle, arcStopAngleExtended, current_scale_color, TFT_TRANSPARENT, true);
                tft->drawSmoothArc(scale_PosAxisX, scale_PosAxisY, scale_hLevel, scale_hLevel - scale_BaseArcWidth, arcZeroAngle, arcStopAngleExtended, current_scale_color, TFT_TRANSPARENT, true);
            }
        }
    }

    tft->unloadFont(); 
}



void AnalogVUMeter::drawNeedle(float targetProgress) {
    if (!enabled) return;

    // LIMITING AND SMOOTHING PROGRESS
    if (targetProgress < 0.0f) targetProgress = 0.0f;
    if (targetProgress > 1.0f) targetProgress = 1.0f;

    smoothProgress += (targetProgress - smoothProgress) * needle_Smooth; 

    if (targetProgress == 0.0f && smoothProgress < 0.01f) smoothProgress = 0.0f;
    if (targetProgress == 1.0f && smoothProgress > 0.99f) smoothProgress = 1.0f;

    float currentAngle = (float)scale_AngleStart + (smoothProgress * (float)(scale_AngleStop - scale_AngleStart));


    // Protection against unnecessary redrawing (deadband)
    if (targetProgress > 0.00f && targetProgress < 1.00f) {
        if ( (lastNeedleAngle >= 0.0f) && (abs(currentAngle - lastNeedleAngle) < needle_Smooth) ) { return; }
    } else {
        if ( (lastNeedleAngle >= 0.0f) && (abs(currentAngle - lastNeedleAngle) < 0.5f) ) { return; }
    }

    // Protection against unnecessary redrawing (deadband)
    //if ( (lastNeedleAngle >= 0.0f) && (abs(currentAngle - lastNeedleAngle) < 0.5f) ) { return; }

    float rad = (currentAngle - 270.0f) * PI / 180.0f;
    float cos_new = cosf(rad);
    float sin_new = sinf(rad);

    float deltaY = (float)scale_PosAxisY - (float)needle_PosAxisY;
    float R_arc = 0.0f;

    // NEEDLE LENGTH CALCULATION LOGIC(R_arc)
    // Situation A: Dynamic length change disabled (FIXED NEEDLE LENGTH)
    if (!needle_VarLength) {
        // Calculate a constant radius for the center of the scale (0.5 progress)
        float midAngle = (float)scale_AngleStart + (0.5f * (float)(scale_AngleStop - scale_AngleStart));
        float midRad = (midAngle - 270.0f) * PI / 180.0f;
        float b_mid = deltaY * sinf(midRad);
        R_arc = b_mid + sqrtf(b_mid * b_mid + ((float)scale_hLevel * (float)scale_hLevel) - (deltaY * deltaY));
    } 
    // Situation B: Linear scale (scale_linear = true) + needle_VarLength = true
    else if (scale_linear) {
        float straightLineY = (float)scale_PosAxisY - (float)scale_hLevel;
        float y0_f = straightLineY;
        float x0_f = (float)scale_PosAxisX;

        // Find a point on the scale base line for a given angle
        if (fabsf(sin_new) > 0.001f) {
            x0_f = (float)scale_PosAxisX + (y0_f - (float)needle_PosAxisY) * (cos_new / sin_new);
        }
        float dx0 = x0_f - (float)scale_PosAxisX;
        float dy0 = y0_f - (float)needle_PosAxisY;
        float R_needle_base = sqrtf(dx0 * dx0 + dy0 * dy0);

        if (scale_linearTicks) {
            // Flat tops (cut to a perfect horizontal line)
            float y1_f = straightLineY - (float)scale_MajorLen; 
            float x1_f = (float)scale_PosAxisX;
            if (fabsf(sin_new) > 0.001f) {
                x1_f = (float)scale_PosAxisX + (y1_f - (float)needle_PosAxisY) * (cos_new / sin_new);
            }
            float dx1 = x1_f - (float)scale_PosAxisX;
            float dy1 = y1_f - (float)needle_PosAxisY;
            
            // Length to the horizontal line of vertices
            R_arc = sqrtf(dx1 * dx1 + dy1 * dy1) - (float)scale_MajorLen;
        } else {
            // The base is straight, but the vertices form an arc
            R_arc = R_needle_base;
        }
    } 
    // Situation C: Classic arc scale (scale_linear = false) + needle_VarLength = true
    else {
        float b = deltaY * sin_new;
        R_arc = b + sqrtf(b * b + ((float)scale_hLevel * (float)scale_hLevel) - (deltaY * deltaY));
    }

    // DETERMINE DRAWING RANGE (rStart -> rEnd)
    float edgeSnapCorrection = (smoothProgress == 0.0f || smoothProgress == 1.0f) ? 0.5f : 0.0f;
    float rEnd = R_arc + (float)needle_AboveScale + edgeSnapCorrection;

    // Determine rStart for the bottom mask (hideNeedleBelowY)
    float rStart = 0.0f;
    if (fabsf(sin_new) > 0.01f) {
        rStart = ((float)hideNeedleBelowY - (float)needle_PosAxisY) / sin_new;
    } else {
        rStart = (float)needle_PosAxisY - (float)hideNeedleBelowY;
    }
    float minAllowedR = (float)needle_PosAxisY - (float)hideNeedleBelowY;
    if (rStart < minAllowedR) rStart = minAllowedR;
    if (rStart < 0.0f) rStart = 0.0f;

    // STEP A: ERASING THE OLD NEEDLE (TILES)
    if (oldTileCount > 0 && screenBackupBuffer != nullptr) {
        tft->startWrite(); 
        for (int i = 0; i < oldTileCount; i++) {
            int32_t tx = oldTileX[i]; 
            int32_t ty = oldTileY[i];
            if (tx >= 0 && (tx + TILE_SIZE) <= DISPLAY_WIDTH && ty >= 0 && (ty + TILE_SIZE) <= DISPLAY_HEIGHT) {
                for (int32_t sy = 0; sy < TILE_SIZE; sy++) {
                    memcpy(&tileRamBuffer[sy * TILE_SIZE], &screenBackupBuffer[(ty + sy) * DISPLAY_WIDTH + tx], TILE_SIZE * sizeof(uint16_t));
                }
                tft->pushImage(tx, ty, TILE_SIZE, TILE_SIZE, tileRamBuffer);
            }
        }
        tft->endWrite();
    }

    // STEP B: GENERATING NEW TILES FOR THE CORRECTION DATABASE
    int32_t newTileCount = 0;
    const float step = (float)(TILE_SIZE - 8); 

    tft->startWrite();
    for (float r = rStart; r <= rEnd + tileMargin; r += step) {
        int32_t needleX = scale_PosAxisX + (int32_t)roundf(r * cos_new);
        int32_t needleY = needle_PosAxisY + (int32_t)roundf(r * sin_new);

        if (needleY > hideNeedleBelowY + tileMargin) continue; 

        int32_t tx = needleX - (TILE_SIZE / 2);
        int32_t ty = needleY - (TILE_SIZE / 2);

        if (tx < 0) tx = 0; 
        if (ty < 0) ty = 0;
        if (tx + TILE_SIZE > DISPLAY_WIDTH)  tx = DISPLAY_WIDTH - TILE_SIZE;
        if (ty + TILE_SIZE > DISPLAY_HEIGHT) ty = DISPLAY_HEIGHT - TILE_SIZE;

        if (newTileCount < MAX_TILES) {
            oldTileX[newTileCount] = tx;
            oldTileY[newTileCount] = ty;
            newTileCount++;
        }

        if (screenBackupBuffer != nullptr && tx >= 0 && (tx + TILE_SIZE) <= DISPLAY_WIDTH && ty >= 0 && (ty + TILE_SIZE) <= DISPLAY_HEIGHT) {
            for (int32_t sy = 0; sy < TILE_SIZE; sy++) {
                memcpy(&tileRamBuffer[sy * TILE_SIZE], &screenBackupBuffer[(ty + sy) * DISPLAY_WIDTH + tx], TILE_SIZE * sizeof(uint16_t));
            }
            tft->pushImage(tx, ty, TILE_SIZE, TILE_SIZE, tileRamBuffer);
        }
    }
    tft->endWrite();

    // STEP C: DRAWING A NEW NEEDLE
    float raw_x0 = (float)scale_PosAxisX  + (rStart * cos_new);
    float raw_y0 = (float)needle_PosAxisY + (rStart * sin_new);
    float raw_x1 = (float)scale_PosAxisX  + (rEnd * cos_new);
    float raw_y1 = (float)needle_PosAxisY + (rEnd * sin_new);

    // Cutting the lower part of the edge to the Y cover
    if (raw_y0 > (float)hideNeedleBelowY) raw_y0 = (float)hideNeedleBelowY;
    if (raw_y1 > (float)hideNeedleBelowY) raw_y1 = (float)hideNeedleBelowY;

    // Crop to screen border (0 ... DISPLAY_WIDTH - 1)
    float maxW = (float)(DISPLAY_WIDTH - 1);
    if (raw_x0 < 0.0f) raw_x0 = 0.0f; if (raw_x0 > maxW) raw_x0 = maxW;
    if (raw_x1 < 0.0f) raw_x1 = 0.0f; if (raw_x1 > maxW) raw_x1 = maxW;

    if (raw_y0 < (float)hideNeedleBelowY || raw_y1 < (float)hideNeedleBelowY) {
        tft->drawWideLine(raw_x0, raw_y0, raw_x1, raw_y1, (float)needle_Width, needle_Color, TFT_TRANSPARENT);
    }

    oldTileCount    = newTileCount;
    lastNeedleAngle = currentAngle;    
}


void AnalogVUMeter::drawString(const char *string, int32_t poX, int32_t poY)                     { drawString(string, poX, poY, scale_TextColor, TFT_TRANSPARENT); }
void AnalogVUMeter::drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor)  { drawString(string, poX, poY, txtColor, TFT_TRANSPARENT); }
void AnalogVUMeter::drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor, uint16_t bkgColor) {
    tft->loadFont(text_Font);           
    tft->setTextColor(txtColor, bkgColor);
    tft->drawString(string, poX, poY);
    tft->unloadFont(); // Free up RAM after drawing the shield background
}


void AnalogVUMeter::redraw() {
    drawBackground();
    drawScale();
}



void AnalogVUMeter::setRange(int minValue, int maxValue)
{
    scale_ValMin = minValue;
    scale_ValMax = maxValue;
    redraw();
}



void AnalogVUMeter::setVUTextFont(uint8_t size) {
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



void  AnalogVUMeter::setVUScaleFont(uint8_t size) {
    switch (size) {
        case  8: { scale_Font = Final_Frontier8 ; break; }
        case  9: { scale_Font = Final_Frontier9 ; break; }
        case 10: { scale_Font = Final_Frontier10; break; }
        case 11: { scale_Font = Final_Frontier11; break; }
        case 12: { scale_Font = Final_Frontier12; break; }
        case 13: { scale_Font = Final_Frontier13; break; }
        case 14: { scale_Font = Final_Frontier14; break; }
        case 15: { scale_Font = Final_Frontier15; break; }
        case 16: { scale_Font = Final_Frontier16; break; }
        default: scale_Font = Final_Frontier13;
    }
}



void  AnalogVUMeter::loop() {
    if ((adc_owner != ADC_OWNER_ANALOG) || (adc_handle == nullptr)) return;

    uint8_t  result[BUFFER_SIZE] = {0};
    uint32_t ret_num = 0;
    unsigned long nowTime = millis();

    if ( nowTime - oldLoopTime < 1000) Serial.printf(">AVUM loop time: %ld\n", nowTime - oldLoopTime);
    oldLoopTime = nowTime;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

        }
        
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        float overallVolume = 0;

        // CALCULATION OF SIGNAL ENERGY (RMS)
        float sumOfSquares = 0;
        for (int band = 0; band < NUM_BANDS; band++) {
            // Square it so that louder signals have more weight
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
       
        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        
        // Draw results...
        if (nowTime - lastTftWriteTime >= MIN_REDRAW_TIME) {
            drawNeedle( interpolatedVolume  );
            lastTftWriteTime = nowTime;
        }
    }

}



