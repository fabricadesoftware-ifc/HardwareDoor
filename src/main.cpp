#include <SPI.h>
#include <EthernetENC.h>
#include "FS.h"
#include <WiFi.h>
#include <WebServer.h>
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

WebServer server(80); // Inicializa o WebServer na porta 80
void serverConfig()
{

  Serial.println("Servidor Web iniciado.");
};

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Falha ao iniciar a Ethernet, usando IP fixo...");
    Ethernet.begin(mac);
  }
  delay(1000);

  Serial.print("Conectado na Ethernet com IP: ");
  Serial.println(Ethernet.localIP());

  serverConfig();
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
