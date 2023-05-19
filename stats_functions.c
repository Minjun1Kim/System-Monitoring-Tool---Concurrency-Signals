#include "a3.h"

// We will implement a version of it which allows the program to run the different queries of the system concurrently.

// For doing this the following is critical to be implemented as follow:

//         you should fix any issues raised in your A1.
//         your implementation for A1 should be modular, meaning that you should have at least one function for each of the reported analysis: memory utilization, connected users and CPU utilization.
//         your implementation should have at least two different C files, one containing the main program and the other one, e.g. names stats_functions.c, the implementation of the functions described above.
//      Additionally, 
//             The new version of the code will intercept signals coming from Ctrl-Z and Ctrl-C.
//             For the former, it will just ignore it as the program should not be run in the background while running interactively.
//             For the latter, the program will ask the user whether it wants to quit or not the program.
//             The code will include error-checking for any possible circumstance where resources to be requested from the system may not be available and report them to standard error.

 
// Important details:

//     The goal is to have one process launched per function reporting memory utilization, connected users and CPU utilization.
//     There are different ways in which this can be achieved, but full marks will be given to implementations employing pipes to communicate the results to the main process.


// return a pointer to a newly allocated queue

users *setUp(void){
    users *q = (users *)calloc(1, sizeof(users)); // allocate 1 memory of sizeof(c_queue)
    if(q) q->head = q->tail = NULL; // set head and tail to NULL upon successful allocation

    return q; // return pointer to queue
}

void enqueue(users *q, char line[UT_LINESIZE + UT_NAMESIZE + UT_HOSTSIZE + 10]){

    User *node = (User *)calloc(1, sizeof(User)); // allocate 1 memory of sizeof(c_table)

    if(node==NULL) return;  // if allocation fails, return early

    // initialize node's fields
    strcpy(node->line, line);

    if(q->head == NULL){ // if queue is empty, set head and tail to new node
        q->head = node; 
    }else{ // otherwise, set tail's next to new node and set tail to new node
        q->tail->next = node;
    }
    q->tail = node;
}

void write_memory_pipe(int write_fd, double *prev_virt, char str[MAX_LEN], int i, int graphics){

    mem_struct memory; // declare a struct of type mem_struct
    double virt_used = write_memory(str, i); // write memory information to strArr at index i and return virtual memory used

    strcpy(memory.mem_str, str); // store str in memory struct
    memory.virt_used = virt_used; // store virtual memory used in memory struct
    
    write(write_fd, &memory, sizeof(memory)); // write str to pipe
}


// stores calculated physical and virtual memory information as a string in strArr at index i and returns virtual used memory
double write_memory(char str[MAX_LEN], int i){
    struct sysinfo sys_info;    //a struct of type sysinfo is declared to store system information (from <sys/sysinfo.h>)
    double phys_used, phys_total, virt_used, virt_total; 
    
    sysinfo(&sys_info); //retrieves information about the system into sys_info

    //physical memory used calculated by subtracting free physical memory from the total physical memory and converting to gigabytes
    phys_used = (float)(sys_info.totalram - sys_info.freeram)/1024/1024/1024; 
    phys_total = (float)sys_info.totalram/1024/1024/1024; //storing total physical memory and dividing to convert the value to gigabytes
    
    //virtual memory used calculated by adding the used physical memory to the used virtual memory obtained by subtracting the free virtual memory
    //from the total virtual memory and dividing by 1024^3 to convert it to gigabytes.
    virt_used = phys_used + (float)(sys_info.totalswap - sys_info.freeswap) /1024/1024/1024;
    
    virt_total = (float)(sys_info.totalram + sys_info.totalswap)/1024/1024/1024; //total virtual memory calculated by adding total physical memory to total virtual memory

    sprintf(str, "%.2f GB / %.2f GB -- %.2f GB / %.2f GB",  
    phys_used, phys_total, virt_used, virt_total); //store the calculated physical and virtual memory information as a string into strArr at index i

    return virt_used; //returns the virtual used memory
}

// modifies a string representation of the virtual memory usage of the system for future printing use
void modify_memory_graphics(int i, double virt_used, double *prev_virt, char strArr[][1024]){
    
    char line[1024]="\0", temp[1024]="\0"; // initialize empty strings
    int iter=0;
    double diff=0.00;

    strcpy(line, "   |");
    
    if(i==0)
        diff=0.00; //on first iteration, set diff to 0 since no previous iteration exists
    else
        diff=virt_used - *(prev_virt); //calculates the difference between the current virtual memory usage and the previous usage
    
    if(diff>=0.00 && diff<0.01){
        strcat(line, "o "); //if the differenece is nonnegative and less than 0.01 GB, then "o" is concatenated to 'line'
    } else if (diff<0 && diff>-0.01){
        strcat(line, "@ "); //if the difference is negative and greater than -0.01 GB, then "@" is concatenated to 'line'
    } else {
        iter = fabs((int) ((diff-(int)diff+0.005)*100)); //otherwise, stores the first two decimal places of the difference into a variable as an integer
        
        if(diff<0){
            for(int i=0; i<iter; i++) {strcat(line, ":");} //if difference is negative, the loop concatenates ':' to 'line' ('iter' number of times)
            strcat(line, "@ "); //then concatenates "@ " to the string
        } else {
            for(int i=0; i<iter; i++) {strcat(line, "#");} //if difference is nonnegative, the loop concatenates '#' to 'line' ('iter' number of times)
            strcat(line, "* "); //then concatenates "* " to the string
        }  

    }

    *(prev_virt)=virt_used; //the value of virt_used memory is stored in prev_virt for future calculation/use of prev_virt in the upcoming iterations of sampling
    sprintf(temp, "%.2f (%.2f)", diff, virt_used); //stores the difference of virtual memory (prev and cur) and virtual used memory into the 'temp' string 
    strcat(line, temp); //concatenates temp to line
    strcat(strArr[i], line); //'line' string that has been formatted is concatenated to strArr at the current iteration index passed from main
    
}

//displays memory usage information stored in an array of strings 'strArr' according to sequential flag
void display_memory_line(int sequential, int samples, int i, char strArr[][1024]){
    int j=0;
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot)\n"); //prints header

    if(sequential){ //checks if sequential is true (i.e: 1)
        for(j=0; j<samples; j++){ //goes through all the elements in strArr
            if(j==i) //if the current iternation index equals the current index samples loop from main 
                printf("%s\n", strArr[j]); //then the corresponding current memory information stored in strArr at the index is printed
            else
                printf("\n"); //otherwise fill the lines with a new line
        }
    } else {
        for(j=0; j<=i; j++) printf("%s\n", strArr[j]); //if sequential is false, prints all the elements in strArr upto the current index 'i' passed from main
        for(int k=j+1; k<=samples; k++) printf("\n"); //fills the rest of the lines with a new line until it reaches # of samples
    }
}



void write_users_pipe(int write_fd){

     //printf("### Sessions/users ###\n"); //prints a header

    setutent(); //resets the internal stream of the utmp database to the beginning for reading utmp.h file
    
    for(struct utmp *user=NULL; (user=getutent());){ //read the records of the utmp database one by one until it returns a NULL pointer signalling the end

        if(user->ut_type != USER_PROCESS) continue; //checks if the user is currently logged into the system and running a process
        
        int len = snprintf(NULL, 0, " %s\t%s\t(%s)\n", user->ut_user, user->ut_line, user->ut_host);

        if(len<0) {
            perror("snprintf");
            return;
        } else {
            len++;
        }

        char userStr[len];
        memset(userStr, 0, sizeof(userStr));
        snprintf(userStr, len, " %s\t%s\t(%s)\n", user->ut_user, user->ut_line, user->ut_host);
        
        write(write_fd, userStr, len);
        
    }

    endutent(); //closes the internal stream of the utmp database
    //write(write_fd, "END\0", 4); //write end of string to pipe
    
}

users* read_users_pipe(int read_fd){

    users *queue = setUp(); //initialize queue

    if(queue==NULL){
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    //read from pipe
    char userStr[UT_NAMESIZE+UT_LINESIZE+UT_HOSTSIZE+10];
    int bytesRead=0;

    while((bytesRead = read(read_fd, userStr, sizeof(userStr))) > 0){
        
        if (bytesRead < sizeof(userStr)) {
            userStr[bytesRead] = '\0';
        } else {
            userStr[sizeof(userStr) - 1] = '\0';
        }


        enqueue(queue, userStr);
    }

    return queue;
}

users *delete_users(users *queue){

    if(queue==NULL) return NULL;

    User *node = queue->head;
    User *temp = NULL;

    while(node!=NULL){
        temp = node;
        node = node->next;
        free(temp);
    }

    free(queue);
    return NULL;
}

//displays the users logged into the system
void print_users(users *queue){

    for(User *node=queue->head; node!=NULL; node=node->next){
        printf("%s", node->line);
    }
}


/*###############################################################################################*/

void write_cpu_pipe(int write_fd, cpu_struct *prevSample){
    
    cpu_struct curSample; //declaring two structs of type cpu_struct
    float cur_cpu_usage=0.00; //declaring a float variable to store the calculated cpu usage

    set_cpu_values(&curSample); //setting cpu values of 'curSample'

    //calculating cpu usage
    cur_cpu_usage = calculate_cpu_usage(prevSample, &curSample); //calculates and assigns 'cur_cpu_usage' based on 'prevSample' and 'curSample'
    write(write_fd, &cur_cpu_usage, sizeof(cur_cpu_usage)); //writes the calculated cpu usage to the pipe

}


void set_cpu_values(cpu_struct *sample)
{
    FILE *fp = fopen("/proc/stat", "r"); //opening /proc/stat file in read-mode and assigns the returned file pointer to fp

    if(fp==NULL){ //checks if file opening is successful
        fprintf(stderr, "File could not be opened\n"); //if there was an error opening file, the message is printed to stderr
        exit(1); //exit program
    }

    char buffer[255]; //a char array used to store the format string
    //fscanf is used to read data from the file into the variables
    int success = fscanf(fp, "%s %lu %lu %lu %lu %lu %lu %lu", buffer, &(sample->user), &(sample->nice), 
        &(sample->system), &(sample->idle), &(sample->iowait), &(sample->irq), &(sample->softirq));
    //specifies that the function should read one string followed by seven unsigned long integers

    if(success!=8){ //checks the number of items successfully read by fscanf
        fprintf(stderr, "Error reading file\n");    //prints error message to stderr if error occurs
        fclose(fp); //close file pointer
        exit(1); //exit program
    }
    fclose(fp); //close file pointer
}


/*
 *given two unsigned long arrays prev and cur representing the 7 fields of cpu information as obtained from
 * /proc/stat that were sampled at different times, it calculates and returns the cpu usage percentage.
 */
double calculate_cpu_usage(cpu_struct *prev, cpu_struct *cur)
{
    int prev_total = prev->user + prev->nice + prev->system + prev->idle + prev->iowait + prev->irq + prev->softirq; //sum of all fields of /proc/stat in prev sample
    int cur_total = cur->user + cur->nice + cur->system + cur->idle + cur->iowait + cur->irq + cur->softirq; //sum of all fields of /proc/stat in cur sample
    
    int prev_util = prev_total - prev->idle; //calculating the total cpu utilization in prev sample
    int cur_util = cur_total - cur->idle; //calculating the total cpu utilization in cur sample

    return (double)(cur_util - prev_util) / (cur_total - prev_total) * 100; //calculating and returning the cpu usage percentage

}


//  displays the number of samples and tdelay between samples
void display_header(int i, int sequential, int samples, int tdelay){

    if(sequential){
        printf(">>> iteration %d\n", i); //prints iteration number if sequential
    } else {
        printf("\033[H\033[2J"); //clears the terminal screen and move the cursor to the top-left corner
        printf("Nbr of samples: %d -- every %d secs\n", samples, tdelay); //prints the number of samples and tdelay between samples
    }

    struct rusage mem_usage; //declaring a struct of type rusage found in <sys/resource.h>
    struct rusage *mem_ptr = &mem_usage; //creating pointer pointing to the address of mem_usage

    if(getrusage(RUSAGE_SELF, mem_ptr)==0){ //checks if the call to getrusage is successful
        printf(" Memory usage: %ld kilobytes\n", mem_ptr->ru_maxrss); //prints memory usage through ru_maxrss field of rusage struct
    }
}

//  prints the number of currently available cores using sysconf
void print_cores(void){
    int n_cpu = sysconf(_SC_NPROCESSORS_ONLN); //sysconf returns the number of processors (found in <unistd.h>)
    printf("Number of cores: %d\n", n_cpu); //printing number of cores to stdout
}


//prints system information about the machine the program is running on
void print_machine_info(void){
    struct utsname sysData; //a struct of type utsname is declared (found in <sys/utsname.h>)
    uname(&sysData); //fills in the fields of the sysData struct with information about the current Operating System
    printf("### System Information ###\n");
    printf(" System Name = %s\n", sysData.sysname); //prints system name
    printf(" Machine Name = %s\n", sysData.nodename); //prints machine name
    printf(" Version = %s\n", sysData.version); //prints OS version
    printf(" Release = %s\n", sysData.release); //prints OS/software release
    printf(" Architecture = %s\n", sysData.machine); //prints machine architecture (computer hardware type)
}

//  updates and displays the current CPU usage graphics in a graphical format
void cpu_graphics(char cpuArr[][200], int i, int sequential, int *num_bar, float cur_cpu_usage, float *prev_cpu_usage){
    int diff_bar=0; //initialize a variable that records the difference in bars ("|") between iterations
    char cpuStr[200]="\0"; //initialize an empty string

    if(i==0){
        sprintf(cpuArr[i], "         |||%.2f", cur_cpu_usage); //on first iteration, stores default of 3 bars followed by current cpu usage into first element of cpuArr
    } else {
        diff_bar = (int)cur_cpu_usage - (int)(*(prev_cpu_usage)); //otherwise, calculates the difference in CPU usage between prev and cur iterations (only looking at the integer part)
        *(num_bar) += diff_bar; //updates the current 'num_bar' value with the new difference in number of bars calculated above
        strcpy(cpuArr[i], "         "); //copies space into cpuArr at index i
        
        for(int m=0; m<*(num_bar); m++) {
            strcat(cpuArr[i], "|"); //appends the number of bars represented by 'num_bar' and writes to cpuArr at index i
        }
        sprintf(cpuStr, "%.2f", cur_cpu_usage); //writes the current cpu usage upto 2 decimal places into cpuStr string
        strcat(cpuArr[i], cpuStr); //concatenates cpuStr into the already graphically formatted string of cpuArr at index i
    }

    if(sequential){ //checks if sequential is true (i.e. 1)
        for(int j=0; j<=i; j++){ //then iterate through the cpuArr upto the current iteration index i from main
            if(j==i) //if index j matches the current iteration index i from main
                printf("%s\n", cpuArr[j]); //prints the current iteration of cpuArr at index j
            else
                printf("\n"); //otherwise leave the rest blank (i.e. fill in the links with a new line)
        }
    } else { //if sequential is false
        for(int h=0; h<=i; h++){ //iterate through cpuArr upto the current iteration index i from main
            printf("%s\n", cpuArr[h]); //prints the entire cpuArr upto and including the current iteration
        }
    }

    *(prev_cpu_usage) = cur_cpu_usage; //sets the 'prev_cpu_usage' to 'cur_cpu_usage' for later iteration(s)
}
