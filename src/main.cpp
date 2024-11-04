#include <SPI.h>
#include <WiFi.h>
#include "FS.h"
#include <EthernetENC.h>

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // Endereço MAC                   // IP do Arduino
unsigned int localPort = 19003;                     // Porta UDP para escutar

const String BEARER_TOKEN = "fabdor-dPluQTwdJJ4tamtnP0i7J34UqphHuJTdUugKt2YMJgQeoAS5qs1fFi4My";

EthernetServer server(8087); // Objeto para comunicação TCP

bool test = false;

void sendHttpPost(String json)
{
  EthernetClient client;
  if (client.connect("191.52.56.62", 19003))
  {
    Serial.println("Conexão bem-sucedida ao servidor.");

    // Enviar requisição HTTP POST
    client.println("POST /api/health/ip HTTP/1.1");
    client.println("Host: 192.168.1.10");
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(json.length());
    client.println(); // Linha em branco que finaliza os cabeçalhos
    client.println(json);

    // Ler a resposta do servidor
    String response = client.readString();

    Serial.println("Resposta do servidor:");
    Serial.println(response); // Imprime a resposta no Serial Monitor

    // Extrair status code
    int firstSpaceIndex = response.indexOf('H');                       // Encontrar o primeiro espaço
    int secondSpaceIndex = response.indexOf('\n'); // Encontrar o segundo espaço

    if (firstSpaceIndex != -1 && secondSpaceIndex != -1)
    {
      // Extrai o status code
      String statusCodeString = response.substring(firstSpaceIndex, secondSpaceIndex);
      int httpCodee = statusCodeString.toInt(); // Converte para inteiro
      Serial.print("Status Code: ");
      Serial.println(httpCodee); // Imprime o status code
    }
    else
    {
      Serial.println("Status code não encontrado.");
    }
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
  Ethernet.begin(mac);

  Serial.print("Conectando ao Ethernet...");
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());

  server.begin();
}

void loop()
{
  EthernetClient client = server.available();
  if (client)
  {
    Serial.println("new client");
    // an HTTP request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the HTTP request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank)
        {
          // send a standard HTTP response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close"); // the connection will be closed after completion of the response
          client.println("Refresh: 5");        // refresh the page automatically every 5 sec
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          // output the value of each analog input pin
            client.print("led input");
            client.print(LED_BUILTIN);
            client.println("<br />");

          client.println("</html>");
          break;
        }
        if (c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}
