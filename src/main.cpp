#include <UIPEthernet.h> // Biblioteca para ENC28J60
#include <HTTPClient.h> // Biblioteca para HTTP Client
#include <SPI.h>
#include "FS.h"
#include <WebServer.h>
#define ETH_CS 5         // Defina o pino CS usado pelo ENC28J60

byte mac[] = {0x7a, 0x12, 0x58, 0x5e, 0x6c, 0x33}; // Defina seu endereço MAC aqui

EthernetClient client;

void setup()
{
  Serial.begin(9600);

  // Inicia Ethernet com DHCP
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Falha na configuração Ethernet com DHCP");
  }

  // Exibe o IP obtido via DHCP
  Serial.print("Conectado com IP: ");
  Serial.println(Ethernet.localIP());
  WebServer server(19003);
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

void loop()
{
  if (Ethernet.localIP() != IPAddress(0, 0, 0, 0))
  {
    HTTPClient http;
    http.begin("http://www.google.com");
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      Serial.printf("Código HTTP: %d\n", httpCode);
      String payload = http.getString();
      Serial.println("Resposta:");
      Serial.println(payload);
    }
    else
    {
      Serial.printf("Erro na requisição: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
  else
  {
    Serial.println("Conexão de rede não estabelecida.");
  }

  delay(10000);
}