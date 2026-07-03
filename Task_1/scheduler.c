#include <stdio.h>

typedef struct {
    int id;
    int burst_time;     // total work needed
    int remaining_time;
} Process;

int main() {
    Process processes[3] = {
        {1, 10, 10},
        {2, 5, 5},
        {3, 8, 8}
    };

    int quantum = 3;
    int n = 3;
    int completed = 0;
    int time = 0;

    printf("Round-Robin Scheduler Simulation (quantum = %d)\n\n", quantum);

    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (processes[i].remaining_time > 0) {
                int run_time = (processes[i].remaining_time < quantum)
                                ? processes[i].remaining_time
                                : quantum;

                printf("Time %d: Process %d runs for %d units\n",
                       time, processes[i].id, run_time);

                processes[i].remaining_time -= run_time;
                time += run_time;

                if (processes[i].remaining_time == 0) {
                    printf("         -> Process %d completed at time %d\n",
                           processes[i].id, time);
                    completed++;
                }
            }
        }
    }

    printf("\nAll processes completed. Total time: %d\n", time);
    return 0;
}
