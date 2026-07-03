#include <stdio.h>
#include <pthread.h>

int counter = 0;

// A mutex acts like a single key to a shared resource.
// Only one thread can hold the lock at a time - everyone else waits.
pthread_mutex_t lock;

void* worker(void* arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&lock);    // acquire the lock before touching counter
        counter++;                     // now this read-modify-write is protected
        pthread_mutex_unlock(&lock);  // release the lock so another thread can go
    }
    return NULL;
}

int main() {
    pthread_t threads[3];
    pthread_mutex_init(&lock, NULL);  // initialize the mutex before use

    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, worker, NULL);
    }
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    // With the mutex in place, this should always be exactly 300000,
    // unlike race.c where the result varied and was usually lower.
    printf("Expected: 300000\n");
    printf("Actual:   %d\n", counter);

    pthread_mutex_destroy(&lock);  // clean up the mutex when done
    return 0;
}
