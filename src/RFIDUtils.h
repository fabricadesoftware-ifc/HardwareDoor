#ifndef RFIDUTILS_H
#define RFIDUTILS_H

#include <Arduino.h>

void initRFID();
void verificarCartaoRFID();
void cadastrar_rfid(String rfidCode); // Declaração da função
bool auth_rfid(String rfidCode);      // Declaração da função

#endif
