#include "ServerUtils.h"
#include <WebServer.h>
#include "RelayUtils.h"
#include "Logging.h"

extern bool cadastroAtivo; // Declaração como extern para acessar a variável

extern String ApiUrl;
String BEARER_TOKEN = "fabdor-dPluQTwdJJ4tamtnP0i7J34UqphHuJTdUugKt2YMJgQeoAS5qs1fFi4My";
WebServer server(19003);

void startServer()
{
    server.on("/toggle-mode", HTTP_GET, []()
              {
        if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
            alternarModoCadastro();
            server.send(200, "application/json", "{\"success\": true, \"mode\": \"" + String(cadastroAtivo ? "cadastro" : "operacao") + "\"}");
        } else {
            server.send(401, "application/json", "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
        } });

    server.on("/open-door", HTTP_GET, []()
              {
        if (server.hasHeader("Authorization") && server.header("Authorization") == "Bearer " + BEARER_TOKEN) {
            unlock_door();
            server.send(200, "application/json", "{\"success\": true, \"message\": \"Porta aberta com sucesso\"}");
        } else {
            server.send(401, "application/json", "{\"success\": false, \"message\": \"Token Bearer inválido\"}");
        } });

    server.begin();
}

void handleServerRequests()
{
    server.handleClient();
}
