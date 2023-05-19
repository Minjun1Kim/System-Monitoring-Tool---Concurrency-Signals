#ifndef __A3_header
#define __A3_header

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <utmp.h>
#include <getopt.h>
#include <math.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LEN 1024
#define NOTHING -1

/**
 *  @brief Represents a memory struct.
 *  stores information about a memory's total, free, available, buffers, cached, and swap.
**/
typedef struct {
    char mem_str[MAX_LEN];
    double virt_used;
} mem_struct;

/**
 *  @brief Represents a cpu struct.
 *  stores information about a cpu's user, nice, system, idle, iowait, irq, and softirq.
**/
typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
} cpu_struct;

/**
 *  @brief Represents a user struct.
 *  stores information about a user's line, name, host, and next.
**/
typedef struct user {
    char line[UT_LINESIZE + UT_NAMESIZE + UT_HOSTSIZE + 10];
    struct user *next;
} User;

/**
 *  @brief Represents a queue of User nodes.
 *  stores pointers to the head and tail of the queue.
**/
typedef struct queue {
    User *head;  // pointer to head of queue
    User *tail;  // pointer to tail of queue
} users;

/**
 *  @brief Represents a sample struct.
 *  stores information about a sample's previous and current virtual memory.
**/
typedef struct sample {
    double prev_virt; // previous virtual memory
    double cur_virt; // current virtual memory
} sample;



/**
* @brief Write the memory information to the pipe.
* @param write_fd the file descriptor to write to
* @param prev_virt the previous virtual memory
* @param str the string to write to the pipe
* @param i the index of the sample
* @param graphics the graphics flag
* @return None
*/
void write_memory_pipe(int write_fd, double *prev_virt, char str[MAX_LEN], int i, int graphics);

/**
* @brief Write the memory information to the pipe.
* @param str the string to write to the pipe
* @param i the index of the sample
* @return double the virtual memory used
*/
double write_memory(char str[MAX_LEN], int i);


/**
* @brief Enqueue a newly allocated and initialized User node to the tail of the users 'q' in O(1) time
* @param q the queue to enqueue to
* @param line the string to store to the node that we enqueue
* @return None
*/
void enqueue(users *q, char line[UT_LINESIZE + UT_NAMESIZE + UT_HOSTSIZE + 10]);


/**
* @brief Allocate and initialize a new User queue
* @param None
* @return users* the allocated queue
*/
users *setUp(void);

/**
* @brief Write the user information to the pipe.
* @param write_fd the file descriptor to write to
* @return None
*/
void write_users_pipe(int write_fd);

/**
* @brief Read the user information from the pipe.
* @param read_fd the file descriptor to read from
* @return users* the queue of users
*/
users* read_users_pipe(int read_fd);

/**
* @brief Write the cpu information to the pipe.
* @param write_fd the file descriptor to write to
* @param prev the previous cpu struct
* @return None
*/
void write_cpu_pipe(int write_fd, cpu_struct *prev);

/**
* @brief Delete the users in the queue.
* @param queue the queue to delete
* @return users* the queue of users
*/
users *delete_users(users *queue);

/**
* display the header of the program
* @param i the index of the sample
* @param sequential the sequential flag
* @param samples the number of samples
* @param tdelay the delay between samples
* @return None
*/
void display_header(int i, int sequential, int samples, int tdelay);

/**
* @brief Print the number of cores.
* @param None
* @return None
*/
void print_cores(void);

/**
* @brief Set the cpu values.
* @param sample the cpu struct to set
* @return None
*/
void set_cpu_values(cpu_struct *sample);

/**
* @brief Calculate the cpu usage.
* @param prev the previous cpu struct
* @param cur the current cpu struct
* @return double the cpu usage
*/
double calculate_cpu_usage(cpu_struct *prev, cpu_struct *cur);

/**
* @brief Print the users in the queue.
* @param queue the queue to print
* @return None
*/
void print_users(users *queue);

/**
* @brief Print the machine information.
* @param None
* @return None
*/
void print_machine_info(void);

/**
* @brief Modify the memory graphics.
* @param i the index of the sample
* @param virt_used the virtual memory used
* @param prev_virt the previous virtual memory
* @param strArr the array of strings to modify
* @return None
*/
void modify_memory_graphics(int i, double virt_used, double *prev_virt, char strArr[][1024]);

/**
* @brief Display the memory string at strArr[i].
* @param sequential the sequential flag
* @param samples the number of samples
* @param i the index of the sample
* @param strArr the array of strings to display
* @return None
*/
void display_memory_line(int sequential, int samples, int i, char strArr[][1024]);

/**
* @brief Display the cpu string at cpuArr[i].
* @param sequential the sequential flag
* @param samples the number of samples
* @param i the index of the sample
* @param cpuArr the array of strings to display
* @return None
*/
void cpu_graphics(char cpuArr[][200], int i, int sequential, int *num_bar, float cur_cpu_usage, float *prev_cpu_usage);

#endif