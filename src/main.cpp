#include <Arduino.h>
#include <Ticker.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <vector>
#include <map>
#include <ArduinoJson.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <HardwareSerial.h>

#define LED_BUILTIN 2
#define RELAY_PIN 13  // Pino para o relé (controle da porta)
#define BUZZER_PIN 12 // Pino para o buzzer
#define RFID_RX 16    // Pino RX do RDM6300
#define RFID_TX 18    // Pino TX do RDM6300

#define BEARER_TOKEN_N BEARER_TOKEN_ENV
#define MODE_ENV_N MODE_ENV
const String BEARER_TOKEN = String(BEARER_TOKEN_N);
const String MODE = String(MODE_ENV_N);

class RFIDReader
{
private:
  HardwareSerial &rfidSerial;

public:
  RFIDReader(HardwareSerial &serial) : rfidSerial(serial)
  {
    rfidSerial.begin(9700, SERIAL_8N1, RFID_RX, RFID_TX);
  }

  String readCard()
  {
    String rfidCode = "";
    while (rfidSerial.available())
    {
      char c = rfidSerial.read();
      if (c == 0x02) // Início do frame
        rfidCode = "";
      else if (c == 0x03) // Fim do frame
        break;
      else
        rfidCode += c;
    }

    if (!rfidCode.isEmpty() && rfidCode.length() >= 10)
    {
      rfidCode = rfidCode.substring(0, 10); // Limita o tamanho do código RFID
      return rfidCode;
    }
    return "";
  }

  bool isCardPresent()
  {
    return rfidSerial.available() > 0;
  }

  void clearBuffer()
  {
    while (rfidSerial.available() > 0)
    {
      rfidSerial.read();
    }
  }
};


class HardwareController
{
public:
  HardwareController()
  {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
  }

  void unlockDoor()
  {
    digitalWrite(RELAY_PIN, HIGH);
    delay(50);
    digitalWrite(RELAY_PIN, LOW);
  }

  void setLED(bool state)
  {
    digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
  }

  void beepShort()
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  }

  void beepError()
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
  }

  void beepSuccess()
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
  }

  void beepWarning()
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
  }
};


class APIClient
{
private:
  String apiUrl;
  String bearerToken;

public:
  APIClient(const String &url, const String &token) : apiUrl(url), bearerToken(token) {}

  void logEvent(String type, String message)
  {
    int temp = hallRead();
    Serial.println(message);

    DynamicJsonDocument doc(256);
    doc["type"] = type;
    doc["message"] = message + " Temp: C°" + String(temp);

    String json;
    serializeJson(doc, json);

    HTTPClient http;
    http.begin(apiUrl + "/logs");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + bearerToken);
    http.POST(json);
    http.end();
  }

  void reportHealth()
  {
    HTTPClient http;
    String json = "{\"ip\": \"" + WiFi.localIP().toString() + "\"}";
    http.begin(apiUrl + "/health/ip");
    http.addHeader("Content-Type", "application/json");
    http.POST(json);
    http.end();
  }

  bool registerRFID(const String &rfidCode)
  {
    HTTPClient http;
    logEvent("INFO", "Cadastrando RFID: " + rfidCode);
    http.begin(apiUrl + "/tags/" + rfidCode);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + bearerToken);
    int httpCode = http.POST("{}");
    http.end();

    if (httpCode == HTTP_CODE_CREATED)
    {
      logEvent("SUCCESS", "RFID cadastrado com sucesso");
      return true;
    }
    else if (httpCode == HTTP_CODE_CONFLICT)
    {
      logEvent("WARN", "RFID já cadastrado");
      return false;
    }
    else
    {
      logEvent("ERROR", "Falha ao criar RFID: " + http.errorToString(httpCode));
      return false;
    }
  }

  std::map<String, std::pair<bool, unsigned long>> updateCache()
  {
    std::map<String, std::pair<bool, unsigned long>> rfidCache;
    HTTPClient http;
    http.begin(apiUrl + "/tags/");
    http.addHeader("Authorization", "Bearer " + bearerToken);

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      DynamicJsonDocument doc(4096);
      DeserializationError error = deserializeJson(doc, payload);

      if (error)
      {
        logEvent("ERROR", "Erro ao analisar JSON: " + String(error.c_str()));
        http.end();
        return rfidCache;
      }

      JsonArray tags = doc["data"].as<JsonArray>();

      for (JsonObject tag : tags)
      {
        String rfid = tag["rfid"].as<String>();
        bool isValid = tag["valid"].as<bool>();

        if (isValid)
        {
          rfidCache[rfid] = std::make_pair(true, millis());
        }
      }

      logEvent("INFO", "Cache atualizado com sucesso com " + String(rfidCache.size()) + " tags válidas.");
    }
    else if (httpCode == HTTP_CODE_UNAUTHORIZED)
    {
      logEvent("ERROR", "Token Bearer inválido");
    }
    else if (httpCode == HTTP_CODE_FORBIDDEN)
    {
      logEvent("ERROR", "Acesso negado ao recurso solicitado");
    }
    else if (httpCode == HTTP_CODE_NOT_FOUND)
    {
      logEvent("ERROR", "Recurso não encontrado na API");
    }
    else if (httpCode == HTTP_CODE_INTERNAL_SERVER_ERROR)
    {
      logEvent("ERROR", "Erro interno do servidor da API");
    }
    else
    {
      logEvent("ERROR", "Erro ao conectar com a API: " + http.getString());
    }

    http.end();
    return rfidCache;
  }
};

// Classe principal para o sistema de controle de porta
class DoorControlSystem
{
private:
  const char *ssid;
  const char *password;
  const int tickerTimer;
  const int tickerTimer2;

  Ticker tickerImAlive;
  Ticker tickerAtualizarCache;
  WebServer server;
  RFIDReader rfidReader;
  HardwareController hardware;
  APIClient apiClient;

  bool registrationMode;
  std::map<String, std::pair<bool, unsigned long>> rfidCache;

  static DoorControlSystem *instance;

public:
  DoorControlSystem(
      const char *wifi_ssid,
      const char *wifi_password,
      const String &api_url,
      const String &bearer_token,
      int health_timer,
      int cache_timer,
      int server_port,
      HardwareSerial &serial) : ssid(wifi_ssid),
                                password(wifi_password),
                                tickerTimer(health_timer),
                                tickerTimer2(cache_timer),
                                server(server_port),
                                rfidReader(serial),
                                hardware(),
                                apiClient(api_url, bearer_token),
                                registrationMode(false)
  {
    instance = this;
  }

  void setup()
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    apiClient.logEvent("INFO", "Conectado ao WiFi. IP: " + WiFi.localIP().toString());

    setupServer();

    tickerImAlive.attach(tickerTimer, reportHealthWrapper);
    tickerAtualizarCache.attach(tickerTimer2, updateCacheWrapper);

    updateCache();
  }

  void loop()
  {
    server.handleClient();
    checkRFIDCard();
  }

private:
  static void reportHealthWrapper()
  {
    if (instance)
      instance->reportHealth();
  }

  static void updateCacheWrapper()
  {
    if (instance)
      instance->updateCache();
  }

  void reportHealth()
  {
    apiClient.reportHealth();
  }

  void updateCache()
  {
    rfidCache = apiClient.updateCache();
  }

  void toggleRegistrationMode()
  {
    registrationMode = !registrationMode;
    apiClient.logEvent("INFO", registrationMode ? "Modo de cadastro ativado" : "Modo de operação normal ativado");
    hardware.setLED(registrationMode);
  }

  bool authenticateRFID(const String &rfidCode)
  {
    if (rfidCache.find(rfidCode) != rfidCache.end() && rfidCache[rfidCode].first)
    {
      return true;
    }

    hardware.beepError();
    apiClient.logEvent("WARN", "RFID não autorizado: " + rfidCode);
    return false;
  }

  void checkRFIDCard()
  {
    if (rfidReader.isCardPresent())
    {
      hardware.beepShort();

      const String tag = rfidReader.readCard();
      if (!tag.isEmpty())
      {
        

        if (registrationMode)
        {
          if (apiClient.registerRFID(tag))
          {
            hardware.beepSuccess();
            updateCache();
          }
          else
          {
            hardware.beepWarning();
          }
        }
        else if (authenticateRFID(tag))
        {
          hardware.unlockDoor();
        }
        apiClient.logEvent("INFO", "Cartão detectado: " + tag);
      }

      delay(800);
      rfidReader.clearBuffer();
    }
  }

  void setupServer()
  {
    server.on("/mode", HTTP_GET, []()
              {
            if (instance->validateToken()) {
                instance->server.send(200, "application/json", 
                    "{\"success\": true, \"mode\": \"" + 
                    String(instance->registrationMode ? "cadastro" : "operacao") + "\"}");
            } });

    server.on("/toggle-mode", HTTP_GET, []()
              {
            if (instance->validateToken()) {
                instance->toggleRegistrationMode();
                instance->server.send(200, "application/json", 
                    "{\"success\": true, \"mode\": \"" + 
                    String(instance->registrationMode ? "cadastro" : "operacao") + "\"}");
            } });

    server.on("/open-door", HTTP_GET, []()
              {
            if (instance->validateToken()) {
                instance->hardware.unlockDoor();
                instance->server.send(200, "application/json", 
                    "{\"success\": true, \"message\": \"Porta aberta com sucesso\"}");
            } });

    server.on("/cache", HTTP_GET, []()
              {
            if (instance->validateToken()) {
                instance->server.send(200, "application/json", 
                    "{\"success\": true, \"message\": \" Cache atualizado com sucesso \"}");
                instance->updateCache();
            } });

    server.begin();
  }

  bool validateToken()
  {
    if (server.hasHeader("Authorization") &&
        server.header("Authorization") == "Bearer " + BEARER_TOKEN)
    {
      return true;
    }

    server.send(401, "application/json",
                "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
    return false;
  }
};

DoorControlSystem *DoorControlSystem::instance = nullptr;

// Configurações globais
const char *ssid = "ifc_wifi";
const char *password = "";
const int TickerTimer = 500;
const int TickerTimer2 = 600;

String getApiUrl() {
    if (MODE == "PROD") {
        return "https://door-api.fabricadesoftware.ifc.edu.br/api";
    }
    else if (MODE == "DEV") {
        return "http://meu-ip:8000/api";
    }
    return "http://meu-ip:8000/api";
}

HardwareSerial rfidSerial(1);
const String ApiUrl = getApiUrl();
DoorControlSystem doorSystem(ssid, password, ApiUrl, BEARER_TOKEN, TickerTimer, TickerTimer2, 19003, rfidSerial);

void setup()
{
  Serial.begin(9600);
  doorSystem.setup();
}

void loop()
{
  doorSystem.loop();
}