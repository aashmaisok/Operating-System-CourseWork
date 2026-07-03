#include <stdio.h>
#include <pthread.h>

// Shared variable with NO protection - this is intentional,
// to demonstrate what a race condition looks like.
int counter = 0;

// Each thread runs this function, incrementing the shared counter 100,000 times.
void* worker(void* arg) {
    for (int i = 0; i < 100000; i++) {
        // counter++ looks like one operation, but it's actually 3 steps:
        // 1) read counter, 2) add 1, 3) write counter back.
        // If two threads interleave these steps, increments get lost.
        counter++;
    }
    return NULL;
}

int main() {
    pthread_t threads[3];

    // Launch 3 threads, all incrementing the same shared counter simultaneously.
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }

    // Wait for all threads to finish.
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    // Expected: 3 threads x 100,000 increments = 300,000.
    // Actual: usually less, and different each run, due to the race condition.
    printf("Expected: 300000\n");
    printf("Actual:   %d\n", counter);
    return 0;
}
