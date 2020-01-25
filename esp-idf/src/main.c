#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"

//sleep includes
#include "esp_wifi.h"

// sntp/time includes
#include "lwip/apps/sntp.h"
static void initialize_sntp(void);

static const char *TAG = "THYRMAL";

int get_temp();
void mqtt_app_start(void);
void wifi_init(void);

static void obtain_time(struct tm *timeinfo)
{
    time_t now = 0;

    initialize_sntp();

    // TODO: add this in when platformio updates to esp-idf version 4.0 (remove delay below) - srosti
    // 
    //    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
    //        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    //        vTaskDelay(2000 / portTICK_PERIOD_MS);
    //    }
    vTaskDelay(500 / portTICK_PERIOD_MS);

    // Set timezone to Mountain Standard Time
    setenv("TZ", "MST7MDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    time(&now);
    localtime_r(&now, timeinfo);
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void app_main()
{
    char strftime_buf[64];

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    //get_temp();
    nvs_flash_init();
    wifi_init();
    mqtt_app_start();

    // get the current time
    struct tm timeinfo = {0};
    obtain_time(&timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);

    // start deep sleep shutdown
    vTaskDelay(5000 / portTICK_RATE_MS);
    esp_wifi_stop();
    const int deep_sleep_sec = 10;
    ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
    esp_deep_sleep(1000000LL * deep_sleep_sec);
}
