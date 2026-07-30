#ifndef ANALOG_VU_METER_H
#define ANALOG_VU_METER_H

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



class AnalogVUMeter {
public:
    bool     enabled             =       false;

    // Background
    uint16_t backgroundColor     =   TFT_BLUE ;    //0xb5aa;  // Background color or more precisely - coloring the background image
    uint16_t foregroundColor     =   TFT_BLACK;    // Color of points for dithering
    uint8_t  backgroundDither    =           4;    // Dither size

    // Scale - The scale looks much better when it is light in color and the background is dark.
    uint32_t scale_Color         = 0x9ca6;        // Scale color
    uint32_t scale_ColorHLevel   = TFT_RED;       // Scale color above zero
    uint32_t scale_TextColor     = 0x9ca6;        // Text color
    uint32_t scale_TextBkgColor  = TFT_TRANSPARENT; // Background Text color
    uint32_t scale_BaseLineColor = TFT_YELLOW;    // Scale Baseline Color

    int scale_PosAxisX           =    160;        // X-axis position common to both scale and pointer
    int scale_PosAxisY           =    550;        // Y-axis position for drawing scale profile (flattening) - remember to set hLevel!
    int needle_PosAxisY          =    220;        // Y-axis position for pointer rotation and scale angles (Technics SE-A style)
    bool scale_linear            =   true;        // Flat scale (line)
    bool scale_linearTicks       =   true;        // flat scale description (along the lines)
    bool scale_HighLevelZone     =   true;        // The scale has a high level area - overload
    bool scale_DrawBaseLine      =  false;        // Draw a baseline (line or arc)

    int scale_hLevel             =    410;        // Base scale arc radius
    int scale_BaseArcWidth       =      4;        // Sscale line thickness
    int scale_MarginPx           =     10;        // Margin in pixels from the edge of the screen

    float scale_MinorWidth       =    1.0;        // Minor pitch width
    float scale_MajorWidth       =    2.0;        // Major pitch width

    float scale_MinorLen         =   20.0;        // Minor pitch length
    float scale_MajorLen         =   50.0;        // Major pitch length

    int scale_MajorCnt           =     10;        // Number of coarse divisions (major)
    int scale_MinorCnt           =      5;        // Number of fine divisions (minor)

    int scale_ValMin             =    -20;        // Minimum value at the leftmost edge
    int scale_ValMax             =      5;        // Maximum value at the right edge

    int scale_MajorStep          =      5;        // Main lines with signature every ...
    int scale_MinorStep          =      1;        // Small lines every ... units

    int scale_LineExtensionPx    =     20;        // 20 pixels extend beyond the outermost lines
    int scale_ArcExtensionDeg    =      3;        // Baseline magnification when arc scale. In degrees
    
    // Needle
    uint16_t needle_Color        = 0x8c51;        // Needle color
    float    needle_Width        =    3.0;        // Needle thickness
    float    needle_Smooth       =    0.5;        // Needle behavior 0-1 => 1 - fast, 0.25 smooth) => 0 - makes no sense
    float    needle_AboveScale   =   52.0;        // The tip of the needle above the scale
    bool     needleRotateCircle  =   true;        // A wheel imitating the needle rotation mechanism
    int32_t  hideNeedleBelowY    =    195;        // Lower boundary of the housing
    bool     needle_VarLength    =   true;        // variable length needle



    AnalogVUMeter();
    ~AnalogVUMeter();

    TFT_eSPI& getTft() { return *tft; }                                         // Return TFT_eSPI object

    void  begin(TFT_eSPI &tftRef);                                              // Sets parameters and draws initial state - adc_handle will be useful when switching VU meters
    void  loop();                                                               // Responsible for reading, FFT and drawing results (takes about 9 ms)

    // Save the background after you've drawn everything you need. 
    // The program uses this to refresh the drawing under the needle.
    // Using this function is necessary after drawing all the background elements. 
    // Without it, the needle will not be drawn correctly.
    void  saveBackground();                                                     // Saves the background - Call when you've drawn everything on the screen!
    void  restoreBackground();                                                  // Restores the background

    void  fillScreen();                                                         // Fills screen with backgroundColor
    void  fillScreen(uint16_t bgColor);                                         // Fills screen with bgColor

    void  drawBackground();                                                     // Draw background
    void  drawBackground(uint16_t bgColor);                                     // Draw background with bgColor
    void  drawBackground(uint16_t bgColor, uint16_t fgColor);                   // Draw background with bgColor and fgColor
    void  drawBackground(uint16_t bgColor, uint16_t fgColor, uint8_t dither);   // Draw background with bgColor and fgColor and dither

    void  drawRotateCircle();                                                   // Draws a circle at the needle's axis

    void  drawScale();                                                          // Draws all scale

    // the parameter takes the value 0 - 1 (0% to 100%) -> scale the input level first!
    void  drawNeedle(float targetProgress);                                     // Draws the needle at position 0.0 - 1.0 (0% - 100%) of the scale

    void  drawString(const char *string, int32_t poX, int32_t poY);             // Draws text at position
    void  drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor);                      // Draws text at position and in txtColor
    void  drawString(const char *string, int32_t poX, int32_t poY, uint16_t txtColor, uint16_t bkgColor);   // Draws text at position and in txtColor and bkgColor

    void  redraw();                                                             // Draw the background from scratch with the current settings

    // Settings - call before begin()
    void  setVUTextFont (uint8_t size);                                         // 8, 9, 10, 11, 12, 13, 14, 15, 16
    void  setVUScaleFont(uint8_t size);                                         // 8, 9, 10, 11, 12, 13, 14, 15, 16

    void  setRange(int minValue, int maxValue);                                 // Sets the scale range (minimum - maximum


private:
    // Display const  (ILI9341)
    static constexpr uint16_t DISPLAY_WIDTH     =  320;                         // Display width
    static constexpr uint16_t DISPLAY_HEIGHT    =  240;                         // Display height

    static constexpr uint16_t NUM_BANDS         =   16;                         // Number of bands 

    static constexpr uint8_t TILE_SIZE          =   16;                         // Size of tile
    static constexpr uint8_t MAX_TILES          =   70;                         // Number of tiles

    // --- CONFIG AUDIO (DMA + FFT) ---
    static constexpr uint8_t  ADC_UNIT          =  ADC_UNIT_1;                  // ADC unit
    static constexpr uint8_t  ADC_CHANNEL       =  ADC_CHANNEL_0;               // ADC channel
    static constexpr uint32_t SAMPLE_FREQ       =  80000;                       // ADC sampling rate
    static constexpr uint16_t SAMPLES           =    512;                       // ADC numer of samples
    static constexpr uint32_t BUFFER_SIZE       =  (SAMPLES * 2);               // ADC buffer size
    static constexpr uint16_t ADC_OWNER_ANALOG  =  1357;                        // ADC owner id

    static constexpr uint8_t MIN_REDRAW_TIME    =      10;                       // Minimum time between redraws in ms

    float vReal[SAMPLES];                                                       // Buffer for FFT data (real) 
    float vImag[SAMPLES];                                                       // Buffer for FFT data (imaginary)
    
    ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, SAMPLES, SAMPLE_FREQ);  // FFT object

    // Memory arrays of tile positions from the previous frame
    int32_t oldTileX[MAX_TILES];
    int32_t oldTileY[MAX_TILES];
    int32_t oldTileCount  = 0;

    TFT_eSPI    *tft                            = nullptr;                      // TFT_eSPI object 
    TFT_eSprite *imgBuffer                      = nullptr;                      // Background coloring and dithering buffer
    uint16_t*    screenBackupBuffer             = nullptr;                      // Background image copy buffer (screen shadow)
    static inline    bool tft_initialized       =   false;                      // TFT_eSPI initialized flag

    // Auxiliary variables
    float smoothProgress                        =    0.0f;                      // Smooth progress
    float lastNeedleAngle                       =   -1.0f;                      // Last needle angle
    int   scale_AngleStart                      =       0;                      // Initial angle
    int   scale_AngleStop                       =       0;                      // End angle

    float interpolatedVolume                    =     0.0;
    unsigned long lastTftWriteTime              =       0;

    const uint8_t *text_Font                    = Final_Frontier10;             // Pointer to the text font
    const uint8_t *scale_Font                   = Final_Frontier14;             // Pointer to the scale font

    uint16_t tileRamBuffer[TILE_SIZE * TILE_SIZE];                              // Small workboard in fast SRAM for one tile
    int   tileMargin                            =       5;                      // margin for tiles

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
         10200.0 -  7000  // 16	97-101	14062-17187	15000.00 Hz . // Beware of noise!
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


    int oldLoopTime = 0;

    // Auxiliary function - calculates the angles within which the needle moves
    void calculateSafeAnglesPx(int scale_PosAxisY, int needle_PosAxisY, int scale_hLevel, int scale_MajorLen, int scale_MarginPx, int &scale_AngleStart, int &scale_AngleStop);
    void init_adc_continuous();                                                 // ADC init


};


#endif // ANALOG_VU_METER_H
