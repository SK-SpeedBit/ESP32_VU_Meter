#include <Arduino.h>
#include <TFT_eSPI.h> 
#include "AnalogVUMeter.h"
#include "SpectrumVUMeter.h"
#include "WaveVUMeter.h"
#include "..\lib\Global_VUMeter\global_VUMeter.h"



#define ANALOG_VUM   0
#define SPECTRUM_VUM 1
#define WAVE_VUM     2

#define SWITCH_TIME  15000           // Time after which we switch the display type   

unsigned long oldSwitchTime      = millis();
unsigned long oldSwitchColorTime = millis();
int           currentTypeVUM     = 0;
int           colorIndex         = 0;
uint16_t      colorTable[]       = {TFT_GREEN, TFT_BLUE, TFT_RED, TFT_PURPLE, TFT_YELLOW, TFT_PINK}; 



AnalogVUMeter   avu;
SpectrumVUMeter svu;
WaveVUMeter     wvu;


void setup() {
    Serial.begin(115200);

    // -----------------------------------------------------------------------------------------------
    // Full init analog VU meter
    // Using this saveBackground() function is necessary (!) after drawing all the background elements. 
    // Without it, the needle will not be drawn correctly.
    avu.begin(tft);
    //avu.backgroundColor = 0xb5aa;
    avu.scale_Color     = TFT_BLACK;
    avu.scale_ColorHLevel = 0x8000;
    
    avu.needle_Color    = TFT_RED;
    avu.needle_Width    = 1;

    avu.drawBackground();
    // or
    //avu.fillScreen(TFT_BLUE);

    avu.scale_MinorWidth = 1;
    avu.scale_MajorWidth = 2;

    avu.scale_ValMax = 3;
    avu.scale_ValMin = -20;

    avu.setVUScaleFont(2);
    avu.scale_SysFontSize = 1;
    avu.drawScale();

   
    avu.text_Color =TFT_BLACK;
    avu.setVUTextFont(2);
    avu.text_SysTextFontSize = 1;
    avu.drawText("POWER OUTPUT"        , 160,  25);
    
    avu.text_Color =TFT_BLACK;
    avu.setVUTextFont(1);
    avu.text_SysTextFontSize = 1;
    avu.drawText("VU METER by SpeedBit", 160, 175);

    avu.drawRotateCircle();

    avu.saveBackground(); // (!)
    avu.drawNeedle(0.0);

    // -----------------------------------------------------------------------------------------------
    // Full init spectrum VU meter
    // We create permanent background elements and save them to memory
    // It's not necessary to use the saveBackground() function for this pointer, 
    // but it speeds up redrawing when you add your own text, etc. 
    // The first use of the saveBackground() function will reserve memory for the screen buffer. 
    // If you don't use this function, no memory will be reserved.
    svu.begin(tft);
    svu.drawTopMarginText("Spectrum analyzer by SpeedBit", 15, TFT_DARKGREY);
    tft.drawLine(0, svu.margin_top, 320, svu.margin_top, 0x4208);
    tft.drawLine(0, 240 - svu.margin_bottom, 320, 240 - svu.margin_bottom, 0x4208);
    svu.saveBackground();

    // -----------------------------------------------------------------------------------------------
    // Full init wave VU meter
    wvu.begin(tft);

}



void loop() {
    avu.loop();
    svu.loop();
    wvu.loop();


    // Analog VU Meter
    if (((millis() - oldSwitchTime) > SWITCH_TIME) && (currentTypeVUM == ANALOG_VUM)) {
        oldSwitchTime = millis();
        currentTypeVUM = SPECTRUM_VUM;
    
        avu.begin(tft);
        avu.restoreBackground();  // Background from memory
    }

    // Spectrum VU Meter
    if ( ((millis() - oldSwitchTime) > SWITCH_TIME) && (currentTypeVUM == SPECTRUM_VUM)) {
        oldSwitchTime = millis();
        currentTypeVUM = WAVE_VUM;
        currentTypeVUM = ANALOG_VUM;
        
        svu.begin(tft);
        svu.restoreBackground();  // Background from memory
    }

    // Wave VU Meter
    if (((millis() - oldSwitchTime) > SWITCH_TIME) && (currentTypeVUM == WAVE_VUM)) {
        oldSwitchTime = millis();
        currentTypeVUM = ANALOG_VUM;
    
        wvu.begin(tft);
    }

    // Dynamic color change (for WaveVUMeter)
    if (((millis() - oldSwitchColorTime) > 3000) ) {
        oldSwitchColorTime = millis();
        wvu.foregroundColor = colorTable[colorIndex];
        colorIndex++;
        Serial.printf("size = %d\n", sizeof(colorTable));
        if (colorIndex > ( (sizeof(colorTable) / (sizeof(uint16_t)) ) - 1) ) colorIndex = 0;

        // BTW: RAM test...
        Serial.printf(">Free RAM: %lu\n", ESP.getFreeHeap());
        Serial.printf(">Max free block: %lu\n", ESP.getMaxAllocHeap());
        Serial.printf(">Min free heap : %lu\n", ESP.getMinFreeHeap());
    }

}





