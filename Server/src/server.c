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
#define BACKLOG 128
#define THREAD_POOL_SIZE 4
#define QUEUE_CAPACITY 64

typedef struct {
    int sockets[QUEUE_CAPACITY];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} ClientQueue;

static ClientQueue g_client_queue = {
    .head = 0,
    .tail = 0,
    .count = 0,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .not_empty = PTHREAD_COND_INITIALIZER,
    .not_full = PTHREAD_COND_INITIALIZER
};

static void queuePush(int client_fd) {
    pthread_mutex_lock(&g_client_queue.lock);

    /* אם התור מלא, ה-Main Thread ממתין שיתפנה מקום */
    while (g_client_queue.count == QUEUE_CAPACITY) {
        pthread_cond_wait(&g_client_queue.not_full, &g_client_queue.lock);
    }

    g_client_queue.sockets[g_client_queue.tail] = client_fd;
    g_client_queue.tail = (g_client_queue.tail + 1) % QUEUE_CAPACITY;
    g_client_queue.count++;

    /* מעיר Worker אחד שממתין בתור */
    pthread_cond_signal(&g_client_queue.not_empty);
    pthread_mutex_unlock(&g_client_queue.lock);
}

static int queuePop(void) {
    pthread_mutex_lock(&g_client_queue.lock);

    /* Worker ממתין עד שיש לקוח בתור */
    while (g_client_queue.count == 0) {
        pthread_cond_wait(&g_client_queue.not_empty, &g_client_queue.lock);
    }

    int client_fd = g_client_queue.sockets[g_client_queue.head];
    g_client_queue.head = (g_client_queue.head + 1) % QUEUE_CAPACITY;
    g_client_queue.count--;

    /* מעיר את ה-Main Thread אם הוא המתין בגלל תור מלא */
    pthread_cond_signal(&g_client_queue.not_full);
    pthread_mutex_unlock(&g_client_queue.lock);

    return client_fd;
}

static void handleClientConnection(int client_fd, int worker_id) {
    printf("[Worker %d] Started serving client_fd=%d\n", worker_id, client_fd);

    ProtocolMessage req;

    // כל עוד הלקוח שולח פקודות בסשן הנוכחי 
    while (readAll(client_fd, &req, sizeof(req)) > 0) {
        if (ntohl(req.magic) != PROTOCOL_MAGIC) {
            fprintf(stderr, "[Worker %d] Invalid magic from client_fd=%d\n", worker_id, client_fd);
            break;
        }

        handleClientSession(client_fd, &req);
    }

    close(client_fd);
    printf("[Worker %d] Finished client_fd=%d, ready for next client\n", worker_id, client_fd);
}

static void *workerRoutine(void *arg) {
    int worker_id = *(int *)arg;
    free(arg);

    while (1) {
        int client_fd = queuePop();

        handleClientConnection(client_fd, worker_id);
    }

    return NULL;
}

static int startWorkerPool(pthread_t *threads, int pool_size) {
    for (int i = 0; i < pool_size; i++) {
        int *id = malloc(sizeof(int));
        if (!id) {
            perror("Failed to allocate worker id");
            return -1;
        }
        *id = i + 1;

        if (pthread_create(&threads[i], NULL, workerRoutine, id) != 0) {
            perror("Failed to create worker thread");
            free(id);
            return -1;
        }
        pthread_detach(threads[i]);
    }
    return 0;
}

static int initServerSocket(uint16_t port, int backlog) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(port)
    };

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, backlog) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

static void runServerListener(int server_fd) {
    printf("Server up on port %d. %d workers ready to accept connections.\n", SERVER_PORT, THREAD_POOL_SIZE);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        queuePush(client_fd);
    }
}

int main(void) {
    pthread_t workers[THREAD_POOL_SIZE];
    if (startWorkerPool(workers, THREAD_POOL_SIZE) != 0) {
        return EXIT_FAILURE;
    }

    int server_fd = initServerSocket(SERVER_PORT, BACKLOG);
    if (server_fd < 0) {
        return EXIT_FAILURE;
    }

    runServerListener(server_fd);

    close(server_fd);
    return EXIT_SUCCESS;
}