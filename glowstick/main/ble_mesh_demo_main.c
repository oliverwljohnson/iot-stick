/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* 

    BLE Mesh Glowstick Node Code
        - Gen User Data Server Model

*/

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "board.h"

#define TAG "ble_mesh_glowstick_node"

#define CID_ESP 0x02E5

//extern struct _led_state led_state[4];
typedef enum {EDM, Classical, Rock, None} GENRE; // Type for music selections 
GENRE my_genre;
// static enum genre my_selection = EDM;
//static uint8_t test_data = 1;

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

static esp_ble_mesh_client_t genre_client; // Music genre Client
static esp_ble_mesh_model_pub_t genre_model_pub = {
    .msg = NET_BUF_SIMPLE(2 + 1),
    .update = NULL,
    .dev_role = ROLE_PROVISIONER,
}; // Music genre model publication type

/*
static esp_ble_mesh_client_t genre_client;

// Model publication context 
static esp_ble_mesh_model_pub_t user_property_pub = { 
     Publication Buffer, containing the publication msg. Can be created by NET_BUF_SIMPLE 
    when the publication context is defined. 
    .msg = NET_BUF_SIMPLE(16), 
    .update = NULL, // A callback used only by the BLE Mesh layer, not the application layer. 
    .dev_role = ROLE_NODE, // Role of the device that is going to publish messages
};
*/

/* Opcodes defined for this demo, which demonstrate the 3 States of this Model.
    3 variables are needed for these definitions:
        - Opcode: the opcode that corresponds to each State, should be 1-2 bytes for SIG Models, 3 bytes for Vendor Models.
        - Min Length: the minimum length of the messages received by the State. E.g. OnOff Get State is 0 bytes, OnOff Set is 2 bytes (for some reason)
        - param_cb: Just for the BLE Mesh protocol, applications should just initialise to 0.  
 */
static esp_ble_mesh_model_op_t user_property_op[] = {
    { ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET, 2, 0}, // User Property Get
    { ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET, 2, 0}, // User Property Set
    { ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET_UNACK, 2, 0}, // User Property Set Unacknowledged
    /* Each model operation struct array must use this terminator
     * as the end tag of the operation uint. */
    ESP_BLE_MESH_MODEL_OP_END,
};

/* Array containing different Model definitions, which are defined using Macros which depend 
on where the Model has been implemented (e.g. Vendor, SIG or ESP BLE Mesh Stack) */
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server), // Configuration Server Model, implemented in ESP BLE Mesh Stack - left unchanged from demo
    ESP_BLE_MESH_MODEL_GEN_PROPERTY_CLI(&genre_model_pub, &genre_client) ,// Music genre client model
    ESP_BLE_MESH_SIG_MODEL(ESP_BLE_MESH_MODEL_ID_GEN_USER_PROP_SRV, user_property_op,
&genre_model_pub, &my_genre),
};

// The definition of an Element
/* The ESP_BLE_MESH_ELEMENT macro accepts 3 inputs:
    _loc - The Location Descriptor defined by SIG (set to 0 in the demo?) 
    _mods - The pointer to the definition of the defined SIG Models
    _vnd_mods - The pointer to the definition of the defined Vender Models (set to NONE in this case) */
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

/* Related to the composition data - "Composition Data is a State value which belongs to the 
Configuration Server Model and is always present in all Primary Elements." Contains info about the node,
it's elements and the features it supports. */
static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

/* Disable OOB security for SILabs Android app */
static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
#if 0
    .output_size = 4,
    .output_actions = ESP_BLE_MESH_DISPLAY_NUMBER,
    .input_actions = ESP_BLE_MESH_PUSH,
    .input_size = 4,
#else
    .output_size = 0,
    .output_actions = 0,
#endif
}; 

static int output_number(esp_ble_mesh_output_action_t action, uint32_t number)
{
    board_output_number(action, number);
    return 0;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    board_prov_complete();
}

/* Called when the Model callback is called and the event opcode corresponds with the User_Property_Get Model Operation */
static void gen_user_prop_get_handler(esp_ble_mesh_model_t *model,
                                  esp_ble_mesh_msg_ctx_t *ctx,
                                  uint16_t length, uint8_t *data)
{
    ESP_LOGI(TAG, " User Property Get handler called..");

    ESP_LOGI(TAG," User property received is.. %d", *data);
}

/* Called when the Model callback is called and the event opcode corresponds with the User_Property_Set Model Operation */
static void gen_user_prop_set_handler(esp_ble_mesh_model_t *model,
                                  esp_ble_mesh_msg_ctx_t *ctx,
                                  uint16_t length, uint8_t *data)
{
    ESP_LOGE(TAG, " User Property Set handler called..");
}

/* Called when the Model callback is called and the event opcode corresponds with the 
User_Property_Set Unacknowledged Model Operation */
static void gen_user_prop_set_unack_handler(esp_ble_mesh_model_t *model,
                                  esp_ble_mesh_msg_ctx_t *ctx,
                                  uint16_t length, uint8_t *data)
{
    ESP_LOGE(TAG, " User Property Set Unack handler called..");
}


/* Provides more readable outputs for errors relating to the BLE Mesh Configuration process (in esp_ble_mesh_prov_cb) */
static char *esp_ble_mesh_prov_event_to_str(esp_ble_mesh_prov_cb_event_t event)
{
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        return "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT";
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        return "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT";
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        return "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT";
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        return "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT";
    case ESP_BLE_MESH_NODE_PROV_OUTPUT_NUMBER_EVT:
        return "ESP_BLE_MESH_NODE_PROV_OUTPUT_NUMBER_EVT";
    case ESP_BLE_MESH_NODE_PROV_OUTPUT_STRING_EVT:
        return "ESP_BLE_MESH_NODE_PROV_OUTPUT_STRING_EVT";
    case ESP_BLE_MESH_NODE_PROV_INPUT_EVT:
        return "ESP_BLE_MESH_NODE_PROV_INPUT_EVT";
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        return "ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT";
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        return "ESP_BLE_MESH_NODE_PROV_RESET_EVT";
    default:
        return "Invalid BLE Mesh provision event";
    }

    return NULL;
}

/* This callback function executes during the BLE Mesh network configuration process, 
which allows the BLE Mesh Stack to notify the application layer about the important network configuration 
processes in the form of events. */
static void esp_ble_mesh_prov_cb(esp_ble_mesh_prov_cb_event_t event,
                                 esp_ble_mesh_prov_cb_param_t *param)
{
    ESP_LOGI(TAG, "%s, event = %s", __func__, esp_ble_mesh_prov_event_to_str(event)); // Logging
    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT: //This is to notify whether the BLE Mesh application has been successfully initialized
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT: //The event received by the application layer when the provisioning is completed.
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT: // The event returned when provisioner and unprovisioned device establish a link.
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
                 param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT: // The event to notify the application layer that the link has been closed when BLE Mesh bottom-layer protocol sends or receives the message of The Link Close.
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
                 param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_OUTPUT_NUMBER_EVT: // The event received by the application layer when output_actions is set as ESP_BLE_MESH_DISPLAY_NUMBER, and the target peer sets input_actions as ESP_BLE_MESH_ENTER_NUMBER during the configuration process.
        output_number(param->node_prov_output_num.action, param->node_prov_output_num.number);
        break;
    case ESP_BLE_MESH_NODE_PROV_OUTPUT_STRING_EVT: // The event received by the application layer when output_actions is set as ESP_BLE_MESH_DISPLAY_STRING, and the target peer sets input_actions as ESP_BLE_MESH_ENTER_STRING during the configuration process.
        break;
    case ESP_BLE_MESH_NODE_PROV_INPUT_EVT: // The event received by the application layer when input_actions is set as anything except ESP_BLE_MESH_NO_INPUT during the configuration process.
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT: // The event received by the application layer when the provisioning is completed.
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr);
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT: // The event received by the application layer when the network reset is done.
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT: // Set the unprovisioned device name completion event 
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code); 
        break;
    default:
        break;
    }
    return;
}

/* Registers the model operation callback function. 
This callback function is used when the target peer operates the model state of the source peer 
after the BLE Mesh has completed the network configuration. */
static void esp_ble_mesh_model_cb(esp_ble_mesh_model_cb_event_t event,
                                  esp_ble_mesh_model_cb_param_t *param)
{
    ESP_LOGE(TAG, "Model Operation callback fired..");

    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT: { 
    /* The event triggered by the two scenarios below:
        Server Model receives Get Status or Set Status from Client Model;
        Client Model receives Status State from Server Model. */
        if (!param->model_operation.model || !param->model_operation.model->op || !param->model_operation.ctx) { // Parameter is null
            ESP_LOGE(TAG, "ESP_BLE_MESH_MODEL_OPERATION_EVT parameter is NULL");
            return;
        }
        switch (param->model_operation.opcode) { // Each case calls a different function (above) for their respective Model Operations
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET: // Get Status ?
            ESP_LOGE(TAG, "GEN USER PROP GET..");
            gen_user_prop_get_handler(param->model_operation.model, param->model_operation.ctx,
                                  param->model_operation.length, param->model_operation.msg);
            break;
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET: // Set Status ?
            gen_user_prop_set_handler(param->model_operation.model, param->model_operation.ctx,
                                  param->model_operation.length, param->model_operation.msg);
            break;
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET_UNACK: // Set Status without Acknowledgement 
            gen_user_prop_set_unack_handler(param->model_operation.model, param->model_operation.ctx,
                                        param->model_operation.length, param->model_operation.msg);
            break;

        case ESP_BLE_MESH_CLIENT_MODEL_RECV_PUBLISH_MSG_EVT: // Reveived Published Msg
            ESP_LOGI(TAG, "Recieved Published Msg evt fired....");
            break;
        default:
            break;
        }
        break;
    }
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT: // The event returned after the Server Model sends Status State by calling esp_ble_mesh_server_model_send_msg API.
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT: // The event returned after the application has completed calling esp_ble_mesh_model_publish_msg API to publish messages.
        break;
    default:
        break;
    }
}

static void esp_ble_mesh_generic_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                    esp_ble_mesh_generic_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "Mesh generic callback fired..");
    uint32_t opcode;
    //int err;

    ESP_LOGI(TAG, "%s: event is %d, error code is %d, opcode is 0x%x",
             __func__, event, param->error_code, param->params->opcode);

    opcode = param->params->opcode;

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT");
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET: onoff %d", param->status_cb.onoff_status.present_onoff);
            break;
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT");
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
            ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: onoff %d", param->status_cb.onoff_status.present_onoff);
            break;
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT");
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT: {
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT");
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: {
            //esp_ble_mesh_client_common_param_t common = {0};
            //esp_ble_mesh_generic_client_set_state_t set_state = {0};
            /* If failed to get the response of Generic OnOff Set, resend Generic OnOff Set  
            esp_ble_mesh_set_msg_common(&common, &set_state, onoff_client.model,
                                        ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET, remote_onoff);
            //err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "%s: Generic OnOff Set failed", __func__);
                return;
            } 
            */
            break;
        }
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
    return;
}

static esp_err_t ble_mesh_init(void)
{
    int err = 0;

    memcpy(dev_uuid + 2, esp_bt_dev_get_address(), ESP_BD_ADDR_LEN);

    /* Registers the provisioning callback function to the BLE Mesh Stack,
    allowing the BLE Mesh Stack to notify the application layer about events */
    esp_ble_mesh_register_prov_callback(esp_ble_mesh_prov_cb);

    /* Registers the model callback function, used when the target peer operates
    the Model State of the source peer after configuration of the mesh. */
    esp_ble_mesh_register_custom_model_callback(esp_ble_mesh_model_cb);

    esp_ble_mesh_register_generic_client_callback(esp_ble_mesh_generic_cb);

    // Calls BT Status Check - Returns Ok/Fail
    err = esp_ble_mesh_init(&provision, &composition);
    if (err) {
        ESP_LOGE(TAG, "Initializing mesh failed (err %d)", err);
        return err;
    }

    // Enables Advertising and Scan functions once the Mesh configuration is complete
    esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);

    ESP_LOGI(TAG, "BLE Mesh Node initialized"); // Logging

    // board_led_operation(LED_BOARD, LED_ON); // Turns on the Green LED

    return err;
}

static esp_err_t bluetooth_init(void) // Initialises the Bluetooth Stack
{
    esp_err_t ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s initialize controller failed", __func__);
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed", __func__);
        return ret;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed", __func__);
        return ret;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed", __func__);
        return ret;
    }

    return ret;
}

void app_main(void)
{
    int err; 

    ESP_LOGI(TAG, "Initializing...");

    board_init(); // Initialises the board/LEDs

    // Initialises the Bluetooth Protocol Stack
    err = bluetooth_init();
    if (err) {
        ESP_LOGE(TAG, "esp32_bluetooth_init failed (err %d)", err);
        return;
    }

    /* Initialize the Bluetooth Mesh Subsystem */
    err = ble_mesh_init();
    if (err) {
        ESP_LOGE(TAG, "Bluetooth mesh init failed (err %d)", err);
    }

}
