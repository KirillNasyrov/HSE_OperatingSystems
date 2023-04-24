#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>

#define QUEUE_NAME "/my_queue"

#define SEMR_KEY 7890
#define SEMW_KEY 7891

struct shared {
    int table[3][10];
    int current_x;
    int current_y;
    bool found;
};

int main(int argc, char **argv) {
    char *name = argv[1];

    int sem_read, sem_write;
    struct sembuf sem_buf;

    mqd_t mq;
    char buffer[sizeof(struct shared)];
    struct mq_attr attr;

    // Определение атрибутов очереди сообщений
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(struct shared);
    attr.mq_curmsgs = 0;


    // Создание очереди сообщений
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);

    struct shared data;



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


    // дочерний процесс
    for (;;) {
        sleep(1);

        sem_buf.sem_num = 0;
        sem_buf.sem_op = -1;
        sem_buf.sem_flg = 0;
        semop(sem_read, &sem_buf, 1);


        // Получение структуры из очереди
        mq_receive(mq, buffer, sizeof(struct shared), NULL);
        memcpy(&data, buffer, sizeof(struct shared));


        if (data.found == true) {
            mq_send(mq, (char*)&data, sizeof(struct shared), 0);


            sem_buf.sem_op = 1;
            semop(sem_write, &sem_buf, 1);


            // закрываем и удаляем семафор
            semctl(sem_read, 0, IPC_RMID, 0);
            semctl(sem_write, 0, IPC_RMID, 0);


            // Закрытие очереди сообщений
            mq_close(mq);
            mq_unlink(QUEUE_NAME);


            exit(EXIT_SUCCESS);
        }

        if (data.table[data.current_x][data.current_y] == 100) {
            data.found = true;

            mq_send(mq, (char*)&data, sizeof(struct shared), 0);

            printf("группа <%s> нашла клад в [%d][%d]\n", name, data.current_x, data.current_y);
            fflush(stdout);

            sem_buf.sem_op = 1;
            semop(sem_write, &sem_buf, 1);


            // закрываем и удаляем семафор
            semctl(sem_read, 0, IPC_RMID, 0);
            semctl(sem_write, 0, IPC_RMID, 0);


            // Закрытие очереди сообщений
            mq_close(mq);
            mq_unlink(QUEUE_NAME);


            exit(EXIT_SUCCESS);
        } else {
            mq_send(mq, (char*)&data, sizeof(struct shared), 0);


            printf("группа <%s> не нашла клад в [%d][%d]\n", name, data.current_x, data.current_y);
            fflush(stdout);

            sem_buf.sem_op = 1;
            semop(sem_write, &sem_buf, 1);
        }
    }
}