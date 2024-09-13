#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

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

/* Função de logging */
void logEvent(String message)
{
  String logTime = String(millis() / 1000); // Tempo em segundos desde o início
  Serial.println("[" + logTime + "s] " + message);
}

/* Função para verificar o status da API */
bool check_api()
{
  HTTPClient http;
  logEvent("Verificando a API...");
  http.begin(ApiUrl);        // Inicia a requisição HTTP
  http.setTimeout(5000);     // Timeout de 5 segundos
  int httpCode = http.GET(); // Envia requisição GET

  logEvent("[HTTP] GET... code: " + String(httpCode));
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    logEvent("API está no ar. Resposta: " + payload);
  }
  else
  {
    logEvent("API falhou, código: " + String(httpCode));
    return false; // Retorna false se a API não respondeu corretamente
  }

  http.end();
  return true; // Retorna true se a API está operando corretamente
}

/* Função para autenticar RFID na API */
bool auth_rfid(String rfidCode)
{
  HTTPClient http;
  logEvent("Autenticando RFID...");
  http.begin(ApiUrl + "/door/");                      // Define o endpoint da API para autenticação
  http.addHeader("Content-Type", "application/json"); // Define o cabeçalho da requisição
  http.setTimeout(5000);                              // Timeout de 5 segundos

  // Monta o payload em formato JSON
  String json = "{\"rfid\": \"" + rfidCode + "\"}";
  logEvent("Enviando POST com payload: " + json);

  int httpCode = http.POST(json); // Envia a requisição POST

  logEvent("[HTTP] POST... code: " + String(httpCode));
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    logEvent("Autenticação bem-sucedida. Resposta: " + payload);
    return true; // Retorna true se o RFID foi autorizado
  }
  else if (httpCode == HTTP_CODE_UNAUTHORIZED)
  {
    logEvent("RFID não autorizado.");
    return false; // Retorna false se o RFID não foi autorizado
  }
  else
  {
    logEvent("Falha na autenticação. Código HTTP: " + String(httpCode));
  }

  http.end();
  return false; // Retorna false em caso de erro
}

/* Função para desbloquear a porta */
void unlock_door()
{
  logEvent("Desbloqueando porta...");
  digitalWrite(RELAY_PIN, HIGH); // Liga o relé para abrir a porta
  delay(1000);                   // Mantém a porta aberta por mais tempo (1 segundo)
  digitalWrite(RELAY_PIN, LOW);  // Desliga o relé para fechar a porta
  logEvent("Porta bloqueada.");
  SPI.begin();
  rfid.PCD_Init();
  logEvent("Leitor RFID reiniciando, aguardando cartões...");
}

/* Função para validar o token Bearer */
bool validate_bearer_token(WebServer &request)
{
  if (!request.hasHeader("Authorization"))
  {
    logEvent("Cabeçalho de autorização ausente.");
    return false;
  }

  String authHeader = request.header("Authorization");
  if (authHeader == "Bearer " + BEARER_TOKEN)
  {
    logEvent("Token Bearer validado com sucesso.");
    return true;
  }

  logEvent("Token Bearer inválido.");
  return false;
}

/* Função para reconectar ao Wi-Fi */
void reconnectWiFi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    logEvent("WiFi desconectado. Tentando reconectar...");
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
      logEvent("Reconectado ao WiFi.");
    }
    else
    {
      logEvent("Falha na reconexão ao WiFi.");
    }
  }
}

void setup()
{
  // Inicia a comunicação serial
  Serial.begin(9600);
  logEvent("Iniciando...");

  // Configura o pino do relé como saída
  pinMode(RELAY_PIN, OUTPUT);

  // Conecta ao WiFi
  WiFi.mode(WIFI_STA); // Define o WiFi como estação
  WiFi.begin(ssid, password);
  logEvent("Conectando ao WiFi...");

  // Loop para aguardar conexão WiFi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  logEvent("Conectado ao WiFi.");
  logEvent("Endereço IP: " + WiFi.localIP().toString()); // Exibe o IP do dispositivo

  // Inicializa o leitor RFID
  SPI.begin();
  rfid.PCD_Init();
  logEvent("Leitor RFID inicializado, aguardando cartões...");

  // Verifica se a API está disponível
  check_api();

  // Configura rota para abrir a porta via API
  server.on("/open-door", HTTP_GET, []()
            {
              if (validate_bearer_token(server)) {
                unlock_door();  // Chama a função para abrir a porta
                server.send(200, "text/json", "Sucess": True, "message": "Porta desbloqueada");  //
              } else {
                server.send(401, "text/json", "Não autorizado");  // Retorna status 401 se o token for inválido
              } });

  // Inicia o servidor
  server.begin();
}

void loop()
{
  // Verifica o WiFi e tenta reconectar se necessário
  reconnectWiFi();

  // Verifica se um novo cartão foi apresentado no leitor
  if (rfid.PICC_IsNewCardPresent() || rfid.PICC_ReadCardSerial())
  {
    logEvent("Cartão detectado.\n");
  }
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    // Verifica o tempo desde a última leitura de cartão
    logEvent("Cartão RFID detectado.");
    if (millis() - lastCardReadTime > CARD_READ_DELAY)
    {
      // Lê o UID do cartão RFID
      String rfidCode = "";
      for (byte i = 0; i < rfid.uid.size; i++)
      {
        rfidCode.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
        rfidCode.concat(String(rfid.uid.uidByte[i], HEX));
      }
      rfidCode.toUpperCase(); // Converte para maiúsculas
      logEvent("RFID lido: " + rfidCode);

      // Se o RFID não for vazio, tenta autenticar na API
      if (!rfidCode.isEmpty())
      {
        if (auth_rfid(rfidCode))
        {
          unlock_door(); // Desbloqueia a porta se o RFID for autorizado
        }
        else
        {
          logEvent("RFID não autorizado. Porta não desbloqueada.");
        }
      }

      // Atualiza o tempo da última leitura de cartão
      lastCardReadTime = millis();
    }
  }

  // Processa as requisições HTTP
  server.handleClient();
}
