#include <stdio.h>
//adc
#include "hal/adc_types.h"
#include "driver/adc.h"
#include "esp_mac.h"
#include "esp_adc/adc_oneshot.h"
//logging
#include "esp_log.h"
//freertos
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//wifi
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "ESP-32";
//reading
//adc_oneshot_read(adc1_handle, moisture_channels[i], &raw_value);
//wifi requires non-volatile storage
void app_main(){
  adc_oneshot_unit_handle_t adc1_handle;
  adc_oneshot_unit_init_cfg_t  init_config = {
        .unit_id = ADC_UNIT_1, // Specifyint ADC UNIT 1
    };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
  // Configure the channel
  adc_oneshot_chan_cfg_t chan_config = {
      .bitwidth = ADC_BITWIDTH_12, 
        .atten = ADC_ATTEN_DB_12,          
  };
 
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &chan_config));
   /*
  if adding 4 sensors
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &chan_config));
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &chan_config));
*/
  while(1){
    int raw;
    adc_oneshot_read(adc1_handle,ADC_CHANNEL_0, &raw);
    ESP_LOGI(TAG, "Value = %d", raw);

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
  adc_oneshot_del_unit(adc1_handle);
  
}