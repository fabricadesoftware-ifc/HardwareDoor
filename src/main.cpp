#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "ifc_wifi";
const char *password = "";

String ApiUrl = "http://191.52.56.197:8087";

#define SS_PIN 21
#define RST_PIN 22
#define CARD_READ_DELAY 2000 // Tempo em milissegundos para esperar antes de ler outro cartão
#define RELAY_PIN 13         // Relay at pin 13
#define DEBOUNCE_DELAY 50    // Tempo em milissegundos para esperar antes de mudar o estado do relé novamente

MFRC522 rfid(SS_PIN, RST_PIN);
unsigned long lastCardReadTime = 0;
bool isRelayOn = false;

/* Funçao para verificar se a Api está no ar*/
bool check_api()
{
  HTTPClient http;
  http.begin(ApiUrl);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    Serial.printf("[HTTP] GET... code: %d", httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println(payload);
    }
  }
  else
  {
    Serial.printf("[HTTP] GET... failed, error: %s n", http.errorToString(httpCode).c_str());
    return false;
  }
  http.end();
  return true;
}

bool auth_rfid(String rfid)
{
  /*
   * Receive the rfid from the reader, and then, make a post to apiUrl/door/
   * Passing the rfid on the body of the request, as a json
   * If the rfid is valid, the api will return a 200 status code
   * If the rfid is invalid, the api will return a 401 status code
   * If the api is down, the api will return a 500 status code
   */

  HTTPClient http;
  http.begin(ApiUrl + "/door/"); // Specify destination for HTTP request
  http.addHeader("Content-Type", "application/json");
  String json = "{\"rfid\": \"" + rfid + "\"}";
  int httpCode = http.POST(json); // Send the request

  if (httpCode > 0)
  {
    Serial.printf("[HTTP] POST... code: %d", httpCode);
    if (httpCode == HTTP_CODE_OK)
    {
      String payload = http.getString();
      Serial.println(payload);
      return true;
    }
    if (httpCode == HTTP_CODE_UNAUTHORIZED)
    {
      Serial.println("Cartao nao autorizado");
      return false;
    }
  }
  else
  {
    Serial.printf("[HTTP] POST... failed, error: %s n", http.errorToString(httpCode).c_str());
    return false;
  }

  http.end();
}

void unlock_door()
{
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("ABRINDO");
  isRelayOn = true;
  delay(DEBOUNCE_DELAY);
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("FECHANDO");
  isRelayOn = false;
  delay(2000);
  rfid.PCD_Init();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando...");

  pinMode(RELAY_PIN, OUTPUT);

  WiFi.mode(WIFI_STA); // Optional
  WiFi.begin(ssid, password);

  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.dnsIP());

  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Leitor RFID inicializado, aguardando cartoes...");
  check_api();
  // pinMode(RELAY_PIN, OUTPUT);
}

void loop()
{
  // Verifica se há um novo cartão presente ou se o cartão atual foi lido novamente

  // if (!rfid.PICC_IsNewCardPresent())
  // {
  //   return;
  // }

  // if (!rfid.PICC_ReadCardSerial())
  // {
  //   return;
  // }

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
      Serial.println("Card content: " + conteudo);
      // if conteudo is not empty, then send to the api
      if (conteudo != "")
      {
        if (auth_rfid(conteudo))
        {
          unlock_door();
        }
        else
        {
          Serial.println("Porta nao desbloqueada");
        }
      }
      return;
    }

    // Registra a hora da última leitura
    lastCardReadTime = millis();
  }
}
