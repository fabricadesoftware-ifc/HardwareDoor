#include <Arduino.h>
#include <Ticker.h>
#include "WiFiUtils.h"
#include "ServerUtils.h"
#include "RFIDUtils.h"
#include "RelayUtils.h"
#include "Logging.h"

const int TickerTimer = 500; // Tempo do ticker
Ticker ticker;

void setup()
{
  Serial.begin(9600);
  initWiFi();
  initRFID();
  startServer();
  ticker.attach_ms(TickerTimer, imAlive); // Configura o Ticker para enviar sinal de vida periodicamente
}

void loop()
{
  handleServerRequests();
  verificarCartaoRFID();
}
