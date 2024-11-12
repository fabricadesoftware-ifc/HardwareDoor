#include "RelayUtils.h"
#include <Arduino.h>
#include "Logging.h"

#define RELAY_PIN 13
#define LED_BUILTIN 2

extern bool cadastroAtivo;

void unlock_door()
{
    digitalWrite(RELAY_PIN, HIGH);
    delay(50);
    digitalWrite(RELAY_PIN, LOW);
}

void alternarModoCadastro()
{
    cadastroAtivo = !cadastroAtivo;
    logEvent("INFO", cadastroAtivo ? "Modo de cadastro ativado" : "Modo de operação normal ativado");
    digitalWrite(LED_BUILTIN, cadastroAtivo ? HIGH : LOW);
}
