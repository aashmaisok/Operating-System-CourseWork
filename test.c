#include <stdio.h>
#include <pthread.h>

void* worker(void* arg) {
    int id = *(int*)arg;
    printf("Hello from thread %d\n", id);
    return NULL;
}

int main() {
    pthread_t threads[3];
    int ids[3] = {1, 2, 3};

    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All threads done.\n");
    return 0;
}
