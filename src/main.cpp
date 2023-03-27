#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "SNOW";
const char *password = "poopfart";

String ApiUrl = "https://doorauthmock-production.up.railway.app/";

#define SS_PIN 21
#define RST_PIN 22
#define CARD_READ_DELAY 2000 // Tempo em milissegundos para esperar antes de ler outro cartão
// #define RELAY_PIN 7
#define DEBOUNCE_DELAY 50 // Tempo em milissegundos para esperar antes de mudar o estado do relé novamente

MFRC522 rfid(SS_PIN, RST_PIN);
unsigned long lastCardReadTime = 0;
bool isRelayOn = false;

/* Funçao para verificar se a Api está no ar*/
bool checkApi()
{
  HTTPClient http;
  http.begin(ApiUrl);
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    Serial.printf("[HTTP] GET... code: %d n", httpCode);
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

void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando...");

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

  SPI.begin();
  rfid.PCD_Init();
  checkApi();
  // pinMode(RELAY_PIN, OUTPUT);
}

void loop()
{
  // Verifica se há um novo cartão presente ou se o cartão atual foi lido novamente
  if (rfid.PICC_IsNewCardPresent() || rfid.PICC_ReadCardSerial())
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

      // Liga o relé e aguarda um tempo antes de desligá-lo novamente
      // digitalWrite(RELAY_PIN, HIGH);
      Serial.println("ABRINDO");
      // isRelayOn = true;
      delay(DEBOUNCE_DELAY);
      // digitalWrite(RELAY_PIN, LOW);
      Serial.println("FECHANDO");
      // isRelayOn = false;

      // Reinicia o leitor RFID para procurar novos cartões
      rfid.PCD_Init();

      // Registra a hora da última leitura
      lastCardReadTime = millis();
    }
  }
}

/* Função para ler o cartão RFID e retornar o conteúdo como uma string.
 * Retorna uma string vazia se não houver cartão presente ou se ocorrer um erro na leitura.
 */
String leCartaoRFID()
{
  if (!rfid.PICC_IsNewCardPresent())
  {
    return "";
  }

  if (!rfid.PICC_ReadCardSerial())
  {
    return "";
  }

  String conteudo = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    conteudo.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    conteudo.concat(String(rfid.uid.uidByte[i], HEX));
  }
  conteudo.toUpperCase();

  return conteudo;
}
