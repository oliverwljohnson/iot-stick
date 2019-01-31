/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* Modified 23/01/19 by David Horsley
* This device will act as both the provisioner and the client for the 
* network, taking the form of the speaker. 
*
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
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "board.h"

#define TAG "ble_mesh_provisioner"

#define CID_ESP         0x02E5
#define CID_NVAL        0xFFFF

#define PROVISIONER_OWN_ADDR    0x0001

#define MSG_SEND_TTL            3 // Time to Live (hope-counter)
#define MSG_SEND_REL            false // Shoud msg be relayed?
#define MSG_TIMEOUT             0 
#define MSG_ROLE                ROLE_PROVISIONER 

#define COMPOSITION_DATA_PAGE_0 0x00

#define ESP_BLE_MESH_APP_IDX    0x0000 // App Key Index
#define APP_KEY_OCTET           0x12

static uint8_t dev_uuid[16];
typedef enum {EDM, Classical, Rock, None} GENRE; // Type for music selections 

typedef struct {
    uint8_t  uuid[16]; // Univerally Unique ID
    uint16_t unicast; // Unicast addr
    uint8_t  elem_num; // Element number?
    // uint8_t  onoff; //OnOff data/state?
    GENRE genre; // enum stores selected music genre
} esp_ble_mesh_node_info_t;

// Array of info to be given to nodes once provisioned
static esp_ble_mesh_node_info_t nodes[CONFIG_BT_MESH_MAX_PROV_NODES] = {
    [0 ... (CONFIG_BT_MESH_MAX_PROV_NODES - 1)] = {
        .unicast = ESP_BLE_MESH_ADDR_UNASSIGNED,
        .elem_num = 0,
        .genre = None
    }
};

// Keys (App/Net) of this Provisioner
static struct esp_ble_mesh_key {
    uint16_t net_idx; // Net Key Index
    uint16_t app_idx; // App Key Index
    uint8_t  app_key[16]; // App Key ??
} prov_key;

// Mesh Client Model Contexts
static esp_ble_mesh_client_t config_client; // Configuration Client
// static esp_ble_mesh_client_t onoff_client; // OnOff Client
static esp_ble_mesh_client_t genre_client; // Music genre Client
static esp_ble_mesh_model_pub_t genre_model_pub; // Music genre model publication type

// Mesh Config Server Model Context
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
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20), // Network Transmit State
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20), // Relay retransmit state
};

// Create model instances
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server), // Config server model
    ESP_BLE_MESH_MODEL_CFG_CLI(&config_client), // Config client model
    ESP_BLE_MESH_MODEL_GEN_PROPERTY_CLI(&genre_model_pub, &genre_client) // Music genre client model
};

// define a BLE Mesh element within an array.
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements),
};

static esp_ble_mesh_prov_t provision = {
    .prov_uuid           = dev_uuid,
    .prov_unicast_addr   = PROVISIONER_OWN_ADDR,
    .prov_start_address  = 0x0005,
    .prov_attention      = 0x00,
    .prov_algorithm      = 0x00,
    .prov_pub_key_oob    = 0x00,
    .prov_pub_key_oob_cb = NULL,
    .prov_static_oob_val = NULL,
    .prov_static_oob_len = 0x00,
    .prov_input_num      = NULL,
    .prov_output_num     = NULL,
    .flags               = 0x00,
    .iv_index            = 0x00,
};

/* 
* Stores the data from a node in the array defined above.
* Stores: UUID, unicast addr, element number, selected genre.
*/
static esp_err_t esp_ble_mesh_store_node_info(const uint8_t uuid[16], uint16_t unicast,
        uint8_t elem_num, GENRE genre)
{
    int i; // Counter

    // If not a valid addr, throw an error
    if (!uuid || !ESP_BLE_MESH_ADDR_IS_UNICAST(unicast)) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Judge if the device has been provisioned before */

    for (i = 0; i < ARRAY_SIZE(nodes); i++) {
        // If it has been prov'd before
        if (!memcmp(nodes[i].uuid, uuid, 16)) {
            ESP_LOGW(TAG, "%s: reprovisioned device 0x%04x", __func__, unicast);
            nodes[i].unicast = unicast; // update its info
            nodes[i].elem_num = elem_num;
            nodes[i].genre = None;
            return ESP_OK;
        }
    }

    for (i = 0; i < ARRAY_SIZE(nodes); i++) {
        // if it hasn't been prov'd before
        if (nodes[i].unicast == ESP_BLE_MESH_ADDR_UNASSIGNED) {
            memcpy(nodes[i].uuid, uuid, 16);
            // store its info in the array
            nodes[i].unicast = unicast;
            nodes[i].elem_num = elem_num;
            nodes[i].genre = None;
            return ESP_OK;
        }
    }

    // Return FAIL if you somehow get here
    return ESP_FAIL; 
}

/*
* Returns the info about a node (if in the array)
*/
static esp_ble_mesh_node_info_t *esp_ble_mesh_get_node_info(uint16_t unicast)
{
    int i; // counter

    // if node if unprov'd (and hence unknown)
    if (!ESP_BLE_MESH_ADDR_IS_UNICAST(unicast)) {
        return NULL;
    }

    // check the array for matching nodes
    for (i = 0; i < ARRAY_SIZE(nodes); i++) {
        if (nodes[i].unicast <= unicast && 
        // if the array addr is less or equal to the new addr,
                nodes[i].unicast + nodes[i].elem_num > unicast) {
                // and the array addr + the elem number is greater than the new addr
            return &nodes[i]; // return the array info

            // This could just be a weird way to find array elements that have the same addr, as
            // the new node, but which attempts to exclude array elements that have the same
            // address but don't have any Elements (??)
        }
    }

    return NULL;
}

/*
* Sets the "common" params for a message: Opcode, model, Context, msg_timeout and msg_role.
*/
static esp_err_t esp_ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
        esp_ble_mesh_node_info_t *node,
        esp_ble_mesh_model_t *model, uint32_t opcode)
{
    if (!common || !node || !model) {
        return ESP_ERR_INVALID_ARG;
    }

    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = prov_key.net_idx;
    common->ctx.app_idx = prov_key.app_idx;
    common->ctx.addr = node->unicast;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->ctx.send_rel = MSG_SEND_REL;
    common->msg_timeout = MSG_TIMEOUT;
    common->msg_role = MSG_ROLE;

    return ESP_OK;
}

static esp_err_t prov_complete(int node_idx, const esp_ble_mesh_octet16_t uuid,
                               uint16_t unicast, uint8_t elem_num, uint16_t net_idx)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_cfg_client_get_state_t get_state = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    char name[10];
    int err;

    ESP_LOGI(TAG, "node index: 0x%x, unicast address: 0x%02x, element num: %d, netkey index: 0x%02x",
             node_idx, unicast, elem_num, net_idx);
    ESP_LOGI(TAG, "device uuid: %s", bt_hex(uuid, 16));

    sprintf(name, "%s%d", "NODE-", node_idx); // sets the name variable
    err = esp_ble_mesh_provisioner_set_node_name(node_idx, name); // sets the provisioned device's name
    if (err) {
        ESP_LOGE(TAG, "%s: Set node name failed", __func__);
        return ESP_FAIL;
    }

    // stores the prov'd node's info in the array
    err = esp_ble_mesh_store_node_info(uuid, unicast, elem_num, LED_OFF); 
    if (err) {
        ESP_LOGE(TAG, "%s: Store node info failed", __func__);
        return ESP_FAIL;
    }

    // set the node variable with the info from the prov'd device
    node = esp_ble_mesh_get_node_info(unicast);
    if (!node) {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return ESP_FAIL;
    }

    // Set the common msg variables and Op Code for COMP data get
    esp_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
    get_state.comp_data_get.page = COMPOSITION_DATA_PAGE_0;

    // Send Config Client Get state message
    err = esp_ble_mesh_config_client_get_state(&common, &get_state);
    if (err) {
        ESP_LOGE(TAG, "%s: Send config comp data get failed", __func__);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void prov_link_open(esp_ble_mesh_prov_bearer_t bearer)
{
    ESP_LOGI(TAG, "%s link open", bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
}

static void prov_link_close(esp_ble_mesh_prov_bearer_t bearer, uint8_t reason)
{
    ESP_LOGI(TAG, "%s link close, reason 0x%02x",
             bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT", reason);
}

static void esp_ble_mesh_recv_unprov_adv_pkt(const uint8_t addr[ESP_BD_ADDR_LEN], const esp_ble_addr_type_t addr_type,
        const uint8_t adv_type, const uint8_t dev_uuid[16],
        uint16_t oob_info, esp_ble_mesh_prov_bearer_t bearer)
{
    esp_ble_mesh_unprov_dev_add_t add_dev = {0};
    int err;

    /* Due to the API esp_ble_mesh_provisioner_set_dev_uuid_match, Provisioner will only
     * use this callback to report the devices, whose device UUID starts with 0xdd & 0xdd,
     * to the application layer.
     */

    ESP_LOGI(TAG, "address: %s, address type: %d, adv type: %d", bt_hex(addr, ESP_BD_ADDR_LEN), addr_type, adv_type);
    ESP_LOGI(TAG, "device uuid: %s", bt_hex(dev_uuid, 16));
    ESP_LOGI(TAG, "oob info: %d, bearer: %s", oob_info, (bearer & ESP_BLE_MESH_PROV_ADV) ? "PB-ADV" : "PB-GATT");

    memcpy(add_dev.addr, addr, ESP_BD_ADDR_LEN); // set addr_dev with addr
    add_dev.addr_type = (uint8_t)addr_type; // set add_dev.addr_type
    memcpy(add_dev.uuid, dev_uuid, 16); // set add_dev UUID
    add_dev.oob_info = oob_info; // set add_dev oob info
    add_dev.bearer = (uint8_t)bearer; // and the bearer
    /* Note: If unprovisioned device adv packets have not been received, we should not add
             device with ADD_DEV_START_PROV_NOW_FLAG set. */
    err = esp_ble_mesh_provisioner_add_unprov_dev(&add_dev,
            ADD_DEV_RM_AFTER_PROV_FLAG | ADD_DEV_START_PROV_NOW_FLAG | ADD_DEV_FLUSHABLE_DEV_FLAG);
    if (err) {
        ESP_LOGE(TAG, "%s: Add unprovisioned device into queue failed", __func__);
    }

    return;
}

/*
* Callback function for events relating to the provisioning functions of this device. 
*/ 
static void esp_ble_mesh_prov_cb(esp_ble_mesh_prov_cb_event_t event,
                                 esp_ble_mesh_prov_cb_param_t *param)
{
    switch (event) {
    case ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_ENABLE_COMP_EVT, err_code %d", param->provisioner_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_PROV_DISABLE_COMP_EVT, err_code %d", param->provisioner_prov_disable_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_OPEN_EVT:
        prov_link_open(param->provisioner_prov_link_open.bearer);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_LINK_CLOSE_EVT:
        prov_link_close(param->provisioner_prov_link_close.bearer, param->provisioner_prov_link_close.reason);
        break;
    case ESP_BLE_MESH_PROVISIONER_PROV_COMPLETE_EVT:
        prov_complete(param->provisioner_prov_complete.node_idx, param->provisioner_prov_complete.device_uuid,
                      param->provisioner_prov_complete.unicast_addr, param->provisioner_prov_complete.element_num,
                      param->provisioner_prov_complete.netkey_idx);
        break;
    case ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_UNPROV_DEV_COMP_EVT, err_code %d", param->provisioner_add_unprov_dev_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_DEV_UUID_MATCH_COMP_EVT, err_code %d", param->provisioner_set_dev_uuid_match_comp.err_code);
        break;
    case ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT: {
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_SET_NODE_NAME_COMP_EVT, err_code %d", param->provisioner_set_node_name_comp.err_code);
        if (param->provisioner_set_node_name_comp.err_code == ESP_OK) {
            const char *name = NULL;
            name = esp_ble_mesh_provisioner_get_node_name(param->provisioner_set_node_name_comp.node_index);
            if (!name) {
                ESP_LOGE(TAG, "Get node name failed");
                return;
            }
            ESP_LOGI(TAG, "Node %d name is: %s", param->provisioner_set_node_name_comp.node_index, name);
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT: {
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_ADD_LOCAL_APP_KEY_COMP_EVT, err_code %d", param->provisioner_add_app_key_comp.err_code);
        if (param->provisioner_add_app_key_comp.err_code == ESP_OK) {
            esp_err_t err = 0;
            prov_key.app_idx = param->provisioner_add_app_key_comp.app_idx;
            err = esp_ble_mesh_provisioner_bind_app_key_to_local_model(PROVISIONER_OWN_ADDR, prov_key.app_idx,
                    ESP_BLE_MESH_MODEL_ID_GEN_PROP_CLI, CID_NVAL);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Provisioner bind local model appkey failed");
                return;
            }
        }
        break;
    }
    case ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROVISIONER_BIND_APP_KEY_TO_MODEL_COMP_EVT, err_code %d", param->provisioner_bind_app_key_to_model_comp.err_code);
        break;
    default:
        break;
    }

    return;
}

/*
* Callback function for model operation events
*/ 
static void esp_ble_mesh_model_cb(esp_ble_mesh_model_cb_event_t event,
                                  esp_ble_mesh_model_cb_param_t *param)
{
    ESP_LOGD(TAG, "Model callback fired..");
    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT:
        break;
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT:
        break;
    default:
        break;
    }
}

/*
* Callback function for config client events
*/ 
static void esp_ble_mesh_config_client_cb(esp_ble_mesh_cfg_client_cb_event_t event,
        esp_ble_mesh_cfg_client_cb_param_t *param)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    uint32_t opcode;
    uint16_t addr;
    int err;

    opcode = param->params->opcode;
    addr = param->params->ctx.addr;

    ESP_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
             __func__, param->error_code, event, param->params->ctx.addr, opcode);

    if (param->error_code) {
        ESP_LOGE(TAG, "Send config client message failed, opcode 0x%04x", opcode);
        return;
    }

    node = esp_ble_mesh_get_node_info(addr);
    if (!node) {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_CFG_CLIENT_GET_STATE_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
            ESP_LOGI(TAG, "composition data %s", bt_hex(param->status_cb.comp_data_status.composition_data->data,
                     param->status_cb.comp_data_status.composition_data->len));
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            esp_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set_state.app_key_add.net_idx = prov_key.net_idx;
            set_state.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
                return;
            }
            break;
        }
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_SET_STATE_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD: {
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            esp_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set_state.model_app_bind.element_addr = node->unicast;
            set_state.model_app_bind.model_app_idx = prov_key.app_idx;
            set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_GEN_PROP_CLI;
            set_state.model_app_bind.company_id = CID_NVAL;
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND: {
            esp_ble_mesh_generic_client_get_state_t get_state = {0};
            esp_ble_mesh_set_msg_common(&common, node, genre_client.model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTIES_GET);
            err = esp_ble_mesh_generic_client_get_state(&common, &get_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Generic User Properties Get failed", __func__);
                return;
            }
            break;
        }
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_PUBLISH_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_STATUS:
            ESP_LOG_BUFFER_HEX("composition data %s", param->status_cb.comp_data_status.composition_data->data,
                               param->status_cb.comp_data_status.composition_data->len);
            break;
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_STATUS:
            break;
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_CFG_CLIENT_TIMEOUT_EVT:
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET: {
            esp_ble_mesh_cfg_client_get_state_t get_state = {0};
            esp_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_COMPOSITION_DATA_GET);
            get_state.comp_data_get.page = COMPOSITION_DATA_PAGE_0;
            err = esp_ble_mesh_config_client_get_state(&common, &get_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config Composition Data Get failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD: {
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            esp_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD);
            set_state.app_key_add.net_idx = prov_key.net_idx;
            set_state.app_key_add.app_idx = prov_key.app_idx;
            memcpy(set_state.app_key_add.app_key, prov_key.app_key, 16);
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config AppKey Add failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND: {
            esp_ble_mesh_cfg_client_set_state_t set_state = {0};
            esp_ble_mesh_set_msg_common(&common, node, config_client.model, ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND);
            set_state.model_app_bind.element_addr = node->unicast;
            set_state.model_app_bind.model_app_idx = prov_key.app_idx;
            set_state.model_app_bind.model_id = ESP_BLE_MESH_MODEL_ID_GEN_USER_PROP_SRV;
            set_state.model_app_bind.company_id = CID_NVAL;
            err = esp_ble_mesh_config_client_set_state(&common, &set_state);
            if (err) {
                ESP_LOGE(TAG, "%s: Config Model App Bind failed", __func__);
                return;
            }
            break;
        }
        default:
            break;
        }
        break;
    default:
        ESP_LOGE(TAG, "Not a config client status message event");
        break;
    }
}

/*
* Callback function for generic client events
*/
static void esp_ble_mesh_generic_client_cb(esp_ble_mesh_generic_client_cb_event_t event,
        esp_ble_mesh_generic_client_cb_param_t *param)
{
    esp_ble_mesh_client_common_param_t common = {0};
    esp_ble_mesh_node_info_t *node = NULL;
    uint32_t opcode;
    uint16_t addr;
    int err;

    opcode = param->params->opcode;
    addr = param->params->ctx.addr;

    ESP_LOGI(TAG, "%s, error_code = 0x%02x, event = 0x%02x, addr: 0x%04x, opcode: 0x%04x",
             __func__, param->error_code, event, param->params->ctx.addr, opcode);

    if (param->error_code) {
        ESP_LOGE(TAG, "Send generic client message failed, opcode 0x%04x", opcode);
        return;
    }

    node = esp_ble_mesh_get_node_info(addr);
    if (!node) {
        ESP_LOGE(TAG, "%s: Get node info failed", __func__);
        return;
    }

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        switch (opcode) {
            case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTIES_GET: { // User prop get
                esp_ble_mesh_generic_client_set_state_t set_state = {0};
                //node->onoff = param->status_cb.onoff_status.present_onoff;
                node->genre = (GENRE) param->status_cb.user_property_status.property_value->data; // just copied the line above
                ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_USER_PROP_GET genre: 0x%02x", node->genre);
                /* After Generic OnOff Status for Generic OnOff Get is received, Generic OnOff Set will be sent */
                esp_ble_mesh_set_msg_common(&common, node, genre_client.model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET);
                // set_state.user_property_set.op_en = false;
                set_state.user_property_set.property_value = (int) None;
                set_state.user_property_set.property_id = 0;
                err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
                if (err) {
                    ESP_LOGE(TAG, "%s: Generic Genre Set failed", __func__);
                    return;
                }
                break;
            }
            default:
                break;
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        switch (opcode) {
            case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET: //ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: 
                //genre temp_dst; 
                //memcpy(temp_dst, param->status_cb.user_property_status.property_value->data, param->status_cb.user_property_status.property_value->len);
                node->genre = (GENRE) param->status_cb.user_property_status.property_value->data;
                ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_USER_PROP_SET genre: 0x%02x", node->genre);
                break;
            default:
                break;
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT:
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_TIMEOUT_EVT:
        /* If failed to receive the responses, these messages will be resend */
        switch (opcode) {
            case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET: { // ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET: {
                esp_ble_mesh_generic_client_get_state_t get_state = {0};
                esp_ble_mesh_set_msg_common(&common, node, genre_client.model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET);
                err = esp_ble_mesh_generic_client_get_state(&common, &get_state);
                if (err) {
                    ESP_LOGE(TAG, "%s: Generic Genre Get failed", __func__);
                    return;
                }
                break;
            }
            case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET: { // ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: {
                esp_ble_mesh_generic_client_set_state_t set_state = {0};
                node->genre = (GENRE) param->status_cb.user_property_status.property_value->data;
                // ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_USER_PROP_SET genre: %s", (String) node->genre);
                esp_ble_mesh_set_msg_common(&common, node, genre_client.model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET);
                //set_state.user_property_set.op_en = false; // are optional params included?
                //set_state.user_property_set = None;
                // set_state.user_property_set.property_id = 0;
                err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
                if (err) {
                    ESP_LOGE(TAG, "%s: Generic Genre Set failed", __func__);
                    return;
                }
                break;
            }
            default:
                break;
        }
        break;
    default:
        ESP_LOGE(TAG, "Not a generic client status message event");
        break;
    }
}

/*
* Initiates the BLE Mesh stack
*/
static int ble_mesh_init(void)
{
    uint8_t match[2] = {0xdd, 0xdd};
    int err = 0;

    prov_key.net_idx = ESP_BLE_MESH_KEY_PRIMARY;
    prov_key.app_idx = ESP_BLE_MESH_APP_IDX;
    memset(prov_key.app_key, APP_KEY_OCTET, sizeof(prov_key.app_key));

    memcpy(dev_uuid, esp_bt_dev_get_address(), ESP_BD_ADDR_LEN);

    esp_ble_mesh_register_prov_callback(esp_ble_mesh_prov_cb);
    esp_ble_mesh_register_custom_model_callback(esp_ble_mesh_model_cb);
    esp_ble_mesh_register_config_client_callback(esp_ble_mesh_config_client_cb);
    esp_ble_mesh_register_generic_client_callback(esp_ble_mesh_generic_client_cb);
    esp_ble_mesh_register_unprov_adv_pkt_callback(esp_ble_mesh_recv_unprov_adv_pkt);

    esp_ble_mesh_provisioner_set_dev_uuid_match(match, sizeof(match), 0x0, false);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err) {
        ESP_LOGE(TAG, "Initializing mesh failed (err %d)", err);
        return err;
    }

    esp_ble_mesh_provisioner_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);

    esp_ble_mesh_provisioner_add_local_app_key(prov_key.app_key, prov_key.net_idx, prov_key.app_idx);

    ESP_LOGI(TAG, "Provisioner initialized");

    return err;
}

/*
* Initiates the bluetooth stack
*/
static esp_err_t bluetooth_init(void)
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

/*
* Main program entry point
*/
void app_main(void)
{
    int err;

    ESP_LOGI(TAG, "Initializing...");

    board_init();

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
