#include <stdio.h>
#include <pthread.h>

int counter = 0;  // shared variable, no protection

void* worker(void* arg) {
    for (int i = 0; i < 100000; i++) {
        counter++;   // NOT thread-safe
    }
    return NULL;
}

int main() {
    pthread_t threads[3];

    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Expected: 300000\n");
    printf("Actual:   %d\n", counter);
    return 0;
}


