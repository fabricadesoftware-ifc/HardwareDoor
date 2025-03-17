# Sistema de Controle de Acesso RFID

## Visão Geral
Este projeto implementa um sistema de controle de acesso baseado em RFID utilizando um ESP32, leitor RFID MFRC522 e componentes adicionais como relé e buzzer. O sistema permite autenticar tags RFID para controlar o acesso a portas ou áreas restritas, além de oferecer funcionalidades de cadastro de novas tags.

## Funcionalidades
- **Autenticação de Tags RFID**: Verifica se uma tag apresentada possui acesso autorizado
- **Cadastro de Tags**: Modo especial para cadastro de novas tags RFID no sistema
- **Cache Local**: Armazena tags autorizadas localmente para operação offline
- **API REST**: Sincroniza com um servidor backend para gerenciamento de tags
- **Servidor Web Embarcado**: Permite controle remoto do dispositivo
- **Monitoramento de Status**: Envia heartbeats periódicos para o servidor

## Componentes de Hardware
- ESP32 (Microcontrolador)
- Módulo RFID MFRC522
- Módulo Relé (para controle da fechadura)
- Buzzer (para feedback sonoro)
- LED interno (para indicação de status)

## Conexões
- **RFID MFRC522**:
  - SS_PIN: 21
  - RST_PIN: 22
- **Relé**: Pino 13
- **Buzzer**: Pino 12
- **LED Integrado**: Pino 2 (LED_BUILTIN)

## Configuração de Rede
O dispositivo se conecta a uma rede WiFi específica:
- SSID: "ifc_wifi" 
- Senha: (sem senha)

## Comunicação com API
O sistema se comunica com um servidor backend através de endpoints REST:
- URL base: http://191.52.58.127:3000/api
- Autenticação: Bearer Token

## Endpoints do Servidor Web Embarcado
O dispositivo executa um servidor web interno na porta 19003 com os seguintes endpoints:

- **/toggle-mode**: Alterna entre modo de operação normal e modo de cadastro
- **/open-door**: Aciona o relé para abrir a porta
- **/cache**: Força uma atualização do cache de tags RFID

Todos os endpoints requerem autenticação via Bearer Token.

## Modos de Operação

### Modo Normal
- Ao aproximar uma tag RFID, o sistema verifica se ela está autorizada
- Se autorizada, aciona o relé para abrir a porta
- Sinais sonoros indicam sucesso ou falha na autenticação

### Modo de Cadastro
- Indicado pelo LED interno aceso
- Ao aproximar uma tag, ela é enviada para o servidor para cadastro
- Diferentes padrões sonoros indicam sucesso ou falha no cadastro

## Funções Principais

### `auth_rfid(String rfidCode)`
Verifica se uma tag RFID está autorizada, consultando o cache local.

### `cadastrar_rfid(String rfidCode)`
Envia uma tag RFID para o servidor para cadastro.

### `unlook_door()`
Aciona o relé para abrir a porta (pulso curto).

### `atualizarCache()`
Sincroniza o cache local com o servidor, obtendo a lista atualizada de tags autorizadas.

### `imAlive()`
Envia um sinal de "heartbeat" para o servidor informando o status do dispositivo.

### `verificarCartaoRFID()`
Monitora a presença de novos cartões RFID e processa quando detectados.

### `alternarModoCadastro()`
Alterna entre o modo de operação normal e o modo de cadastro de tags.

## Registro de Eventos
O sistema mantém um registro de eventos que são enviados para o servidor, incluindo:
- Detecção de cartões
- Tentativas de acesso (bem-sucedidas ou não)
- Cadastro de novas tags
- Mudanças de estado do sistema

## Execução e Inicialização'
Na inicialização o sistema:
1. Configura os pinos GPIO
2. Conecta-se à rede WiFi
3. Inicializa o leitor RFID
4. Atualiza o cache de tags autorizadas
5. Configura timers para atualização periódica e heartbeat
6. Inicia o servidor web interno

## Segurança
- Todas as operações via API e servidor web são protegidas por Bearer Token
- O cache local armazena apenas tags autorizadas
- Logs de eventos são enviados para auditoria

## Manutenção
- O cache é atualizado periodicamente (a cada 600ms)
- Sinais de heartbeat são enviados regularmente (a cada 500ms)
- O servidor web permite gerenciamento remoto