#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define SHM_NAME "/myshm"
#define SEM_WRITE_NAME "semwrite"
#define SEM_READ_NAME "semread"

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

typedef struct {
    int free_x;
    int free_y;
    bool server_found;
} Shared;

typedef struct {
    int x;
    int y;
    bool found;
} Coordinates;

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr,"usage: <port>\n");
        exit(1);
    }
    int my_port = atoi(argv[1]);

    int rows = atoi(argv[2]);
    int columns = atoi(argv[3]);

    int fd;

    shm_unlink(SHM_NAME);

    // создаем разделяемую память и отображаем ее в адресное пространство процессов
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    ftruncate(fd, sizeof(Shared));

    Shared *data = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }


    sem_t *semafor_write, *semafor_read;

    sem_unlink(SEM_WRITE_NAME);
    sem_unlink(SEM_READ_NAME);

    // создаем POSIX-семафор
    semafor_write = sem_open(SEM_WRITE_NAME, O_CREAT | O_RDWR, 0666, 0);
    if (semafor_write == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    semafor_read = sem_open(SEM_READ_NAME, O_CREAT | O_RDWR, 0666, 1);
    if (semafor_read == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }


    int server_socket, client_socket, client_app_socket;
    struct sockaddr_in server_addr;    // локальный адрес
    struct sockaddr_in client_addr; // удаленный адрес
    struct sockaddr_in client_app_addr; // удаленный адрес

    socklen_t sin_size = sizeof(struct sockaddr_in);
    struct sigaction sa;
    int yes=1;

    if ((server_socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    server_addr.sin_family = AF_INET;         // системный порядок байт
    server_addr.sin_port = htons(my_port);     // сетевой порядок байт
    server_addr.sin_addr.s_addr = INADDR_ANY; // автоматически указать локальный IP
    memset(&(server_addr.sin_zero), '\0', 8); // обнулить остаток

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))
        == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // закончить процессы
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    data->free_x = 0;
    data->free_y = -1;
    data->server_found = false;

    if ((client_app_socket = accept(server_socket, (struct sockaddr *)&client_app_addr, &sin_size)) == -1) {
        perror("accept");
    }

    while (1) {  // основной цикл accept()

        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &sin_size)) == -1) {

            perror("accept");
            continue;
        }

        if (data->server_found) {
            break;
        }

        printf("сервер: соединение с %s\n", inet_ntoa(client_addr.sin_addr));

        if (data->free_y == columns - 1) {
            ++data->free_x;
            data->free_y = 0;
        } else {
            ++data->free_y;
        }

        if (!fork()) { // это дочерний процесс

            close(server_socket); // которому не нужно слушать

            Coordinates new_coordinates;

            while (1) {

                new_coordinates.x = data->free_x;
                new_coordinates.y = data->free_y;
                new_coordinates.found = false;

                if (data->server_found) {
                    new_coordinates.found = true;
                }

                char buffer[sizeof(Coordinates)];
                memcpy(buffer, &new_coordinates, sizeof(Coordinates));

                if (send(client_socket, buffer, sizeof(Coordinates), 0) == -1) {
                    perror("send");
                    exit(1);
                }

                if (data->server_found) {
                    break;
                }

                if (recv(client_socket, buffer, sizeof(Coordinates), 0) == -1) {
                    perror("recv");
                    exit(1);
                }
                memcpy(&new_coordinates, buffer, sizeof(Coordinates));

                if (send(client_app_socket, buffer, sizeof(Coordinates), 0) == -1) {
                    perror("send");
                    exit(1);
                }

                if (new_coordinates.found) {
                    printf("клад найден\n");
                    data->server_found = true;
                    break;
                }

                sem_wait(semafor_read); // захватываем семафор

                if (data->free_y == columns - 1) {
                    ++data->free_x;
                    data->free_y = 0;
                } else {
                    ++data->free_y;
                }

                sem_post(semafor_read); // освобождаем семафор

            }
            return 0;
        }

        close(client_socket);  // основному процессу не нужно соединение
    }

    sem_close(semafor_write);
    sem_close(semafor_read);

    sem_unlink(SEM_WRITE_NAME);
    sem_unlink(SEM_READ_NAME);

    munmap(data, sizeof(Shared));
    shm_unlink(SHM_NAME);

    return 0;
}