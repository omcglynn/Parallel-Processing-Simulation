#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>



volatile sig_atomic_t msg_num[3] = {0,0,0}; //holds signals from cores 
volatile sig_atomic_t idle_cores[3] = {1,1,1}; // registers if a core is idle or not, 1 = true, 0 = false


//given sleep method//
void no_interrupt_sleep(int sec) {
    struct timespec req, rem;
    req.tv_sec = sec;
    req.tv_nsec = 0;

    while (nanosleep(&req, &rem) == -1 && errno == EINTR){
        req = rem;
    }
}

//signal handler//
void handle_msg(int sig, siginfo_t *si, void *context) {
    int core = sig - SIGRTMIN; // map signal to core index
    if (core >= 0 && core < 3) {
        ++msg_num[core]; //shows main process that core has completed its task
    }
}


int main(int argc, char *argv[]){
    //incorrect program call handling//
    if(argc != 3 ){
        fprintf(stderr, "Review usage instructions; %d <number of tasks> <max sleep time>\n", atoi(argv[0]));
        exit(1);
    }

    //given variables//
    int tasks = atoi(argv[1]);
    int max_sleep = atoi(argv[2]);
    
    //more incorrect program call handling//
    if(tasks <= 0 || max_sleep <= 0){
        fprintf(stderr, "Please enter valid values for number of tasks and max sleep time. Neither can be less than or equal to 0\n");
        exit(1);
    }

    //task variables//
    int tasksC = 0; //tasks completed
    int tasksA = 0; //tasks assigned to a core 

    //core variables//
    int main_to_core[3][2], core_to_main[3][2]; //array of pipes from main to cores and vice versa
    
    //rand seed initial setup//
    srand(time(NULL)); //only used to futher randomize the actual seeds that will be used

    //signal handling//
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO; // Extended signal handling with additional data
    sa.sa_sigaction = handle_msg;
    sigemptyset(&sa.sa_mask);
    //error handling on sigaction//
    for (int i = 0; i < 3; i++) {
        if (sigaction(SIGRTMIN + i, &sa, NULL) == -1) {
            fprintf(stderr, "sigaction");
            exit(1);
        }
    }   

    //create all the pipes, returning error if one fails//
    for(int i = 0; i < 3; i ++){ 
        if(pipe(main_to_core[i]) == -1 || pipe(core_to_main[i]) == -1){
            fprintf(stderr, "Pipe creation failed for core %d", i+1);
            exit(1);
        }
    }


    //core subprocessing//
    for(int i = 0; i < 3; i++){
        pid_t pid = fork(); //create child (core)
        if(pid < 0){
            fprintf(stderr, "failed fork at for core %d \n", i+1);
            exit(1);
        }else if(pid == 0){
            //close unused pipes in current core//
            close(main_to_core[i][1]);
            close(core_to_main[i][0]);

            //close other cores pipes for current core//
            for (int j = 0; j < 3; j++) {
                if (j != i) {
                    close(main_to_core[j][0]);
                    close(main_to_core[j][1]);
                    close(core_to_main[j][0]);
                    close(core_to_main[j][1]);
                }
            }

            //unique randomizer seed for each core based on process id//
            //(using time made their random sleep times too similar and made it look like the parallel processing wasnt working.)//
            srand(getpid() ^ rand() ^ (i * 1000)); 

            //actual processing loop//
            int task_id, taskcount = 0; 
            while(read(main_to_core[i][0], &task_id, sizeof(int)) > 0){//grabs task from main to core pipe, breaking if pipe closed or error presents
                printf("Core %d processing task %d +++++++++++++++++++++++\n", i+1, task_id+1);
                no_interrupt_sleep(rand() % max_sleep + 1); //fake processing
                
                // code for making sure that cores can complete their tasks asynchronously frpm the other cores by defining times.//
                /*if(i == 0){                   
                    no_interrupt_sleep(1);
                }else if(i == 1){
                    no_interrupt_sleep(5);
                }else if(i == 2){
                    no_interrupt_sleep(10);
                }*/
                
                write(core_to_main[i][1], &task_id, sizeof(int)); //send task id back to main process so it can see what task was completed
                kill(getppid(), SIGRTMIN + i); //send signal to main process so it can send a new task to core
                taskcount++;
            }
            exit(taskcount); //closes child
        }
    }

    //main process//
    while(tasksC < tasks){
        //assign tasks//
        for(int i = 0; i < 3; i++){ //loop through cores
            if(idle_cores[i] == 1 && tasksA < tasks){ //check if core is available and if there are tasks left
                printf("Assigning task %d to core %d $$$$$$$$$$$$$$$$$$$$\n", tasksA + 1, i+1);
                //error handling on write//
                if(write(main_to_core[i][1], &tasksA, sizeof(int))== -1){
                    fprintf(stderr, "write");
                }else{ //if task sent to core, set core to unavailable and increment task ids
                    tasksA++;
                    idle_cores[i] = 0;
                }
            }
        }

        //Check for completed tasks
        for(int i = 0; i < 3; i++){
            if(msg_num[i] > 0){ //if core i signals that it has completed its task
                int result;
                //read error handling//
                if(read(core_to_main[i][0], &result, sizeof(int)) == -1){
                    fprintf(stderr, "reading task at core %d", i+1);
                }else{ 
                    printf("Core %d completed task %d ######################\n", i+1, result + 1);
                    --msg_num[i]; //reset core's signal
                    idle_cores[i] = 1; //set core to avaiable (may be redundant with msg_num but its how i made it and it works)
                    tasksC++;
                }  
            }
        }
    }

    printf("All tasks completed!\n");

    //after run cleanup//
    for(int i = 0; i < 3; i++){
        close(main_to_core[i][1]);
        close(main_to_core[i][0]);
        close(core_to_main[i][1]);
        close(core_to_main[i][0]);
    }

    // child reaping >:)
    for (int i = 0; i < 3; i++) {
        int status;
        wait(&status); 
        if (WIFEXITED(status)) {
            printf("Core %d exited with %d tasks completed.\n", i + 1, WEXITSTATUS(status));
        } else {
            fprintf(stderr, "Core %d did not terminate normally.\n", i + 1);
        }
    }

    return(0);
}

