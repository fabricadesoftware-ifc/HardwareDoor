#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "ifc_wifi";
const char *password = "";

String ApiUrl = "http://191.52.59.34:8087";

#define SS_PIN 21
#define RST_PIN 22
#define CARD_READ_DELAY 1000 // Tempo em milissegundos para esperar antes de ler outro cartão
#define RELAY_PIN 13         // Relay at pin 13
#define DEBOUNCE_DELAY 50    // Tempo em milissegundos para esperar antes de mudar o estado do relé novamente

MFRC522 rfid(SS_PIN, RST_PIN);
unsigned long lastCardReadTime = 0;
bool isRelayOn = false;

/* Função para verificar se a API está no ar */
bool check_api()
{
  HTTPClient http;
  Serial.println("Verificando a API...");
  http.begin(ApiUrl);
  int httpCode = http.GET();

  Serial.printf("[HTTP] GET... code: %d\n", httpCode);

  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println("API está no ar. Resposta:");
      Serial.println(payload);
    }
    else
    {
      Serial.println("API não está respondendo como esperado.");
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... falhou, erro: %s\n", http.errorToString(httpCode).c_str());
    return false;
  }

  http.end();
  return true;
}

bool auth_rfid(String rfid)
{
  HTTPClient http;
  Serial.println("Autenticando RFID...");
  http.begin(ApiUrl + "/door/"); // Specify destination for HTTP request
  http.addHeader("Content-Type", "application/json");
  String json = "{\"rfid\": \"" + rfid + "\"}";

  Serial.print("Enviando POST com payload: ");
  Serial.println(json);

  int httpCode = http.POST(json); // Send the request

  Serial.printf("[HTTP] POST... code: %d\n", httpCode);

  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println("Autenticação bem-sucedida. Resposta:");
      Serial.println(payload);
      return true;
    }
    else if (httpCode == HTTP_CODE_UNAUTHORIZED)
    {
      Serial.println("Cartão não autorizado");
      return false;
    }
    else
    {
      Serial.printf("Resposta inesperada. Código HTTP: %d\n", httpCode);
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... falhou, erro: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return false;
}

void unlock_door()
{
  Serial.println("Desbloqueando porta...");
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("ABRINDO");
  delay(DEBOUNCE_DELAY);
  Serial.println("FECHANDO");
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("Porta desbloqueada");
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando...");

  pinMode(RELAY_PIN, OUTPUT);

  WiFi.mode(WIFI_STA); // Optional
  WiFi.begin(ssid, password);

  Serial.println("\nConectando ao WiFi...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado ao WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.dnsIP());

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Leitor RFID inicializado, aguardando cartões...");

  check_api();
}

void loop()
{
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    // Verifica se já passou tempo suficiente desde a última leitura
    if (millis() - lastCardReadTime > CARD_READ_DELAY)
    {
      // Lê o conteúdo do cartão e o imprime no console
      String conteudo = "";
      for (byte i = 0; i < rfid.uid.size; i++)
      {
        conteudo.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
        conteudo.concat(String(rfid.uid.uidByte[i], HEX));
      }
      conteudo.toUpperCase();
      Serial.println("Conteúdo do cartão: " + conteudo);

      // Se o conteúdo não estiver vazio, envia para a API
      if (conteudo != "")
      {
        if (auth_rfid(conteudo))
        {
          unlock_door();
        }
        else
        {
          Serial.println("Porta não desbloqueada");
        }
      }
      lastCardReadTime = millis();
    }
  }
}
