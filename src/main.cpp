#include <SPI.h>
#include <WiFi.h>
#include "FS.h"
#include <EthernetENC.h>
#include <EthernetUdp.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // Endereço MAC
IPAddress ip(192, 168, 1, 177);                    // IP do Arduino
unsigned int localPort = 8888;                     // Porta UDP para escutar

const String BEARER_TOKEN = "fabdor-dPluQTwdJJ4tamtnP0i7J34UqphHuJTdUugKt2YMJgQeoAS5qs1fFi4My";

EthernetUDP Udp; // Objeto para comunicação UDP
bool test = false;

void sendHttpPost(String json)
{
  EthernetClient client;
  if (client.connect("192.168.1.10", 19003))
  {
    client.println("POST /api/health/ip HTTP/1.1");
    client.println("Host: 192.168.1.10");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(json.length());
    client.println();
    client.println(json);
    client.stop();
  }
  else
  {
    Serial.println("Conexão com o servidor falhou.");
  }
}

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

    // Converte o pacote em uma string
    String mensagem = String(packetBuffer);
    mensagem.trim();

    // Verifica se a mensagem começa com o token Bearer
    if (mensagem.startsWith("Bearer(" + BEARER_TOKEN + ")"))
    {
      // Extrai o comando após o token
      String command = mensagem.substring(mensagem.indexOf(' ') + 1);
      command.trim(); // Remove espaços em branco antes e depois do comando

      // Remove aspas no início e no final, se existirem
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

        // Enviar solicitação HTTP
        String json = "{\"ip\": \"" + WiFi.localIP().toString() + "\"}"; // Use o IP correto aqui
        sendHttpPost(json);

        // Envia uma resposta de volta ao cliente
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write("LED Toggled!");
        Udp.endPacket();
      }
      else
      {
        // Responde que o comando não foi reconhecido
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write("Comando não reconhecido.");
        Udp.endPacket();
      }
    }
    else
    {
      // Responde que a autenticação falhou
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("Autenticação falhou.");
      Udp.endPacket();
    }
  }
}
