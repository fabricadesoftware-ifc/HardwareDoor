#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Configurações de rede
const char *ssid = "ifc_wifi";
const char *password = "";
String ApiUrl = "http://191.52.59.34:8087"; // URL da API

// Pinos de conexão
#define SS_PIN 21    // Pino SS para o leitor RFID
#define RST_PIN 22   // Pino RST para o leitor RFID
#define RELAY_PIN 13 // Pino para o relé (controle da porta)

// Definições de tempo
#define CARD_READ_DELAY 2000 // Tempo entre leituras de cartão RFID (em ms)
#define DEBOUNCE_DELAY 500   // Tempo de debounce para o relé (em ms)

// Inicialização do leitor RFID
MFRC522 rfid(SS_PIN, RST_PIN);
unsigned long lastCardReadTime = 0; // Armazena o último tempo de leitura do cartão

/* Função para verificar o status da API */
bool check_api()
{
  HTTPClient http;
  Serial.println("Verificando a API...");
  http.begin(ApiUrl);        // Inicia a requisição HTTP
  http.setTimeout(10000);    // Timeout de 10 segundos
  int httpCode = http.GET(); // Envia requisição GET

  Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    Serial.println("API está no ar. Resposta:");
    Serial.println(payload);
  }
  else
  {
    Serial.printf("API falhou, código: %d\n", httpCode);
    return false; // Retorna false se a API não respondeu corretamente
  }

  http.end();
  return true; // Retorna true se a API está operando corretamente
}

/* Função para autenticar RFID na API */
bool auth_rfid(String rfidCode)
{
  HTTPClient http;
  Serial.println("Autenticando RFID...");
  http.begin(ApiUrl + "/door/");                      // Define o endpoint da API para autenticação
  http.addHeader("Content-Type", "application/json"); // Define o cabeçalho da requisição
  http.setTimeout(10000);                             // Timeout de 10 segundos

  // Monta o payload em formato JSON
  String json = "{\"rfid\": \"" + rfidCode + "\"}";
  Serial.print("Enviando POST com payload: ");
  Serial.println(json);

  int httpCode = http.POST(json); // Envia a requisição POST

  Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    Serial.println("Autenticação bem-sucedida. Resposta:");
    Serial.println(payload);
    return true; // Retorna true se o RFID foi autorizado
  }
  else if (httpCode == HTTP_CODE_UNAUTHORIZED)
  {
    Serial.println("RFID não autorizado.");
    return false; // Retorna false se o RFID não foi autorizado
  }
  else
  {
    Serial.printf("Falha na autenticação. Código HTTP: %d\n", httpCode);
  }

  http.end();
  return false; // Retorna false em caso de erro
}

/* Função para desbloquear a porta */
void unlock_door()
{
  Serial.println("Desbloqueando porta...");
  digitalWrite(RELAY_PIN, HIGH); // Liga o relé para abrir a porta
  delay(DEBOUNCE_DELAY);         // Mantém a porta aberta pelo tempo de debounce
  digitalWrite(RELAY_PIN, LOW);  // Desliga o relé para fechar a porta
  Serial.println("Porta bloqueada.");
}

void setup()
{
  // Inicia a comunicação serial
  Serial.begin(115200);
  Serial.println("Iniciando...");

  // Configura o pino do relé como saída
  pinMode(RELAY_PIN, OUTPUT);

  // Conecta ao WiFi
  WiFi.mode(WIFI_STA); // Define o WiFi como estação
  WiFi.begin(ssid, password);
  Serial.println("Conectando ao WiFi...");

  // Loop para aguardar conexão WiFi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado ao WiFi.");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP()); // Exibe o IP do dispositivo

  // Inicializa o leitor RFID
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Leitor RFID inicializado, aguardando cartões...");

  // Verifica se a API está disponível
  check_api();
}

void loop()
{
  // Verifica se o WiFi está conectado, tenta reconectar caso necessário
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi desconectado. Tentando reconectar...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nReconectado ao WiFi.");
  }

  // Verifica se um novo cartão foi apresentado no leitor
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    // Verifica o tempo desde a última leitura de cartão
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
      Serial.println("RFID lido: " + rfidCode);

      // Se o RFID não for vazio, tenta autenticar na API
      if (!rfidCode.isEmpty())
      {
        if (auth_rfid(rfidCode))
        {
          unlock_door(); // Desbloqueia a porta se o RFID for autorizado
        }
        else
        {
          Serial.println("RFID não autorizado. Porta não desbloqueada.");
        }
      }

      // Atualiza o tempo da última leitura de cartão
      lastCardReadTime = millis();
    }
  }
}
