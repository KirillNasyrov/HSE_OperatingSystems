#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <mqueue.h>
#include <string.h>

#define QUEUE_NAME "/my_queue"

#define SEM_WRITE_NAME "semwrite"
#define SEM_READ_NAME "semread"

struct shared {
    int table[3][10];
    int current_x;
    int current_y;
    bool found;
};

int main(void) {
    mqd_t mq;
    char buffer[sizeof(struct shared)];
    struct mq_attr attr;

    // Определение атрибутов очереди сообщений
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(struct shared);
    attr.mq_curmsgs = 0;

    mq_unlink(QUEUE_NAME);

    // Создание очереди сообщений
    mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);

    struct shared data;

    data.current_x = 0;
    data.current_y = -1;
    data.found = false;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 10; ++j) {
            data.table[i][j] = 0;
        }
    }
    data.table[1][5] = 100;

    mq_send(mq, (char*)&data, sizeof(struct shared), 0);

    sem_t *semafor_write, *semafor_read;

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
    while (data.found == false) {
        sem_wait(semafor_write); // захватываем семафор


        mq_receive(mq, buffer, sizeof(struct shared), NULL);

        memcpy(&data, buffer, sizeof(struct shared));


        if (data.current_y == 9) {
            ++data.current_x;
            data.current_y = 0;
        } else {
            ++data.current_y;
        }


        mq_send(mq, (char*)&data, sizeof(struct shared), 0);


        sem_post(semafor_read); // освобождаем семафор
    }

    // закрываем и удаляем семафор
    sem_close(semafor_write);
    sem_close(semafor_read);

    sem_unlink(SEM_WRITE_NAME);
    sem_unlink(SEM_READ_NAME);


    mq_close(mq);
    mq_unlink(QUEUE_NAME);

    return 0;
}
