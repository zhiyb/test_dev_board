#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "wifi_network.h"
#include "http.h"
#include "epd.h"
#include "pmic.h"
#include "common.h"

// static const unsigned int max_urllen = 128;
// static const unsigned int max_datalen = 8192;

static ESP8266WiFiMulti WiFiMulti;
static WiFiClient wifiClient;
static WiFiUDP wifiUDP;
static PubSubClient mqttClient(wifiClient);
static NTPClient ntpClient(wifiUDP, NTP_SERVER);

char id[32];

uint32_t mqtt_get(PubSubClient &mqttClient, const char *topic, void *data, uint32_t data_len)
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
    return recv_len;
}

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.printf("ESP8266: %#010x, Flash: %#010x %uMiB\n",
        ESP.getChipId(), ESP.getFlashChipId(),
        ESP.getFlashChipRealSize() / 1024 / 1024);
    sprintf(id, "esp_%08x_%08x", ESP.getChipId(), ESP.getFlashChipId());

    // Workaround for WiFI error: pll_cal exceeds 2ms
    delay(5);
    //WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    // http_init();
    // epd_init();
}

void loop()
{
    // Wait for WiFi connection
    static int connected = -1;
    if (connected < 0) {
        connected = 0;
        Serial.print("Connecting");
    }
    if ((WiFiMulti.run() != WL_CONNECTED)) {
        if (connected)
            connected = -1;
        else
            Serial.print(".");
        return;
    }
    if (!connected) {
        connected = 1;
        Serial.println();
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    }

    // Connect to MQTT server
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    if (mqttClient.connect(id, MQTT_USER, MQTT_PASSWORD)) {
        // Connected
        pmic_init();
        pmic_update(ntpClient, mqttClient, id);
        mqttClient.disconnect();
    }

#if 0
    char url[128];

    // Get scheduling info
    int outdated = 1;
    sprintf(url, "%s?token=%s&action=next", url_base, id);
    const String &next = http_get(url, nullptr);
    outdated = next.indexOf("\"outdated\":");
    outdated = outdated < 0 || next[outdated + 11] != 'f';

    // If key is pressed, force display refresh
    if (!outdated) {
        pmic_init();
        if (pmic_get_key()) {
            outdated = 1;
            // Need to re-init EPD GPIOs
            epd_init();
        }
    }

    // Get display type
    const epd_func_t *epd_func = 0;
    if (outdated) {
        sprintf(url, "%s?token=%s&action=peek&key=type", url_base, id);
        const String &stype = http_get(url, nullptr);
        Serial.print("EPD: ");
        Serial.println(stype);
        if (stype.startsWith("epd_2in13_rwb_122x250"))
            epd_func = epd_2in13_rwb_122x250();
        else if (stype.startsWith("epd_4in2_rwb_400x300"))
            epd_func = epd_4in2_rwb_400x300();
        else if (stype.startsWith("epd_5in65_7c_600x448"))
            epd_func = epd_5in65_7c_600x448();
        else if (stype.startsWith("epd_7in5_rwb4_640x384"))
            epd_func = epd_7in5_rwb4_640x384();
    }

    // Refresh display
    if (epd_func) {
        epd_func->init();
        static const uint32_t block = 1024 * 4;
        uint32_t ofs = 0;
        uint32_t len = 0;
        do {
            len = block;
            sprintf(url, "%s?token=%s&action=read&ofs=%u&len=%u",
                url_base, id, ofs, len);
            const String &data = http_get(url, nullptr);
            len = data.length();
            epd_func->update((const uint8_t *)data.c_str(), ofs, len);
            ofs += len;
        } while(len == block);
    }

    // Configure PMIC for next wakeup
    pmic_init();
    // pmic_update();
#endif

    // Done
    WiFi.disconnect();
#if 0
    if (epd_func)
        epd_func->wait();
#endif
    epd_deinit();

    // Boot mode
    // b0: GPIO02   should be 1
    // b1: GPIO00   should be 1
    // b2: GPIO15   should be 0

    Serial.println("SLEEP");
    pmic_shutdown();
    for (;;)
        ESP.deepSleep(0, WAKE_RF_DISABLED);
}
