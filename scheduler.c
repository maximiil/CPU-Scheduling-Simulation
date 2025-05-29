#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include "scheduler.h"

// Helper function to create a deep copy of processes for simulation.
// Initializes 'remainingTime' for RR and 'startTime' to -1 for all algorithms.
void copy_processes(Process *dest, Process *src, int n) {
    for (int i = 0; i < n; i++) {
        dest[i] = src[i];
        dest[i].remainingTime = src[i].burstTime; // Used by RR to track remaining execution time
        dest[i].startTime = -1; // -1 indicates the process has not started yet
        dest[i].completionTime = 0; // Initialize completion time
    }
}

// Comparison function for qsort: sorts processes primarily by arrival time.
// For tie-breaking (same arrival time), processes are sorted by PID.
int compare_arrival(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    if (p1->arrivalTime == p2->arrivalTime) {
        return p1->pid - p2->pid; // Consistent tie-breaking
    }
    return p1->arrivalTime - p2->arrivalTime;
}

// Comparison function for qsort: sorts processes by burst time for SJF.
// For tie-breaking (same burst time), processes are sorted by arrival time.
int compare_burst(const void *a, const void *b) {
    Process *p1 = *(Process **)a;
    Process *p2 = *(Process **)b;
    if (p1->burstTime == p2->burstTime) {
        return p1->arrivalTime - p2->arrivalTime; // Consistent tie-breaking
    }
    return p1->burstTime - p2->burstTime;
}

// First Come First Serve (FCFS) Scheduling Algorithm
// Processes are executed in the order they arrive. Non-preemptive.
Metrics fcfs_metrics(Process *procs, int n) {
    Process *plist = malloc(sizeof(Process) * n);
    if (plist == NULL) {
        fprintf(stderr, "Memory allocation failed for FCFS\n");
        exit(EXIT_FAILURE);
    }
    copy_processes(plist, procs, n);

    qsort(plist, n, sizeof(Process), compare_arrival); // FCFS relies on arrival order

    int currentTime = 0;
    float totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;

    for (int i = 0; i < n; i++) {
        // Advance currentTime if CPU is idle before the next process arrives
        if (currentTime < plist[i].arrivalTime) {
            currentTime = plist[i].arrivalTime;
        }

        // Record the time the process first gets the CPU
        plist[i].startTime = currentTime;
        
        // Calculate completion time
        plist[i].completionTime = currentTime + plist[i].burstTime;

        // Calculate and accumulate individual metrics
        int turnaround = plist[i].completionTime - plist[i].arrivalTime;
        int waiting = turnaround - plist[i].burstTime;
        int response = plist[i].startTime - plist[i].arrivalTime;

        totalTurnaround += turnaround;
        totalWaiting += waiting;
        totalResponse += response;

        // Update current time to reflect the completion of this process
        currentTime = plist[i].completionTime;
    }

    free(plist);

    Metrics m = {
        .avgTurnaround = totalTurnaround / n,
        .avgWaiting = totalWaiting / n,
        .avgResponse = totalResponse / n
    };
    return m;
}

// Shortest Job First (SJF) Scheduling Algorithm (Non-Preemptive)
// At any given time, the process with the shortest burst time among arrived processes is chosen.
Metrics sjf_metrics(Process *procs, int n) {
    Process *plist = malloc(sizeof(Process) * n);
    if (plist == NULL) {
        fprintf(stderr, "Memory allocation failed for SJF\n");
        exit(EXIT_FAILURE);
    }
    copy_processes(plist, procs, n);

    int completed_count = 0;
    int currentTime = 0;
    float totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;
    bool *is_completed = calloc(n, sizeof(bool)); // To track finished processes

    if (is_completed == NULL) {
        fprintf(stderr, "Memory allocation failed for is_completed\n");
        free(plist);
        exit(EXIT_FAILURE);
    }

    qsort(plist, n, sizeof(Process), compare_arrival); // Initial sort by arrival for general order

    while (completed_count < n) {
        Process *ready_queue[n]; // Temporarily holds processes that have arrived and are not completed
        int ready_count = 0;

        // Populate the ready queue with arrived and uncompleted processes
        for (int i = 0; i < n; i++) {
            if (!is_completed[i] && plist[i].arrivalTime <= currentTime) {
                ready_queue[ready_count++] = &plist[i];
            }
        }

        if (ready_count == 0) {
            // If no processes are ready, advance time to the next process's arrival
            int next_arrival = -1;
            for (int i = 0; i < n; i++) {
                if (!is_completed[i]) {
                    if (next_arrival == -1 || plist[i].arrivalTime < next_arrival) {
                        next_arrival = plist[i].arrivalTime;
                    }
                }
            }
            if (next_arrival != -1) {
                currentTime = next_arrival;
            } else {
                break; // All processes completed or no more to arrive
            }
            continue; // Re-evaluate ready queue at the new time
        }

        // Sort the ready queue by burst time to pick the shortest job
        qsort(ready_queue, ready_count, sizeof(Process *), compare_burst);

        Process *current_process = ready_queue[0]; // The chosen shortest job

        // Set the start time as it gets the CPU for the first (and only) time
        current_process->startTime = currentTime;
        
        // Execute the process to completion (non-preemptive)
        current_process->completionTime = currentTime + current_process->burstTime;

        // Calculate and accumulate metrics for the completed process
        int turnaround = current_process->completionTime - current_process->arrivalTime;
        int waiting = turnaround - current_process->burstTime;
        int response = current_process->startTime - current_process->arrivalTime;

        totalTurnaround += turnaround;
        totalWaiting += waiting;
        totalResponse += response;

        // Update current time
        currentTime = current_process->completionTime;

        // Mark the process as completed
        for (int i = 0; i < n; i++) {
            if (&plist[i] == current_process) { // Find the original index of the completed process
                is_completed[i] = true;
                break;
            }
        }
        completed_count++;
    }

    free(plist);
    free(is_completed);

    Metrics m = {
        .avgTurnaround = totalTurnaround / n,
        .avgWaiting = totalWaiting / n,
        .avgResponse = totalResponse / n
    };
    return m;
}

// Round Robin (RR) Scheduling Algorithm (Preemptive)
// Processes execute for a fixed 'timeQuantum'. If not finished, they go to the back of the queue.
Metrics rr_metrics(Process* procs, int n, int timeQuantum) {
    Metrics m = {0, 0, 0};
    
    Process *plist = malloc(sizeof(Process) * n);
    if (plist == NULL) {
        fprintf(stderr, "Memory allocation failed for RR\n");
        exit(EXIT_FAILURE);
    }
    copy_processes(plist, procs, n); // Ensure all process data is copied and initialized

    int current_time = 0;
    int completed_processes = 0;

    // A simple array-based queue for process IDs
    // Max size is generous to avoid overflow for typical test cases.
    int max_queue_size = n * 100;
    int *queue = malloc(max_queue_size * sizeof(int));
    if (queue == NULL) {
        fprintf(stderr, "Memory allocation failed for RR queue\n");
        free(plist);
        exit(EXIT_FAILURE);
    }
    int front = 0, rear = 0; // Queue pointers

    // Tracks if a process is currently in the ready queue to prevent duplicates
    bool *in_queue = calloc(n, sizeof(bool));
    if (in_queue == NULL) {
        fprintf(stderr, "Memory allocation failed for RR in_queue\n");
        free(plist);
        free(queue);
        exit(EXIT_FAILURE);
    }

    // Add processes that arrive at time 0 to the queue initially
    for (int i = 0; i < n; i++) {
        if (plist[i].arrivalTime == 0) {
            queue[rear++] = i;
            in_queue[i] = true;
        }
    }
    
    // Main simulation loop until all processes are completed
    while (completed_processes < n) {
        if (front == rear) { // If the ready queue is empty (CPU idle)
            // Find the next process that will arrive
            int next_arrival_time = -1;
            for (int i = 0; i < n; i++) {
                if (plist[i].remainingTime > 0 && !in_queue[i]) { // Not completed and not yet in queue
                    if (next_arrival_time == -1 || plist[i].arrivalTime < next_arrival_time) {
                        next_arrival_time = plist[i].arrivalTime;
                    }
                }
            }

            if (next_arrival_time == -1) { // No more processes to schedule
                break;
            }
            // Advance current time to the next arrival time
            // If current_time is already past next_arrival_time, advance by 1 to prevent infinite loop
            current_time = (next_arrival_time > current_time) ? next_arrival_time : current_time + 1;
            
            // Add all processes that have arrived by the new current_time to the queue
            for (int i = 0; i < n; i++) {
                if (plist[i].remainingTime > 0 && !in_queue[i] && plist[i].arrivalTime <= current_time) {
                    queue[rear++] = i;
                    in_queue[i] = true;
                }
            }
            continue; // Re-evaluate the queue
        }

        int current_proc_idx = queue[front++]; // Dequeue the next process to run

        // Set startTime if this is the first time the process is executing
        if (plist[current_proc_idx].startTime == -1) {
            plist[current_proc_idx].startTime = current_time;
        }

        // Determine actual execution time for this quantum (either full quantum or remaining burst)
        int execution_duration = (plist[current_proc_idx].remainingTime < timeQuantum) ? 
                                 plist[current_proc_idx].remainingTime : timeQuantum;
        
        plist[current_proc_idx].remainingTime -= execution_duration; // Reduce remaining burst time
        current_time += execution_duration; // Advance time by executed duration

        // Add any new processes that arrive *during* the execution of this quantum
        // This is critical for fair RR scheduling: new arrivals get in queue before preempted processes
        for (int i = 0; i < n; i++) {
            if (plist[i].remainingTime > 0 && !in_queue[i] && plist[i].arrivalTime <= current_time) {
                queue[rear++] = i;
                in_queue[i] = true;
            }
        }

        // If the process has remaining burst time, re-add it to the end of the queue
        if (plist[current_proc_idx].remainingTime > 0) {
            queue[rear++] = current_proc_idx;
        } else {
            // Process completed: record completion time and update counts
            plist[current_proc_idx].completionTime = current_time;
            completed_processes++;
            in_queue[current_proc_idx] = false; // Mark as no longer in queue
        }
    }

    // Calculate total and average metrics for all processes
    float totalTurnaround = 0, totalWaiting = 0, totalResponse = 0;
    for (int i = 0; i < n; i++) {
        int turnaround = plist[i].completionTime - plist[i].arrivalTime;
        int waiting = turnaround - plist[i].burstTime;
        int response = plist[i].startTime - plist[i].arrivalTime;

        totalTurnaround += turnaround;
        totalWaiting += waiting;
        totalResponse += response;
    }

    m.avgTurnaround = totalTurnaround / n;
    m.avgWaiting = totalWaiting / n;
    m.avgResponse = totalResponse / n; // This is the metric that caused the previous issue

    // Free all dynamically allocated memory
    free(plist);
    free(queue);
    free(in_queue);

    return m;
}