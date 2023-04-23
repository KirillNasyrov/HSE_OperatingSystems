#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>

#define SHM_NAME "/myshm"
#define SEM_WRITE_NAME "semwrite"
#define SEM_READ_NAME "semread"

struct shared {
    sem_t semafor_write;
    sem_t semafor_read;
    int table[3][10];
    int current_x;
    int current_y;
    bool found;
};

int main(void) {
    int fd;
    //sem_t *semafor_write, *semafor_read;
    pid_t pid;

    shm_unlink(SHM_NAME);

    // создаем разделяемую память и отображаем ее в адресное пространство процессов
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    ftruncate(fd, sizeof(struct shared));
    struct shared *data = mmap(NULL, sizeof(struct shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    data->current_x = 0;
    data->current_y = -1;
    data->found = false;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 10; ++j) {
            data->table[i][j] = 0;
        }
    }

    data->table[1][5] = 100;

    sem_unlink(SEM_WRITE_NAME);
    sem_unlink(SEM_READ_NAME);

    // создаем POSIX-семафор
    sem_init(&data->semafor_write, 6, 1);
    sem_init(&data->semafor_read, 6, 0);

    for (int j = 0; j < 5; ++j) {
        // создаем дочерний процесс
        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // дочерний процесс
            for (;;) {
                sleep(1);
                sem_wait(&data->semafor_read); // захватываем семафор
                if (data->found == true) {
                    sem_post(&data->semafor_write); // освобождаем семафор
                    exit(EXIT_SUCCESS);
                }

                if (data->table[data->current_x][data->current_y] == 100) {
                    data->found = true;
                    printf("группа нашла клад в [%d][%d]\n", data->current_x, data->current_y);
                    fflush(stdout);
                    sem_post(&data->semafor_write); // освобождаем семафор
                    exit(EXIT_SUCCESS);
                } else {
                    printf("группа не нашла клад в [%d][%d]\n", data->current_x, data->current_y);
                    fflush(stdout);
                    sem_post(&data->semafor_write); // освобождаем семафор
                }
            }
        }
    }

    // родительский процесс
    while (data->found == false) {
        sem_wait(&data->semafor_write); // захватываем семафор

        if (data->current_y == 9) {
            ++data->current_x;
            data->current_y = 0;
        } else {
            ++data->current_y;
        }

        sem_post(&data->semafor_read); // освобождаем семафор
    }

    // ожидаем завершения дочерних процессов
    //for (int j = 0; j < 5; ++j) {
    //    wait(NULL);
    //}

    // отсоединяем разделяемую память
    munmap(data, sizeof(struct shared));
    shm_unlink(SHM_NAME);

    return 0;
}