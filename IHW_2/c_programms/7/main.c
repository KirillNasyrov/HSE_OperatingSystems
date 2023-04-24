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

int main(void) {
    int fd;
    sem_t *semafor_write, *semafor_read;

    shm_unlink(SHM_NAME);

    // создаем разделяемую память и отображаем ее в адресное пространство процессов
    fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, sizeof(struct shared)) == -1) {
        perror("ftruncate");
        return 1;
    }

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

    sleep(10);
    // родительский процесс
    while (data->found == false) {
        sem_wait(semafor_write); // захватываем семафор

        if (data->current_y == 9) {
            ++data->current_x;
            data->current_y = 0;
        } else {
            ++data->current_y;
        }

        sem_post(semafor_read); // освобождаем семафор
    }

    // закрываем и удаляем семафор
    sem_close(semafor_write);
    sem_close(semafor_read);

    sem_unlink(SEM_WRITE_NAME);
    sem_unlink(SEM_READ_NAME);

    // отсоединяем разделяемую память
    munmap(data, sizeof(struct shared));
    shm_unlink(SHM_NAME);

    return 0;
}
