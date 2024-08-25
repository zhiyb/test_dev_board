#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "wifi_network.h"
#include "http.h"
#include "pmic.h"

static const unsigned int max_urllen = 128;
static const unsigned int max_datalen = 8192;

ESP8266WiFiMulti WiFiMulti;

char id[32];

void setup()
{
    Serial.begin(38400);
    Serial.println();
    Serial.printf("ESP8266: %#010x, Flash: %#010x %uMiB\n",
        ESP.getChipId(), ESP.getFlashChipId(),
        ESP.getFlashChipRealSize() / 1024 / 1024);
    sprintf(id, "esp_%08x_%08x", ESP.getChipId(), ESP.getFlashChipId());

    pmic_init();

    // Workaround for WiFI error: pll_cal exceeds 2ms
    delay(5);
    //WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    http_init();
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

    char url[128];

    // Get scheduling info
    int outdated = 1;
    sprintf(url, "%s?token=%s&action=next", url_base, id);
    const String &next = http_get(url, nullptr);
    outdated = next.indexOf("\"outdated\":");
    outdated = outdated < 0 || next[outdated + 11] != 'f';

    // If key is pressed, force display refresh
    if (!outdated)
        if (pmic_get_key())
            outdated = 1;

    // Configure PMIC for next wakeup
    pmic_update();

    // Done
    WiFi.disconnect();

    // Boot mode
    // b0: GPIO02   should be 1
    // b1: GPIO00   should be 1
    // b2: GPIO15   should be 0

    Serial.println("SLEEP");
    pmic_shutdown();
    for (;;)
        ESP.deepSleep(0, WAKE_RF_DISABLED);
}
