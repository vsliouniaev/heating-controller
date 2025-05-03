#include "esp_zigbee_core.h"

#define MAX_CHILDREN                    10                                   // the max amount of connected devices
#define INSTALLCODE_POLICY_ENABLE       false                                // enable the install code policy for security
#define ESP_ZB_PRIMARY_CHANNEL_MASK     ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK // primary channel mask use

// Basic manufacturer information Attribute values in ZCL string format - The string should started with its length
#define ESP_MANUFACTURER_NAME           "\x0A""vsliouniaev"
#define ESP_MODEL_IDENTIFIER            "\x0E""Heat Controller"

// Test-only things here
#define HA_ESP_LIGHT_ENDPOINT           10 

// Zigbee router configuration
#define ESP_ZB_ZR_CONFIG()                                      \
{                                                               \
    .esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER,                   \
    .install_code_policy = INSTALLCODE_POLICY_ENABLE,           \
    .nwk_cfg.zczr_cfg = {                                       \
        .max_children = MAX_CHILDREN,                           \
    },                                                          \
}

// Zigbee end-device configuration
#define ESP_ZB_ZED_CONFIG()                                     \
{                                                               \
    .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                       \
    .install_code_policy = INSTALLCODE_POLICY_ENABLE,           \
    .nwk_cfg.zed_cfg = {                                        \
        .ed_timeout = ED_AGING_TIMEOUT,                         \
        .keep_alive = ED_KEEP_ALIVE,                            \
    },                                                          \
}

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                           \
{                                                               \
    .radio_mode = ZB_RADIO_MODE_NATIVE,                         \
}

#define ESP_ZB_DEFAULT_HOST_CONFIG()                            \
{                                                               \
    .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,       \
}
