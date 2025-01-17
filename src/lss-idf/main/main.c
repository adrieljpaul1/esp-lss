#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define WIFI_SSID1 "Main Server"
#define WIFI_PASS1 "admin@123"
//sir hotspot
#define WIFI_SSID2 "realme"
#define WIFI_PASS2 "ilakkiya"
//mqtt
#define MQTT_BROKER_URI "mqtt://5.196.78.28"
#define MOISTURE_TOPIC "sector_2" //get
#define SPRINKLER_CONTROL_TOPIC "sector_2_sprinkler" //post
//tags
#define TAG "ESP32"
//hardware 
#define MOISTURE_SENSOR_COUNT 4
#define VALVE_GPIO GPIO_NUM_4

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static esp_mqtt_client_handle_t mqtt_client = NULL;
adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc_cali_handle;

// ADC channels
const adc_channel_t moisture_channels[MOISTURE_SENSOR_COUNT] = {
    ADC_CHANNEL_6, // GPIO34
    ADC_CHANNEL_7, // GPIO35
    ADC_CHANNEL_0, // GPIO36
    ADC_CHANNEL_3  // GPIO39
};

static void init_gpio() {
    // Initialize relay GPIO pin as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << VALVE_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // Set relay to OFF state initially
    gpio_set_level(VALVE_GPIO, 0); 
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            break;
        default:
            break;
    }
}

static void connect_to_wifi() {
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID1,
            .password = WIFI_PASS1
        }
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi Connected");
}

static void init_adc() {
    // Initialize ADC
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    adc_oneshot_new_unit(&init_cfg, &adc1_handle);

    // Configure channels
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    for (int i = 0; i < MOISTURE_SENSOR_COUNT; i++) {
        adc_oneshot_config_channel(adc1_handle, moisture_channels[i], &chan_cfg);
    }

    // Initialize calibration using line fitting
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_cali_handle) == ESP_OK) {
        ESP_LOGI(TAG, "ADC Calibration Success");
    } else {
        ESP_LOGW(TAG, "ADC Calibration Failed");
    }
}


static void publish_moisture_data(int *moisture_values) {
    char payload[128];

    snprintf(payload, sizeof(payload), "{\"moisture\":[%d,%d,%d,%d,%d]}", moisture_values[0], moisture_values[1], moisture_values[2], moisture_values[3],(moisture_values[0] + moisture_values[1] + moisture_values[2] + moisture_values[3]) / 4);
    esp_mqtt_client_publish(mqtt_client, MOISTURE_TOPIC, payload, 0, 1, 1);
}

static void log_mqtt(int *moisture_values){
    ESP_LOGI(TAG, "Moisture = %d %d %d %d Average = %d",moisture_values[0], moisture_values[1], moisture_values[2], moisture_values[3],(moisture_values[0] + moisture_values[1] + moisture_values[2] + moisture_values[3]) / 4);
}
void app_main() {
  //init
    nvs_flash_init();
    init_adc();
    init_gpio();

    connect_to_wifi();

    mqtt_client = esp_mqtt_client_init(&(esp_mqtt_client_config_t){
        .broker = {
            .address.uri = MQTT_BROKER_URI,
        }
    });
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    

    gpio_set_direction(VALVE_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(VALVE_GPIO, 0);

    int moisture_values[MOISTURE_SENSOR_COUNT];

    while (true) {
        for (int i = 0; i < MOISTURE_SENSOR_COUNT; i++) {
            int raw_value;
            adc_oneshot_read(adc1_handle, moisture_channels[i], &raw_value);
            moisture_values[i] = 100 - ((raw_value * 100) / 4095); // Convert raw to percentage
        }

        int avg_moisture = (moisture_values[0] + moisture_values[1] + moisture_values[2] + moisture_values[3]) / 4;
        gpio_set_level(VALVE_GPIO, (avg_moisture <= 20) ? 1 : 0);

        publish_moisture_data(moisture_values);
        log_mqtt(moisture_values);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
