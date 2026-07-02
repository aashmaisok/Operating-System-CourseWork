void* thread_A(void* arg) {
    pthread_mutex_lock(&lock1);
    printf("Thread A locked lock1\n");
    sleep(1);  // give Thread B time to lock lock2

    printf("Thread A waiting for lock2...\n");
    pthread_mutex_lock(&lock2);
    printf("Thread A locked lock2\n");

    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);
    return NULL;
}#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lock1, lock2;

void* thread_A(void* arg) {
    pthread_mutex_lock(&lock1);
    printf("Thread A locked lock1\n");
    sleep(1);  // give Thread B time to lock lock2

    printf("Thread A waiting for lock2...\n");
    pthread_mutex_lock(&lock2);
    printf("Thread A locked lock2\n");

    pthread_mutex_unlock(&lock2);
    pthread_mutex_unlock(&lock1);
    return NULL;
} void* thread_B(void* arg) {
    pthread_mutex_lock(&lock2);
    printf("Thread B locked lock2\n");
    sleep(1);

    printf("Thread B waiting for lock1...\n");
    pthread_mutex_lock(&lock1);
    printf("Thread B locked lock1\n");

    pthread_mutex_unlock(&lock1);
    pthread_mutex_unlock(&lock2);
    return NULL;
}
int main() {
    pthread_t t1, t2;
    pthread_mutex_init(&lock1, NULL);
    pthread_mutex_init(&lock2, NULL);

    pthread_create(&t1, NULL, thread_A, NULL);
    pthread_create(&t2, NULL, thread_B, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Both threads finished (no deadlock this time)\n");
    return 0;
}
