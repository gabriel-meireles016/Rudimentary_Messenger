#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN 1024
#define MAX_NICK_LEN 50
#define MAX_NAME_LEN 100
#define PORT 8080

int socket_fd;                          // Socket de conexão com o servidor
char current_user[MAX_NICK_LEN] = "";   // Usuário logado atualmente

// Função que estabele conexão com o servidor
void connect_to_server() {
    struct sockaddr_in server_addr;

    // Criando socket TCP
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        exit(1);
    }

    // Configurando endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");   // localhost

    // Estabelecendo conexao
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Conectado ao servidor\n");
}

// Função que verifica as mensagens recebidas do servidor
void check_messages() {
    char buffer[MAX_MSG_LEN];
    
    // Verificação não-bloqueante simples
    int bytes_received = recv(socket_fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        if (strncmp(buffer, "DELIVER_MSG", 11) == 0) {
            char from[MAX_NICK_LEN], text[256];
            long ts;
            
            if (sscanf(buffer, "DELIVER_MSG{\"from\":\"%[^\"]\",\"text\":\"%[^\"]\",\"ts\":%ld}", 
                       from, text, &ts) == 3)
                printf("\n>>> Nova mensagem de %s: %s\n", from, text);
        } else {
            printf("Servidor: %s\n", buffer);
        }
    } else if (bytes_received == 0) {
        printf("Servidor desconectou\n");
        exit(1);
    }
}

// Função que exibe o menu principal
void show_menu() {
    printf("\n======== CLIENT ========\n");
    if (strlen(current_user) > 0)
        printf("Logado como: %s\n", current_user);
    printf("1. Registro\n");
    printf("2. Login\n");
    printf("3. Listar Usuários\n");
    printf("4. Enviar Mensagem\n");
    printf("5. Sair\n");
    printf("6. Deletar\n");
    printf("7. Sair\n");

    printf("Escolha: ");
    
}

// Função que envia o comando para o servidor e recebe a resposta
void send_command(const char* command){
    // Enviando comando para o servidor
    send(socket_fd, command, strlen(command), 0);
}

// Interface para registro de um novo usuário
void register_user() {
    char nick[MAX_NICK_LEN], name[MAX_NAME_LEN];
    printf("Apelido: ");
    scanf("%s", nick);
    
    printf("Nome Completo: ");
    scanf(" %[^\n]", name);

    // Formatando e enviando o comanado REGISTER
    char command[MAX_MSG_LEN];
    snprintf(command, sizeof(command), "REGISTER {%s, %s}", nick, name);
    send_command(command);
}

// Interface para o login do usuário
void login_user() {
    char nick[MAX_NICK_LEN];
    printf("Apelido: ");
    scanf("%s", nick);

    // Formatando e enviando comando LOGIN
    char command[MAX_MSG_LEN];
    snprintf(command, sizeof(command), "LOGIN {%s}", nick);
    send_command(command);

    // Atualizando usuário atual
    strcpy(current_user, nick);
}

// Interface para solicitação de lista de usuários do servidor
void list_users() {
    send_command("LIST");
}

// Interface para envio de mensagens
void send_message() {
    if (strlen(current_user) == 0) {
        printf("Faça o LOGIN primeiro.\n");
        return;
    }

    char to[MAX_NICK_LEN], text[256];
    printf("Para: ");
    scanf("%s", to);
    
    printf("Mensagem: ");
    scanf(" %[^\n]", text);
    
    // Formatando e enviando comando SEND_MSG
    char command[MAX_MSG_LEN];
    snprintf(command, sizeof(command), "SEND_MSG {%s, %s}", to, text);
    send_command(command);
}

// Interface para logout de usuário
void logout_user() {
    if (strlen(current_user) == 0) {
        printf("Não está logado.\n");
        return;
    }

    // Formatando e enviando comando LOGOUT
    char command[MAX_MSG_LEN];
    snprintf(command, sizeof(command), "LOGOUT {%s}", current_user);
    send_command(command);

    // Limpando usuário atual
    strcpy(current_user, "");
}

// Interace para exclusão de usuário
void delete_user() {
    char nick[MAX_NICK_LEN];
    printf("Apelido: ");
    scanf("%s", nick);

    // Formatando e enviando comando DELETE
    char command[MAX_MSG_LEN];
    snprintf(command, sizeof(command), "DELETE {%s}", nick);
    send_command(command);
}

int main() {
    connect_to_server();

    int option;
    char input[10];

    while (1) {
        // Verificando as recebeu mensagens
        check_messages();

        show_menu();

        // Lendo entrada do usuário
        if (fgets(input, sizeof(input), stdin) == NULL){
            usleep(100000); // Pequena pausa para não sobrecarregar CPU
            continue;
        }

        // Remover newline se existir
        input[strcspn(input, "\n")] = 0;

        // Verificar se a entrada está vazia
        if (strlen(input) == 0) {
            continue;
        }

        option = atoi(input);

        // Processamento da escolha
        switch (option) {
        case 1:
            register_user();
            break;
        case 2:
            login_user();
            break;
        case 3:
            list_users();
            break;
        case 4:
            send_message();
            break;
        case 5:
            logout_user();
            break;
        case 6:
            delete_user();
            break;
        case 7:
            if (strlen(current_user) > 0)
                logout_user();  // Faz logout caso esteja conectado
            
            // Fechando conexão
            close(socket_fd);
            printf("Até logo!\n");
            exit(0);
        default:
            printf("Opção inválida.\n");
        }
        
    }
    
    close(socket_fd);
    return 0;
}
