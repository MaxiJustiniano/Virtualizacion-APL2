#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>

#define MAX_CLIENTES 10
#define MAX_LONGITUD 512
#define MAX_PALABRAS 100

typedef struct {
    int socket;
    char nickname[50];
    int aciertos;
    int errores;
} Cliente;

typedef struct {
    char palabra[256];
    char visible[256];
} Juego;

Cliente clientes[MAX_CLIENTES];
int clientes_conectados = 0;
Juego juego;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char *lista_palabras[MAX_PALABRAS];
int total_palabras = 0;

void mostrar_ayuda(const char *prog) {
    printf("Uso: %s -p PUERTO -u USUARIOS -a ARCHIVO\n", prog);
    exit(EXIT_SUCCESS);
}

void finalizar(__attribute__((unused)) int sig) {
    printf("\n[INFO] Finalizando servidor...\n");
    for (int i = 0; i < clientes_conectados; i++) {
        close(clientes[i].socket);
    }
    exit(0);
}

void cargar_lista_palabras(const char *archivo) {
    FILE *fp = fopen(archivo, "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), fp) && total_palabras < MAX_PALABRAS) {
        linea[strcspn(linea, "\r\n")] = '\0';
        lista_palabras[total_palabras] = strdup(linea);
        total_palabras++;
    }

    fclose(fp);
    srand(time(NULL));
}

void iniciar_juego() {
    int indice = rand() % total_palabras;
    strncpy(juego.palabra, lista_palabras[indice], sizeof(juego.palabra) - 1);
    juego.palabra[sizeof(juego.palabra) - 1] = '\0';

    size_t len = strlen(juego.palabra);
    for (size_t i = 0; i < len; i++) {
        juego.visible[i] = (juego.palabra[i] == ' ') ? ' ' : '_';
    }
    juego.visible[len] = '\0';
}

void *atender_cliente(void *arg) {
    Cliente *cli = (Cliente *)arg;
    char buffer[MAX_LONGITUD];

    size_t total = 0;
    while ((size_t)total < sizeof(cli->nickname) - 1) {
        ssize_t n = recv(cli->socket, cli->nickname + total, sizeof(cli->nickname) - 1 - total, 0);
        if (n <= 0) return NULL;
        total += n;
        if (cli->nickname[total - 1] == '\n' || cli->nickname[total - 1] == '\0') break;
    }
    cli->nickname[total] = '\0';

    printf("[INFO] %s se ha conectado.\n", cli->nickname);

    snprintf(buffer, sizeof(buffer), "Bienvenido %s. Estado: %s\n", cli->nickname, juego.visible);
    send(cli->socket, buffer, strlen(buffer), 0);

    while (1) {
        char letra;
        int n = recv(cli->socket, &letra, 1, 0);
        if (n <= 0) {
            printf("[INFO] Cliente %s desconectado.\n", cli->nickname);
            break;
        }

        letra = (char)tolower((unsigned char)letra);
        int acierto = 0;

        pthread_mutex_lock(&mutex);
        size_t len = strlen(juego.palabra);
        for (size_t i = 0; i < len; i++) {
            if ((char)tolower((unsigned char)juego.palabra[i]) == letra && juego.visible[i] == '_') {
                juego.visible[i] = juego.palabra[i];
                acierto = 1;
                cli->aciertos++;
            }
        }

        if (!acierto) {
            cli->errores++;
        }

        if (strchr(juego.visible, '_') == NULL) {
            char msg_final[MAX_LONGITUD];

            printf("[INFO] Juego terminado. Ganador: %s\n", cli->nickname);

            for (int i = 0; i < clientes_conectados; i++) {
                snprintf(msg_final, sizeof(msg_final),
                         "Fin del juego!\nGanador: %s\nTu puntaje:\n  Aciertos: %d\n  Errores: %d\n",
                         cli->nickname,
                         clientes[i].aciertos,
                         clientes[i].errores);

                send(clientes[i].socket, msg_final, strlen(msg_final), 0);
                close(clientes[i].socket);
            }

            pthread_mutex_unlock(&mutex);
            exit(0); // Se termina el servidor tras el juego
        }


        if (acierto) {
            snprintf(buffer, sizeof(buffer), "Acierto! Estado: %s\n", juego.visible);
        } else {
            snprintf(buffer, sizeof(buffer), "Fallaste. Estado: %s\n", juego.visible);
        }

        send(cli->socket, buffer, strlen(buffer), 0);
        pthread_mutex_unlock(&mutex);
    }

    close(cli->socket);
    return NULL;
}

int main(int argc, char *argv[]) {
    int puerto = 0, max_usuarios = 0;
    char *archivo = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            mostrar_ayuda(argv[0]);
        } else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            puerto = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-u") && i + 1 < argc) {
            max_usuarios = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-a") && i + 1 < argc) {
            archivo = argv[++i];
        }
    }

    if (puerto == 0 || max_usuarios == 0 || !archivo) {
        fprintf(stderr, "Faltan parámetros obligatorios.\n");
        mostrar_ayuda(argv[0]);
    }

    signal(SIGINT, finalizar);
    cargar_lista_palabras(archivo);
    iniciar_juego();

    int servidor_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(servidor_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(puerto),
        .sin_addr.s_addr = INADDR_ANY
    };

    if (bind(servidor_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    listen(servidor_fd, max_usuarios);
    printf("[INFO] Servidor escuchando en puerto %d...\n", puerto);

    while (clientes_conectados <= max_usuarios) {
        socklen_t clilen = sizeof(struct sockaddr_in);
        struct sockaddr_in cli_addr;
        int newsock = accept(servidor_fd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsock < 0) {
            perror("accept");
            continue;
        }

        clientes[clientes_conectados].socket = newsock;
        clientes[clientes_conectados].aciertos = 0;
        clientes[clientes_conectados].errores = 0;

        pthread_t th;
        pthread_create(&th, NULL, atender_cliente, &clientes[clientes_conectados]);
        pthread_detach(th);
        clientes_conectados++;
    }

    close(servidor_fd);
    return 0;
}


