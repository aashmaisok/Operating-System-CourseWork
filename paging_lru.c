#include <stdio.h>

#define NUM_FRAMES 3

int main() {
    int pages[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int n = sizeof(pages) / sizeof(pages[0]);

    int frames[NUM_FRAMES];
    int last_used[NUM_FRAMES];

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
    printf("\n\n--- LRU Page Replacement ---\n");

    for (int time = 0; time < n; time++) {
        int page = pages[time];
        int found = -1;

        for (int j = 0; j < NUM_FRAMES; j++) {
            if (frames[j] == page) {
                found = j;
                break;
            }
        }

        if (found != -1) {
            page_hits++;
            last_used[found] = time;
            printf("Page %d -> HIT\n", page);
        } else {
            int target = -1;

            for (int j = 0; j < NUM_FRAMES; j++) {
                if (frames[j] == -1) {
                    target = j;
                    break;
                }
            }

            if (target == -1) {
                int oldest_time = last_used[0];
                target = 0;
                for (int j = 1; j < NUM_FRAMES; j++) {
                    if (last_used[j] < oldest_time) {
                        oldest_time = last_used[j];
                        target = j;
                    }
                }
            }

            frames[target] = page;
            last_used[target] = time;
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
