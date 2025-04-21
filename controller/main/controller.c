#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "controller.h"

#include "esp_log.h"

static const char *TAG = "HEAT_CONTROLLER";

void app_main(void)
{
    while(1) {
        ESP_LOGI(TAG, "Test");
    }
}