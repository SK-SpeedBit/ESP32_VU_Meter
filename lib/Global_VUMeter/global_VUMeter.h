#ifndef __GLOBAL_VUM_VARIABLES__
#define __GLOBAL_VUM_VARIABLES__

#include <TFT_eSPI.h> 
#include "esp_adc\adc_continuous.h"

// Global
extern TFT_eSPI tft                      ;  // There is one definition of the TFT_eSPI object in the entire program
extern adc_continuous_handle_t adc_handle;  // ADC handle 
extern uint16_t adc_owner                ;  // id owner of ADC 

#endif //__GLOBAL_VUM_VARIABLES__