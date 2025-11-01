#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_USERS 100
#define MAX_MSG_LEN 1024
#define MAX_NAME_LEN 100
#define MAX_NICK_LEN 50
#define PORT 8080
// Tamanho da fila de conexões pendentes
#define BACKLOG 10

typedef struct {
    char nick[MAX_NICK_LEN];    // Apelido do usuário
    char name[MAX_NAME_LEN];    // Nome do usuário
    int online;                 // Flag que indica se está online <1> ou offline <2>
    int socket;                 // Socket associado ao usuário
    char** message_queue;       // FIla de mensagens pendentes
    int queue_size;             // Número atual de mensagens na fila
    int queue_capacity;         // Capacidade atual da fila
} User;

int user_count = 0;             // Contador de usuários
User users[MAX_USERS];          // Array dos usuários registrados


// Função que inicializa a fila de mensagens de um usuário
void init_user_queue(User* user) {
    user->queue_capacity = 10;  // Capacidade inicial da fila de mensagens
    user->queue_size = 0;       // Fila começa vazia

    // Aloca memória para a fila de mensagens
    user->message_queue = malloc(user->queue_capacity * sizeof(char*));
}

// Função que busca um usuário no sistema pelo apelido
User* find_user(const char* nick) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].nick, nick) == 0)
            return &users[i];   // Usuário encontrado
    }

    // Caso o usuário não seja encontrado, retorna NULL.
    return NULL;
}

char* pop_from_queue(User* user) {
    if (user->queue_size == 0)
        return NULL;
    
    // Primeira mensagem
    char* msg = user->message_queue[0];

    // Desloca todas as mensagens uma posição para frente
    for (int i = 1; i < user->queue_size; i++) {
        user->message_queue[i-1] = user->message_queue[i];
    }
    user->queue_size--;
    
    return msg;
}

// Função que adiciona uma mensagem a fila do usuário
void add_to_queue(User* user, const char* message) {
    // Verificando se precisa expandir a fila
    if (user->queue_size >= user->queue_capacity) {
        // Dobrando a capacidade da fila
        user->queue_capacity *= 2;

        // Realocando a fila com nova capacidade
        user->message_queue = realloc(user->message_queue, user->queue_capacity * sizeof(char*));
    }

    // Alocando espaço para a nova mensagem e copiando
    user->message_queue[user->queue_size] = malloc(strlen(message) + 1);
    strcpy(user->message_queue[user->queue_size], message);
    
    user->queue_size++;
    
}

// Função que registra um novo usuário no sistema
int register_user(const char* nick, const char* name) {
    // Verificando se já existe o apelido
    if (find_user(nick) != NULL)
        return -1;

    // Verificando se já atingiu o limite máximo de usuários
    if (user_count > MAX_USERS)
        return -2;

    // Criando novo usuário
    User* new_user = &users[user_count];
    strcpy(new_user->nick, nick);
    strcpy(new_user->name, name);

    new_user->online = 0;       // Inicia como offline
    new_user->socket = -1;      // Nenhum socket associado ainda

    init_user_queue(new_user);  // Inicializa a fila de mensagens

    user_count++;
    return 0;
}

// Função que deleta um usuário do sistema
int delete_user(const char* nick, int requester_socket) {
    
    User* user = NULL;
    int user_index = -1;

    // Busca o usuário e índice de usuário
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].nick, nick) == 0) {
            user = &users[i];
            user_index = i;
            break;
        }
    }

    // Verificando se o usuário existe/foi encontrado
    if (user == NULL) 
        return -1;
    
    // Verificando autorização (se é o mesmo que requisitou)
    if (user->online && user->socket != requester_socket)
        return -2;

    // Verificando se precisa fazer logout primeiro
    if (user->online)
        return -3;

    // Liberar memória
    init_user_queue(user);

    // Removendo usuário do array (usando shift left)
    for (int i = user_index; i < user_count; i++) {
        users[i] = user[i + 1];
    }
    user_count--;

    return 0;
}

// Função que realiza login do usuário no sistema
int login_user(const char* nick, int socket) {

    // Verificando se o usuário existe
    User* user = find_user(nick);
    if (user == NULL)
        return -1;
    
    // Verificando se o usuário já está online
    if (user->online)
        return -2;
    
    // Atualizando estado do usuário
    user->online = 1;
    user->socket = socket;

    // Entregando mensagens pendentes (store-and_foward)
    while (user->queue_size > 0) {
        char* msg = pop_from_queue(user);
        
        if (msg) {
            send(socket, msg, strlen(msg), 0);  // Enviando a mensagem
            free(msg);                          // Liberando a memória
        }
    }
    
    return 0;
}

// Função que realiza logout do usuário do sistema
int logout_user(const char* nick, int socket) {

    // Verificando se o usuário existe
    User* user = find_user(nick);
    if (user == NULL)
        return -1;

    // Verificando se o usuário esta online e se o socket é o correto
    if (!user->online || (user->socket != socket))
        return -2;

    // Atualizando estado do usuário
    user->online = 0;
    user->socket = -1;

    return 0;
}

// Função que lista todos os usuários do sistema
char* list_users() {
    // Buffer estatico para retorno
    static char list_buffer[4096];
    char temp[256];
    strcpy(list_buffer, "USERS{users:[");

    // Construindo a lista de usuários em formato JSON
    int first = 1;
    for (int i = 0; i < user_count; i++) {
        if (!first)
            strcat(list_buffer, ",");
        snprintf(temp, sizeof(temp), "{\"nick\":\"%s\",\"online\":%d,\"name\":\"%s\"}", users[i].nick, users[i].online, users[i].name);
        strcat(list_buffer, temp);

        first = 0;
    }

    strcat(list_buffer, "]}");

    return list_buffer;
}

// Função que envia uma mensagem de um usuário para outro
int send_message(const char* from, const char* to, const char* text) {

    User* sender = find_user(from);
    User* receiver = find_user(to);

    // Verificando se o destinatário existe
    if (receiver == NULL) 
        return -1;
    
    // Verificando se o remetente existe e esta online
    if (sender == NULL || !sender->online)
        return -2;

    // Criando timestamp e formatando mensagem de entrega
    time_t now = time(NULL);
    char deliver_msg[512];
    snprintf(deliver_msg, sizeof(deliver_msg), "DELIVER_MSG{\"from\":\"%s\",\"text\":\"%s\",\"ts\":%ld}", from, text, now);
    
    // Entregas
    if (receiver->online)
        // Entrega imediata se online
        send(receiver->socket, deliver_msg, strlen(deliver_msg), 0);
    else
        // Entrega store-and-forward se offline
        add_to_queue(receiver, deliver_msg);
    
    return 0;
}

// Função que gerencia a comunicação com o cliente
void handle_client(int client_socket) {
    char buffer[MAX_MSG_LEN];

    while (1){
        // Limpando buffer
        memset(buffer, 0, sizeof(buffer));

        // Recebendo dados do cliente
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        // Verificando se o cliente desconectou
        if (bytes_received <= 0) {
            for (int i = 0; i < user_count; i++) {
                if (users[i].socket == client_socket) {
                    users[i].online = 0;
                    users[i].socket = -1;
                    printf("Cliente desconectado: %s\n", users[i].nick);
                }
            }
            break;
        }

        printf("Recebido: %s\n", buffer);

        // Processa o comando recebido

        char response[512];

        if (strncmp(buffer, "REGISTER", 8) == 0) {
            char nick[MAX_MSG_LEN], name[MAX_NAME_LEN];

            // REGISTER {apelido, nome}
            if (sscanf(buffer, "REGISTER {%[^,], %[^}]}", nick, name) == 2) {
                int result = register_user(nick, name);

                if (result == 0)
                    snprintf(response, sizeof(response), "OK");
                else if (result == -1)
                    snprintf(response, sizeof(response), "ERROR{NICK_TAKEN}");
                else
                    snprintf(response, sizeof(response), "ERROR{LIMIT}");
            } else
                snprintf(response, sizeof(response), "ERROR{BAD_FORMAT}");
        }
        else if (strncmp(buffer, "DELETE", 6) == 0) {
            char nick[MAX_NICK_LEN];
            
            // DELETE {apelido}
            if (sscanf(buffer, "DELETE {%[^}]}", nick) == 1) {
                int result = delete_user(nick, client_socket);
                if (result == 0)
                    snprintf(response, sizeof(response), "OK");
                else if (result == -1)
                    snprintf(response, sizeof(response), "ERROR{NO_SUCH_USER}");
                else if (result == -2)
                    snprintf(response, sizeof(response), "ERROR{UNAUTHORIZED}");
                else
                    snprintf(response, sizeof(response), "ERROR{BAD_STATE}");
            } else
                snprintf(response, sizeof(response), "ERROR{BAD_FORMAT}");    
        }
        else if (strncmp(buffer, "LOGIN", 5) == 0) {
            char nick[MAX_NICK_LEN];

            // LOGIN {apelido}
            if (sscanf(buffer, "LOGIN {%[^}]}", nick) == 1) {
                int result = login_user(nick, client_socket);
                if (result == 0)
                    snprintf(response, sizeof(response), "OK{%s}", nick);
                else if (result == -1)
                    snprintf(response, sizeof(response), "ERROR{NO_SUCH_USER}");
                else
                    snprintf(response, sizeof(response), "ERROR{ALREADY_ONLINE}");
            } else
                snprintf(response, sizeof(response), "ERROR{BAD_FORMAT}");
            
        }
        else if (strncmp(buffer, "LOGOUT", 6) == 0) {
            char nick[MAX_NICK_LEN];

            // LOGOUT {apelido}
            if (sscanf(buffer, "LOGOUT {%[^}]}", nick) == 1) {
                int result = logout_user(nick, client_socket);
                if (result == 0)
                    snprintf(response, sizeof(response), "OK");
                else if (result == 1)
                    snprintf(response, sizeof(response), "ERROR{NO_SUCH_USER}");
                else
                    snprintf(response, sizeof(response), "ERROR{BAD_STATE}");
            } else
                snprintf(response, sizeof(response), "ERROR{BAD_FORMAT}");
        }
        else if (strncmp(buffer, "LIST", 4) == 0) {
            // LIST {usuarios}
            char* list = list_users();
            strcpy(response, list);
        }
        else if (strncmp(buffer, "SEND_MSG", 8) == 0) {
            char to[MAX_NICK_LEN], text[256];
            
            // SEND_MSG {destinatário, texto}
            if (sscanf(buffer, "SEND_MSG {%[^,], %[^}]}", to, text) == 2) {
                char from_nick[MAX_NICK_LEN] = "";    
                for (int i = 0; i < user_count; i++) {
                    if (users[i].socket == client_socket && users[i].online) {
                        strcpy(from_nick, users[i].nick);
                        break;
                    }
                }

                if (strlen(from_nick) == 0)
                    snprintf(response, sizeof(response), "ERROR{UNAUTHORIZED}");
                else {
                    int result = send_message(from_nick, to, text);
                    if (result == 0)
                        snprintf(response, sizeof(response), "OK");
                    else if (result == 1)
                        snprintf(response, sizeof(response), "ERROR{NO_SUCH_USER}");
                    else
                        snprintf(response, sizeof(response), "ERROR{UNAUTHORIZED}");
                }
            } else {
                snprintf(response, sizeof(response), "ERROR{BAD_FORMAT}");
            }

            
            
        }
        else {
            // Nenhum dos comandos anteriores
            snprintf(response, sizeof(response), "ERROR{UNKNOWN_COMMAND}");
        }
        
        // Enviando resposta para o cliente
        send(client_socket, response, strlen(response), 0);
        printf("Sent: %s\n", response);

    }

    // Fechando o socket do cliente
    close(client_socket);
}

int main() {
    
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Socket do servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket < 0) {
        perror("Socket não criado\n");
        exit(1);
    }

    // Configurando opções do socket
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    // Configurando endereço de servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;   // Aceita conexões de qualquer interface
    server_addr.sin_port = htons(PORT);         // Porta do servidor

    // Associando socket ao endereço
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // Socket no modo de ESCUTA
    if (listen(server_socket, BACKLOG) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Servidor está escutando na porta %d\n", PORT);

    while (1) {
        // Nova conexão aceita
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        printf("Novo cliente conectado - %s:%d\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(client_socket);
    }

    

    close(server_socket);
    return 0;
}
