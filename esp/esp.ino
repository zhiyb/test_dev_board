#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "wifi_network.h"
#include "http.h"
#include "epd.h"
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

    epd_init();

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

    // Get display type
    char url[96];
    sprintf(url, "%sid=%s&key=%s", url_base, id, "type");

    const epd_func_t *epd_func = 0;
    {
        String stype = http_get(url, nullptr);
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

    if (epd_func) {
        epd_func->init();
        static const uint32_t block = 1024 * 4;
        uint32_t ofs = 0;
        uint32_t len = 0;
        do {
            len = block;
            sprintf(url, "%sid=%s&key=%s&ofs=%u&len=%u",
                url_base, id, "data", ofs, len);
            const String data = http_get(url, nullptr);
            len = data.length();
            epd_func->update((const uint8_t *)data.c_str(), ofs, len);
            ofs += len;
        } while(len == block);
    }

    // Configure PMIC for next wakeup
    pmic_init();
    pmic_update();

    // Done
    WiFi.disconnect();
    if (epd_func)
        epd_func->wait();
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
