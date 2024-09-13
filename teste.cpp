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

#include <ctime>
#include <iostream>

int main()
{
    std::time_t t = std::time(0); // get time now
    std::tm *now = std::localtime(&t);
    std::cout << (now->tm_year + 1900) << '-'
              << (now->tm_mon + 1) << '-'
              << now->tm_mday
              << "\n";
}

void logEvent(String message)
{
    String date = []() -> String
    {
        time_t now = time(nullptr);
        struct tm *timeinfo = localtime(&now);
        char buffer[20];
        strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
        return String(buffer);
    }();
    String logTime = String(millis() / 1000); // Tempo em segundos desde o início
    Serial.println("[" + date + "]" + "[" + logTime + "s] " + message);
}
void logEvent(String message)
{
    String date = []() -> String
    {
        time_t now = time(nullptr);
        struct tm *timeinfo = localtime(&now);
        char buffer[20];
        strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
        return String(buffer);
    }();
    String logTime = String(millis() / 1000); // Tempo em segundos desde o início
    Serial.println("[" + date + "]" + "[" + logTime + "s] " + message);
}

void loop()
{
    // Liga o relé
    digitalWrite(RELAY_PIN, HIGH);
    logEvent("Relé LIGADO");
    delay(5000); // Espera 5 segundos
    digitalWrite(RELAY_PIN, LOW);
    logEvent("Relé DESLIGADO");
    delay(5000); // Espera 5 segundos
}