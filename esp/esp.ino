#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "wifi_network.h"
#include "epd.h"
#include "pmic.h"
#include "mqtt.h"

static ESP8266WiFiMulti WiFiMulti;
static WiFiClient wifiClient;
static WiFiUDP wifiUDP;
static PubSubClient mqttClient(wifiClient);
static NTPClient ntpClient(wifiUDP, NTP_SERVER);

char id[32];

void setup()
{
    WiFi.persistent(true);

    Serial.begin(115200);
    Serial.println();
    Serial.printf("ESP8266: %#010x, Flash: %#010x %uMiB\n",
        ESP.getChipId(), ESP.getFlashChipId(),
        ESP.getFlashChipRealSize() / 1024 / 1024);
    sprintf(id, "esp_%08x_%08x", ESP.getChipId(), ESP.getFlashChipId());

    // Workaround for WiFI error: pll_cal exceeds 2ms
    delay(5);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
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

    enum {TypeUnknown, TypeSensor, TypeEPD} type = TypeUnknown;
    const epd_func_t *epd_func = nullptr;

    // Connect to MQTT server
    mqttClient.setBufferSize(MQTT_MAX_PACKET_SIZE);
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    if (!mqttClient.connect(id, MQTT_USER, MQTT_PASSWORD)) {
        // Connection failed, shutdown
        pmic_init();
        goto shutdown;
    }

    // NTP update once after boot
    if (!ntpClient.update()) {
        pmic_init();
        goto shutdown;
    }

    sprintf(mqtt_buf_topic, "debug/%s/power_on_ts", id);
    sprintf(mqtt_buf_data, "%lu", ntpClient.getEpochTime());
    mqttClient.publish(mqtt_buf_topic, mqtt_buf_data);

    // Check module type
    sprintf(mqtt_buf_topic, "config/%s/type", id);
    if (!mqtt_get(mqttClient, mqtt_buf_topic, mqtt_buf_data, sizeof(mqtt_buf_data), true)) {
        type = TypeUnknown;
        mqttClient.publish(mqtt_buf_topic, "unknown", true);
    } else if (strcmp(mqtt_buf_data, "sensor") == 0) {
        type = TypeSensor;
    } else if (strcmp(mqtt_buf_data, "epd") == 0) {
        type = TypeEPD;
    }

    if (type == TypeEPD) {
        // Check if display is outdated
        bool upd_req = true;
        if (upd_req) {
            sprintf(mqtt_buf_topic, "epd/%s/disp_ts", id);
            upd_req &= mqtt_get(mqttClient, mqtt_buf_topic, mqtt_buf_data, sizeof(mqtt_buf_data), true) != 0;
        }
        uint32_t disp_ts;
        if (upd_req) {
            disp_ts = strtoul(mqtt_buf_data, nullptr, 0);
        }
        if (upd_req) {
            sprintf(mqtt_buf_topic, "epd/%s/data_ts", id);
            upd_req &= mqtt_get(mqttClient, mqtt_buf_topic, mqtt_buf_data, sizeof(mqtt_buf_data), true) != 0;
        }
        uint32_t data_ts;
        if (upd_req) {
            data_ts = strtoul(mqtt_buf_data, nullptr, 0);
            upd_req &= disp_ts != data_ts;
        }

        if (!upd_req) {
            // It's not time for display update yet
            // Check if key is being pressed, force display refresh
            pmic_init();
            upd_req |= !!pmic_get_key();
        }

        if (upd_req) {
            // Display update needed, read EPD type
            sprintf(mqtt_buf_topic, "epd/%s/type", id);
            upd_req &= mqtt_get(mqttClient, mqtt_buf_topic, mqtt_buf_data, sizeof(mqtt_buf_data), true) != 0;
        }
        if (upd_req) {
            // Check EPD type
            if (strcmp(mqtt_buf_data, "epd_2in13_rwb_122x250") == 0)
                epd_func = epd_2in13_rwb_122x250();
            else if (strcmp(mqtt_buf_data, "epd_4in2_rwb_400x300") == 0)
                epd_func = epd_4in2_rwb_400x300();
            else if (strcmp(mqtt_buf_data, "epd_5in65_7c_600x448") == 0)
                epd_func = epd_5in65_7c_600x448();
            else if (strcmp(mqtt_buf_data, "epd_7in5_rwb4_640x384") == 0)
                epd_func = epd_7in5_rwb4_640x384();
            upd_req &= epd_func != nullptr;
        }

        if (upd_req) {
            // Check numebr of EPD data segments
            sprintf(mqtt_buf_topic, "epd/%s/data/count", id);
            upd_req &= mqtt_get(mqttClient, mqtt_buf_topic, mqtt_buf_data, sizeof(mqtt_buf_data), true) != 0;
        }
        if (upd_req) {
            // Initialise EPD interface
            // Note: This conflicts with PMIC I2C communication
            epd_init();
            epd_func->init();

            // Read EPD display data
            uint32_t data_count = strtoul(mqtt_buf_data, nullptr, 0);
            uint32_t ofs = 0;
            for (uint32_t i = 0; i < data_count; i++) {
                sprintf(mqtt_buf_topic, "epd/%s/data/%lu", id, i);
                uint32_t len = mqtt_get(mqttClient, mqtt_buf_topic, mqtt_buf_data, sizeof(mqtt_buf_data), false);
                epd_func->update((uint8_t *)mqtt_buf_data, ofs, len);
                ofs += len;
            }

            // EPD now refreshing, we have time to process other things

            // Update disp_ts to match data_ts
            sprintf(mqtt_buf_topic, "epd/%s/disp_ts", id);
            sprintf(mqtt_buf_data, "%lu", data_ts);
            mqttClient.publish(mqtt_buf_topic, mqtt_buf_data, true);
        }
    }

    // Update PMIC sensors
    pmic_init();
    pmic_update(ntpClient, mqttClient, id);

shutdown:
    // Done
    sprintf(mqtt_buf_topic, "debug/%s/power_off_ts", id);
    sprintf(mqtt_buf_data, "%lu", ntpClient.getEpochTime());
    mqttClient.publish(mqtt_buf_topic, mqtt_buf_data);

    mqttClient.disconnect();
    WiFi.disconnect(false, false);
    wifi_set_opmode_current(WIFI_OFF);

    if (epd_func) {
        epd_func->wait();
        epd_deinit();
    }

    // Boot mode
    // b0: GPIO02   should be 1
    // b1: GPIO00   should be 1
    // b2: GPIO15   should be 0

    Serial.println("SLEEP");
    pmic_shutdown();
    for (;;)
        ESP.deepSleep(0, WAKE_RF_DISABLED);
}
