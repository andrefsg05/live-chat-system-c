# Live Chat

Uma aplicação de chat em tempo real desenvolvida em C, com suporte a mensagens individuais, broadcasts, grupos e transferência de ficheiros.

## Descrição do Projeto

**Live Chat** é um sistema de comunicação cliente-servidor baseado em UDP que permite múltiplos utilizadores conectarem-se a um servidor centralizado e trocarem mensagens em tempo real. O projeto implementa um protocolo de rede robusto com retransmissão de pacotes, controlo de sequência e gestão de sessões de utilizadores.

### Principais Funcionalidades

- **Autenticação & Registo de Utilizadores** - Sistema completo de login e registo com persistência em CSV
- **Mensagens Privadas** - Comunicação one-to-one entre utilizadores
- **Broadcast** - Envio de mensagens para todos os utilizadores conectados
- **Gestão de Grupos** - Criar, convidar, remover membros e dissolver grupos
- **Transferência de Ficheiros** - Envio de ficheiros entre utilizadores via TCP
- **Heartbeat** - Monitorização de ligações ativas com deteção automática de clientes inativos
- **Retransmissão de Pacotes** - Mecanismo NACK para garantir entrega de mensagens
- **Suporte Unicode** - Suporte completo para caracteres wide-character (wchar_t)

---

## Tech Stack

| Componente | Tecnologia | Detalhes |
|-----------|-----------|---------|
| **Linguagem** | C (C99) | POSIX-compliant |
| **Protocolo** | UDP | Comunicação cliente-servidor |
| **Transferência de Ficheiros** | TCP | Protocolo complementar para ficheiros grandes |
| **Concorrência** | POSIX Threads | Múltiplas threads para I/O simultâneo |
| **Build System** | Makefile | Compilação com GCC |
| **Persistência** | CSV | Armazenamento de utilizadores e grupos |
| **Caracteres** | wchar_t (UTF-8/Unicode) | Suporte completo para internacionalização |
| **Tratamento de Rede** | BSD Sockets | APIs padrão POSIX |
| **Timeout & Select** | sys/select.h | Multiplexação I/O com timeouts |

**Requisitos de Sistema:**
- GCC/Clang
- POSIX-compliant OS (Linux, macOS, Unix)
- pthread library
- Standard C library (glibc)

---

## Estrutura do Projeto

```
Live Chat/
├── src/
│   ├── client/                      # Módulo Cliente
│   │   ├── client.c / .h            # Função main e loop principal do cliente
│   │   ├── client_authentication.c / .h    # Autenticação e registo de utilizadores
│   │   ├── client_messages.c / .h   # Envio e receção de mensagens
│   │   ├── client_groups.c / .h     # Interface e lógica de grupos
│   │   ├── client_threads.c / .h    # Threads de receção, heartbeat e NACK
│   │   ├── client_terminal.c / .h   # Controlo do terminal (modo raw, input)
│   │   ├── clientFile_transfer.c / .h # Protocolo TCP de transferência de ficheiros
│   │   ├── packet_types.h           # Enumeração de tipos de pacotes
│   │   ├── list.c / .h              # Implementação de lista ligada genérica
│   │   └── makefile                 # Compilação do cliente
│   │
│   ├── server/                      # Módulo Servidor
│   │   ├── server.c / .h            # Função main e loop principal do servidor
│   │   ├── userAuthentication.c / .h # Gestão de clientes conectados e sessões
│   │   ├── userRegister.c / .h      # Carregamento/gravação de utilizadores em CSV
│   │   ├── groups.c / .h            # Estrutura de grupos e operações I/O CSV
│   │   ├── serverGroups.c / .h      # Lógica servidor de gestão de grupos
│   │   ├── serverMessages.c / .h    # Roteamento e processamento de mensagens
│   │   ├── serverFile_transfer.c / .h # Coordenação de transferências de ficheiros
│   │   ├── list.c / .h              # Implementação de lista ligada (cópia servidor)
│   │   ├── makefile                 # Compilação do servidor
│   │   │
│   │   └── Database/                # Persistência de Dados
│   │       ├── users.csv            # Armazenamento de utilizadores (username,password)
│   │       └── groups.csv           # Armazenamento de grupos e membros
│   │
│   └── [Configuração Comum]
│       └── packet_types.h           # Definição de tipos de pacotes compartilhados
│
└── README.md                        # Este ficheiro
```

### Descrição Detalhada de Cada Ficheiro

#### **CLIENT** (lado do cliente)

| Ficheiro | Propósito |
|----------|-----------|
| `client.c/.h` | Entry point do cliente. Inicializa socket UDP, threads de receção/heartbeat, loop principal de comando com parsing de comandos `/exit`, `/broadcast`, `/group`, `/message` |
| `client_authentication.c/.h` | Implementa protocolo de autenticação. Envia credenciais ao servidor, trata timeouts com retry automático (MAX_TIMEOUT=3) |
| `client_messages.c/.h` | Gerencia envio/receção de mensagens. Envia caractere a caractere, implementa buffer de sequência para out-of-order, envia NACK para retransmissão |
| `client_groups.c/.h` | Interface para operações de grupo: message, invite, leave, kick, create, disband. Parse de argumentos e envio de pacotes |
| `client_threads.c/.h` | Três threads críticas: (1) `receiver_thread` - recebe pacotes UDP; (2) `heartbeat_thread` - envia heartbeat a cada 60s; (3) `nack_thread` - retransmite caracteres perdidos. Usa `ThreadStruct` com atomic flags para IPC |
| `client_terminal.c/.h` | Controlo baixo nível do terminal: modo raw para input sem eco, disable/enable de input durante receção, landing page interativa |
| `clientFile_transfer.c/.h` | Protocolo híbrido UDP+TCP: (1) UDP para negociação de ficheiro; (2) TCP para transferência actual. Threads separadas para servidor TCP e cliente TCP. Timeout de 60s |
| `packet_types.h` | Enumeração centralizada: LOGIN, REGISTER, MSG_ONE/BROADCAST/GROUP, FILE_SEND_REQUEST, HEARTBEAT, etc. |
| `list.c/.h` | Estrutura genérica de lista ligada FIFO para buffers de sequência e armazenamento de dados |

#### **SERVER** (lado do servidor)

| Ficheiro | Propósito |
|----------|-----------|
| `server.c/.h` | Main loop com select() para UDP. Carrega users e grupos de CSV. Check periódico de inatividade (a cada 10s, timeout 120s). Dispatch de pacotes para handlers |
| `userAuthentication.c/.h` | Estrutura `Client` - armazena username, IP, port, estado (IDLE/SENDING/RECEIVING), chat partner, expected_seq. Funções: `add_client`, `find_client_by_username/address`, `authenticate_client`, `exit_client` |
| `userRegister.c/.h` | Carrega utilizadores de `users.csv` em memória (List). Cria novos users, verifica existência, salva em CSV format `username,password\n` |
| `groups.c/.h` | Estrutura `Group` com nome, owner, lista de membros. Funções: carrega de `groups.csv`, cria grupo, adiciona/remove membro de ficheiro e lista, deleta grupo |
| `serverGroups.c/.h` | Lógica servidor de grupos: `server_create_group`, `server_invite_to_group`, `server_kick_from_group`, `server_leave_from_group`, `server_delete_group`. Valida permissões e envia pacotes de resposta |
| `serverMessages.c/.h` | Roteamento de mensagens: (1) `start_broadcast` - marca clientes como RECEIVING_MSG; (2) `start_message_one` - setup one-to-one; (3) `start_message_group` - setup grupo; (4) `send_message_*` - roteamento actual com NACK handling |
| `serverFile_transfer.c/.h` | Coordena transferências: `handle_file_send_request_server` envia aviso ao recipient, `handle_file_send_accept_decline` roteia resposta ao sender |

#### **DATABASE**

| Ficheiro | Formato |
|----------|---------|
| `users.csv` | Linhas: `username,password` (campos wchar_t, codificação UTF-8) |
| `groups.csv` | Linhas: `groupname,owner` para header; `groupname,username` para membros |

---

## Lógica do Projeto

### **Arquitetura Geral**

O projeto segue um modelo **cliente-servidor centralizado** com **UDP para controlo/chat e TCP para ficheiros**:

```
┌─────────────────┐                    ┌──────────────┐
│  Client 1       │                    │  Server      │
│  (UDP 127.0.0.1)│◄────── UDP ───────►│  (UDP :12345)│
│                 │                    │              │
│  ┌─ receiver    │                    │ ┌─ select   │
│  ├─ heartbeat   │                    │ ├─ handlers │
│  └─ nack_thread │                    │ └─ threads  │
└─────────────────┘                    └──────────────┘
         │
      TCP 1234                      ┌─────────────────┐
         └──────────────────────────►│  Client 2       │
                              File    │  (UDP 127.0.0.1)│
                            Transfer  │ ┌─ receiver    │
                                      │ ├─ heartbeat   │
                                      │ └─ nack_thread │
                                      └─────────────────┘
```

### **Fluxo de Autenticação**

```
Client                          Server
  │                              │
  ├─ INPUT username/password     │
  │                              │
  ├─ SEND: LOGIN|user|pass ──────►│
  │                              ├─ Verifica credenciais
  │                              ├─ Cria estrutura Client
  │◄───── LOGIN_OK ──────────────┤
  │                              │
  └─ Entra em main loop          │
```

### **Fluxo de Mensagem Privada**

```
Client A                 Server                 Client B
  │                        │                      │
  ├─ /message user_b ─────►│                      │
  │  (request)             ├─ Valida recipient    │
  │                        ├─ Marca como CHAT    │
  │◄─── MSG_OK ────────────┤                      │
  │                        ├─ MSG_ONE_WARNING ────►│
  │                        │                      ├─ Prompts receção
  │                        │                      │
  ├─ Entra raw mode        │                      ├─ Entra mode receção
  │ type "hello"           │                      │
  │ envia seq|char chars   │                      │
  ├─ 0|h ─────────────────►│                      │
  │                        ├─ Valida seq         │
  ├─ 1|e ─────────────────►│                      ├─ MSG_ONE|0|h ────►
  │                        │                      │ recebe 'h'
  │ (ctrl-d ou enter)      │                      │
  │ envia MSG_END ─────────►│                      ├─ MSG_ONE|1|e ────►
  │                        │                      │ recebe 'e'
  │                        │                      │
  │                        │                      ├─ Aguarda MSG_END
  │                        ├─ MSG_END ────────────►│
  │                        │                      └─ Fim receção
  └─ Marca como IDLE       │
```

### **Fluxo de Gestão de Grupo**

```
Creator                  Server              Members
  │                        │                    │
  ├─ /group create g1 ────►│                    │
  │                        ├─ Cria Group        │
  │                        ├─ Salva em CSV      │
  │◄─── GROUP_OK ──────────┤                    │
  │                        │                    │
  ├─ /group invite g1 u2 ─►│                    │
  │                        ├─ Valida owner      │
  │                        ├─ Adiciona membro   │
  │◄─── GROUP_OK ──────────┤                    │
  │                        ├─ GROUP_INFO ──────►│
  │                        │    (aviso)         │
  │                        │                    ├─ Recebe aviso
  │                        │                    │
  └─ /group message g1     │                    │
     "hello group"         │                    │
     envia MSG_GROUP_REQUEST
     ├─ MSG_OK ◄───────────┤                    │
     │                     ├─ Envia aviso ─────►│
     │                     │                    ├─ Entra modo receção
     ├─ envia chars ──────►│                    │
     │                     ├─ Roteia para membros
     │                     ├─ MSG_GROUP|username ──►
     │                     │                    └─ Recebe
```

### **Mecanismo de Retransmissão (NACK)**

```
Receiver                                    Sender
  │                                          │
  │◄─── SEQ 0|'h' ──────────────────────────┤
  │ (processa)                               │
  │                                          │
  │◄─── SEQ 2|'l' ──────────────────────────┤
  │ (fora de ordem! buffer)                  │
  │                                          │
  ├─ NACK|1 ───────────────────────────────►│
  │                                          ├─ Retransmite SEQ 1
  │◄─── SEQ 1|'e' ──────────────────────────┤
  │ (processa, desbloqueia buffered)         │
  │ exibe: h, e, l                           │
```

### **Heartbeat & Timeout**

- **Cliente:** Envia heartbeat a cada 60 segundos (constante `HEARTBEAT_TIME`)
- **Servidor:** Check de inatividade a cada 10 segundos
  - Se cliente não responde em 120 segundos → remove da lista
  - Notifica client e limpa estruturas

### **Transferência de Ficheiros (UDP + TCP Híbrido)**

```
Sender                      Server                  Recipient
  │                           │                        │
  ├─ FILE_SEND_REQUEST ──────►│                        │
  │  (filename, IP:port)      ├─ Validates            │
  │                           ├─ FILE_SEND_WARNING ──►│
  │                           │                       ├─ Prompts yn_with_timeout
  │                           │◄─ FILE_SEND_ACCEPT ──┤
  │◄─── SERVER warning ───────┤                        │
  │ "Recipient accepted!"      │                        │
  │                           └─ Forwards ACCEPT      │
  ├─ Abre TCP server :random ─┤                        │
  │ envia FILE_SEND_WARNING   │                        │
  │ com IP:port               │                        │
  │                           ├─ FILE_SEND_WARNING ──►│
  │                           │  com sender IP:port    │
  │                           │                        ├─ Conecta TCP
  │◄─── TCP Connection ───────────────────────────────┤
  │ envia ficheiro bytes      │                        │
  │                           │                        ├─ Recebe ficheiro
  │ fecha TCP                 │                        ├─ Fecha TCP
  └─ Fim                      │                        └─ Fim
```

### **Estrutura de Pacotes**

Todos os pacotes UDP seguem formato: **`TYPE|ARG1|ARG2|...|DATA`**

Exemplos:
- `1|username|password` → LOGIN
- `2|username|password` → REGISTER
- `5|0|h` → MSG_ONE com seq 0, char 'h'
- `10` → MSG_END
- `11|1` → MSG_NACK para seq 1
- `18|recipient` → GROUP_INFO
- `30|g1|u2` → GROUP_INVITE

---

## Como Executar

### **1. Compilação**

#### Compilar Servidor:
```bash
cd src/server
make clean
make
```

Gera executável: `server`

#### Compilar Cliente:
```bash
cd src/client
make clean
make
```

Gera executável: `client`

### **2. Executar Servidor (Terminal 1)**

```bash
cd src/server
./server
```

Saída esperada:
```
Server is connected. Waiting on port 12345...
```

O servidor aguarda conexões na porta **12345** (UDP). A cada conexão/ação, imprime:
```
127.0.0.1:54321 -> 1|username|password
[USER] User authenticated: username
...
```

### **3. Executar Cliente(s) (Terminal 2, 3, ...)**

```bash
cd src/client
./client
```

**Fluxo de Interação:**

1. **Landing Page:**
   ```
   ===== Live Chat =====
   1. Login
   2. Register
   0. Exit
   
   Choose an option: 
   ```

2. **Após Autenticação:**
   ```
   Welcome back, andre!
   Type a command:
   /broadcast  - Send message to all
   /message    - Send private message
   /group      - Manage groups
   /exit       - Exit application
   
   > _
   ```

3. **Exemplos de Comandos:**

   **Broadcast:**
   ```
   > /broadcast
   [Waiting for permission...]
   > hello everyone
   [Message sent]
   ```

   **Mensagem Privada:**
   ```
   > /message user2
   [Waiting for user2 to be available...]
   > hello user2
   [Message sent]
   ```

   **Gestão de Grupos:**
   ```
   > /group
   1. message <group_name>
   2. invite <group_name> <username>
   3. leave <group_name>
   4. kick <group_name> <username>
   5. create <group_name>
   6. disband <group_name>
   0. Back
   
   Choose option: 5
   Group name: mygroup
   [Group created]
   ```

### **4. Base de Dados**

Os dados são persistidos em:
- `src/server/Database/users.csv` - Utilizadores registados
- `src/server/Database/groups.csv` - Grupos e membros

**Formato users.csv:**
```
alice,password123
bob,securepass
charlie,pass456
```

**Formato groups.csv:**
```
developers,alice
developers,bob
marketing,charlie
```

---

## Âmbito

Este projeto foi desenvolvido como trabalho académico para a disciplina de Redes de Computadores na Universidade de Évora.

---

## Autores
- [Miguel Rocha](https://github.com/miguelrocha1)
- [Miguel Pombeiro](https://github.com/MiguelPombeiro)
- André Gonçalves
- [André Zhan](https://github.com/andr-zhan)
