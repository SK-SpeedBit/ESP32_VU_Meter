#include "global_VUMeter.h"



TFT_eSPI tft                       = TFT_eSPI();   // There is one definition of the TFT_eSPI object in the entire program
adc_continuous_handle_t adc_handle =       NULL;   // ADC handle 
uint16_t adc_owner                 = 0;  // id owner of ADC 