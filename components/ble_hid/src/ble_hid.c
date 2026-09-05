#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"

#include "ble_hid.h"

static const char *TAG = "BLE_HID";

#define DEVICE_NAME "ESP32 Multi-HID"

// GATT Service UUIDs
#define BLE_UUID_GAP                0x1800
#define BLE_UUID_GATT               0x1801
#define BLE_UUID_HID_SERVICE        0x1812

// GATT Characteristic UUIDs
#define BLE_UUID_HID_INFO           0x2A4A
#define BLE_UUID_REPORT_MAP         0x2A4B
#define BLE_UUID_HID_CONTROL_POINT  0x2A4C
#define BLE_UUID_PROTOCOL_MODE      0x2A4E
#define BLE_UUID_HID_REPORT         0x2A4D

// GATT Descriptor UUIDs
#define BLE_UUID_CLIENT_CHAR_CFG    0x2902
#define BLE_UUID_REPORT_REF         0x2908

// HID Profile Attributes Table Index
enum {
    HID_SVC_IDX,
    
    // Protocol Mode
    HID_PROTOCOL_MODE_CHAR_IDX,
    HID_PROTOCOL_MODE_VAL_IDX,

    // HID Info
    HID_INFO_CHAR_IDX,
    HID_INFO_VAL_IDX,

    // Control Point
    HID_CONTROL_POINT_CHAR_IDX,
    HID_CONTROL_POINT_VAL_IDX,

    // Report Map
    HID_REPORT_MAP_CHAR_IDX,
    HID_REPORT_MAP_VAL_IDX,

    // Keyboard Input Report (Report ID 1)
    HID_KB_REPORT_CHAR_IDX,
    HID_KB_REPORT_VAL_IDX,
    HID_KB_REPORT_CCCD_IDX,
    HID_KB_REPORT_REF_IDX,

    // Consumer Control Input Report (Report ID 2)
    HID_CC_REPORT_CHAR_IDX,
    HID_CC_REPORT_VAL_IDX,
    HID_CC_REPORT_CCCD_IDX,
    HID_CC_REPORT_REF_IDX,

    // Mouse Input Report (Report ID 4)
    HID_MOUSE_REPORT_CHAR_IDX,
    HID_MOUSE_REPORT_VAL_IDX,
    HID_MOUSE_REPORT_CCCD_IDX,
    HID_MOUSE_REPORT_REF_IDX,

    // LaunchPad 16-Button Matrix Input Report (Report ID 3)
    HID_LP_REPORT_CHAR_IDX,
    HID_LP_REPORT_VAL_IDX,
    HID_LP_REPORT_CCCD_IDX,
    HID_LP_REPORT_REF_IDX,

    HID_IDX_MAX
};

static uint16_t hid_handle_table[HID_IDX_MAX];
static uint16_t gatts_if_for_app = ESP_GATT_IF_NONE;
static uint16_t conn_id = 0;
static bool is_connected = false;

// Flags to synchronize advertising start
#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)
#define PROFILE_CONFIG_FLAG         (1 << 2)
static uint8_t adv_config_done = 0;

/**
 * Raw Advertising Data (Total: 20 bytes <= 31 bytes limit)
 * - Flags (3 bytes)
 * - Complete 16-bit Service UUID: HID Service 0x1812 (4 bytes)
 * - Appearance: Generic HID 0x03C0 (4 bytes)
 * - Shortened Local Name: "ESP-HID" (9 bytes)
 */
static uint8_t raw_adv_data[] = {
    0x02, 0x01, 0x06,
    0x03, 0x03, 0x12, 0x18,
    0x03, 0x19, 0xc0, 0x03,
    0x08, 0x08, 'E', 'S', 'P', '-', 'H', 'I', 'D'
};

/**
 * Raw Scan Response Data (Total: 17 bytes <= 31 bytes limit)
 * - Complete Local Name: "ESP32 Multi-HID" (17 bytes)
 */
static uint8_t raw_scan_rsp_data[] = {
    0x10, 0x09, 'E', 'S', 'P', '3', '2', ' ', 'M', 'u', 'l', 't', 'i', '-', 'H', 'I', 'D'
};

// Composite Multi-function HID Report Map Descriptor (Keyboard + Consumer + LaunchPad/Gamepad + Mouse)
static const uint8_t hid_report_map[] = {
    // ==========================================
    // Report ID 1: Keyboard (8-byte standard report)
    // ==========================================
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,       // USAGE (Keyboard)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x01,       //   REPORT_ID (1)
    0x05, 0x07,       //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,       //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,       //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x95, 0x08,       //   REPORT_COUNT (8)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x95, 0x01,       //   REPORT_COUNT (1) - Reserved padding byte
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x81, 0x03,       //   INPUT (Cnst,Var,Abs)
    0x95, 0x06,       //   REPORT_COUNT (6) - 6 Keycodes
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x65,       //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,       //   USAGE_PAGE (Keyboard)
    0x19, 0x00,       //   USAGE_MINIMUM (0)
    0x29, 0x65,       //   USAGE_MAXIMUM (101)
    0x81, 0x00,       //   INPUT (Data,Ary,Abs)
    0xc0,             // END_COLLECTION

    // ==========================================
    // Report ID 2: Consumer Control (Media Keys)
    // ==========================================
    0x05, 0x0c,       // USAGE_PAGE (Consumer Devices)
    0x09, 0x01,       // USAGE (Consumer Control)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x02,       //   REPORT_ID (2)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x95, 0x07,       //   REPORT_COUNT (7)
    0x09, 0xb5,       //   USAGE (Scan Next Track)
    0x09, 0xb6,       //   USAGE (Scan Previous Track)
    0x09, 0xb7,       //   USAGE (Stop)
    0x09, 0xcd,       //   USAGE (Play/Pause)
    0x09, 0xe2,       //   USAGE (Mute)
    0x09, 0xe9,       //   USAGE (Volume Increment)
    0x09, 0xea,       //   USAGE (Volume Decrement)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x95, 0x01,       //   REPORT_COUNT (1) - 1 bit padding
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x81, 0x03,       //   INPUT (Cnst,Var,Abs)
    0xc0,             // END_COLLECTION

    // ==========================================
    // Report ID 3: LaunchPad 16-Button Matrix / Gamepad
    // ==========================================
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,       // USAGE (Game Pad)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x03,       //   REPORT_ID (3)
    0x05, 0x09,       //   USAGE_PAGE (Button)
    0x19, 0x01,       //   USAGE_MINIMUM (Button 1)
    0x29, 0x10,       //   USAGE_MAXIMUM (Button 16)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x95, 0x10,       //   REPORT_COUNT (16)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0xc0,             // END_COLLECTION

    // ==========================================
    // Report ID 4: Mouse (Buttons + X + Y + Wheel)
    // ==========================================
    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,       // USAGE (Mouse)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x04,       //   REPORT_ID (4)
    0x09, 0x01,       //   USAGE (Pointer)
    0xa1, 0x00,       //   COLLECTION (Physical)
    0x05, 0x09,       //     USAGE_PAGE (Button)
    0x19, 0x01,       //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,       //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,       //     LOGICAL_MINIMUM (0)
    0x25, 0x01,       //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,       //     REPORT_COUNT (3 buttons)
    0x75, 0x01,       //     REPORT_SIZE (1)
    0x81, 0x02,       //     INPUT (Data,Var,Abs)
    0x95, 0x01,       //     REPORT_COUNT (1) - 5 bit padding
    0x75, 0x05,       //     REPORT_SIZE (5)
    0x81, 0x03,       //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,       //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,       //     USAGE (X)
    0x09, 0x31,       //     USAGE (Y)
    0x09, 0x38,       //     USAGE (Wheel)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,       //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x95, 0x03,       //     REPORT_COUNT (3 axis: X, Y, Wheel)
    0x81, 0x06,       //     INPUT (Data,Var,Rel)
    0xc0,             //   END_COLLECTION
    0xc0              // END_COLLECTION
};

// Report Reference Constants (Report ID, Report Type: 0x01 = Input Report)
static const uint8_t report_ref_kb[2]    = {HID_REPORT_ID_KEYBOARD, 0x01};
static const uint8_t report_ref_cc[2]    = {HID_REPORT_ID_CONSUMER, 0x01};
static const uint8_t report_ref_lp[2]    = {HID_REPORT_ID_LAUNCHPAD, 0x01};
static const uint8_t report_ref_mouse[2] = {HID_REPORT_ID_MOUSE, 0x01};

// HID Info (bcdHID=1.11, bCountryCode=0, Flags=0x02 Remote Wakeup)
static const uint8_t hid_info[4] = {0x11, 0x01, 0x00, 0x02};
static const uint8_t protocol_mode = 0x01;
static const uint8_t control_point = 0x00;
static const uint16_t cccd_init = 0x0000;

// UUID Definitions
static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = BLE_UUID_CLIENT_CHAR_CFG;
static const uint16_t report_ref_uuid = BLE_UUID_REPORT_REF;

static const uint16_t hid_service_uuid = BLE_UUID_HID_SERVICE;

static const uint16_t protocol_mode_uuid = BLE_UUID_PROTOCOL_MODE;
static const uint16_t hid_info_uuid = BLE_UUID_HID_INFO;
static const uint16_t hid_control_point_uuid = BLE_UUID_HID_CONTROL_POINT;
static const uint16_t report_map_uuid = BLE_UUID_REPORT_MAP;
static const uint16_t report_uuid = BLE_UUID_HID_REPORT;

// Characteristic Properties
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_read_write_nr = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

// GATT Attribute Table
static const esp_gatts_attr_db_t gatt_db[HID_IDX_MAX] = {
    [HID_SVC_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(hid_service_uuid), (uint8_t *)&hid_service_uuid}},

    [HID_PROTOCOL_MODE_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_write_nr), (uint8_t *)&char_prop_read_write_nr}},
    [HID_PROTOCOL_MODE_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&protocol_mode_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint8_t), sizeof(protocol_mode), (uint8_t *)&protocol_mode}},

    [HID_INFO_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read), (uint8_t *)&char_prop_read}},
    [HID_INFO_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_info_uuid, ESP_GATT_PERM_READ,
      sizeof(hid_info), sizeof(hid_info), (uint8_t *)hid_info}},

    [HID_CONTROL_POINT_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_write_nr), (uint8_t *)&char_prop_read_write_nr}},
    [HID_CONTROL_POINT_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&hid_control_point_uuid, ESP_GATT_PERM_WRITE,
      sizeof(uint8_t), sizeof(control_point), (uint8_t *)&control_point}},

    [HID_REPORT_MAP_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read), (uint8_t *)&char_prop_read}},
    [HID_REPORT_MAP_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_map_uuid, ESP_GATT_PERM_READ,
      sizeof(hid_report_map), sizeof(hid_report_map), (uint8_t *)hid_report_map}},

    // ---------------- Keyboard Report (ID 1) ----------------
    [HID_KB_REPORT_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    [HID_KB_REPORT_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_uuid, ESP_GATT_PERM_READ,
      sizeof(keyboard_report_t), 0, NULL}},
    [HID_KB_REPORT_CCCD_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(cccd_init), (uint8_t *)&cccd_init}},
    [HID_KB_REPORT_REF_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_ref_uuid, ESP_GATT_PERM_READ,
      sizeof(report_ref_kb), sizeof(report_ref_kb), (uint8_t *)report_ref_kb}},

    // ---------------- Consumer Report (ID 2) ----------------
    [HID_CC_REPORT_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    [HID_CC_REPORT_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), 0, NULL}},
    [HID_CC_REPORT_CCCD_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(cccd_init), (uint8_t *)&cccd_init}},
    [HID_CC_REPORT_REF_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_ref_uuid, ESP_GATT_PERM_READ,
      sizeof(report_ref_cc), sizeof(report_ref_cc), (uint8_t *)report_ref_cc}},

    // ---------------- Mouse Report (ID 4) ----------------
    [HID_MOUSE_REPORT_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    [HID_MOUSE_REPORT_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_uuid, ESP_GATT_PERM_READ,
      sizeof(mouse_report_t), 0, NULL}},
    [HID_MOUSE_REPORT_CCCD_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(cccd_init), (uint8_t *)&cccd_init}},
    [HID_MOUSE_REPORT_REF_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_ref_uuid, ESP_GATT_PERM_READ,
      sizeof(report_ref_mouse), sizeof(report_ref_mouse), (uint8_t *)report_ref_mouse}},

    // ---------------- LaunchPad Matrix Report (ID 3) ----------------
    [HID_LP_REPORT_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    [HID_LP_REPORT_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), 0, NULL}},
    [HID_LP_REPORT_CCCD_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(cccd_init), (uint8_t *)&cccd_init}},
    [HID_LP_REPORT_REF_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_ref_uuid, ESP_GATT_PERM_READ,
      sizeof(report_ref_lp), sizeof(report_ref_lp), (uint8_t *)report_ref_lp}},
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min       = 0x20, // 20ms
    .adv_int_max       = 0x40, // 40ms
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void check_and_start_advertising(void) {
    uint8_t target_mask = (ADV_CONFIG_FLAG | SCAN_RSP_CONFIG_FLAG | PROFILE_CONFIG_FLAG);
    if ((adv_config_done & target_mask) == target_mask) {
        esp_err_t ret = esp_ble_gap_start_advertising(&adv_params);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "BLE ADVERTISING STARTED! Device is discoverable as '%s'", DEVICE_NAME);
        } else {
            ESP_LOGE(TAG, "BLE Advertising start failed: %s", esp_err_to_name(ret));
        }
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Raw Advertising Data Set Complete (status = %d)", param->adv_data_raw_cmpl.status);
            if (param->adv_data_raw_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                adv_config_done |= ADV_CONFIG_FLAG;
                check_and_start_advertising();
            }
            break;

        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "Raw Scan Response Data Set Complete (status = %d)", param->scan_rsp_data_raw_cmpl.status);
            if (param->scan_rsp_data_raw_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                adv_config_done |= SCAN_RSP_CONFIG_FLAG;
                check_and_start_advertising();
            }
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "🎉 BLE Advertising active! Ready to pair.");
            } else {
                ESP_LOGE(TAG, "BLE Advertising start failed, status = 0x%x", param->adv_start_cmpl.status);
            }
            break;

        case ESP_GAP_BLE_SEC_REQ_EVT:
            ESP_LOGI(TAG, "BLE Security Request from host, accepting pairing...");
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;

        case ESP_GAP_BLE_AUTH_CMPL_EVT:
            if (param->ble_security.auth_cmpl.success) {
                ESP_LOGI(TAG, "BLE Host Authenticated & Paired Successfully!");
            } else {
                ESP_LOGE(TAG, "BLE Authentication Failed, reason: 0x%x", param->ble_security.auth_cmpl.fail_reason);
            }
            break;

        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
            if (param->reg.status == ESP_GATT_OK) {
                gatts_if_for_app = gatts_if;
                esp_ble_gap_set_device_name(DEVICE_NAME);
                esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
                esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
                esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HID_IDX_MAX, 0);
            } else {
                ESP_LOGE(TAG, "GATTS App Register Failed, status = %d", param->reg.status);
            }
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status == ESP_GATT_OK) {
                memcpy(hid_handle_table, param->add_attr_tab.handles, sizeof(hid_handle_table));
                esp_ble_gatts_start_service(hid_handle_table[HID_SVC_IDX]);
                ESP_LOGI(TAG, "HID GATT Service started successfully!");
                adv_config_done |= PROFILE_CONFIG_FLAG;
                check_and_start_advertising();
            } else {
                ESP_LOGE(TAG, "Create Attribute Table failed, status = 0x%x", param->add_attr_tab.status);
            }
            break;

        case ESP_GATTS_CONNECT_EVT:
            conn_id = param->connect.conn_id;
            is_connected = true;
            ESP_LOGI(TAG, "BLE Host Connected! conn_id = %d", conn_id);
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_NO_MITM);
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            is_connected = false;
            ESP_LOGI(TAG, "BLE Host Disconnected. Restarting Advertising...");
            esp_ble_gap_start_advertising(&adv_params);
            break;

        default:
            break;
    }
}

esp_err_t ble_hid_init(void) {
    esp_err_t ret;

    // Release Classic BT memory (safe for ESP32 and no-op for BLE-only chips like ESP32-S3)
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(0);

    // SMP Security Configuration (BOND, Just Works, Key Distribution)
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    ESP_LOGI(TAG, "BLE HID Component Initialized");
    return ESP_OK;
}

bool ble_hid_is_connected(void) {
    return is_connected;
}

esp_err_t ble_hid_send_keyboard(uint8_t modifier, const uint8_t *key_codes, uint8_t key_count) {
    if (!is_connected) return ESP_ERR_INVALID_STATE;

    keyboard_report_t report;
    memset(&report, 0, sizeof(report));
    report.modifiers = modifier;

    if (key_codes && key_count > 0) {
        uint8_t count = (key_count > 6) ? 6 : key_count;
        memcpy(report.key_codes, key_codes, count);
    }

    return esp_ble_gatts_send_indicate(gatts_if_for_app, conn_id, 
                                       hid_handle_table[HID_KB_REPORT_VAL_IDX],
                                       sizeof(report), (uint8_t *)&report, false);
}

esp_err_t ble_hid_send_mouse(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) {
    if (!is_connected) return ESP_ERR_INVALID_STATE;

    mouse_report_t report = {
        .buttons = buttons,
        .x = x,
        .y = y,
        .wheel = wheel,
    };

    return esp_ble_gatts_send_indicate(gatts_if_for_app, conn_id,
                                       hid_handle_table[HID_MOUSE_REPORT_VAL_IDX],
                                       sizeof(report), (uint8_t *)&report, false);
}

esp_err_t ble_hid_send_consumer(uint8_t consumer_bitmask) {
    if (!is_connected) return ESP_ERR_INVALID_STATE;

    return esp_ble_gatts_send_indicate(gatts_if_for_app, conn_id,
                                       hid_handle_table[HID_CC_REPORT_VAL_IDX],
                                       sizeof(consumer_bitmask), &consumer_bitmask, false);
}

esp_err_t ble_hid_send_launchpad(uint16_t pad_mask) {
    if (!is_connected) return ESP_ERR_INVALID_STATE;

    return esp_ble_gatts_send_indicate(gatts_if_for_app, conn_id,
                                       hid_handle_table[HID_LP_REPORT_VAL_IDX],
                                       sizeof(pad_mask), (uint8_t *)&pad_mask, false);
}
