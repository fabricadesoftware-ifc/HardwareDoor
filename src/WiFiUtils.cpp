#include "WiFiUtils.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "Logging.h"

const char *ssid = "ifc_wifi";
const char *password = "";
String ApiUrl = "http://191.52.56.62:19003/api";

void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    logEvent("INFO", "Conectado ao WiFi. IP: " + WiFi.localIP().toString());
}

void imAlive()
{
    HTTPClient http;
    String json = "{\"ip\": \"" + WiFi.localIP().toString() + "\"}";
    http.begin(ApiUrl + "/health/ip");
    http.addHeader("Content-Type", "application/json");
    http.POST(json);
    http.end();
}
