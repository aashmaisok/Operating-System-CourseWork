#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_frames>\n", argv[0]);
        return 1;
    }

    int NUM_FRAMES = atoi(argv[1]);

    int pages[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int n = sizeof(pages) / sizeof(pages[0]);

    int frames[NUM_FRAMES];
    for (int i = 0; i < NUM_FRAMES; i++) {
        frames[i] = -1;
    }

    int front = 0;
    int page_faults = 0;
    int page_hits = 0;

    printf("Page reference sequence: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", pages[i]);
    }
    printf("\n");
    printf("Number of frames: %d\n", NUM_FRAMES);

    printf("\n--- FIFO Page Replacement ---\n");

    for (int i = 0; i < n; i++) {
        int page = pages[i];
        int found = 0;

        for (int j = 0; j < NUM_FRAMES; j++) {
            if (frames[j] == page) {
                found = 1;
                break;
            }
        }

        if (found) {
            page_hits++;
            printf("Page %d -> HIT\n", page);
        } else {
            frames[front] = page;
            front = (front + 1) % NUM_FRAMES;
            page_faults++;
            printf("Page %d -> FAULT (loaded into frame)\n", page);
        }
    }

    printf("\nTotal Page Faults: %d\n", page_faults);
    printf("Total Page Hits:   %d\n", page_hits);
    printf("Hit Ratio: %.2f%%\n", (page_hits * 100.0) / n);
    printf("Fault Ratio: %.2f%%\n", (page_faults * 100.0) / n);

    return 0;
}
