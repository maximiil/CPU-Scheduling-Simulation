# CPU Scheduling Algorithms

This project implements three fundamental CPU scheduling algorithms in C as part of an Operating Systems course project.

## Algorithms Implemented

1. **FCFS (First Come First Serve)**
2. **SJF (Shortest Job First - Non-Preemptive)**
3. **Round Robin (RR)**

Each algorithm computes the following performance metrics:
- Average Turnaround Time
- Average Waiting Time
- Average Response Time

## File Structure

```
.
├── scheduler.h         # Declarations for structs and scheduling functions
├── scheduler.c         # Implementation of the scheduling algorithms
├── scheduler_test.c    # Test cases and assertions for validating correctness
├── Makefile            # Build automation script
└── README.md           # Project documentation (this file)
```

## How to Compile

To build the project, simply use:

```bash
make
```

This compiles the source files and produces an executable named `scheduler_test`.

## How to Run

To run the compiled test program:

```bash
make run
```

This will execute the scheduler tests and print the calculated vs. expected metrics for:
- FCFS
- SJF
- Round Robin (with specified quantum)

If all assertions pass, you will see:

```
>>> Test Case X PASSED.
...
ALL TESTS PASSED.
```

## How to Clean

To remove the compiled binary:

```bash
make clean
```

## Example Output (Truncated)

```
==== Test Case 1 ====
FCFS: Calculated: Turnaround: 15.00, Waiting: 7.33, Response: 7.33
      Expected:   Turnaround: 15.00, Waiting: 7.33, Response: 7.33
SJF:  ...
RR (Quantum = 4): ...
>>> Test Case 1 PASSED.
```

## Notes

- The `Process` struct contains fields for tracking all timing metrics.
- All algorithms are non-preemptive except RR which uses time slicing.
- The project is written in standard C and should work on any Unix-like system (Linux, macOS, WSL).
