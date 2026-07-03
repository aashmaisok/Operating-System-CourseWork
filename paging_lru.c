#include <stdio.h>

#define NUM_FRAMES 3

int main() {
    int pages[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int n = sizeof(pages) / sizeof(pages[0]);

    int frames[NUM_FRAMES];
    int last_used[NUM_FRAMES];  // tracks when each frame was last accessed

    for (int i = 0; i < NUM_FRAMES; i++) {
        frames[i] = -1;
        last_used[i] = -1;
    }

    int page_faults = 0;
    int page_hits = 0;

    printf("Page reference sequence: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", pages[i]);
    }
    printf("\n");

    return 0;
}
