#include <stdio.h>
#include <pthread.h>

// Function that each thread will run.
// pthreads requires this exact signature: takes a void*, returns a void*.
void* worker(void* arg) {
    int id = *(int*)arg;  // cast the void* argument back to an int pointer, then read its value
    printf("Hello from thread %d\n", id);
    return NULL;
}

int main() {
    pthread_t threads[3];   // array to hold 3 thread identifiers
    int ids[3] = {1, 2, 3}; // each thread gets a unique ID to print

    // Create 3 threads. Each one runs worker() with a pointer to its own id.
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    // Wait for all 3 threads to finish before the program exits.
    // Without this, main() could finish and exit before the threads are done.
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads done.\n");
    return 0;
}
