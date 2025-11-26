# Sistema de Mensagens Cliente-Servidor
Mensageiro “um-a-um” cliente/servidor, com utilização apenas de chamadas de sockets de baixo nível para enviar e receber mensagens via TCP, com fila online (em memória) e tratamento de erros na camada de aplicação.

# Funcionalidades
#### Cliente
- Registro: Cria uma nova conta de usuário
- Login/Logout: Autentica e desconecta do sistema
- Listar Usuários: Visualiza todos os usuários registrados e seu status (online/offline)
- Enviar Mensagens: Envia mensagens para outros usuários
- Deletar Conta: Remove permanentemente uma conta do sistema
- Recebimento de Mensagens: Notificações em tempo real de novas mensagens

#### Servidor
- Gerenciamento de Usuários: Registro, autenticação e exclusão de contas
- Armazenamento de Mensagens: Sistema store-and-forward para mensagens offline
- Entrega de Mensagens: Encaminhamento de mensagens entre usuários
- Listagem de Usuários: Fornece lista completa de usuários com status
- Comunicação Concorrente: Suporte a múltiplos clientes simultaneamente

# Compilação

#### Pré-requisitos
- GCC (GNU Compiler Collection)
- Sistema Linux/Unix
- Biblioteca pthread

#### Build

```
# Compilar ambos cliente e servidor
make all
```

# Execução

#### Iniciar o Servidor
```
./bin/server
```

#### Executar o Cliente
```
./bin/client
```
