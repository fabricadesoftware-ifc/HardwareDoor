#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <time.h>

// Configurações de rede
const char *ssid = "ifc_wifi";
const char *password = "";
String ApiUrl = "http://191.52.59.34:8087"; // URL da API

const String BEARER_TOKEN = "fabrica2420"; // Defina seu token secreto aqui

// Pinos de conexão
#define SS_PIN 21    // Pino SS para o leitor RFID
#define RST_PIN 22   // Pino RST para o leitor RFID
#define RELAY_PIN 13 // Pino para o relé (controle da porta)

// Definições de tempo
#define CARD_READ_DELAY 10 // Tempo entre leituras de cartão RFID (em ms)
#define DEBOUNCE_DELAY 50  // Tempo de debounce para o relé (em ms)

// Inicialização do leitor RFID
MFRC522 rfid(SS_PIN, RST_PIN);
WebServer server(80);               // Servidor HTTP na porta 80
unsigned long lastCardReadTime = 0; // Armazena o último tempo de leitura do cartão

/* Função de logging com data, hora e tipo, grava em arquivo .log */
void logEvent(String type, String description)
{
  // Obtém a data e hora atual
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("[ERROR] Falha ao obter a hora.");
    return;
  }

  // Formata a data e hora
  char timeString[25];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);

  // Constrói a mensagem de log
  String logMessage = "[" + String(timeString) + "][" + type + "] " + description;
  Serial.println(logMessage); // Exibe o log no Serial Monitor

  // Grava o log no arquivo .log em SPIFFS
  File logFile = SPIFFS.open("/logs.log", FILE_APPEND);
  if (!logFile)
  {
    Serial.println("[ERROR] Falha ao abrir o arquivo de log.");
    return;
  }

  logFile.println(logMessage); // Escreve o log no arquivo
  logFile.close();             // Fecha o arquivo
}

/* Função para verificar o status da API */
bool check_api()
{
  HTTPClient http;
  logEvent("INFO", "Verificando a API...");
  http.begin(ApiUrl);        // Inicia a requisição HTTP
  http.setTimeout(5000);     // Timeout de 5 segundos
  int httpCode = http.GET(); // Envia requisição GET

  logEvent("INFO", "[HTTP] GET... code: " + String(httpCode));
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    logEvent("INFO", "API está no ar. Resposta: " + payload);
  }
  else
  {
    logEvent("ERROR", "API falhou, código: " + String(httpCode));
    return false; // Retorna false se a API não respondeu corretamente
  }

  http.end();
  return true; // Retorna true se a API está operando corretamente
}

/* Função para desbloquear a porta */
void unlock_door()
{
  logEvent("INFO", "Desbloqueando porta...");
  digitalWrite(RELAY_PIN, HIGH); // Liga o relé para abrir a porta
  delay(1000);                   // Mantém a porta aberta por mais tempo (1 segundo)
  digitalWrite(RELAY_PIN, LOW);  // Desliga o relé para fechar a porta
  logEvent("INFO", "Porta bloqueada.");
}

/* Função para reconectar ao Wi-Fi */
void reconnectWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    logEvent("INFO", "WiFi desconectado. Tentando reconectar...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
    {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      logEvent("INFO", "Reconectado ao WiFi.");
    }
    else
    {
      logEvent("ERROR", "Falha na reconexão ao WiFi.");
    }
  }
}

void setup()
{
  // Inicia a comunicação serial
  Serial.begin(9600);
  logEvent("INFO", "Iniciando...");

  // Inicializa o SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("Erro ao montar o sistema de arquivos");
    return;
  }
  logEvent("INFO", "SPIFFS montado com sucesso.");

  // Configura o pino do relé como saída
  pinMode(RELAY_PIN, OUTPUT);

  // Configura NTP para obter hora
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Conecta ao WiFi
  WiFi.mode(WIFI_STA); // Define o WiFi como estação
  WiFi.begin(ssid, password);
  logEvent("INFO", "Conectando ao WiFi...");

  // Loop para aguardar conexão WiFi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  logEvent("INFO", "Conectado ao WiFi.");
  logEvent("INFO", "Endereço IP: " + WiFi.localIP().toString()); // Exibe o IP do dispositivo


  check_api(); // Verifica a API
  // Configura a rota para retornar os logs
  server.on("/logs", HTTP_GET, []()
            {
              if (!SPIFFS.exists("/logs.log"))
              {
                server.send(404, "text/plain", "Arquivo de logs não encontrado.");
                return;
              }

              File logFile = SPIFFS.open("/logs.log", FILE_READ);
              if (!logFile)
              {
                server.send(500, "text/plain", "Erro ao abrir o arquivo de logs.");
                return;
              }

              String logs = "";
              while (logFile.available())
              {
                logs += logFile.readStringUntil('\n') + "\n"; // Lê cada linha do arquivo e adiciona à string
              }
              logFile.close();

              server.send(200, "text/plain", logs); // Envia os logs como resposta
            });

  server.begin(); // Inicia o servidor
}

void loop()
{
  // Verifica o WiFi e tenta reconectar se necessário
  reconnectWiFi();

  // Processa as requisições HTTP
  server.handleClient();
}
