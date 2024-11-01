#include <UIPEthernet.h> // Biblioteca para ENC28J60
#include <HTTPClient.h> // Biblioteca para HTTP Client
#include <SPI.h>
#include "FS.h"
#include <WebServer.h>
#define ETH_CS 5   
      // Defina o pino CS usado pelo ENC28J60
WebServer server(19003);

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
server.on("/open-door", HTTP_GET, []()
          {
            server.send(200, "text/plain", "Porta aberta!");
          });

server.begin();
}

void loop()
{

}