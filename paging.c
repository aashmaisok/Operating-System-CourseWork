#include <stdio.h>

#define NUM_FRAMES 3   // how many pages can fit in RAM at once

int main() {
    int pages[] = {1, 2, 3, 4, 1, 2, 5, 1, 2, 3, 4, 5};
    int n = sizeof(pages) / sizeof(pages[0]);

    printf("Page reference sequence: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", pages[i]);
    }
    printf("\n");

    return 0;
}
