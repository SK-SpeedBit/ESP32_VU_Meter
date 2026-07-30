#ifndef WAVE_VU_METER_H
#define WAVE_VU_METER_H

#include <TFT_eSPI.h>
#include <esp_adc/adc_continuous.h>
#include <arduinoFFT.h>
#include "..\Global_VUMeter\global_VUMeter.h"



class WaveVUMeter {
public:
    bool     enabled             =       false;

    // Background
    uint16_t backgroundColor     =   TFT_BLACK;                                 // Background color
    uint16_t foregroundColor     =   TFT_GREEN;                                 // Color of line

    bool drawZeroLine            =       false;                                 // Draw a line "level 0"
    bool drawPixelinsteadLine    =       false;                                 // Draw a pixels instead line
    
    float adc_multiplier         =        1.45;                                 // ADC data multiplier (to cover the entire screen - amplification)
    float adc_divider            =        3300;                                 // ADC data divider - the full scale is 0..3.3V - so the zero line is 1.65V (uważaj przed przeciążeniem!)
    float levelZeroCorrection    =          18;                                 // Zero level correction on screen

   
    WaveVUMeter();
    ~WaveVUMeter();

    TFT_eSPI& getTft() { return *tft; }                                         // Return TFT_eSPI object

    void  begin(TFT_eSPI &tftRef);                                              // Sets parameters and draws initial state - adc_handle will be useful when switching VU meters
    void  loop();                                                               // Responsible for reading and drawing results (takes about 40 ms)


private:
    // Display const  (ILI9341)
    static constexpr uint16_t DISPLAY_WIDTH     =     320;                      // Display width
    static constexpr uint16_t DISPLAY_HEIGHT    =     240;                      // Display height

    // --- CONFIG AUDIO (DMA + FFT) ---
    static constexpr uint8_t  ADC_UNIT          =  ADC_UNIT_1;                  // ADC unit
    static constexpr uint8_t  ADC_CHANNEL       =  ADC_CHANNEL_0;               // ADC channel
    static constexpr uint32_t SAMPLE_FREQ       =   20000;                      // ADC sampling rate
    static constexpr uint16_t SAMPLES           =     512;                      // ADC numer of samples => 25ms
    static constexpr uint32_t BUFFER_SIZE       =  (SAMPLES * 2);               // ADC buffer size
    static constexpr uint16_t ADC_OWNER_WAVE    =   54321;                      // ADC owner id

    static constexpr uint8_t MIN_REDRAW_TIME    =      10;                      // Minimum time between redraws in ms

    float adc_DataBuffer[SAMPLES];                                              // Buffer for data 
    
    TFT_eSPI    *tft                            = nullptr;                      // TFT_eSPI object 
    static inline bool tft_initialized          =   false;                      // TFT_eSPI initialized flag

    // Auxiliary variables
    unsigned long lastTftWriteTime              =       0;
    int oldLoopTime                             =       0;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////


    // Auxiliary function - calculates the angles within which the needle moves
    void init_adc_continuous();                                                 // ADC init

};


#endif // WAVE_VU_METER_H
