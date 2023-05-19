#include "a3.h"


/* Function to handle the SIGINT signal. Takes a single parameter, 'sig' (representing the signal number).
 * The function prints a message to the user asking if they want to quit the program. If the user enters
 * 'y' or 'Y', the program exits. If the user enters 'n' or 'N', the program continues to run.
 */
void sigint_handler(int sig) {
    char response;

    printf("Do you want to quit? (y/n): ");
    fflush(stdout); // Make sure the output is printed immediately

    while (read(STDIN_FILENO, &response, 1) > 0) {
        if (response == 'y' || response == 'Y') {
            exit(0);
        } else if (response == 'n' || response == 'N') {
            return;
        }
    }
}

/* Function to handle the SIGTSTP signal. Takes a single parameter, 'sig' (representing the signal number).
 * Ignores the signal, allowing the program to continue running.
 */
void sigtstp_handler(int sig) {
    return;
}


/* Main function, implementing the functionality to display memory, user, cpu usage of the system.
 * Takes two parameters, 'argc' (representing the number of arguments passed to the program) and
 * 'argv' (representing an array of the arguments passed to the program).
 */
int main(int argc, char *argv[]) {

    //initializing variables and flags used to control the display of information in the program
    int i, samples = 10, tdelay = 1, system = 0, user = 0, graphics = 0, sequential = 0, cmd; 
    int memory_pipe[2], users_pipe[2], cpu_pipe[2];
    pid_t pid_memory=getpid(), pid_users=getpid(), pid_cpu=getpid();
    users* user_queue = NULL;
    struct sigaction sa;
    cpu_struct prev_cpu_struct;

    //uses getopt_long to parse the command line options passed to the program
    struct option long_options[] = { //an array of 'struct option' objects, each line representing a single command line option
        {"system", no_argument, 0, 's'}, //takes "system" with no argument, returns 's' if option is present
        {"user", no_argument, 0, 'u'}, //takes "user" with no argument, returns 'u' if option is present
        {"graphics", no_argument, 0, 'g'}, //takes "graphics" with no argument, returns 'g' if option is present        
        {"sequential", no_argument, 0, 'q'}, //takes "sequential" with no argument, returns 'q' if option is present
        {"samples", optional_argument, 0, 'n'}, //takes "samples" with optional argument, returns 'n' if option is present
        {"tdelay", optional_argument, 0, 't'}, //takes "tdelay" with optional argument, returns 't' if option is present
        {0,0,0,0} //indicates the end of options
    };

    double virt_used=0.0, prev_virt=0.00; //declares and initializes variables for virtual current and previous memory usage
    float cur_cpu_usage=0.00, prev_cpu_usage=0.00; //declares and initializes variables for current and previous cpu usage
    char strArr[samples][1024]; //a 2D char array for storing memory display information
    char cpuArr[samples][200]; //a 2D char array for storing cpu display information
    int num_bar=3; //sets default num of bars for cpu graphics to 3

    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    //retrieves and processes command line options passed to the program using the 'getopt_long' function
    // stored in argv array, and returns the next option found in the argument list
    //loop continues until getopt_long returns -1, meaning all the options have been processed

    while ((cmd=getopt_long(argc, argv, "sugqn::t::", long_options, NULL)) != -1){ 
        //the string "sugqn::t::" specifies that he options -s, -u, -g, -q, -n and -t are available. 
        //The (::) following the letters n and t indicate that an optional argument, which the user can specify by appending a value to the option on the command line
        
        switch (cmd) { //switch statment to determine action to take based on the option returned by getopt_long
            case 's':
                system = 1; //in case cmd is 's', 'system' is set to 1
                break;
            case 'u':
                user = 1; //in case cmd is 'u', 'user' is set to 1
                break;
            case 'g':
                graphics = 1; //in case cmd is 'g', 'graphics' is set to 1
                break;
            case 'q':
                sequential = 1; //in case cmd is 'q', 'sequential' is set to 1
                break;
            case 'n':
                //in case cmd is 'n', if option has an argument, atoi converts the argument from string to integer and updates the value of samples 
                if (optarg) samples = atoi(optarg);
                break;
            case 't':
                //in case cmd is 't', if option has an argument, atoi converts the argument from string to integer and updates the value of tdelay 
                if (optarg) tdelay = atoi(optarg); 
                break;
        }

    }

    //performs iterations over the remaining command line arguments after the options
    // have been processed by 'getopt_long'. Starting from the index of the first non-option argument
    // given by 'optind', the loop iterates until ind reaches the number of arguments (argc)
    for(int ind = optind, iter=0; ind < argc; ind++, iter++) {
        //value of iter is used to determine which argument is being processed
        switch(iter){
            case 0: 
                samples = atoi(argv[ind]); //if iter is 0, then samples gets updated with the integer represenation of the string argument
                break;
            case 1:
                tdelay = atoi(argv[ind]); //if iter is 1, then tdelay gets updated with the integer representation of the string argument
                break;
        }
        //the use of iter maintains the order and functionality of the postional arguments
    }

    signal(SIGINT, sigint_handler);
    //signal(SIGTSTP, sigtstp_handler);
    
    for (i = 0; i < samples; i++) { // iterate through the number of samples

        set_cpu_values(&prev_cpu_struct); //sets the values of prev_cpu_usage to the current cpu usage values
        sleep(tdelay);

        if (pipe(memory_pipe) == -1 || pipe(users_pipe) == -1 || pipe(cpu_pipe) == -1) { //if pipe fails, an error message is printed and the program exits
            fprintf(stderr, "Pipe Failed" );
            return 1;
        }

        //creates a child process for the memory information
        
        if(!user || (user && system))
            pid_memory = fork(); //forks a child process for memory information

        if (pid_memory < 0) {
            fprintf(stderr, "Fork Failed");
            return 2;
        } else if (pid_memory == 0) {
            // This is the memory information child process
            signal(SIGINT, SIG_DFL); // Reset signal handler for SIGINT in child process
            
            close(memory_pipe[0]); //close the read end of the pipe
            char str[MAX_LEN]={0};
            write_memory_pipe(memory_pipe[1], &prev_virt, str, i, graphics); //write to the write end of the pipe
            close(memory_pipe[1]); //close the write end of the pipe
            exit(0); //exit the child process

        } else {
            if((user && system)||!system)
                pid_users = fork();

            if (pid_users < 0) {
                fprintf(stderr, "Fork Failed");
                return 2;
            } else if (pid_users == 0) {
                signal(SIGINT, SIG_DFL); // Reset signal handler for SIGINT in child process
                //signal(SIGTSTP, SIG_DFL); // Reset signal handler for SIGTSTP in child process
                close(users_pipe[0]); //close the read end of the pipe
                write_users_pipe(users_pipe[1]); //write to the write end of the pipe
                close(users_pipe[1]); //close the write end of the pipe
                exit(0); //exit the child process
            } else {
                if(!user || (user && system))
                    pid_cpu = fork();

                if (pid_cpu < 0) {
                    fprintf(stderr, "Fork Failed");
                    return 2;
                } else if (pid_cpu == 0) {
                    // This is the cpu information child process
                    signal(SIGINT, SIG_DFL); // Reset signal handler for SIGINT in child process
                    //signal(SIGTSTP, SIG_DFL); // Reset signal handler for SIGTSTP in child process
                    close(cpu_pipe[0]); //close the read end of the pipe
                    write_cpu_pipe(cpu_pipe[1], &prev_cpu_struct); //write to the write end of the pipe
                    close(cpu_pipe[1]); //close the write end of the pipe
                    exit(0); //exit the child process
                } else {
                    // This is the main (parent) process

                    close(memory_pipe[1]); //close the write end of the pipe
                    close(users_pipe[1]); //close the write end of the pipe
                    close(cpu_pipe[1]); //close the write end of the pipe
                    
                    waitpid(pid_memory, NULL, 0);
                    waitpid(pid_cpu, NULL, 0);

                    if(!user || (user && system)){
                        mem_struct mem; //struct to store memory information
                        read(memory_pipe[0], &mem, sizeof(mem));  //read from the read end of the pipe
                        virt_used = mem.virt_used; //update the value of virt_used
                        strcpy(strArr[i], mem.mem_str); //copy the memory information to strArr at index i

                        read(cpu_pipe[0], &cur_cpu_usage, sizeof(cur_cpu_usage)); //reads the current cpu usage from the pipe
                    }

                    close(memory_pipe[0]); //close the read end of the pipe

                    waitpid(pid_users, NULL, 0);
                    if((user && system)||!system){
                        user_queue = read_users_pipe(users_pipe[0]); //read from the read end of the pipe
                    }

                    close(users_pipe[0]); //close the read end of the pipe
                    close(cpu_pipe[0]); //close the read end of the pipe
                    

                    display_header(i, sequential, samples, tdelay); //displays header information
                    if(!user || (user && system)){ //runs so long as the argument doesn't contain just '--user'
                        printf("---------------------------------------\n");

                        if(graphics){
                            modify_memory_graphics(i, virt_used, &prev_virt, strArr); //modify strArr graphically if graphics is an option
                        }

                        display_memory_line(sequential, samples, i, strArr); //displays lines of memory information according to sequential and strArr
                        
                        if((user && system)||!system){ //prints users if user and system are both options and skips if system option was given without user
                            printf("---------------------------------------\n");
                            printf("### Sessions/users ###\n"); //prints a header
                            print_users(user_queue); //prints current user information on server
                            printf("---------------------------------------\n");
                        }

                        print_cores(); //print the number of cores
                        
                        
                        printf(" total cpu use: %.2f%%\n", cur_cpu_usage); //prints current cpu usage upto 2 decimal places


                        if(graphics)
                            cpu_graphics(cpuArr, i, sequential, &num_bar, cur_cpu_usage, &prev_cpu_usage); //if graphics option is given, display cpu graphics
                    
                    }else{ //runs when only user option is given
                        printf("---------------------------------------\n");
                        print_users(user_queue);
                        printf("---------------------------------------\n");
                    }

                    user_queue = delete_users(user_queue); //deletes the user queue
                }
            }
        }

    }

    close(memory_pipe[0]); //close the read end of the pipe
    close(users_pipe[0]); //close the read end of the pipe
    close(cpu_pipe[0]); //close the read end of the pipe

    printf("---------------------------------------\n");
    print_machine_info(); //prints machine information all the time at the end
    printf("---------------------------------------\n");

    return 0;

    
}