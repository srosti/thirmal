
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);
static const char *MQTT_TAG = "MQTT";
static const esp_mqtt_client_config_t mqtt_cfg = {
    .uri = "mqtt://srosti.hopto.org:1883", //for mqtt over tcp
    .event_handle = mqtt_event_handler,
};
static esp_mqtt_client_handle_t client;

#define EXTERNAL_TEMP_TOPIC "cabin/hottub/external-temp"
#define INTERNAL_TEMP_TOPIC "cabin/hottub/internal-temp"
#define TIME_TOPIC "cabin/hottub/time"
#define DEBUG_TOPIC "cabin/hottub/debug"
#define TEST_TOPIC "test"

void mqtt_publish(char *topic, char *data)
{
    int msg_id;
    msg_id = esp_mqtt_client_publish(client, topic, data, 0, 0, 0);
    ESP_LOGI(MQTT_TAG, "sent publish successful, msg_id=%d", msg_id);
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, TEST_TOPIC, 0);
        ESP_LOGI(MQTT_TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, DEBUG_TOPIC, 1);
        ESP_LOGI(MQTT_TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, DEBUG_TOPIC);
        ESP_LOGI(MQTT_TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        break;
    }
    return ESP_OK;
}

void mqtt_app_start(void)
{
    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_log_level_set(MQTT_TAG, ESP_LOG_VERBOSE);

    ESP_LOGI(MQTT_TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_start(client);
}
