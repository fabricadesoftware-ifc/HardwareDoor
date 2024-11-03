#include <SPI.h>
#include "FS.h"
#include <WiFi.h>
#include <EthernetENC.h>
#include <EthernetUdp.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // Endereço MAC
IPAddress ip(192, 168, 1, 177);                    // IP do Arduino
unsigned int localPort = 8888;                     // Porta UDP para escutar

EthernetUDP Udp; // Objeto para comunicação UDP
bool test = false;

void setup()
{
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // Iniciar o Ethernet com o endereço MAC e IP
  Ethernet.init(5); // pino CS do módulo Ethernet
  Ethernet.begin(mac, ip);

  Serial.print("Conectando ao Ethernet...");
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  // Iniciar UDP na porta especificada
  Udp.begin(localPort);
  Serial.print("Escutando na porta UDP: ");
  Serial.println(localPort);
}

void loop()
{
  // Verifica se há pacotes UDP recebidos
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // Buffer para armazenar os dados recebidos
    char packetBuffer[255];
    int len = Udp.read(packetBuffer, 255);
    if (len > 0)
    {
      packetBuffer[len] = 0; // Finaliza a string recebida
    }

    Serial.print("Pacote recebido: ");
    Serial.println(packetBuffer);

    // Remove aspas no início e no final, se existirem
    String command = String(packetBuffer);
    command.trim();
    if (command.startsWith("\"") && command.endsWith("\""))
    {
      command = command.substring(1, command.length() - 1);
    }
    command.toLowerCase();
    // Se o comando recebido for "toggle", altera o estado do LED
    Serial.print("Comando recebido: " + command);
    if (command == "toggle")
    {
      digitalWrite(LED_BUILTIN, test);
      test = !test;

      // Envia uma resposta de volta ao cliente
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("LED Toggled!");
      Udp.endPacket();
    }
    else
    {
      // Responde que o comando não foi reconhecido
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("Comando nao reconhecido.");
      Udp.endPacket();
    }
  }
}