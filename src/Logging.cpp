#include "Logging.h"
#include <HTTPClient.h>

extern String ApiUrl;

void logEvent(String type, String message)
{
    Serial.println(message);
    HTTPClient http;
    String json = "{\"type\": \"" + type + "\", \"message\": \"" + message + "\"}";
    http.begin(ApiUrl + "/logs");
    http.addHeader("Content-Type", "application/json");
    http.POST(json);
    http.end();
}
