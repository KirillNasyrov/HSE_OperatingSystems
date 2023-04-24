#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdbool.h>
#include <unistd.h>

#define SHM_KEY 6789
#define SEMR_KEY 7890
#define SEMW_KEY 7891

struct shared {
    int table[3][10];
    int current_x;
    int current_y;
    bool found;
};

int main(void) {
    int shmid;
    struct shared *data;
    int sem_read, sem_write;
    struct sembuf sem_buf;

    // создаем разделяемую память и отображаем ее в адресное пространство процессов
    shmid = shmget(SHM_KEY, sizeof(struct shared), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    data = (struct shared *) shmat(shmid, NULL, 0);
    if (data == (struct shared *) -1) {
        perror("shmat");
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


    sem_read = semget(SEMR_KEY, 1, IPC_CREAT | 0666);
    if (sem_read == -1) {
        perror("semget() failed");
        exit(EXIT_FAILURE);
    }
    semctl(sem_read, 0, SETVAL, 0);

    sem_write = semget(SEMW_KEY, 1, IPC_CREAT | 0666);
    if (sem_write == -1) {
        perror("semget() failed");
        exit(EXIT_FAILURE);
    }
    semctl(sem_write, 0, SETVAL, 1);


    sleep(10);
    // родительский процесс
    while (data->found == false) {

        sem_buf.sem_num = 0;
        sem_buf.sem_op = -1;
        sem_buf.sem_flg = 0;
        semop(sem_write, &sem_buf, 1);

        if (data->current_y == 9) {
            ++data->current_x;
            data->current_y = 0;
        } else {
            ++data->current_y;
        }

        sem_buf.sem_op = 1;
        semop(sem_read, &sem_buf, 1);
    }

    // закрываем и удаляем семафор
    semctl(sem_read, 0, IPC_RMID, 0);
    semctl(sem_write, 0, IPC_RMID, 0);

    // отсоединяем разделяемую память
    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}