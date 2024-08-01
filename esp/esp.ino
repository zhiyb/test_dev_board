#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "wifi_network.h"
#include "http.h"
#include "epd.h"

static const unsigned int max_urllen = 128;
static const unsigned int max_datalen = 8192;

static const char url_base[] = "https://zhiyb.me/nas/api/disp.php?";

ESP8266WiFiMulti WiFiMulti;
WiFiClientSecure client;
char id[32];

void setup()
{
    Serial.begin(38400);
    Serial.println();
    Serial.printf("ESP8266: %#010x, Flash: %#010x %uMiB\n",
        ESP.getChipId(), ESP.getFlashChipId(),
        ESP.getFlashChipRealSize() / 1024 / 1024);
    sprintf(id, "esp_%08x_%08x", ESP.getChipId(), ESP.getFlashChipId());

    init_epd();

    //WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

    client.setInsecure();
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
    static char url[96];
    sprintf(url, "%sid=%s&key=%s", url_base, id, "type");

    const epd_func_t *func = 0;
    {
        String stype = http_get(client, url, nullptr);
        if (stype.startsWith("epd_2in13_rwb_122x250"))
            func = epd_2in13_rwb_122x250();
        else if (stype.startsWith("epd_7in5_rwb4_640x384"))
            func = epd_7in5_rwb4_640x384();
    }

    if (func) {
        func->init();
        static const uint32_t block = 1024 * 4;
        uint32_t ofs = 0;
        uint32_t len = 0;
        do {
            len = block;
            sprintf(url, "%sid=%s&key=%s&ofs=%u&len=%u",
                url_base, id, "data", ofs, len);
            const String data = http_get(client, url, nullptr);
            len = data.length();
            func->update((const uint8_t *)data.c_str(), ofs, len);
            Serial.printf("%d, %d\n", ofs, len);
            ofs += len;
        } while(len == block);
    }

    WiFi.disconnect();

    if (func)
        func->wait();

    // Boot mode
    // b0: GPIO02   should be 1
    digitalWrite(2, HIGH);
    // b1: GPIO00   should be 1
    digitalWrite(0, HIGH);
    // b2: GPIO15   should be 0
    digitalWrite(15, LOW);

    Serial.println("#PMIC.OFF");
    for (;;)
        ESP.deepSleep(0, WAKE_RF_DISABLED);
}
