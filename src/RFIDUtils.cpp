#include "RFIDUtils.h"
#include <SPI.h>
#include <MFRC522.h>
#include "RelayUtils.h"
#include "Logging.h"
#include "ServerUtils.h"
#include <HTTPClient.h>

#define SS_PIN 21
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);
bool cadastroAtivo = false;
#define BUZZER_PIN 12

extern String ApiUrl;
extern String BEARER_TOKEN;

void initRFID()
{
    SPI.begin();
    rfid.PCD_Init();
}

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
        logEvent("ERROR", "Erro ao cadastrar RFID");
    }
}

bool auth_rfid(String rfidCode)
{
    HTTPClient http;
    http.begin(ApiUrl + "/tags/door");
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
        digitalWrite(BUZZER_PIN, HIGH);
        delay(500);
        digitalWrite(BUZZER_PIN, LOW);
        logEvent("WARN", "RFID não autorizado " + rfidCode);
        return false;
    }
    http.end();
    return false;
}

void verificarCartaoRFID()
{
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
        String rfidCode = "";
        for (byte i = 0; i < rfid.uid.size; i++)
        {
            rfidCode += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX);
        }
        rfidCode.toUpperCase();
        digitalWrite(BUZZER_PIN, HIGH);
        delay(50);
        digitalWrite(BUZZER_PIN, LOW);

        if (cadastroAtivo)
        {
            cadastrar_rfid(rfidCode);
        }
        else
        {
            if (auth_rfid(rfidCode))
            {
                unlock_door();
            }
        }
        logEvent("INFO", "Cartão detectado. " + rfidCode);
        rfid.PICC_HaltA();
    }
}
