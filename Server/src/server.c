#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "protocol.h"
#include "net_utilities.h"
#include "server_session.h"
#include "files_operations.h"

#define SERVER_PORT 8000
#define BACKLOG 10
#define THREAD_POOL_SIZE 4
#define QUEUE_CAPACITY 64

typedef struct {
    int sockets[QUEUE_CAPACITY];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
} TaskQueue;

static TaskQueue tasks_queue = {
    .head = 0,
    .tail = 0,
    .count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER
};

static void queuePush(int client_fd) {
    pthread_mutex_lock(&tasks_queue.lock);
    if (tasks_queue.count < QUEUE_CAPACITY) {
        tasks_queue.sockets[tasks_queue.tail] = client_fd;
        tasks_queue.tail = (tasks_queue.tail + 1) % QUEUE_CAPACITY;
        tasks_queue.count++;
        pthread_cond_signal(&tasks_queue.not_empty);
    } else {
        close(client_fd);
    }
    pthread_mutex_unlock(&tasks_queue.lock);
}

static int queuePop(void) {
    pthread_mutex_lock(&tasks_queue.lock);
    while (tasks_queue.count == 0) {
        pthread_cond_wait(&tasks_queue.not_empty, &tasks_queue.lock);
    }
    int client_fd = tasks_queue.sockets[tasks_queue.head];
    tasks_queue.head = (tasks_queue.head + 1) % QUEUE_CAPACITY;
    tasks_queue.count--;
    pthread_mutex_unlock(&tasks_queue.lock);
    return client_fd;
}

static void *workerThread(void *arg) {
    (void)arg;
    while (1) {
        int client_fd = queuePop();

        ProtocolMessage req;
        if (read_all(client_fd, &req, sizeof(req)) > 0 && ntohl(req.magic) == PROTOCOL_MAGIC) {
            handleClientSession(client_fd, &req);
        }

        close(client_fd);
    }
    return NULL;
}

int main(void) {
    pthread_t threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        if (pthread_create(&threads[i], NULL, workerThread, NULL) != 0) {
            perror("Failed to create worker thread");
            return -1;
        }
        pthread_detach(threads[i]);
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int enable_port_use = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable_port_use, sizeof(enable_port_use));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(SERVER_PORT)
    };

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 || listen(server_fd, BACKLOG) < 0) {
        perror("Server setup failed");
        return -1;
    }

    printf("Server listening on port %d with %d workers...\n", SERVER_PORT, THREAD_POOL_SIZE);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd >= 0) {
            queuePush(client_fd);
        }
    }

    close(server_fd);
    return 0;
}