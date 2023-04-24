#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>

#define SHM_NAME "/myshm"
#define SEM_WRITE_NAME "semwrite"
#define SEM_READ_NAME "semread"

struct shared {
    int table[3][10];
    int current_x;
    int current_y;
    bool found;
};

int main(int argc, char **argv) {
    int fd;
    sem_t *semafor_write, *semafor_read;

    char *name = argv[1];

    // создаем разделяемую память и отображаем ее в адресное пространство процессов
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    struct shared *data = mmap(NULL, sizeof(struct shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }


    // создаем POSIX-семафоры
    semafor_write = sem_open(SEM_WRITE_NAME, O_CREAT | O_RDWR, 0666, 1);
    if (semafor_write == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    semafor_read = sem_open(SEM_READ_NAME, O_CREAT | O_RDWR, 0666, 0);
    if (semafor_read == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }


    // Процесс группы
    for (;;) {
        sleep(1);
        sem_wait(semafor_read); // захватываем семафор

        if (data->found == true) {
            sem_post(semafor_write); // освобождаем семафор

            sem_close(semafor_write);
            sem_close(semafor_read);

            sem_unlink(SEM_WRITE_NAME);
            sem_unlink(SEM_READ_NAME);

            munmap(data, sizeof(struct shared));
            shm_unlink(SHM_NAME);

            exit(EXIT_SUCCESS);
        }

        if (data->table[data->current_x][data->current_y] == 100) {
            data->found = true;

            printf("группа <%s> нашла клад в [%d][%d]\n", name, data->current_x, data->current_y);
            fflush(stdout);

            sem_post(semafor_write); // освобождаем семафор

            sem_close(semafor_write);
            sem_close(semafor_read);

            sem_unlink(SEM_WRITE_NAME);
            sem_unlink(SEM_READ_NAME);

            munmap(data, sizeof(struct shared));
            shm_unlink(SHM_NAME);

            exit(EXIT_SUCCESS);
        } else {
            printf("группа <%s> не нашла клад в [%d][%d]\n", name, data->current_x, data->current_y);
            fflush(stdout);
            sem_post(semafor_write); // освобождаем семафор
        }
    }
}
