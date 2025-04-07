#include "mqtt.h"

char mqtt_buf_topic[128];
char mqtt_buf_data[MQTT_MAX_PACKET_SIZE];

uint32_t mqtt_get(PubSubClient &mqttClient, const char *topic, void *data, uint32_t data_len, bool str_null)
{
    static const uint32_t timeout_sec = 5;

    static uint32_t recv_len;
    recv_len = 0;
    mqttClient.setCallback([data, data_len](const char *topic, const uint8_t *pld, unsigned int pld_len) {
        uint32_t len = std::min(pld_len, data_len);
        memcpy(data, pld, len);
        recv_len = len;
    });

    mqttClient.subscribe(topic, 1);
    uint32_t ms = millis();
    while (!recv_len && (uint32_t)(millis() - ms) < timeout_sec * 1000)
        mqttClient.loop();
    mqttClient.unsubscribe(topic);
    mqttClient.setCallback(nullptr);

    if (str_null) {
        // Add string null terminator
        uint32_t len = std::min(recv_len, data_len - 1);
        ((char *)data)[len] = 0;
    }
    return recv_len;
}
