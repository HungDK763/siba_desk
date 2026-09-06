#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

// Name matching Xbox Wireless Controller triggers Bluepad32's built-in Xbox parser immediately!
#define DEVICE_NAME "Xbox Wireless Controller"

// GATT Service UUIDs
#define BLE_UUID_GAP                0x1800
#define BLE_UUID_GATT               0x1801
#define BLE_UUID_DEVICE_INFO        0x180A
#define BLE_UUID_HID_SERVICE        0x1812

// GATT Characteristic UUIDs
#define BLE_UUID_PNP_ID             0x2A50
#define BLE_UUID_MANUFACTURER_NAME  0x2A29
#define BLE_UUID_MODEL_NUMBER       0x2A24
#define BLE_UUID_HID_INFO           0x2A4A
#define BLE_UUID_REPORT_MAP         0x2A4B
#define BLE_UUID_HID_CONTROL_POINT  0x2A4C
#define BLE_UUID_PROTOCOL_MODE      0x2A4E
#define BLE_UUID_HID_REPORT         0x2A4D

// GATT Descriptor UUIDs
#define BLE_UUID_CLIENT_CHAR_CFG    0x2902
#define BLE_UUID_REPORT_REF         0x2908

// Device Information Service Attributes Table Index (0x180A)
enum {
    DEV_INFO_SVC_IDX,
    DEV_INFO_PNP_ID_CHAR_IDX,
    DEV_INFO_PNP_ID_VAL_IDX,
    DEV_INFO_MFR_CHAR_IDX,
    DEV_INFO_MFR_VAL_IDX,
    DEV_INFO_MODEL_CHAR_IDX,
    DEV_INFO_MODEL_VAL_IDX,
    DEV_INFO_IDX_MAX
};

// HID Service Attributes Table Index (0x1812)
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

    // 1. Xbox Gamepad Input Report (Report ID 1)
    HID_GP_REPORT_CHAR_IDX,
    HID_GP_REPORT_VAL_IDX,
    HID_GP_REPORT_CCCD_IDX,
    HID_GP_REPORT_REF_IDX,

    // 2. Keyboard Input Report (Report ID 2)
    HID_KB_REPORT_CHAR_IDX,
    HID_KB_REPORT_VAL_IDX,
    HID_KB_REPORT_CCCD_IDX,
    HID_KB_REPORT_REF_IDX,

    // 3. Mouse Input Report (Report ID 3)
    HID_MOUSE_REPORT_CHAR_IDX,
    HID_MOUSE_REPORT_VAL_IDX,
    HID_MOUSE_REPORT_CCCD_IDX,
    HID_MOUSE_REPORT_REF_IDX,

    // 4. Consumer Control Report (Report ID 4)
    HID_CC_REPORT_CHAR_IDX,
    HID_CC_REPORT_VAL_IDX,
    HID_CC_REPORT_CCCD_IDX,
    HID_CC_REPORT_REF_IDX,

    HID_IDX_MAX
};

// PC Sync Service Attributes Table Index (0xFFE0)
#define BLE_UUID_PC_SYNC_SVC        0xFFE0
#define BLE_UUID_PC_SYNC_CHAR       0xFFE1

enum {
    PC_SYNC_SVC_IDX,
    PC_SYNC_CHAR_IDX,
    PC_SYNC_VAL_IDX,
    PC_SYNC_IDX_MAX
};

static uint16_t dev_info_handle_table[DEV_INFO_IDX_MAX];
static uint16_t hid_handle_table[HID_IDX_MAX];
static uint16_t pc_sync_handle_table[PC_SYNC_IDX_MAX];
static ble_pc_sync_rx_cb_t s_pc_sync_cb = NULL;
static ble_connection_status_cb_t s_conn_status_cb = NULL;
static uint16_t gatts_if_for_app = ESP_GATT_IF_NONE;
static uint16_t conn_id = 0;
static bool is_connected = false;
static esp_bd_addr_t current_remote_bda = {0};

// Flags to synchronize advertising start
#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)
#define PROFILE_CONFIG_FLAG         (1 << 2)
static uint8_t adv_config_done = 0;

/**
 * Raw Advertising Data (Total: 17 bytes <= 31 bytes limit)
 * - Flags (3 bytes)
 * - 16-bit Service UUID: HID Service 0x1812 (4 bytes)                                                                            
 * - Appearance: 0x03C4 = Gamepad (4 bytes)
 * - Shortened Local Name: "Xbox" (6 bytes)
 */
static uint8_t raw_adv_data[] = {                                                      
    0x02, 0x01, 0x06,
    0x03, 0x03, 0x12, 0x18,
    0x03, 0x19, 0xc4, 0x03,
    0x05, 0x08, 'X', 'b', 'o', 'x'
};

/**
 * Raw Scan Response Data (Total: 26 bytes <= 31 bytes limit)
 * - Complete Local Name: "Xbox Wireless Controller" (26 bytes)
 */
static uint8_t raw_scan_rsp_data[] = {
    0x19, 0x09, 'X', 'b', 'o', 'x', ' ', 'W', 'i', 'r', 'e', 'l', 'e', 's', 's', ' ', 'C', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r'
};

// Composite HID Report Descriptor (Gamepad Report 1 + Keyboard Report 2 + Mouse Report 3 + Consumer Report 4) - 328 bytes
static const uint8_t hid_report_map[] = {
    // === 1. GAMEPAD (REPORT ID 1 - Xbox One Compatible) ===
    0x05, 0x01, 0x09, 0x05, 0xa1, 0x01, 0x85, 0x01, 0x09, 0x01, 0xa1, 0x00, 0x09, 0x30, 0x09, 0x31,
    0x15, 0x00, 0x27, 0xff, 0xff, 0x00, 0x00, 0x95, 0x02, 0x75, 0x10, 0x81, 0x02, 0xc0, 0x09, 0x01,
    0xa1, 0x00, 0x09, 0x32, 0x09, 0x35, 0x15, 0x00, 0x27, 0xff, 0xff, 0x00, 0x00, 0x95, 0x02, 0x75,
    0x10, 0x81, 0x02, 0xc0, 0x05, 0x02, 0x09, 0xc5, 0x15, 0x00, 0x26, 0xff, 0x03, 0x95, 0x01, 0x75,
    0x0a, 0x81, 0x02, 0x15, 0x00, 0x25, 0x00, 0x75, 0x06, 0x95, 0x01, 0x81, 0x03, 0x05, 0x02, 0x09,
    0xc4, 0x15, 0x00, 0x26, 0xff, 0x03, 0x95, 0x01, 0x75, 0x0a, 0x81, 0x02, 0x15, 0x00, 0x25, 0x00,
    0x75, 0x06, 0x95, 0x01, 0x81, 0x03, 0x05, 0x01, 0x09, 0x39, 0x15, 0x01, 0x25, 0x08, 0x35, 0x00,
    0x46, 0x3b, 0x01, 0x66, 0x14, 0x00, 0x75, 0x04, 0x95, 0x01, 0x81, 0x42, 0x75, 0x04, 0x95, 0x01,
    0x15, 0x00, 0x25, 0x00, 0x35, 0x00, 0x45, 0x00, 0x65, 0x00, 0x81, 0x03, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x0f, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x0f, 0x81, 0x02, 0x15, 0x00, 0x25, 0x00,
    0x75, 0x01, 0x95, 0x01, 0x81, 0x03, 0x05, 0x0c, 0x0a, 0x24, 0x02, 0x15, 0x00, 0x25, 0x01, 0x95,
    0x01, 0x75, 0x01, 0x81, 0x02, 0x15, 0x00, 0x25, 0x00, 0x75, 0x07, 0x95, 0x01, 0x81, 0x03, 0xc0,

    // === 2. KEYBOARD (REPORT ID 2 - Standard 8-byte Keyboard) ===
    0x05, 0x01, 0x09, 0x06, 0xa1, 0x01, 0x85, 0x02, 0x05, 0x07, 0x19, 0xe0, 0x29, 0xe7, 0x15, 0x00,
    0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02, 0x95, 0x01, 0x75, 0x08, 0x81, 0x01, 0x95, 0x06,
    0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00, 0xc0,

    // === 3. MOUSE (REPORT ID 3 - Standard 4-byte Mouse) ===
    0x05, 0x01, 0x09, 0x02, 0xa1, 0x01, 0x85, 0x03, 0x09, 0x01, 0xa1, 0x00, 0x05, 0x09, 0x19, 0x01,
    0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x03, 0x81, 0x02, 0x75, 0x05, 0x95, 0x01,
    0x81, 0x03, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81, 0x25, 0x7f, 0x75, 0x08,
    0x95, 0x03, 0x81, 0x06, 0xc0, 0xc0,

    // === 4. CONSUMER CONTROL (REPORT ID 4 - Media Keys) ===
    0x05, 0x0c, 0x09, 0x01, 0xa1, 0x01, 0x85, 0x04, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08,
    0x09, 0xe9, 0x09, 0xea, 0x09, 0xe2, 0x09, 0xcd, 0x09, 0xb5, 0x09, 0xb6, 0x09, 0xb7, 0x09, 0xb8,
    0x81, 0x02, 0xc0
};

// Report Reference Descriptors {Report ID, Report Type (1=Input)}
static const uint8_t report_ref_gp[2]    = {HID_REPORT_ID_GAMEPAD, 0x01};
static const uint8_t report_ref_kb[2]    = {HID_REPORT_ID_KEYBOARD, 0x01};
static const uint8_t report_ref_mouse[2] = {HID_REPORT_ID_MOUSE, 0x01};
static const uint8_t report_ref_cc[2]    = {HID_REPORT_ID_CONSUMER, 0x01};

// PnP ID: VID = 0x045E (Microsoft), PID = 0x02E0 (Xbox One Controller), Version = 0x0100
static const uint8_t pnp_id_val[7] = {0x02, 0x5e, 0x04, 0xe0, 0x02, 0x00, 0x01};
static const char manufacturer_name[] = "Microsoft";
static const char model_number[] = "Xbox Wireless Controller";

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

static const uint16_t dev_info_service_uuid = BLE_UUID_DEVICE_INFO;
static const uint16_t pnp_id_uuid = BLE_UUID_PNP_ID;
static const uint16_t mfr_name_uuid = BLE_UUID_MANUFACTURER_NAME;
static const uint16_t model_num_uuid = BLE_UUID_MODEL_NUMBER;

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

// === Table 1: Device Information Service GATT Table (0x180A) ===
static const esp_gatts_attr_db_t dev_info_gatt_db[DEV_INFO_IDX_MAX] = {
    [DEV_INFO_SVC_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(dev_info_service_uuid), (uint8_t *)&dev_info_service_uuid}},

    [DEV_INFO_PNP_ID_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read), (uint8_t *)&char_prop_read}},
    [DEV_INFO_PNP_ID_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&pnp_id_uuid, ESP_GATT_PERM_READ,
      sizeof(pnp_id_val), sizeof(pnp_id_val), (uint8_t *)pnp_id_val}},

    [DEV_INFO_MFR_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read), (uint8_t *)&char_prop_read}},
    [DEV_INFO_MFR_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&mfr_name_uuid, ESP_GATT_PERM_READ,
      sizeof(manufacturer_name), sizeof(manufacturer_name), (uint8_t *)manufacturer_name}},

    [DEV_INFO_MODEL_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read), (uint8_t *)&char_prop_read}},
    [DEV_INFO_MODEL_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&model_num_uuid, ESP_GATT_PERM_READ,
      sizeof(model_number), sizeof(model_number), (uint8_t *)model_number}},
};

// === Table 2: HID Service GATT Table (0x1812) ===
static const esp_gatts_attr_db_t hid_gatt_db[HID_IDX_MAX] = {
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

    // ---------------- 1. Xbox Gamepad Input Report (Report ID 1) ----------------
    [HID_GP_REPORT_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_notify), (uint8_t *)&char_prop_read_notify}},
    [HID_GP_REPORT_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_uuid, ESP_GATT_PERM_READ,
      sizeof(xbox_gamepad_report_t), 0, NULL}},
    [HID_GP_REPORT_CCCD_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(cccd_init), (uint8_t *)&cccd_init}},
    [HID_GP_REPORT_REF_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&report_ref_uuid, ESP_GATT_PERM_READ,
      sizeof(report_ref_gp), sizeof(report_ref_gp), (uint8_t *)report_ref_gp}},

    // ---------------- 2. Keyboard Input Report (Report ID 2) ----------------
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

    // ---------------- 3. Mouse Input Report (Report ID 3) ----------------
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

    // ---------------- 4. Consumer Control Report (Report ID 4) ----------------
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
};

// === Table 3: PC Sync GATT Table (0xFFE0) ===
static const uint16_t pc_sync_service_uuid = BLE_UUID_PC_SYNC_SVC;
static const uint16_t pc_sync_char_uuid = BLE_UUID_PC_SYNC_CHAR;
static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

static const esp_gatts_attr_db_t pc_sync_gatt_db[PC_SYNC_IDX_MAX] = {
    [PC_SYNC_SVC_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(pc_sync_service_uuid), (uint8_t *)&pc_sync_service_uuid}},

    [PC_SYNC_CHAR_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(char_prop_read_write), (uint8_t *)&char_prop_read_write}},

    [PC_SYNC_VAL_IDX] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&pc_sync_char_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      512, 0, NULL}},
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
            ESP_LOGI(TAG, "🎮 XBOX CONTROLLER BLE ADVERTISING STARTED! Discoverable as '%s'", DEVICE_NAME);
        } else {
            ESP_LOGE(TAG, "BLE Advertising start failed: %s", esp_err_to_name(ret));
        }
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            if (param->adv_data_raw_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                adv_config_done |= ADV_CONFIG_FLAG;
                check_and_start_advertising();
            }
            break;

        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            if (param->scan_rsp_data_raw_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                adv_config_done |= SCAN_RSP_CONFIG_FLAG;
                check_and_start_advertising();
            }
            break;

        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                ESP_LOGI(TAG, "🎉 Xbox Gamepad Advertising active! Ready to connect.");
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

        case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT:
            ESP_LOGI(TAG, "Bond device removed (status = %d)", param->remove_bond_dev_cmpl.status);
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
                // Step 1: Create Device Information Service Table (inst_id = 0)
                esp_ble_gatts_create_attr_tab(dev_info_gatt_db, gatts_if, DEV_INFO_IDX_MAX, 0);
            }
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status == ESP_GATT_OK) {
                if (param->add_attr_tab.svc_inst_id == 0) {
                    memcpy(dev_info_handle_table, param->add_attr_tab.handles, sizeof(dev_info_handle_table));
                    esp_ble_gatts_start_service(dev_info_handle_table[DEV_INFO_SVC_IDX]);
                    ESP_LOGI(TAG, "Device Info Service (0x180A) created & started!");
                    // Step 2: Create HID Service Table (inst_id = 1)
                    esp_ble_gatts_create_attr_tab(hid_gatt_db, gatts_if, HID_IDX_MAX, 1);
                } else if (param->add_attr_tab.svc_inst_id == 1) {
                    memcpy(hid_handle_table, param->add_attr_tab.handles, sizeof(hid_handle_table));
                    esp_ble_gatts_start_service(hid_handle_table[HID_SVC_IDX]);
                    ESP_LOGI(TAG, "HID Service (0x1812) created & started successfully!");
                    // Step 3: Create PC Sync Service Table (inst_id = 2)
                    esp_ble_gatts_create_attr_tab(pc_sync_gatt_db, gatts_if, PC_SYNC_IDX_MAX, 2);
                } else if (param->add_attr_tab.svc_inst_id == 2) {
                    memcpy(pc_sync_handle_table, param->add_attr_tab.handles, sizeof(pc_sync_handle_table));
                    esp_ble_gatts_start_service(pc_sync_handle_table[PC_SYNC_SVC_IDX]);
                    ESP_LOGI(TAG, "PC Sync Service (0xFFE0) created & started successfully!");
                    adv_config_done |= PROFILE_CONFIG_FLAG;
                    check_and_start_advertising();
                }
            } else {
                ESP_LOGE(TAG, "Create Attribute Table failed, status = 0x%x, svc_inst_id = %d",
                         param->add_attr_tab.status, param->add_attr_tab.svc_inst_id);
            }
            break;

        case ESP_GATTS_CONNECT_EVT:
            conn_id = param->connect.conn_id;
            is_connected = true;
            memcpy(current_remote_bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            ESP_LOGI(TAG, "BLE Host Connected! conn_id = %d, BD Addr: %02x:%02x:%02x:%02x:%02x:%02x",
                     conn_id,
                     current_remote_bda[0], current_remote_bda[1], current_remote_bda[2],
                     current_remote_bda[3], current_remote_bda[4], current_remote_bda[5]);
            if (s_conn_status_cb) {
                s_conn_status_cb(true);
            }
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            is_connected = false;
            memset(current_remote_bda, 0, sizeof(esp_bd_addr_t));
            ESP_LOGI(TAG, "BLE Host Disconnected. Restarting Advertising...");
            esp_ble_gap_start_advertising(&adv_params);
            if (s_conn_status_cb) {
                s_conn_status_cb(false);
            }
            break;

        case ESP_GATTS_WRITE_EVT:
            if (param->write.handle == pc_sync_handle_table[PC_SYNC_VAL_IDX]) {
                ESP_LOGI(TAG, "BLE Received %d bytes on PC Sync Characteristic", param->write.len);
                if (s_pc_sync_cb) {
                    s_pc_sync_cb(param->write.value, param->write.len);
                }
                if (param->write.need_rsp) {
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
                break;
            }

            if (param->write.len == 2) {
                uint16_t cccd_val = param->write.value[0] | (param->write.value[1] << 8);
                if (param->write.handle == hid_handle_table[HID_GP_REPORT_CCCD_IDX]) {
                    ESP_LOGI(TAG, "Gamepad CCCD: 0x%04x (%s)",
                             cccd_val, (cccd_val & 0x0001) ? "NOTIF ENABLED" : "NOTIF DISABLED");
                } else if (param->write.handle == hid_handle_table[HID_KB_REPORT_CCCD_IDX]) {
                    ESP_LOGI(TAG, "Keyboard CCCD: 0x%04x (%s)",
                             cccd_val, (cccd_val & 0x0001) ? "NOTIF ENABLED" : "NOTIF DISABLED");
                } else if (param->write.handle == hid_handle_table[HID_MOUSE_REPORT_CCCD_IDX]) {
                    ESP_LOGI(TAG, "Mouse CCCD: 0x%04x (%s)",
                             cccd_val, (cccd_val & 0x0001) ? "NOTIF ENABLED" : "NOTIF DISABLED");
                } else if (param->write.handle == hid_handle_table[HID_CC_REPORT_CCCD_IDX]) {
                    ESP_LOGI(TAG, "Consumer CCCD: 0x%04x (%s)",
                             cccd_val, (cccd_val & 0x0001) ? "NOTIF ENABLED" : "NOTIF DISABLED");
                }
            }
            break;

        default:
            break;
    }
}

esp_err_t ble_hid_init(void) {
    esp_err_t ret;

    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) return ret;

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) return ret;

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) return ret;

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) return ret;

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(0);

    // Set local ATT MTU to 517 to receive larger payloads easily
    esp_ble_gatt_set_local_mtu(517);

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

    ESP_LOGI(TAG, "BLE HID Xbox Gamepad Component Initialized");
    return ESP_OK;
}

bool ble_hid_is_connected(void) {
    return is_connected;
}

esp_err_t ble_hid_clear_bonds(void) {
    ESP_LOGW(TAG, "Clearing all paired bonds from NVS...");

    if (is_connected) {
        esp_ble_gap_disconnect(current_remote_bda);
    }

    int dev_num = esp_ble_get_bond_device_num();
    if (dev_num > 0) {
        esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
        if (dev_list) {
            esp_ble_get_bond_device_list(&dev_num, dev_list);
            for (int i = 0; i < dev_num; i++) {
                esp_ble_remove_bond_device(dev_list[i].bd_addr);
            }
            free(dev_list);
        }
    }

    ESP_LOGI(TAG, "All bonding keys removed. Ready for new pairing!");
    return esp_ble_gap_start_advertising(&adv_params);
}

esp_err_t ble_hid_send_xbox_gamepad(uint16_t left_x, uint16_t left_y,
                                    uint16_t right_x, uint16_t right_y,
                                    uint16_t brake, uint16_t accelerator,
                                    uint16_t buttons, uint8_t dpad) {
    if (!is_connected) return ESP_ERR_INVALID_STATE;

    xbox_gamepad_report_t report = {
        .left_x      = left_x,
        .left_y      = left_y,
        .right_x     = right_x,
        .right_y     = right_y,
        .brake       = brake,
        .accelerator = accelerator,
        .dpad        = dpad,
        .buttons     = buttons,
        .record      = 0,
    };

    return esp_ble_gatts_send_indicate(gatts_if_for_app, conn_id,
                                       hid_handle_table[HID_GP_REPORT_VAL_IDX],
                                       sizeof(report), (uint8_t *)&report, false);
}

// Convert -127..127 to 0..65535
static inline uint16_t axis_to_xbox(int8_t val) {
    int32_t v = val; // -127 to 127
    int32_t mapped = ((v + 127) * 65535) / 254;
    if (mapped < 0) mapped = 0;
    if (mapped > 65535) mapped = 65535;
    return (uint16_t)mapped;
}

esp_err_t ble_hid_send_gamepad(int8_t left_x, int8_t left_y, int8_t right_x, int8_t right_y,
                               uint8_t brake, uint8_t throttle,
                               uint16_t buttons, uint8_t dpad) {
    // Map brake/throttle (0..255) to 0..1023
    uint16_t brk = ((uint16_t)brake * 1023) / 255;
    uint16_t thr = ((uint16_t)throttle * 1023) / 255;

    return ble_hid_send_xbox_gamepad(axis_to_xbox(left_x),
                                     axis_to_xbox(left_y),
                                     axis_to_xbox(right_x),
                                     axis_to_xbox(right_y),
                                     brk, thr, buttons, dpad);
}

esp_err_t ble_hid_send_launchpad(uint16_t pad_mask) {
    return ble_hid_send_xbox_gamepad(32768, 32768, 32768, 32768, 0, 0, pad_mask, 0);
}

esp_err_t ble_hid_send_keyboard(uint8_t modifier, uint8_t key_code) {
    if (!is_connected) return ESP_ERR_INVALID_STATE;

    keyboard_report_t report = {
        .modifier = modifier,
        .reserved = 0,
        .key_codes = {key_code, 0, 0, 0, 0, 0}
    };

    return esp_ble_gatts_send_indicate(gatts_if_for_app, conn_id,
                                       hid_handle_table[HID_KB_REPORT_VAL_IDX],
                                       sizeof(report), (uint8_t *)&report, false);
}

esp_err_t ble_hid_send_keyboard_report(uint8_t modifier, const uint8_t *key_codes, uint8_t key_count) {
    if (!is_connected) return ESP_ERR_INVALID_STATE;

    keyboard_report_t report = {
        .modifier = modifier,
        .reserved = 0,
        .key_codes = {0}
    };

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
        .x       = x,
        .y       = y,
        .wheel   = wheel
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

void ble_hid_register_pc_sync_cb(ble_pc_sync_rx_cb_t cb) {
    s_pc_sync_cb = cb;
}

void ble_hid_register_conn_status_cb(ble_connection_status_cb_t cb) {
    s_conn_status_cb = cb;
}


