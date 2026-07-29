
#include <Arduino.h>
#include <TFT_eSPI.h> 
#include "AnalogVUMeter.h"
#include "SpectrumVUMeter.h"
#include "WaveVUMeter.h"
#include "..\lib\Global_VUMeter\global_VUMeter.h"


AnalogVUMeter   avu;
SpectrumVUMeter svu;
WaveVUMeter     wvu;


void setup() {
    Serial.begin(115200);
    
    // Full init analog VU meter
    svu.begin(tft);
    svu.drawTopMarginText("Spectrum analyzer", 15, TFT_DARKGREY);
    tft.drawLine(0, svu.margin_top, 320, svu.margin_top, 0x4208);
    tft.drawLine(0, 240 - svu.margin_bottom, 320, 240 - svu.margin_bottom, 0x4208);
   
    // Full init spectrum VU meter
    avu.begin(tft);
    //avu.drawBackground();
    avu.fillScreen(0x0002);
    avu.drawScale();
    avu.drawString("POWER OUTPUT"        , 160,  50, 0x528a);
    avu.drawString("UV METER by SpeedBit", 160, 180, 0x39c7);
    avu.drawRotateCircle();
    avu.saveBackground();
    avu.drawNeedle(0.0);

    // Full init wave VU meter
    wvu.begin(tft);

}

#define ANALOG_VUM   0
#define SPECTRUM_VUM 1
#define WAVE_VUM     2

#define SWITCH_TIME  15000           // Time after which we switch the display type   
unsigned long shitchTime = millis();
int           nextTypeVUM  = 0;

void loop() {
    svu.loop();
    avu.loop();
    wvu.loop();



    // Analog VU Meter
    if (((millis() - shitchTime) > 15000) && (nextTypeVUM == ANALOG_VUM)) {
        shitchTime = millis();
        nextTypeVUM = SPECTRUM_VUM;
    
        avu.begin(tft);
        avu.restoreBackground();
    }

    // Spectrum VU Meter
    if ( ((millis() - shitchTime) > 15000) && (nextTypeVUM == SPECTRUM_VUM)) {
        shitchTime = millis();
        nextTypeVUM = WAVE_VUM;
        
        svu.begin(tft);
        svu.drawTopMarginText("Spectrum analyzer", 15, TFT_DARKGREY);
        tft.drawLine(0, svu.margin_top, 320, svu.margin_top, 0x4208);
        tft.drawLine(0, 240 - svu.margin_bottom, 320, 240 - svu.margin_bottom, 0x4208);       

    }

    // Wave VU Meter
    if (((millis() - shitchTime) > 15000) && (nextTypeVUM == WAVE_VUM)) {
        shitchTime = millis();
        nextTypeVUM = ANALOG_VUM;
    
        wvu.begin(tft);
    }


}





