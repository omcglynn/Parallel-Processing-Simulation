#Implementation Overview

-**Usage**
    -To use this program, cd to the project folder, run the make commmand to generate an executable, and then run 
    >"./main {number of tasks} {maximum processing time}
    -Both arguments must be >= 1

-**Implementation**
    -My code runs pretty much entirely in the main method. Upon starting, it first ensures the user inputted arguments satisfy the programs requirements, and then begins setup.
    -First, it initializes practically all of the variables we will be using at various times, starting with the tasks counts, and the arrays holding the pipes.
    -Then it initializes the pipes and signal handling, checking for errors of course.
    -Now we get to the actual meat of the program. A for loop begins that has the main process create forks off of itself (the cores). After the main process creates a fork, it does nothing else in this loop aside from continuing to iterate through and make more cores. 
        -Each of these cores then closes all unnecessary pipes, and generates a new random seed dependent on variables only it holds (so processing time is ACTUALLY random)
        -then it begins processing in a while loop.
        -While the main to core read pipeline is not presenting errors or signaling the pipe has been closed, the core will keep attempting to process tasks from said pipeline, writing the processed task back to the main process (using the core to main pipe) and then sending a signal that it's processing has completed. 
        -When the pipe is closed, the core exits this while loop, exiting with the number of tasks it completed. 
    -The main process on the other hand begins its own while loop, only concerned with whether or not all the tasks have been completed
        -in this while loop it constantly iterates through eeach of the cores, checking whether or not they are available to be assigned a new task or if they have sent a signal that their task has been completed.
        -If the core is available, the main process will send it a task through the main to core pipe, and setting it to unavailable
        -if the core has sent a signal that it has finished it's task, the main process resets its signal, sets it to be available to recieve another task, and continues on with its loop
    -Once all tasks have been completed, the main process exits its loop, and closes all of the pipes which then sends an EOF signal to the read calls the cores use, causing them to exit their loops sending a status of the amount of tasks they completed. 
    -After the pipes are closed, the main program then insures all children are reaped, preventing zombie processes, outputting number of tasks completed for each core if it exits normally and a mesage stating that the core did not exit normally if not. Finally the main program returns, ending the process completely. 

#Project Requirements and Justification

-**User processes are correctly created as stated in the requirement (4 pts)**
    -3 child processes are created in my core sub processing loop using fork, handing errors (if pid < 0) effectively
-**Processes are reaped (3 pts)**
    -After completion of all tasks, and once all the pipes are closed, the cores are reaped using wait() 
-**Pipes are created to support two-way communication correctly (3 pts)**
    -There are separate pipes for main to core and core to main communication and pipe creation errors are handled, exiting if one appears. 
-**The corresponding ends of pipes are correctly closed on both the parent and child process ends (8 pts)**
    -Upon creation of each core (just after forking), we immediately close the main to core write channel and the core to main read channel as neither are needed within the core.
    -Since pipes were created prior to the cores, I also close all the other core's pipes for each core. 
-**All pipes are correctly closed in the clean-up stage (4 pts)**
    -I close every end of every pipe at the end of the code in a for loop. 
-**Correctly calls the write function to send data to the pipe (4 pts)**
    -When the main process assigns a task to a core, it writes said task to the main to core write end of the pipe, allowing the core to grab it from the read end and begin processing it 
-**Correctly call the read function to read data from the pipe (4 pts)**
    -When the core completes its processing it writes the task id to the write end of the core to main pipe allowing the main process to register that said task id has been completed upon recieving the signal 
-**Choose appropriate signals to register (3 pts)**
    -I use signals SIGMIN, SIGMIN + 1, and SIGMIN + 2 to identify what signal comes from what core.
-**Correctly use sigaction to register signal handlers  (3 pts)**
    -I used sigaction in the same way that it was shown to be used in the man pages for it. I also only used one signal handler as all of my unique core information was stored in arrays 
-**Signal handler should be correctly implemented. Its operation should keep atomic. (6 pts)**
    -handle_msg increments values within msg_num[] which is atomic.
-**Use the proper type of global variable as the indicator of new results (2 pts)**
    -Defined sig_atomic_t msg_num[] as stipulated int he instructions document
-**Signal should be properly sent out when a message is sent out (2 pts)**
    -Upon completion of a task, the core executes a kill() command, sending a signal to the parent process with identification of which core it came from.
-**The core subprocess can detect the close of pipes and quit the status of the wait (3 pts)**
    -The cores run within a while loop that breaks if the read method presents an error or if it reaches end of file (which occurs upon pipe close).
-**Error handling on all the functions and system calls including read/write/fork/pipe... (4 pts)**
    -If statements handle errors for read, write, fork, pipe, and sigaction
-**Implement the command line arguments to allow user to control the number of total tasks (4 pts)**
    -Used argc and argv, forcing the user to input 2 command line arguments which follow the appropriate guidelines. Upon incorrect input, an error is presented providing context for what may have been wrong. 
-**Do not cause memory leakage. (3 pts)**
    -All pipes are ultimately closed, all unused pipes for each process are closed immediately and all child processes are reaped. 