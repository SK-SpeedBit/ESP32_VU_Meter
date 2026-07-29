#ifndef __SpectrumVUMeter__
#define __SpectrumVUMeter__

#include <TFT_eSPI.h>
#include <esp_adc/adc_continuous.h>
#include <arduinoFFT.h>
#include "..\Global_VUMeter\global_VUMeter.h"

// fonts
#include "Final_Frontier8.h"
#include "Final_Frontier9.h"
#include "Final_Frontier10.h"
#include "Final_Frontier11.h"
#include "Final_Frontier12.h"
#include "Final_Frontier13.h"
#include "Final_Frontier14.h"
#include "Final_Frontier15.h"
#include "Final_Frontier16.h"



class SpectrumVUMeter {

public:
    bool     enabled                  =  true;
    
    uint16_t margin_left              =     2;                                // screen margin left
    uint16_t margin_right             =     2;                                // screen margin right
    uint16_t margin_top               =    20;                                // screen margin top    - when title exist set value >= (2 + fontSize + 2)
    uint16_t margin_bottom            =    14;                                // screen margin bottom - when drawDescriptions is true set 35, when showLineVUMeter is true set value above lineVUWidth (best 2*lineVUWidth)

    uint16_t gap_X                    =     2;                                // Horizontal spacing between posts
    uint16_t gap_Y                    =     2;                                // Vertical spacing between bricks

    uint16_t segments_per_band        =    25;                                // Number of bricks in the entire column
    uint16_t threshold_red            =    22;                                // Red brics    - level
    uint16_t threshold_yellow         =    19;                                // Yellow brics - level

    uint32_t text_color               = TFT_DARKGREY;                         // Text color

    // --- Colors RGB565 ---
    uint16_t background_color         = 0x0000;                               // Background color
    uint16_t frame_color              = 0x18C3;                               // Frames color  (Dark gray graphite)
    
    // Brics - colors
    uint16_t base_color_normal        = TFT_BLUE;                             // Normal bricks color 
    uint16_t base_color_warningZone   = TFT_ORANGE;                           // color of the bricks in the warning area 
    uint16_t base_color_highZone      = TFT_RED;                              // color of the bricks in the overdrive area

    // Settings
    bool     drawtileFrame            =  false;                               // Draw a frame around the bricks? (LED appearance)

    bool     drawDescriptions         =  false;                               // Draw descriptions? Remember set margin_bottom!
    bool     descriptionsAsFreq       =  false;                               // Draw descriptions as center frequency
    bool     autoSizeWidth            =   true;                               // automatically adjust the screen width?
    bool     autoCenterWidth          =   true;                               // automatically center the screen?

    bool     showWarningZoneTiles     =   true;                               // Show bricks in warning area in lineVUColorWarningZone color
    bool     showHighZoneTiles        =   true;                               // Show bricks in overload area in lineVUHighZoneLevel color
    bool     showPeaks                =   true;                               // Show picks?
    
    uint8_t  peaksHeight              =      1;                               // Below or equal segment height. If not - auto height
    uint16_t peaksHoldTime            =    300;                               // Peaks Hold Time in ms
    uint16_t peaksFallTime            =    250;                               // Peaks Fall Time in ms


    // Linear VU Meter (takes about 5-8ms but only if it is nescessary)
    bool     showLineVUMeter          =   true;                               // Show VU meter? 
    bool     lineVUMeterAsBricks      =  false;                               // Liner VU Meter as brics or line

    int      lineVUWidth              =      4;                               // Linear VU meter heigh
    int      lineVU_segments          =     22;                               // For brics mode: brics count
    int      lineVU_threshold_red     =     19;                               // For brics mode: the color is red from
    int      lineVU_threshold_yellow  =     16;                               // For brics mode: the color is yellow from
    
    float    lineVUWarningZoneLevel   = (float)lineVU_threshold_yellow / (float)lineVU_segments; // or ie. 0.80; 0-1 (0%-100%) - for line mode
    float    lineVUHighZoneLevel      = (float)lineVU_threshold_red    / (float)lineVU_segments; // or ie. 0.90; 0-1 (0%-100%) - for line mode

    bool     lineShowWarningZoneTiles =   true;                               // Show line/bricks in warning  area in lineVUColorWarningZone color
    bool     lineShowHighZoneTiles    =   true;                               // Show line/bricks in overload area in lineVUHighZoneLevel color

    int      lineVUColor              = TFT_BLUE;                             // Normal line/bricks color 
    int      lineVUColorWarningZone   = TFT_YELLOW;                           // Color of the line/bricks in the warning area 
    int      lineVUColorHighZone      = TFT_RED;                              // Color of the line/bricks in the overdrive area
    uint16_t lineVUInactiveBricksColor= tft->color565(30, 30, 30);            // For brics mode - inactive brics color or background_color

    float    lineVUPos_Y              = (float)DISPLAY_HEIGHT - (float)margin_bottom/2 - (float)lineVUWidth/2 ; // Line/brics vu meter Y position center in margin_bottom or manual
    int      lineVULeftMargin         = margin_left;                          // line/brics vu meter left margin
    int      lineVURightMargin        = margin_right;                         // line/brics vu meter right margin    
    bool     lineVUWidthAsSpectrum    =   true;                               // line/brics vu meter margins as spectrum screen


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    SpectrumVUMeter();
    ~SpectrumVUMeter();

    TFT_eSPI& getTft() { return *tft; }                                       // Return TFT_eSPI object

    
    void  begin(TFT_eSPI &tftRef);        // Sets parameters and draws initial state - adc_handle will be useful when switching VU meters
    void  loop();                                                             // Responsible for reading, FFT and drawing results (takes about 18 ms)

    void  drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor);  
    void  drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor, uint16_t bkgColor);


    // Settings - call before begin()
    void  setVUTextFont (uint8_t size);                                       // Size: 8, 9, 10, 11, 12, 13, 14, 15, 16
    void  drawTopMarginText(String text, uint8_t size, uint16_t color);       // Empty string - top margin empty, otherwise it prints center the text with the specified font and color (make space min. 20px)


private:
    // Display const  (ILI9341)
    static constexpr uint16_t DISPLAY_WIDTH     =  320;                       // Display width
    static constexpr uint16_t DISPLAY_HEIGHT    =  240;                       // Display height
    
    static constexpr uint16_t NUM_BANDS         =   16;                       // Number of bands 

    // --- CONFIG AUDIO (DMA + FFT) ---
    static constexpr uint8_t  ADC_UNIT          =  ADC_UNIT_1;                // ADC unit
    static constexpr uint8_t  ADC_CHANNEL       =  ADC_CHANNEL_0;             // ADC channel
    static constexpr uint32_t SAMPLE_FREQ       =  80000;                     // ADC sampling rate
    static constexpr uint16_t SAMPLES           =    512;                     // ADC numer of samples 1024 -> ~20ms, 512 -> ~7.5-10ms!
    static constexpr uint32_t BUFFER_SIZE       =  (SAMPLES * 2);             // ADC buffer size
    static constexpr uint16_t ADC_OWNER_SPECTRUM=   2468;                     // ADC owner id

    static constexpr uint8_t MIN_REDRAW_TIME    =     10;                     // Minimum time between redraws in ms


    TFT_eSPI    *tft                   = nullptr;                             // TFT_eSPI object 
    static inline bool tft_initialized = false;                               // TFT_eSPI initialized flag

    float vReal[SAMPLES];                                                     // Buffer for FFT data (real) 
    float vImag[SAMPLES];                                                     // Buffer for FFT data (imaginary)
    
    ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, SAMPLES, SAMPLE_FREQ);  // FFT

    //  Internal variables
    int segWidth  = 0;   
    int segHeight = 0;  

    struct PeakTrack {
        int           lastMaxSegment;   
        unsigned long peakTime; 
    };
    PeakTrack bandPeaks[NUM_BANDS];
    float smoothedBands[NUM_BANDS] = {0.0}; 


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // The following data are for SAMPLES = 512 loop ~10ms
    
    // Bin table (100 * 156.25 Hz = 15 625 Hz) band: 16 kHz)
    const int bandCutoffTable[NUM_BANDS + 1] = {
        1, 2, 3, 4, 5, 7, 9, 11, 15, 19, 25, 33, 45, 59, 74, 90, 100
    };

    // Descriptions to display on screen:
    const String bandLabels[NUM_BANDS] = {
        "150", "300" , "470" , "650" , "1k", "1.2k", "1.5k", "2k",  "2.6k", "3.5k", "4.5k", "6k", "8k", "10k", "13k" , "16k"
    };

    // Divisor table
    // The divisors are determined using a generator. 
    // The additional numbers are my manual corrections.
    // Additional empirical correction for music playback.
    // Generator calibration alone does not produce a natural visual balance
    // because different bands contain different numbers of FFT bins and
    // real music has a different spectral distribution than a single sine wave.
    const float bandGainTable[NUM_BANDS] = {
     //|Divisor  +Magic_numer|No.|cnt_bins| Band   |    Middle  |
         72000.0 + 72000, //  1	  1-1	  156-  156   156.25 Hz .
         72000.0 + 72000, //  2	  2-2	  312-  312   312.50 Hz .
         72000.0 + 40500, //  3	  3-3	  469-  469   468.75 Hz . 
         72000.0 + 30000, //  4	  4-5	  625-  781   625.00 Hz .
         56000.0 + 30000, //  5	  6-7	  781- 1093	  937.50 Hz .
         55000.0 + 10000, //  6	  8-10	 1093- 1406	 1250.00 Hz .
         55000.0 +  5000, //  7	 11-14	 1406- 1718	 1562.50 Hz .
         42000.0 -   500, //  8	 15-19	 1718- 2343	 2031.25 Hz .
         41000.0 -  1000, //  9	 20-26	 2343- 2968	 2656.25 Hz .
         34000.0 -  2000, // 10	 27-35	 2968- 3906	 3437.50 Hz .
         28000.0 -  3000, // 11	 36-47	 3906- 5156	 4531.25 Hz .
         22000.0 -  4000, // 12	 48-62	 5156- 7031	 6093.75 Hz .
         18000.0 -  5000, // 13	 63-77	 7031- 9218	 8125.00 Hz .
         15000.0 -  5000, // 14	 78-89	 9218-11562	10390.63 Hz .
         11500.0 -  6000, // 15	 90-96	11562-14062	12812.50 Hz .
         10200.0 -  7100  // 16	97-101	14062-17187	15000.00 Hz . // Beware of noise!
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
    // The following data are for SAMPLES = 1024 loop ~20ms!
    // Bin table (200 * 78.125 Hz = 15 625 Hz  band: 16 kHz)
    const int bandCutoffTable[NUM_BANDS + 1] = {
          1,   2,   3,   4,   6,   8,   11,  15,  20,  27,  36,  48,  65,  88, 119, 160, 200 
    };

    // Descriptions to display on screen:
    const String bandLabels[NUM_BANDS] = {
        "80", "125" , "200" , "315" , "500", "630", "1k", "1.25k",  "1.6k", "2.5k", "3.15k", "4k", "6.3k", "8k", "12.5k" , "16k"
    };

    // Divisor table
    const float bandGainTable[NUM_BANDS] = {
    //   Divisor           Band    Middle
        165000.0, //  ~   80  Hz    78 Hz
        165000.0, //  ~  125  Hz   156 Hz
        165000.0, //  ~  200  Hz   234 Hz
        135000.0, //  ~  315  Hz   390 Hz 
        135000.0, //  ~  500  Hz   546 Hz  
        102000.0, //  ~  630  Hz   703 Hz 
         75900.0, //  ~ 1.0  kHz  1015 Hz 
         60000.0, //  ~ 1.25 kHz  1328 Hz 
         43000.0, //  ~ 1.6  kHz  1796 Hz 
         33200.0, //  ~ 2.5  kHz  2421 Hz  
         26000.0, //  ~ 3.15 kHz  3281 Hz 
         17500.0, //  ~ 4.0  kHz  4375 Hz 
         12000.0, //  ~ 6.3  kHz  5937 Hz 
          8290.0, //  ~ 8.0  kHz  8046 Hz 
          5160.0, //  ~12.5  kHz 10859 Hz 
          3590.0  //  ~16.0  kHz 14062 Hz 
    };
*/
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////


    int lastDrawnSegments  [NUM_BANDS] = {0};   // Cache table – stores how many bricks were lit in the previous frame
    int lastMaxDrawnSegment[NUM_BANDS] = {0};   // Memory for Peak Hold line
    int lastDrawnVUSegments            =   0;   // Previous frame memory

    // Auxiliary variables
    int max_pixel_width             =   0;
    int currentVUPixelWidth         =   0;
    float interpolatedVolume        = 0.0;
    int lastDrawVUPixelWidth        =   0;   // Remembers how long the strip was in the previous frame
    int oldLoopTime                 =   0;
    unsigned long lastTftWriteTime  =   0;
   
    const uint8_t *text_Font = Final_Frontier10; // Pointer to the selected font
    
    uint16_t mixColors(uint16_t baseColor, uint16_t bkgColor, float alfa);  // Color mixing function (ALPHA/transparency effect)
    void calculateGeometry();                                               // Calculates bricks data
    void init_adc_continuous();                                             // ADC init

};



#endif // __SpectrumVUMeter__
