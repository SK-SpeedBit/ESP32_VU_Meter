#include <pgmspace.h>
#include <algorithm>

#include "WaveVUMeter.h"
#include "image\background_Image.h"

#include "..\..\include\config.h"


WaveVUMeter::WaveVUMeter()
    : enabled(true)
{
   
}

WaveVUMeter::~WaveVUMeter() {
    ;
}



void WaveVUMeter::init_adc_continuous() {

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

    adc_owner = ADC_OWNER_WAVE;
}



void WaveVUMeter::begin(TFT_eSPI &tftRef) {
    tft = &tftRef;

    if (!tft_initialized) {
        tft->init();
        tft->setRotation(1); 
        tft->fillScreen(TFT_BLACK);
        tft_initialized = true;
    }

    init_adc_continuous();
    
    tft->fillScreen(backgroundColor);
    
    // last loop time
    oldLoopTime = millis();
    enabled = true;
}



void  WaveVUMeter::loop() {
    if (!enabled) return;
    if ((adc_owner != ADC_OWNER_WAVE) || (adc_handle == nullptr)) return;

    uint8_t  result[BUFFER_SIZE] = {0};
    uint32_t ret_num = 0;
    unsigned long nowTime = millis();

    if ( nowTime - oldLoopTime < 1000) Serial.printf(">WVUM loop time: %ld\n",  nowTime - oldLoopTime);
    oldLoopTime = nowTime;

    if (nowTime - lastTftWriteTime >= MIN_REDRAW_TIME) {    

        // Read from DMA
        esp_err_t ret = adc_continuous_read(adc_handle, result, BUFFER_SIZE, &ret_num, 0);
        
        if (ret == ESP_OK && ret_num == BUFFER_SIZE) {
            int sample_idx = 0;
            for (int i = 0; i < ret_num; i += 2) {
                adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
                adc_DataBuffer[sample_idx] = (float)(p->type1.data) * 8; // ADC: 9bit: *8 / 10bit *4 / 11bit *2 / 12 bit * 1 
                sample_idx++;
            }
            
            int halfDisplay = DISPLAY_HEIGHT / 2;

            tft->fillScreen(backgroundColor);
            if (drawZeroLine) tft->drawLine(  0, halfDisplay, DISPLAY_WIDTH, halfDisplay, 0x4208);

            ////////////////////////////////////////////////////////////

            float y0=(halfDisplay + (halfDisplay * ((adc_DataBuffer[0])/adc_divider) ) * adc_multiplier) + levelZeroCorrection;
            float y1=0;

            for (int i = 1; i < DISPLAY_WIDTH; i++) {
                y1 = (halfDisplay + (halfDisplay * ((adc_DataBuffer[i])/adc_divider) ) * adc_multiplier) + levelZeroCorrection;

                // Draw....
                // It won't be faster than using this code?
                if (drawPixelinsteadLine)
                    tft->drawPixel(i, y0-halfDisplay, foregroundColor);
                else
                    tft->drawLine (i, y0-halfDisplay, i+1, y1-halfDisplay, foregroundColor);
                    //tft->drawFastVLine(i, y0-halfDisplay, y1-y0, foregroundColor);
                y0 = y1;
            }

            ////////////////////////////////////////////////////////////

            lastTftWriteTime = nowTime;
        }

    }

}



