#include "controller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "zcl_utility.h"

static const char *TAG = "HEAT_CONTROLLER";

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id) {
    default:
        ESP_LOGI(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_RETURN_ON_FALSE(esp_zb_bdb_start_top_level_commissioning(mode_mask) == ESP_OK, , TAG, "Failed to start Zigbee commissioning");
}


// This function has to be defined: 
// https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32/api-reference/esp_zigbee_core.html#_CPPv425esp_zb_app_signal_handlerP19esp_zb_app_signal_t
// This is an out-of-the-box function that handles what happens when the device is connecting to the network.
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct) {
    uint32_t *p_sg_p       = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK) {
            //ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new()) {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
            } else {
                ESP_LOGI(TAG, "Device rebooted");
            }
        } else {
            // commissioning failed
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        } else {
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type,
                 esp_err_to_name(err_status));
        break;
    }
}

static void esp_zb_task(void *pvParameters) 
{
    // Configure this as a router, since it will always be plugged in.
    esp_zb_cfg_t zb_nwk_cfg = ESP_ZB_ZR_CONFIG();
    esp_zb_init(&zb_nwk_cfg);

    // We need a custom cluster that's not part of the ZCL to be able to receive and send data
    // We need to define a list of 3 standard thermostat ZCL-compliant endpoints
    // We should have the custom cluster report its internal state, the sensor infos it gathered and the hash of the config we sent it
    #define CUSTOM_SERVER_ENDPOINT 0x01
    #define CUSTOM_CLIENT_ENDPOINT 0x01
    #define CUSTOM_CLUSTER_ID 0xff00   
    #define CUSTOM_COMMAND_RESP 0x0001

    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_attribute_list_t *custom_cluster = esp_zb_zcl_attr_list_create(CUSTOM_CLUSTER_ID);
    uint16_t custom_attr1_id = 0x0000;
    uint8_t custom_attr1_value[] = "\x0b""hello world";
    uint16_t custom_attr2_id = 0x0001;
    uint16_t custom_attr2_value = 0x1234;
    esp_zb_custom_cluster_add_custom_attr(custom_cluster, custom_attr1_id, ESP_ZB_ZCL_ATTR_TYPE_CHAR_STRING,
                                        ESP_ZB_ZCL_ATTR_ACCESS_WRITE_ONLY | ESP_ZB_ZCL_ATTR_ACCESS_REPORTING, custom_attr1_value);
    esp_zb_custom_cluster_add_custom_attr(custom_cluster, custom_attr2_id, ESP_ZB_ZCL_ATTR_TYPE_U16, ESP_ZB_ZCL_ATTR_ACCESS_READ_WRITE,
                                        &custom_attr2_value);

    /* Mandatory clusters */
    uint8_t mains_power;
    mains_power = 0x01;

    esp_zb_attribute_list_t *esp_zb_basic_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_BASIC);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, ESP_MANUFACTURER_NAME);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, ESP_MODEL_IDENTIFIER);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_POWER_SOURCE_ID, &mains_power);
    esp_zb_cluster_list_add_basic_cluster(cluster_list, esp_zb_basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    

    /* identify cluster create with fully customized */
    // esp_zb_attribute_list_t *esp_zb_identify_cluster = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY);
    // esp_zb_identify_cluster_add_attr(esp_zb_identify_cluster, custom_attr1_id, &custom_attr1_value);
    // esp_zb_identify_cluster_add_attr(esp_zb_identify_cluster, custom_attr2_id, &custom_attr2_value);
    
    // esp_zb_cluster_list_add_identify_cluster(cluster_list, esp_zb_identify_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(cluster_list, esp_zb_identify_cluster_create(NULL), ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    
    
    
    
    /* Custom cluster */
    esp_zb_cluster_list_add_custom_cluster(cluster_list, custom_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);


    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = CUSTOM_SERVER_ENDPOINT,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_CUSTOM_ATTR_DEVICE_ID,
        .app_device_version = 0,
    };
    esp_zb_ep_list_add_ep(ep_list, cluster_list, endpoint_config);
    esp_zb_device_register(ep_list);

    // Removed: Test std light configuration
    // Create a standard light configuration
    // esp_zb_on_off_light_cfg_t light_cfg = ESP_ZB_DEFAULT_ON_OFF_LIGHT_CONFIG();
    // esp_zb_ep_list_t *esp_zb_light_test_ep = esp_zb_on_off_light_ep_create(HA_ESP_LIGHT_ENDPOINT, &light_cfg);
    // zcl_basic_manufacturer_info_t info = {
    //     .manufacturer_name = ESP_MANUFACTURER_NAME,
    //     .model_identifier = ESP_MODEL_IDENTIFIER,
    // };
    // esp_zcl_utility_add_ep_basic_manufacturer_info(esp_zb_light_test_ep, HA_ESP_LIGHT_ENDPOINT, &info);
    // esp_zb_device_register(esp_zb_light_test_ep);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));

    // To receive some arbitrary data we can create a custom cluster https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32/user-guide/zcl_custom.html
    // then receive ESP_ZB_ZCL_ATTR_TYPE_OCTET_STRING

    // This appears to be the same function call for all device types, including
    // Zigbee Router and Zigbee End Device based on the samples
    esp_zb_stack_main_loop();
}

void app_main(void) 
{
    // Nothing strange in these configurations - standard radio and disable host connection.
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    // Init storage // TODO: Is this actually necessary? What happens if you don't do this at all?
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    // Create a "Task", which appears to be a FreeRTOS concept - 
    // https://docs.espressif.com/projects/esp-idf/en/v4.3/esp32/api-reference/system/freertos.html#_CPPv411xTaskCreate14TaskFunction_tPCKcK8uint32_tPCv11UBaseType_tPC12TaskHandle_t
    xTaskCreate(
        esp_zb_task,    // The loop
        "Zigbee_main",  // Task name
        4096,           // Stack depth
        NULL,           // pvParameters, whatever that is
        5,              // uxPriority
        NULL);          // handle
}