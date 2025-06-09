// cliente.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>

#define BUFFER_SIZE 256

int sockfd;

void cerrar(__attribute__((unused)) int sig) {
    printf("\n[INFO] Finalizando cliente...\n");
    if (sockfd > 0) close(sockfd);
    exit(0);
}

void mostrar_ayuda(const char *prog) {
    printf("Uso: %s -n NICKNAME -p PUERTO -s SERVIDOR\n", prog);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    char *nickname = NULL;
    char *servidor = NULL;
    int puerto = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            mostrar_ayuda(argv[0]);
        } else if (!strcmp(argv[i], "-n") && i + 1 < argc) {
            nickname = argv[++i];
        } else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            puerto = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
            servidor = argv[++i];
        }
    }

    if (!nickname || !servidor || puerto == 0) {
        fprintf(stderr, "Faltan parámetros obligatorios.\n");
        mostrar_ayuda(argv[0]);
    }

    signal(SIGINT, cerrar);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct hostent *host = gethostbyname(servidor);
    if (!host) {
        fprintf(stderr, "Servidor desconocido.\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(puerto);
    memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // Enviar nickname con '\0' incluido
    if (send(sockfd, nickname, strlen(nickname) + 1, 0) < 0) {
        perror("send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    while (1) {
        int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            printf("[INFO] Conexion cerrada por el servidor.\n");
            break;
        }
        buffer[n] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "Fin del juego")) {
            break;
        }

        printf("Ingrese una letra: ");
        char entrada[10];
        if (!fgets(entrada, sizeof(entrada), stdin)) {
            continue;
        }

        char letra = entrada[0];
        if (!isalpha(letra)) {
            printf("[WARN] Debe ingresar una letra válida.\n");
            continue;
        }

        if (send(sockfd, &letra, 1, 0) < 0) {
            perror("send");
            break;
        }
    }

    close(sockfd);
    return 0;
}
