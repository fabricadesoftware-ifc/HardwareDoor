#include <Arduino.h>

#define RELAY_PIN 13 // Pino do relé

void setup()
{
    Serial.begin(115200);       // Inicializa o monitor serial
    pinMode(RELAY_PIN, OUTPUT); // Configura o pino do relé como saída

    // Inicialmente, garante que o relé está desligado
    digitalWrite(RELAY_PIN, LOW);
}

void loop()
{
    // Liga o relé
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Relé LIGADO");
    delay(50); // Espera 5 segundos
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Relé DESLIGADO");
    delay(5000); // Espera 5 segundos
}
