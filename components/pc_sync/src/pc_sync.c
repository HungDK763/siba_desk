#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "pc_sync.h"

static const char *TAG = "PC_SYNC";

static pc_sync_state_t current_state = PC_SYNC_STATE_UNINITIALIZED;
static pc_quota_packet_t latest_quota;
static pc_quota_update_cb_t registered_callback = NULL;

const char* pc_sync_state_to_str(pc_sync_state_t state) {
    switch (state) {
        case PC_SYNC_STATE_UNINITIALIZED: return "UNINITIALIZED";
        case PC_SYNC_STATE_IDLE:          return "IDLE (WAITING_DATA)";
        case PC_SYNC_STATE_RECEIVING:     return "RECEIVING_PACKET";
        case PC_SYNC_STATE_PARSED_SUCCESS:return "PARSED_SUCCESS";
        case PC_SYNC_STATE_ERROR_FORMAT:  return "ERROR_JSON_FORMAT";
        case PC_SYNC_STATE_ERROR_INVALID_VAL: return "ERROR_INVALID_VALUES";
        default:                          return "UNKNOWN_STATE";
    }
}

pc_sync_state_t pc_sync_get_state(void) {
    return current_state;
}

esp_err_t pc_sync_get_latest_quota(pc_quota_packet_t *out_data) {
    if (!out_data) return ESP_ERR_INVALID_ARG;
    if (current_state == PC_SYNC_STATE_UNINITIALIZED) return ESP_ERR_INVALID_STATE;

    memcpy(out_data, &latest_quota, sizeof(pc_quota_packet_t));
    return ESP_OK;
}

esp_err_t pc_sync_init(pc_quota_update_cb_t callback) {
    registered_callback = callback;

    memset(&latest_quota, 0, sizeof(pc_quota_packet_t));
    strncpy(latest_quota.model_name, "Antigravity Ready", sizeof(latest_quota.model_name) - 1);
    latest_quota.antigrav_quota_percent = 100;
    latest_quota.alert_level = QUOTA_ALERT_SAFE;
    latest_quota.received_timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000);

    current_state = PC_SYNC_STATE_IDLE;
    ESP_LOGI(TAG, "✅ PC Sync Component Initialized. State: %s", pc_sync_state_to_str(current_state));
    return ESP_OK;
}

esp_err_t pc_sync_process_packet(const uint8_t *raw_data, size_t len) {
    if (!raw_data || len == 0) {
        current_state = PC_SYNC_STATE_ERROR_FORMAT;
        ESP_LOGE(TAG, "❌ Received empty or NULL packet!");
        return ESP_ERR_INVALID_ARG;
    }

    current_state = PC_SYNC_STATE_RECEIVING;

    // Create a null-terminated buffer for cJSON
    char json_buffer[513];
    size_t copy_len = (len >= sizeof(json_buffer)) ? (sizeof(json_buffer) - 1) : len;
    memcpy(json_buffer, raw_data, copy_len);
    json_buffer[copy_len] = '\0';

    cJSON *root = cJSON_Parse(json_buffer);
    if (!root) {
        current_state = PC_SYNC_STATE_ERROR_FORMAT;
        ESP_LOGE(TAG, "❌ Failed to parse JSON string: %s", json_buffer);
        return ESP_ERR_INVALID_ARG;
    }

    // Parse model
    cJSON *item_model = cJSON_GetObjectItem(root, "model");
    if (!item_model) item_model = cJSON_GetObjectItem(root, "m");
    if (item_model && cJSON_IsString(item_model)) {
        strncpy(latest_quota.model_name, item_model->valuestring, sizeof(latest_quota.model_name) - 1);
        latest_quota.model_name[sizeof(latest_quota.model_name) - 1] = '\0';
    }

    // Parse Vietnam realtime string (time / t)
    cJSON *item_time = cJSON_GetObjectItem(root, "time");
    if (!item_time) item_time = cJSON_GetObjectItem(root, "t");
    if (item_time && cJSON_IsString(item_time)) {
        strncpy(latest_quota.time_vn, item_time->valuestring, sizeof(latest_quota.time_vn) - 1);
        latest_quota.time_vn[sizeof(latest_quota.time_vn) - 1] = '\0';
    }

    // Parse Gemini 5h quota % (primary quota)
    cJSON *item_quota = cJSON_GetObjectItem(root, "quota");
    if (!item_quota) item_quota = cJSON_GetObjectItem(root, "g_5h");
    if (!item_quota) item_quota = cJSON_GetObjectItem(root, "q");
    if (item_quota && cJSON_IsNumber(item_quota)) {
        int q = item_quota->valueint;
        if (q < 0) q = 0;
        if (q > 100) q = 100;
        latest_quota.antigrav_quota_percent = (uint8_t)q;
    }

    // Parse Gemini Weekly quota %
    cJSON *item_gw = cJSON_GetObjectItem(root, "g_w");
    if (!item_gw) item_gw = cJSON_GetObjectItem(root, "gemini_weekly");
    if (item_gw && cJSON_IsNumber(item_gw)) {
        int q = item_gw->valueint;
        if (q < 0) q = 0;
        if (q > 100) q = 100;
        latest_quota.gemini_weekly_percent = (uint8_t)q;
    }

    // Parse Claude 5h quota %
    cJSON *item_c5h = cJSON_GetObjectItem(root, "c_5h");
    if (!item_c5h) item_c5h = cJSON_GetObjectItem(root, "claude_5h");
    if (item_c5h && cJSON_IsNumber(item_c5h)) {
        int q = item_c5h->valueint;
        if (q < 0) q = 0;
        if (q > 100) q = 100;
        latest_quota.claude_5h_percent = (uint8_t)q;
    }

    // Parse Claude Weekly quota %
    cJSON *item_cw = cJSON_GetObjectItem(root, "c_w");
    if (!item_cw) item_cw = cJSON_GetObjectItem(root, "claude_weekly");
    if (item_cw && cJSON_IsNumber(item_cw)) {
        int q = item_cw->valueint;
        if (q < 0) q = 0;
        if (q > 100) q = 100;
        latest_quota.claude_weekly_percent = (uint8_t)q;
    }

    // Parse prompts count
    cJSON *item_prompts = cJSON_GetObjectItem(root, "prompts");
    if (!item_prompts) item_prompts = cJSON_GetObjectItem(root, "p");
    if (item_prompts && cJSON_IsNumber(item_prompts)) {
        latest_quota.prompt_count = (uint32_t)item_prompts->valueint;
    }

    // Parse CPU %
    cJSON *item_cpu = cJSON_GetObjectItem(root, "cpu");
    if (!item_cpu) item_cpu = cJSON_GetObjectItem(root, "c");
    if (item_cpu && cJSON_IsNumber(item_cpu)) {
        int c = item_cpu->valueint;
        if (c < 0) c = 0;
        if (c > 100) c = 100;
        latest_quota.cpu_percent = (uint8_t)c;
    }

    // Parse RAM %
    cJSON *item_ram = cJSON_GetObjectItem(root, "ram");
    if (!item_ram) item_ram = cJSON_GetObjectItem(root, "r");
    if (item_ram && cJSON_IsNumber(item_ram)) {
        int r = item_ram->valueint;
        if (r < 0) r = 0;
        if (r > 100) r = 100;
        latest_quota.ram_percent = (uint8_t)r;
    }

    // Determine alert level based on quota percentage
    if (latest_quota.antigrav_quota_percent >= 50) {
        latest_quota.alert_level = QUOTA_ALERT_SAFE;
    } else if (latest_quota.antigrav_quota_percent >= 20) {
        latest_quota.alert_level = QUOTA_ALERT_WARNING;
    } else {
        latest_quota.alert_level = QUOTA_ALERT_CRITICAL;
    }

    latest_quota.received_timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000);
    current_state = PC_SYNC_STATE_PARSED_SUCCESS;

    cJSON_Delete(root);

    // Notify registered callback
    if (registered_callback) {
        registered_callback(&latest_quota);
    }

    return ESP_OK;
}
