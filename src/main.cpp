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

const char *ssid = "ifc_wifi";
const char *password = "";
const int TickerTimer = 500;
const int TickerTimer2 = 600;
String ApiUrl = "http://191.52.59.130:3000/api";
Ticker tickerImAlive;
Ticker tickerAtualizarCache; 

#define BEARER_TOKEN_N BEARER_TOKEN_ENV // Correto! Sem `=`
const String BEARER_TOKEN = String(BEARER_TOKEN_N); // Correto!

#define LED_BUILTIN 2

#define SS_PIN 21     // Pino SS para o leitor RFID
#define RST_PIN 22    // Pino RST para o leitor RFID

#define RELAY_PIN 13  // Pino para o relé (controle da porta)
#define BUZZER_PIN 12 // Pino para o buzzer

#define RFID_RX 16 // Pino RX do RDM6300
#define RFID_TX 18 // Pino TX do RDM6300

HardwareSerial rfidSerial(1);
WebServer server(19003);

bool cadastroAtivo = false;

// Cache de tags autorizadas
std::map<String, std::pair<bool, unsigned long>> rfidCache;

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
  http.begin(ApiUrl + "/logs");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + BEARER_TOKEN);
  http.POST(json);
  http.end();
} //subir. a função me questão houve modificações: erro de json no back

void imAlive()
{
  HTTPClient http;
  String json = "{\"ip\": \"" + WiFi.localIP().toString() + "\"}";
  http.begin(ApiUrl + "/health/ip");
  http.addHeader("Content-Type", "application/json");
  http.POST(json);
  http.end();
}

void atualizarCache()
{
  HTTPClient http;
  http.begin(ApiUrl + "/tags/");
  // http.addHeader("Content-Type", "application/json"); (dando erro de "SyntaxError: Expected ',' or '}' after property value in JSON" na requisição)
  http.addHeader("Authorization", "Bearer " + BEARER_TOKEN);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
      logEvent("ERROR", "Erro ao analisar JSON: " + String(error.c_str()));
      return;
    }

    JsonArray tags = doc["data"].as<JsonArray>();
    rfidCache.clear(); // Limpa o cache antes de atualizar

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
  else
  {
    logEvent("ERROR", "Falha ao conectar com a API: " + http.errorToString(httpCode));
  }

  http.end();
}

void unlock_door()
{
  digitalWrite(RELAY_PIN, HIGH);
  delay(50);
  digitalWrite(RELAY_PIN, LOW);
}

bool auth_rfid(String rfidCode)
{
  if (rfidCache.find(rfidCode) != rfidCache.end() && rfidCache[rfidCode].first)
  {
    return true; // Tag encontrada no cache e válida
  }

  // Caso contrário, a tag não é válida
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
  logEvent("WARN", "RFID não autorizado: " + rfidCode);
  return false;
}

void cadastrar_rfid(String rfidCode)
{
  HTTPClient http;
  logEvent("INFO", "Cadastrando RFID: " + rfidCode);
  http.begin(ApiUrl + "/tags/" + rfidCode);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + BEARER_TOKEN);
  int httpCode = http.POST("{}");

  if (httpCode == HTTP_CODE_CREATED)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    logEvent("SUCCESS", "RFID cadastrado com sucesso");
    atualizarCache();

  }
  else if (httpCode == HTTP_CODE_CONFLICT)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    delay(50);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
    digitalWrite(BUZZER_PIN, LOW);
    logEvent("WARN", "RFID já cadastrado");
  }
  else
  {
    logEvent("ERROR", "Falha ao criar RFID: " + http.errorToString(httpCode));
  }
}

void alternarModoCadastro()
{
  cadastroAtivo = !cadastroAtivo;
  logEvent("INFO", cadastroAtivo ? "Modo de cadastro ativado" : "Modo de operação normal ativado");
  digitalWrite(LED_BUILTIN, cadastroAtivo ? HIGH : LOW); // LED indica modo ativo
}

String processarCartao()
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
    if (cadastroAtivo)
      cadastrar_rfid(rfidCode);
    else if (auth_rfid(rfidCode))
      unlock_door();
  }
  return rfidCode;
}

void verificarCartaoRFID()
{

  if (rfidSerial.available())
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    const String tag = processarCartao();
    logEvent("INFO", "Cartão detectado: " + tag);
    delay(800);
    while (rfidSerial.available() > 0)
    {
      rfidSerial.read();
    }
    //limpar cache do rfid

    //HTTPClient http;
    //String json = "{\"rfid\": \"" + tag + "\"}";
    //http.begin(ApiUrl + "/tags/door");
    //http.addHeader("Content-Type", "application/json");
    //http.addHeader("Authorization", "Bearer " + BEARER_TOKEN);
    //int httpCode = http.POST(json);
    //rfid.PICC_HaltA();
  }
}

void iniciarServidor()
{
  server.on("/mode", HTTP_GET, []()
            {
    if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
      server.send(200, "application/json", "{\"success\": true, \"mode\": \"" + String(cadastroAtivo ? "cadastro" : "operacao") + "\"}");
    } else {
      server.send(401, "application/json", "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
    } });

  server.on("/toggle-mode", HTTP_GET, []()
            {
    if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
      alternarModoCadastro();
      server.send(200, "application/json", "{\"success\": true, \"mode\": \"" + String(cadastroAtivo ? "cadastro" : "operacao") + "\"}");
    } else {
      server.send(401, "application/json", "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
    } });

  server.on("/open-door", HTTP_GET, []()
            {
    if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
      unlock_door();
      server.send(200, "application/json", "{\"success\": true, \"message\": \"Porta aberta com sucesso\"}");
    } else {
      server.send(401, "application/json", "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
    } });

  server.on("/cache", HTTP_GET, []()
            {
    if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
      server.send(200, "application/json", "{\"success\": true, \"message\": \" Cache atualizado com sucesso \"}");
      atualizarCache(); //buildar e subir seu macaco
    } else {
      server.send(401, "application/json", "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
    } });

  server.begin();
}

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  logEvent("INFO", "Conectando ao WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  logEvent("INFO", "Conectado ao WiFi. IP: " + WiFi.localIP().toString());

  rfidSerial.begin(9700, SERIAL_8N1, RFID_RX, RFID_TX);

  // Atualiza o cache imediatamente
  atualizarCache();

  tickerImAlive.attach(TickerTimer, imAlive);
  tickerAtualizarCache.attach(TickerTimer2, atualizarCache);

  iniciarServidor();
}

void loop()
{
  server.handleClient();
  verificarCartaoRFID();
}
