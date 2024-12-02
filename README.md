# NOTE
- I am unsure whether or not this works on Windows or Mac; I have only used it on Linux.

# Implementation Overview

## Usage
- To use this program, `cd` to the project folder, run the `make` command to generate an executable, and then run:  
  `./main {number of tasks} {maximum processing time}`
  - Both arguments must be `>= 1`.

## Implementation
- My code runs primarily in the main method. Upon starting, it ensures the user-provided arguments satisfy the program's requirements and then begins setup.
  - **Initialization**:
    - Initializes variables, starting with task counts and arrays for the pipes.
    - Initializes the pipes and signal handling, checking for errors.
  - **Main Process**:
    - A `for` loop creates cores (child processes) using `fork()`.
    - Each core:
      - Closes unnecessary pipes.
      - Generates a unique random seed for random task processing times.
      - Processes tasks in a `while` loop, writing results to the parent process and sending signals upon completion.
    - When the main-to-core pipe is closed, the core exits its loop and reports the number of tasks it completed.
  - **Core Subprocess**:
    - The main process runs a `while` loop to monitor task completion:
      - Checks cores for availability or completion signals.
      - Assigns tasks via pipes when cores are available.
      - Updates core status after task completion.
    - After all tasks are completed:
      - Closes all pipes to signal cores to exit their loops.
      - Reaps child processes, reporting results or errors if a core did not exit normally.

# Project Requirements and Justification

### User processes are correctly created as stated in the requirement (4 pts)
- 3 child processes are created in the core sub-processing loop using `fork()`, with error handling for `pid < 0`.

### Processes are reaped (3 pts)
- After all tasks are completed and pipes are closed, cores are reaped using `wait()`.

### Pipes are created to support two-way communication correctly (3 pts)
- Separate pipes for main-to-core and core-to-main communication, with error handling.

### Corresponding ends of pipes are correctly closed on both the parent and child process ends (8 pts)
- After forking:
  - Main-to-core write and core-to-main read channels are closed in each core.
  - Unused pipes are also closed for each core.

### All pipes are correctly closed in the clean-up stage (4 pts)
- All pipe ends are closed in a loop at the end of the program.

### Correctly calls the write function to send data to the pipe (4 pts)
- Main process writes tasks to the main-to-core pipe for cores to process.

### Correctly calls the read function to read data from the pipe (4 pts)
- Cores write completed task IDs to the core-to-main pipe for the main process.

### Choose appropriate signals to register (3 pts)
- Signals `SIGMIN`, `SIGMIN + 1`, and `SIGMIN + 2` identify the signal's origin core.

### Correctly use `sigaction` to register signal handlers (3 pts)
- `sigaction` is used as shown in the manual pages, with a single handler managing core information via arrays.

### Signal handler correctly implemented; operation keeps atomic (6 pts)
- `handle_msg` updates `msg_num[]`, which is atomic.

### Use proper global variable type as the indicator of new results (2 pts)
- `sig_atomic_t msg_num[]` is defined per instructions.

### Signal is properly sent when a message is sent (2 pts)
- Core sends a signal using `kill()` after completing a task.

### Core subprocess detects pipe closure and exits with status (3 pts)
- Cores exit their `while` loops upon read errors or EOF.

### Error handling for all functions and system calls (4 pts)
- Errors for `read`, `write`, `fork`, `pipe`, and `sigaction` are handled via `if` statements.

### Command-line arguments allow user to control the number of tasks (4 pts)
- `argc` and `argv` ensure two arguments are provided. Errors provide context for incorrect input.

### No memory leaks (3 pts)
- Pipes are closed, unused pipes are closed immediately, and child processes are reaped.
