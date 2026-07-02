#include <stdio.h>
#include <pthread.h>

int counter = 0;
pthread_mutex_t lock;   // the mutex

void* worker(void* arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&lock);    // only one thread can pass this at a time
        counter++;
        pthread_mutex_unlock(&lock);  // release so another thread can go
    }
    return NULL;
}

int main() {
    pthread_t threads[3];
    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected: 300000\n");
    printf("Actual:   %d\n", counter);

    pthread_mutex_destroy(&lock);
    return 0;
}
