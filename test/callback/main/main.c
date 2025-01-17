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
//init for non-volatile storage
void nvs_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}
//wifi requires event handlers
static void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data) {
   ESP_LOGI("EVENT_HANDLER", "Event base: %s, Event ID: %ld", event_base, event_id);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
       ESP_LOGI("EVENT_HANDLER", "Wi-Fi station started!");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect(); // Retry connection
        ESP_LOGI("WIFI", "Retrying connection...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("WIFI", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}
//wifi-init and config
void wifi_init_sta(const char* ssid, const char* password) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    //"Hey, whenever a Wi-Fi or IP-related event occurs, call my event_handler() function and pass the event data to it."
    //indirect call to event_handler -- refer &event_handler in below lines
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,ESP_EVENT_ANY_ID,&event_handler,NULL,&instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,IP_EVENT_STA_GOT_IP,&event_handler,NULL,&instance_got_ip));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "Main Server",
            .password = "admin@123",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}


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
 nvs_init();
 wifi_init_sta("Main Server", "admin@123");
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