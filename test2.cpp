#include <SPI.h>
#include <WiFi.h>
#include "FS.h"
#include <Arduino.h>
#include <EthernetENC.h>
#include <EthernetUDP.h>
#include <MFRC522.h>
#include <Ticker.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // Endereço MAC
IPAddress ip(192, 168, 1, 177);                    // OPCIONAL

const int TickerTimer = 500;
String ApiUrl = "http://192.168.1.111:19003/api";
Ticker ticker;

const String BEARER_TOKEN = "fabdor-dPluQTwdJJ4tamtnP0i7J34UqphHuJTdUugKt2YMJgQeoAS5qs1fFi4My";
#define LED_BUILTIN 2
#define SS_PIN 21    // Pino SS para o leitor RFID
#define RST_PIN 22   // Pino RST para o leitor RFID
#define RELAY_PIN 13 // Pino para o relé (controle da porta)

MFRC522 rfid(SS_PIN, RST_PIN);
EthernetUDP Udp;
unsigned int localUdpPort = 8888; // Porta para receber comandos via UDP
bool cadastroAtivo = false;       // Indica se está no modo de cadastro

void sendHttpPost(String json, String url)
{
    EthernetClient client;
    if (client.connect("192.168.1.10", 19003))
    {
        Serial.println("Conexão bem-sucedida ao servidor.");

        // Enviar requisição HTTP POST
        client.println("POST /api" + url + " HTTP/1.1");
        client.println("Host: 192.168.1.10");
        client.println("Content-Type: application/json");
        client.println("Authorization: Bearer " + BEARER_TOKEN);
        client.print("Content-Length: ");
        client.println(json.length());
        client.println(); // Linha em branco que finaliza os cabeçalhos
        client.println(json);

        // Ler a resposta do servidor
        String response = "";
        while (client.available())
        {
            char c = client.read();
            response += c; // Adiciona o caractere lido à resposta
        }
        Serial.println(client.status());
        Serial.println("Resposta do servidor:");
        Serial.println(response); // Imprime a resposta no Serial Monitor
    }
    else
    {
        Serial.println("Conexão com o servidor falhou.");
    }
   // client.stop();
}

void logEvent(String type, String message)
{
    String logTime = String(millis() / 1000);
    Serial.println("[" + logTime + "s] " + message);
    String json = "{\"type\": \"" + type + "\", \"message\": \"" + message + "\"}";
    sendHttpPost(json, "/logs");
}

void imAlive()
{
    HTTPClient http;
    String json = "{\"ip\": \"" + Ethernet.localIP().toString() + "\"}";
    sendHttpPost(json, "/health/ip");
    logEvent("INFO", "[HTTP] POST... imAlive: ");
}

void unlock_door()
{
    digitalWrite(RELAY_PIN, HIGH);
    delay(50);
    digitalWrite(RELAY_PIN, LOW);
}

bool auth_rfid(String rfidCode)
{

    String json = "{\"rfid\": \"" + rfidCode + "\"}";
    httpCode = sendHttpPost(json, "/tags/door");

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

void cadastrar_rfid(String rfidCode)
{
    logEvent("INFO", "Cadastrando RFID: " + rfidCode);
    HTTPClient http;
    http.begin(ApiUrl + "/tags/" + rfidCode);
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

void alternarModoCadastro()
{
    cadastroAtivo = !cadastroAtivo;
    logEvent("INFO", cadastroAtivo ? "Modo de cadastro ativado" : "Modo de operação normal ativado");
    digitalWrite(LED_BUILTIN, cadastroAtivo ? HIGH : LOW);
}

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
        cadastrar_rfid(rfidCode);
    }
    else
    {
        if (auth_rfid(rfidCode))
        {
            unlock_door();
        }
    }
    SPI.begin();
    rfid.PCD_Init();
}

void verificarCartaoRFID()
{
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
    {
        logEvent("INFO", "Cartão detectado.");
        processarCartao();
        rfid.PICC_HaltA();
    }
}

void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    Ethernet.init(5); // pino CS do módulo Ethernet
    Ethernet.begin(mac, ip);

    Serial.print("Conectando ao Ethernet...");
    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());

    SPI.begin();
    rfid.PCD_Init();
    ticker.attach(TickerTimer, imAlive);

    // Iniciar UDP
    Udp.begin(localUdpPort);
    Serial.print("Escutando na porta UDP: ");
    Serial.println(localUdpPort);
}

void processarComandoUDP(String comando)
{
    comando.trim();
    comando.toLowerCase();

    if (comando == "toggle")
    {
        alternarModoCadastro();
    }
    else if (comando == "open")
    {
        unlock_door();
    }
    else
    {
        logEvent("ERROR", "Comando UDP desconhecido: " + comando);
    }
}

void loop()
{
    verificarCartaoRFID();

    int packetSize = Udp.parsePacket();
    if (packetSize)
    {
        char packetBuffer[255];
        int len = Udp.read(packetBuffer, 255);
        if (len > 0)
        {
            packetBuffer[len] = 0;
        }
        Serial.print("Pacote UDP recebido: ");
        Serial.println(packetBuffer);

        processarComandoUDP(String(packetBuffer));
    }
}
