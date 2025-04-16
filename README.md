# Documentação do Sistema Fábrica Door

## Visão Geral

O **Fábrica Door** é um sistema de controle de acesso baseado em tecnologia RFID desenvolvido pela Fábrica de Software do IFC. O sistema permite controlar o acesso a ambientes por meio de cartões RFID, oferecendo funcionalidades de autenticação, registro de novos cartões e controle remoto da porta via API.

## Arquitetura do Sistema

O sistema é composto por:

1. **Hardware**:
   - ESP32 (Microcontrolador)
   - Leitor RFID RDM6300
   - Relé para controle da porta
   - Buzzer para feedback sonoro
   - LED para indicação visual do modo de operação

2. **Software**:
   - Firmware em C++ para ESP32
   - Servidor web embutido para controle remoto
   - Integração com API externa para autenticação e gerenciamento de tags RFID
   - PlataformIO como ambiente de desenvolvimento

3. **Comunicação**:
   - Wi-Fi para conexão com a rede
   - API REST para integração com o sistema de gerenciamento

## Estrutura do Código

O código foi organizado utilizando o paradigma de Programação Orientada a Objetos (POO), com as seguintes classes principais:

### Classes Principais

1. **RFIDReader**
   - Responsável pela leitura e interpretação dos dados do leitor RFID.

2. **HardwareController**
   - Gerencia os componentes físicos (relé, buzzer e LED).

3. **APIClient**
   - Implementa as chamadas à API externa, incluindo logs, verificação de saúde e autenticação.

4. **DoorControlSystem**
   - Classe principal que integra todos os componentes e gerencia o fluxo de operação.

## Detalhamento das Classes

### RFIDReader

Esta classe gerencia a comunicação com o leitor RFID via UART.

**Métodos principais:**
- `readCard()`: Lê e decodifica o cartão RFID presente no leitor.
- `isCardPresent()`: Verifica se existe um cartão no campo de leitura.
- `clearBuffer()`: Limpa o buffer de dados do serial RFID.

### HardwareController

Responsável pelo controle dos componentes eletrônicos do sistema.

**Métodos principais:**
- `unlockDoor()`: Aciona o relé para destravar a porta.
- `setLED()`: Controla o LED de status (indica modo de cadastro ou operação normal).
- `beepShort()`, `beepError()`, `beepSuccess()`, `beepWarning()`: Diferentes padrões sonoros para feedback ao usuário.

### APIClient

Implementa a comunicação com a API externa do sistema de gerenciamento.

**Métodos principais:**
- `logEvent()`: Registra eventos no sistema de log.
- `reportHealth()`: Envia status de saúde do dispositivo para a API.
- `registerRFID()`: Cadastra um novo cartão RFID na API.
- `updateCache()`: Atualiza o cache local de cartões autorizados.

### DoorControlSystem

Classe principal que integra todos os componentes e implementa a lógica de negócio.

**Métodos principais:**
- `setup()`: Configura o sistema (Wi-Fi, servidor web, timers).
- `loop()`: Loop principal de execução.
- `toggleRegistrationMode()`: Alterna entre modo de operação normal e modo de cadastro.
- `authenticateRFID()`: Verifica se um cartão RFID está autorizado.
- `checkRFIDCard()`: Processa a leitura de cartões.
- `setupServer()`: Configura as rotas do servidor web embutido.

## Fluxos de Operação

### Fluxo de Autenticação RFID

1. Sistema detecta a presença de um cartão RFID
2. Emite um beep curto para indicar leitura
3. Lê o código do cartão
4. Verifica se o código está presente no cache local de cartões autorizados
5. Se autorizado, aciona o relé para abrir a porta
6. Se não autorizado, emite um beep de erro e registra o evento no log

### Fluxo de Cadastro de Cartão

1. Sistema deve estar em modo de cadastro (LED aceso)
2. Administrador aproxima um novo cartão do leitor
3. Sistema lê o código e envia para a API de cadastro
4. Se o cadastro for bem-sucedido, emite um beep duplo curto e atualiza o cache
5. Se o cartão já estiver cadastrado, emite um beep de aviso

## Interface Web

O sistema disponibiliza um servidor web na porta 19003 com as seguintes rotas:

- **GET /mode**: Retorna o modo atual de operação (cadastro ou operação normal)
- **GET /toggle-mode**: Alterna entre modo de cadastro e operação normal
- **GET /open-door**: Abre a porta remotamente
- **GET /cache**: Força a atualização do cache de cartões autorizados

Todas as rotas requerem autenticação via token Bearer.

## Configuração do Sistema

### Definições Principais

```cpp
const char *ssid = "ifc_wifi";          // Nome da rede Wi-Fi
const char *password = "";              // Senha da rede Wi-Fi
const int TickerTimer = 500;            // Timer para heartbeat (ms)
const int TickerTimer2 = 600;           // Timer para atualização de cache (ms)
String ApiUrl = "https://door-api.fabricadesoftware.ifc.edu.br/api";  // URL da API
```

### Pinos Utilizados

- **LED_BUILTIN (2)**: LED de status
- **RELAY_PIN (13)**: Relé para controle da porta
- **BUZZER_PIN (12)**: Buzzer para feedbacks sonoros
- **RFID_RX (16)**: Pino RX para o leitor RFID
- **RFID_TX (18)**: Pino TX para o leitor RFID

## Segurança

- O sistema utiliza autenticação via token Bearer para todas as chamadas à API.
- As credenciais de acesso à API são definidas através de variáveis de ambiente.
- O cache local de cartões autorizados evita sobrecarga na API e permite funcionamento offline.

## Manutenção e Troubleshooting

### Logs do Sistema

O sistema envia logs para a API externa com os seguintes tipos:
- **INFO**: Informações gerais de operação
- **WARN**: Avisos (ex: cartão não autorizado)
- **ERROR**: Erros de operação
- **SUCCESS**: Operações bem-sucedidas (ex: cadastro de cartão)

### Indicações de LED Azul

- **LED Apagado**: Modo de operação normal
- **LED Aceso**: Modo de cadastro de cartões

### Indicações Sonoras

- **Beep Médio**: Leitura de cartão
- **Beep Longo**: Cartão não autorizado
- **Beep Médio + Duplo Curto**: Cartão cadastrado com sucesso
- **Beep Médio + Longo**: Cartão já cadastrado

## Ampliações Futuras

1. **Integração com Câmera**: Para registro fotográfico de tentativas de acesso
2. **Autenticação Biométrica**: Adicionar leitor de impressão digital
3. **Interface de Usuário**: Adicionar display LCD para feedback visual
4. **Suporte a Múltiplas Portas**: Expansão do sistema para controlar múltiplos pontos de acesso
5. **Funcionalidade Offline Aprimorada**: Maior autonomia em caso de falha de conectividade

## Limitações Conhecidas

1. O sistema depende de conexão Wi-Fi para funcionalidades completas
2. A autenticação é baseada apenas na leitura do cartão RFID (fator único)
3. O relé é acionado por um pulso curto, adequado para fechaduras elétricas específicas

## Referências

- [Documentação do ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [Biblioteca ArduinoJson](https://arduinojson.org/)
- [Especificação do leitor RDM6300](https://www.itead.cc/wiki/RDM6300)