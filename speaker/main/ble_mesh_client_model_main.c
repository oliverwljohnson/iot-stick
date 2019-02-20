/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 * Additional Copyright (c) 2018 Espressif Systems (Shanghai) PTE LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"

#include "board.h"
#include "inttypes.h"

#define TAG "ble_mesh_client"

#define CID_ESP 0x02E5

#define MSG_SEND_TTL    3
#define MSG_SEND_REL    false
#define MSG_TIMEOUT     0   /* 0 indicates that timeout value from menuconfig will be used */
#define MSG_ROLE        ROLE_NODE

extern struct _led_state led_state[3];

static uint8_t dev_uuid[16] = { 0xdd, 0xdd };
static uint16_t node_net_idx = ESP_BLE_MESH_KEY_UNUSED;
static uint16_t node_app_idx = ESP_BLE_MESH_KEY_UNUSED;
static uint8_t remote_genre = LED_OFF;
static bool device_provisioned = false;

/* The remote node address shall be input through UART1, see board.c */
uint16_t remote_addr = ESP_BLE_MESH_ADDR_ALL_NODES;
void app_pub_msg_test(uint16_t);

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

/*
static esp_ble_mesh_model_pub_t genre_srv_pub = {
    .msg = NET_BUF_SIMPLE(2 + 1),
    .update = NULL,
    .dev_role = MSG_ROLE,
};
*/

static esp_ble_mesh_model_pub_t genre_cli_pub = {
    .msg = NET_BUF_SIMPLE(6 + 1),
    .update = NULL,
    .dev_role = MSG_ROLE,
    //.retransmit = ESP_BLE_MESH_TRANSMIT(2,20), // 3 transmission with 20ms intervals
    //.period = 100,
    //.period_start = 1,
};

static esp_ble_mesh_model_op_t genre_op[] = {
    { ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET, 0, 0},
    { ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET, 2, 0},
    { ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET_UNACK, 2, 0},
    /* Each model operation struct array must use this terminator
     * as the end tag of the operation uint. */
    ESP_BLE_MESH_MODEL_OP_END,
};

static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_PROPERTY_CLI(&genre_cli_pub, &genre_client),
};

static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE),
};

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

static void prov_complete(uint16_t net_idx, uint16_t addr)
{
    ESP_LOGI(TAG, "net_idx: 0x%04x, addr: 0x%04x", net_idx, addr);
    board_prov_complete();
    node_net_idx = net_idx;
    // app_pub_msg_test(remote_addr);
    uint8_t my_data = 0;
    int err = 0;
    device_provisioned = true;

    /*
    esp_ble_mesh_model_publish(genre_cli_pub.model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET,sizeof(my_data),&my_data,MSG_ROLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: Publish failed in prov_complete", __func__);
        return;
    }
    */
}

static void gen_genre_get_handler(esp_ble_mesh_model_t *model,
                                  esp_ble_mesh_msg_ctx_t *ctx,
                                  uint16_t length, uint8_t *data)
{
    ESP_LOGI(TAG, "Generic Genre Get Handler fired..");
    /*
    struct _led_state *led = (struct _led_state *)model->user_data;
    uint8_t send_data;
    esp_err_t err;

    ESP_LOGI(TAG, "%s, addr 0x%04x genre 0x%02x", __func__, model->element->element_addr, led->current);

    send_data = led->current;
    // Send Generic genre Status as a response to Generic genre Get 
    err = esp_ble_mesh_server_model_send_msg(model, ctx, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_STATUS,
            sizeof(send_data), &send_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: Generic genre Status failed", __func__);
        return;
    }
    */
}

static void gen_genre_set_unack_handler(esp_ble_mesh_model_t *model,
                                        esp_ble_mesh_msg_ctx_t *ctx,
                                        uint16_t length, uint8_t *data)
{
    ESP_LOGI(TAG, "Generic Genre Set Unack Handler fired..");
    /*
    struct _led_state *led = (struct _led_state *)model->user_data;
    uint8_t prev_genre;
    esp_err_t err;

    ESP_LOGI(TAG, "%s, addr 0x%02x genre 0x%02x", __func__, model->element->element_addr, led->current);

    prev_genre = led->previous;
    led->current = data[0];
    remote_genre = led->current;

    board_led_operation(led->pin, led->current);

     If Generic genre state is changed, and the publish address of Generic genre Server
     * model is valid, Generic genre Status will be published.
     
    if (prev_genre != led->current && model->pub->publish_addr != ESP_BLE_MESH_ADDR_UNASSIGNED) {
        ESP_LOGI(TAG, "Publish previous 0x%02x current 0x%02x", prev_genre, led->current);
        err = esp_ble_mesh_model_publish(model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_STATUS,
                                         sizeof(led->current), &led->current, ROLE_NODE);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "%s: Publish Generic genre Status failed", __func__);
            return;
        }
    }
    */
}

static void gen_genre_set_handler(esp_ble_mesh_model_t *model,
                                  esp_ble_mesh_msg_ctx_t *ctx,
                                  uint16_t length, uint8_t *data)
{
    ESP_LOGI(TAG, "Generic Genre Set Handler fired..");
    /*
    struct net_buf_simple *msg = model->pub->msg;
    struct _led_state *led = (struct _led_state *)model->user_data;
    uint8_t prev_genre, send_data;
    esp_err_t err;

    ESP_LOGI(TAG, "%s, addr 0x%02x genre 0x%02x", __func__, model->element->element_addr, led->current);

    prev_genre = led->previous;
    led->current = data[0];
    remote_genre = led->current;

    board_led_operation(led->pin, led->current);

    send_data = led->current;
    // Send Generic genre Status as a response to Generic genre Get 
    err = esp_ble_mesh_server_model_send_msg(model, ctx, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_STATUS,
            sizeof(send_data), &send_data);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: Generic genre Status failed", __func__);
        return;
    }

     If Generic genre state is changed, and the publish address of Generic genre Server
     model is valid, Generic genre Status will be published.
     
    if (prev_genre != led->current && model->pub->publish_addr != ESP_BLE_MESH_ADDR_UNASSIGNED) {
        ESP_LOGI(TAG, "Publish previous 0x%02x current 0x%02x", prev_genre, led->current);
        bt_mesh_model_msg_init(msg, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_STATUS);
        err = esp_ble_mesh_model_publish(model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_STATUS,
                                         sizeof(send_data), &send_data, ROLE_NODE);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "%s: Publish Generic genre Status failed", __func__);
            return;
        }
    }
    */
}

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

static void esp_ble_mesh_prov_cb(esp_ble_mesh_prov_cb_event_t event,
                                 esp_ble_mesh_prov_cb_param_t *param)
{
    ESP_LOGI(TAG, "%s, event = %s", __func__, esp_ble_mesh_prov_event_to_str(event));

    switch (event) {
    case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_PROV_REGISTER_COMP_EVT, err_code %d", param->prov_register_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT, err_code %d", param->node_prov_enable_comp.err_code);
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT, bearer %s",
                 param->node_prov_link_open.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT, bearer %s",
                 param->node_prov_link_close.bearer == ESP_BLE_MESH_PROV_ADV ? "PB-ADV" : "PB-GATT");
        break;
    case ESP_BLE_MESH_NODE_PROV_OUTPUT_NUMBER_EVT:
        output_number(param->node_prov_output_num.action, param->node_prov_output_num.number);
        break;
    case ESP_BLE_MESH_NODE_PROV_OUTPUT_STRING_EVT:
        break;
    case ESP_BLE_MESH_NODE_PROV_INPUT_EVT:
        break;
    case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
        prov_complete(param->node_prov_complete.net_idx, param->node_prov_complete.addr);
        //gen_genre_get_handler(genre_cli_pub.model,param->ct)
        break;
    case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
        break;
    case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT, err_code %d", param->node_set_unprov_dev_name_comp.err_code);
        break;
    default:
        break;
    }

    return;
}

static esp_err_t esp_ble_mesh_set_msg_common(esp_ble_mesh_client_common_param_t *common,
        esp_ble_mesh_generic_client_set_state_t *set,
        esp_ble_mesh_model_t *model, uint32_t opcode,
        uint8_t genre)
{
    if (!common || !set || !model) {
        return ESP_ERR_INVALID_ARG;
    }

    common->opcode = opcode;
    common->model = model;
    common->ctx.net_idx = node_net_idx;
    common->ctx.app_idx = node_app_idx;
    common->ctx.addr = remote_addr;
    common->ctx.send_ttl = MSG_SEND_TTL;
    common->ctx.send_rel = MSG_SEND_REL;
    common->msg_timeout = MSG_TIMEOUT;
    common->msg_role = MSG_ROLE;
    //set->genre_set.op_en = false;
    //set->genre_set.genre = genre;
    //set->genre_set.tid = 0x0;

    return ESP_OK;
}


static void esp_ble_mesh_model_cb(esp_ble_mesh_model_cb_event_t event,
                                  esp_ble_mesh_model_cb_param_t *param)
{
    ESP_LOGI(TAG, "Mesh Model callback fired..");

    esp_ble_mesh_client_common_param_t common = {0};
    int err;

    switch (event) {
    case ESP_BLE_MESH_MODEL_OPERATION_EVT: {
        if (!param->model_operation.model || !param->model_operation.model->op || !param->model_operation.ctx) {
            ESP_LOGE(TAG, "ESP_BLE_MESH_MODEL_OPERATION_EVT parameter is NULL");
            return;
        }
        if (node_app_idx == ESP_BLE_MESH_KEY_UNUSED) {
            /* Generic genre Server/Client Models need to bind with the same app key */
            node_app_idx = param->model_operation.ctx->app_idx;
        }
        switch (param->model_operation.opcode) {
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET:
            gen_genre_get_handler(param->model_operation.model, param->model_operation.ctx,
                                  param->model_operation.length, param->model_operation.msg);
            break;
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET: {
            esp_ble_mesh_generic_client_set_state_t set_state = {0};
            gen_genre_set_handler(param->model_operation.model, param->model_operation.ctx,
                                  param->model_operation.length, param->model_operation.msg);
            /* This node has a Generic genre Client and Server Model.
             * When Generic genre Server Model receives a Generic genre Set message, after
             * this message is been handled, the Generic genre Client Model will send the
             * Generic genre Set message to another node(contains Generic genre Server Model)
             * identified by the remote_addr.
             */
            esp_ble_mesh_set_msg_common(&common, &set_state, genre_client.model,
                                        ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET, remote_genre);
            err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "%s: Generic genre Set failed", __func__);
                return;
            }
            break;
        }
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET_UNACK: {
            esp_ble_mesh_generic_client_set_state_t set_state = {0};
            gen_genre_set_unack_handler(param->model_operation.model, param->model_operation.ctx,
                                        param->model_operation.length, param->model_operation.msg);
            /* This node has a Generic genre Client and Server Model.
             * When Generic genre Client Model receives a Generic genre Set Unack message,
             * after this message is been handled, the Generic genre Client Model will send
             * the Generic genre Set Unack message to another node(contains Generic genre
             * Server Model) identified by the remote_addr.
             */
            esp_ble_mesh_set_msg_common(&common, &set_state, genre_client.model,
                                        ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET_UNACK, remote_genre);
            err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "%s: Generic genre Set Unack failed", __func__);
                return;
            }
            break;
        }
        default:
            break;
        }
        break;
    }
    case ESP_BLE_MESH_MODEL_SEND_COMP_EVT:
        break;
    case ESP_BLE_MESH_MODEL_PUBLISH_COMP_EVT:
        break;
    default:
        break;
    }
    return;
}

static void esp_ble_mesh_generic_cb(esp_ble_mesh_generic_client_cb_event_t event,
                                    esp_ble_mesh_generic_client_cb_param_t *param)
{
    ESP_LOGI(TAG, "Mesh Generic callback fired..");

    uint32_t opcode;
    int err;

    ESP_LOGI(TAG, "%s: event is %d, error code is %d, opcode is 0x%x",
             __func__, event, param->error_code, param->params->opcode);

    opcode = param->params->opcode;

    switch (event) {
    case ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT");
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET:
            //ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_USER_PROP_GET: genre %d", param->status_cb.genre_status.present_genre);
            ESP_LOGI(TAG,"GEN_USER_PROP_GET success..");
            break;
        default:
            break;
        }
        break;
    case ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT:
        ESP_LOGI(TAG, "ESP_BLE_MESH_GENERIC_CLIENT_SET_STATE_EVT");
        switch (opcode) {
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET:
            //ESP_LOGI(TAG, "ESP_BLE_MESH_MODEL_OP_GEN_USER_PROP_SET: genre %d", param->status_cb.genre_status.present_genre);
            ESP_LOGI(TAG,"GEN_USER_PROP_SET success..");
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
        case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET: {
            esp_ble_mesh_client_common_param_t common = {0};
            esp_ble_mesh_generic_client_set_state_t set_state = {0};
            /* If failed to get the response of Generic genre Set, resend Generic genre Set  */
            esp_ble_mesh_set_msg_common(&common, &set_state, genre_client.model,
                                        ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET, remote_genre);
            err = esp_ble_mesh_generic_client_set_state(&common, &set_state);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "%s: Generic genre Set failed", __func__);
                return;
            }
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

static int ble_mesh_init(void)
{
    int err = 0;

    memcpy(dev_uuid + 2, esp_bt_dev_get_address(), ESP_BD_ADDR_LEN);

    esp_ble_mesh_register_prov_callback(esp_ble_mesh_prov_cb);
    esp_ble_mesh_register_custom_model_callback(esp_ble_mesh_model_cb);
    esp_ble_mesh_register_generic_client_callback(esp_ble_mesh_generic_cb);

    err = esp_ble_mesh_init(&provision, &composition);
    if (err) {
        ESP_LOGE(TAG, "Initializing mesh failed (err %d)", err);
        return err;
    }

    esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);

    ESP_LOGI(TAG, "BLE Mesh node initialized");

    board_led_operation(LED_G, LED_ON);

    return err;
}

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
* Simple test for messages
*/
void app_pub_msg_test(uint16_t in_addr)
{
    if (device_provisioned) {
        esp_err_t error;
        enum genre {EDM, classical, rock}; // Defines the music genre selected by the user/glowstick
        enum genre my_selection = classical;

        uint8_t my_data = (uint8_t) my_selection;
        esp_ble_mesh_model_t *my_model = &root_models[1];

        esp_ble_mesh_model_msg_opcode_init(&my_data, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET);

        ESP_LOGI(TAG, "Attempting message publication..."); // debugging
        ESP_LOGI(TAG, "Data size..%d", sizeof(my_data));
        //printf("%" PRIu8, (my_data));
        error = esp_ble_mesh_model_publish(my_model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET, sizeof(my_data), &my_data, MSG_ROLE);
        // error = esp_ble_mesh_model_publish(my_model, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET, sizeof(my_data), my_data, MSG_ROLE);
        //error = esp_ble_mesh_client_model_send_msg(my_model, &my_ctx, ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET, sizeof(my_data), my_data, 1000, false, MSG_ROLE);
        if (error) {
            ESP_LOGE(TAG, " message publication failed"); // debugging
        }
        else {
            ESP_LOGI(TAG, " message publication succesful!"); // debugging
        }
    }

}

static void periodic_timer_callback(void* arg)
{
    int64_t time_since_boot = esp_timer_get_time();
    ESP_LOGI(TAG, "Periodic timer called, time since boot: %lld us", time_since_boot);
    app_pub_msg_test(remote_addr);
}

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

    //esp_ble_mesh_client_model_init(&root_models[1]);

    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &periodic_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
            .name = "periodic"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));   
    /* The timer has been created but is not running yet */

    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 5000000));
    ESP_LOGI(TAG, "Started timers, time since boot: %lld us", esp_timer_get_time());

    // This has been set to true to avoid having to provision the device before publishing but MUST be removed!
    //ESP_LOGE(TAG,"Testing publishing.. must remove!");
    //device_provisioned = true;
    // Remove this

}
