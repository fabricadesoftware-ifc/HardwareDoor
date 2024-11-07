#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Ticker.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

const char *ssid = "ifc_wifi";
const char *password = "";
const int TickerTimer = 500;                     
String ApiUrl = "http://191.52.56.62:19003/api";
Ticker ticker;

const String BEARER_TOKEN = "fabdor-dPluQTwdJJ4tamtnP0i7J34UqphHuJTdUugKt2YMJgQeoAS5qs1fFi4My";
#define LED_BUILTIN 2

#define SS_PIN 21    // Pino SS para o leitor RFID
#define RST_PIN 22   // Pino RST para o leitor RFID
#define RELAY_PIN 13 // Pino para o relé (controle da porta)
#define BUZZER_PIN 12 // Pino para o buzzer

MFRC522 rfid(SS_PIN, RST_PIN);
WebServer server(19003);

bool cadastroAtivo = false; 

void logEvent(String type, String message)
{
  String logTime = String(millis() / 1000); 
  Serial.println("[" + logTime + "s] " + message);
  HTTPClient http;
  String json = "{\"type\": \"" + type + "\", \"message\": \"" + message + "\"}";
  http.begin(ApiUrl + "/logs");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(json);
  http.end();
}

void imAlive()
{
  HTTPClient http;
  String json = "{\"ip\": \"" + WiFi.localIP().toString() + "\"}";
  http.begin(ApiUrl + "/health/ip");
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(json);
  http.end();
}

void unlock_door()
{
  digitalWrite(RELAY_PIN, HIGH);
  delay(50);
  digitalWrite(RELAY_PIN, LOW); 
}

// Função para verificar e autenticar RFID na API
bool auth_rfid(String rfidCode)
{
  HTTPClient http;
  http.begin(ApiUrl + "/tags/door"); // Define o endpoint da API;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + BEARER_TOKEN);
  http.setTimeout(5000);
  String json = "{\"rfid\": \"" + rfidCode + "\"}";
  int httpCode = http.POST(json);


  if (httpCode == HTTP_CODE_OK)
  {
    return true;
  }
  else if (httpCode == HTTP_CODE_UNAUTHORIZED)
  {
    logEvent("WARN", "RFID não autorizado");
    return false;
  }
  http.end();
  return false;
}

// Função para cadastrar um novo RFID
void cadastrar_rfid(String rfidCode)
{
  HTTPClient http;
  logEvent("INFO", "Cadastrando RFID: " + rfidCode);
  http.begin(ApiUrl + "/tags/" + rfidCode); // Define o endpoint da API
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + BEARER_TOKEN);
  int httpCode = http.POST("{}");

  if (httpCode == HTTP_CODE_CREATED)
  {
    logEvent("SUCESS", "RFID cadastrado com sucesso");
  }
  else if (httpCode == HTTP_CODE_CONFLICT)
  {
    logEvent("WARN", "RFID já cadastrado");
  }
  else
  {
    logEvent("ERROR","Erro ao cadastrar RFID");
  }
}

// Função para alternar entre modo cadastro e normal
void alternarModoCadastro()
{
  cadastroAtivo = !cadastroAtivo;
  logEvent("INFO", cadastroAtivo ? "Modo de cadastro ativado" : "Modo de operação normal ativado");
  digitalWrite(LED_BUILTIN, cadastroAtivo ? HIGH : LOW); // LED indica modo ativo
}

// Função para processar cartões RFID
void processarCartao()
{
  String rfidCode = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    rfidCode += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    rfidCode += String(rfid.uid.uidByte[i], HEX);
  }
  rfidCode.toUpperCase();

  if (cadastroAtivo)
  {
    cadastrar_rfid(rfidCode); // Cadastra o RFID no modo de cadastro
  }
  else
  {
    if (auth_rfid(rfidCode))
    {
      unlock_door(); // Desbloqueia a porta se o RFID for autorizado
    }
  }
  SPI.begin();
  rfid.PCD_Init();
}

// Função para processar leituras de RFID
void verificarCartaoRFID()
{
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
    digitalWrite(BUZZER_PIN, LOW);
    processarCartao();
    logEvent("INFO", "Cartão detectado.");
    rfid.PICC_HaltA(); // Finaliza a leitura do cartão
  }
}

// Função para iniciar o servidor e rotas
void iniciarServidor()
{
  // Rota para alternar modo cadastro/normal
  server.on("/toggle-mode", HTTP_GET, []()
            {
    if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
      alternarModoCadastro();
      server.send(200, "application/json", "{\"success\": true, \"mode\": \"" + String(cadastroAtivo ? "cadastro" : "operacao") + "\"}");
    } else {
      server.send(401, "application/json", "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
    } });

  // Rota para abrir a porta manualmente
  server.on("/open-door", HTTP_GET, []()
            {
    if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
      unlock_door();
      server.send(200, "application/json", "{\"success\": true, \"message\": \"Porta aberta com sucesso\"}");
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

  SPI.begin();
  rfid.PCD_Init();
  ticker.attach(TickerTimer, imAlive);

  iniciarServidor();
}

void loop()
{
  server.handleClient(); // Processa as requisições HTTP
  verificarCartaoRFID(); // Verifica se há cartões RFID
}
