#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <semaphore.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <argsint.h>

#define SHARED_MEM_NAME "/filosofi_shared_memory"

void execute_philosopher(int id, sem_t* semaphore1, sem_t* semaphore2);

void initial_setup();

void global_variables_setup();

void signals_handlers_setup();
void terminator_signal_handler_setup();

void terminator_signals_aggregator(int signal);
void sigint_handler(int signal);
void sigterm_handler(int signal);
void sigquit_handler(int signal);
void sighup_handler(int signal);

void shared_memory_setup();
void create_shared_memory();
void set_shared_memory_size();

void shared_semaphores_setup();
void map_shared_semaphores();
void init_shared_semaphore();
void free_global_variables();
void destroy_shared_semaphores();
void destroy_shared_memory();

int shm_fd;
struct application_options app_opt;
sem_t* shared_semaphores = NULL;
int* philosophers_pids = NULL;
int parent_pid;

int main(int argc, char *argv[])
{
    // Interpretation of arguments
    int_args(argc, argv, &app_opt);
    initial_setup();

    if(app_opt.verbose) { printf("Creating philosophers..\n"); }
    for(int i = 0; i < app_opt.count; i++) {
        int pid = fork();
        if(pid < 0) {
            perror("fork");
            exit(1);
        }
        if(pid == 0) {
            // Child process
            if(i == 0) {
                execute_philosopher(i, &shared_semaphores[app_opt.count - 1], &shared_semaphores[0]);
            }else {
                execute_philosopher(i, &shared_semaphores[i - 1], &shared_semaphores[i]);
            }
        } else {
            // Parent Process
            philosophers_pids[i] = pid;
        }
    }
    if(app_opt.verbose) { printf("Philosophers created.\n"); }

    for(int i = 0; i < app_opt.count; i++) {
        waitpid(philosophers_pids[i], NULL, 0);
    }
    if(app_opt.verbose) { printf("Philosophers processes ended.\n"); }

    return 0;
}

// Functions

void execute_philosopher(int id, sem_t* semaphore1, sem_t* semaphore2) {
    int semaphore_value;
    srand(time(NULL));
    while(true) {
        
        // Locks the semaphore if it is not locked by any one else
        printf("[Philosopher %d] Is waiting for the right fork.\n", id);
        if(sem_wait(semaphore1) < 0) {
            perror("sem_wait");
            exit(2);
        }
        printf("[Philosopher %d] Has taken the right fork.\n", id);

        // Locks the semaphore if it is not locked by any one else
        printf("[Philosopher %d] Is waiting for the left fork.\n", id);
        if(sem_wait(semaphore2) < 0) {
            perror("sem_wait");
            exit(2);
        }
        printf("[Philosopher %d] Has taken the left fork.\n", id);

        // "Eating" for a random time between 1 and 5 seconds
        int eating_time = rand() % 5 + 1;
        printf("[Philosopher %d] Eating for %d seconds..\n", id, eating_time);
        sleep(eating_time);
        printf("[Philosopher %d] Done eating.\n", id);

        // Releases the semaphore
        printf("[Philosopher %d] Is putting down the right fork .\n", id);
        if(sem_post(semaphore1) < 0) {
            perror("sem_post");
            exit(3);
        }
        printf("[Philosopher %d] Has put down the right fork.\n", id);

        // Releases the semaphore
        printf("[Philosopher %d] Is putting down the left fork .\n", id);
        if(sem_post(semaphore2) < 0) {
            perror("sem_post");
            exit(3);
        }
        printf("[Philosopher %d] Has put down the left fork.\n", id);
    }
    
    exit(0);
}

// Utility functions

void initial_setup() {
    if(app_opt.verbose) { printf("Initial setup started.\n"); }

    global_variables_setup();
    signals_handlers_setup();
    shared_memory_setup();
    shared_semaphores_setup();

    if(app_opt.verbose) { printf("Initial setup completed.\n"); }
}

void free_global_variables() {
    if(app_opt.verbose) { printf("Releasing memory..\n"); }
    free(shared_semaphores);
    free(philosophers_pids);
    if(app_opt.verbose) { printf("Memory released.\n"); }
}

void destroy_shared_semaphores() {
    if(app_opt.verbose) { printf("Destroying shared semaphores.\n"); }

    for(int i = 0; i < app_opt.count; i++) {
        if(sem_destroy(&shared_semaphores[i]) < 0) {
            perror("sem_destroy");
            exit(5);
        }
        if(app_opt.verbose) { printf("Destroyed shared semaphore [%d].\n", i); }
    }
    
    if(app_opt.verbose) { printf("Shared semaphores destroyed.\n"); }
}

void destroy_shared_memory() {
    if(app_opt.verbose) { printf("Destroying shared memory.\n"); }

    // Unmap all semaphores from the shared memory
    if(app_opt.verbose) { printf("Unmapping all semaphores..\n"); }
    if(munmap(shared_semaphores, sizeof(sem_t) * app_opt.count) < 0) {
        perror("munmap");
        exit(10);
    }
    if(app_opt.verbose) { printf("Semaphores unmapped.\n"); }

    // Close the shared memory File Descriptor
    if(app_opt.verbose) { printf("Closing shared memory file descriptor.\n"); }
    if(close(shm_fd) < 0) {
        perror("close");
        exit(11);
    }
    if(app_opt.verbose) { printf("Shared memory file descriptor closed.\n"); }

    // Unlink and remove the shared memory from the system
    if(app_opt.verbose) { printf("Unlinking shared memory.\n"); }
    if(shm_unlink(SHARED_MEM_NAME) < 0) {
        perror("shm_unlink");
        exit(12);
    }
    if(app_opt.verbose) { printf("Shared memory unlinked.\n"); }
    
    if(app_opt.verbose) { printf("Shared memory destroyed.\n"); }
}

// Signals handlers setup section

void signals_handlers_setup() {
    if(app_opt.verbose) { printf("Signals handlers setup started.\n"); }

    terminator_signal_handler_setup();
    
    if(app_opt.verbose) { printf("Signals handlers setup completed.\n"); }
}

void terminator_signal_handler_setup() {
    struct sigaction sa;
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = &sigint_handler;
    if(app_opt.verbose) { printf("Created sigaction options.\n"); }

    sigaction(SIGINT, &sa, NULL);
    if(app_opt.verbose) { printf("Registered SIGINT sigaction handler.\n"); }
}

// General variables setup section
void global_variables_setup() {
    /* shared_semaphores = (sem_t **)malloc(app_opt.count * sizeof(sem_t*)); */
    philosophers_pids = (int *)malloc(app_opt.count * sizeof(int));
    parent_pid = getpid();
}

// Shared memory setup section

void shared_memory_setup() {
    if(app_opt.verbose) { printf("Shared memory setup started.\n"); }

    create_shared_memory();
    set_shared_memory_size();

    if(app_opt.verbose) { printf("Shared memory setup completed.\n"); }
}

void create_shared_memory() {
    if(app_opt.verbose) { printf("Creating shared memory.\n"); }

    shm_fd = shm_open(SHARED_MEM_NAME, O_CREAT | O_RDWR, 0777);
    if(shm_fd < 0) {
        perror("shm_open");
        exit(6);
    }

    if(app_opt.verbose) { printf("Shared memory created.\n"); }
}

void set_shared_memory_size() {
    if(app_opt.verbose) { printf("Setting shared memory size.\n"); }

    if (ftruncate(shm_fd, sizeof(sem_t) * app_opt.count) == -1) {
        perror("ftruncate");
        exit(7);
    }

    if(app_opt.verbose) { printf("Shared memory size set.\n"); }
}

// Shared semaphore setup section

void shared_semaphores_setup() {
    if(app_opt.verbose) { printf("Shared semaphores setup started.\n"); }

    map_shared_semaphores();
    init_shared_semaphore();

    if(app_opt.verbose) { printf("Shared semaphores setup started.\n"); }
}

void map_shared_semaphores() {
    if(app_opt.verbose) { printf("Shared semaphores mapping started.\n"); }

    shared_semaphores = mmap(NULL, sizeof(sem_t) * app_opt.count, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    /* for(int i = 0; i < app_opt.count; i++) {
        off_t offset = i * sizeof(sem_t);
        shared_semaphores[i] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, offset);
        if(shared_semaphores[i] == MAP_FAILED) {
            perror("mmap");
            exit(8);
        }
        if(app_opt.verbose) { printf("Shared semaphore [%d] mapped.\n", i); }
    } */
    
    if(app_opt.verbose) { printf("Shared semaphores mapping completed.\n"); }
}

void init_shared_semaphore() {
    if(app_opt.verbose) { printf("Shared semaphores initialization started.\n"); }

    for(int i = 0; i < app_opt.count; i++) {
        if(sem_init(&shared_semaphores[i], 1, 1) < 0) {
            perror("sem_init");
            exit(9);
        }
        if(app_opt.verbose) { printf("Shared semaphores [%d] initialized.\n", i); }
    }

    if(app_opt.verbose) { printf("Shared semaphores initialization completed.\n"); }
}

// Signals handlers

void sigint_handler(int signal) {
    if(getpid() == parent_pid) {
        // Parent Process
        if(app_opt.verbose) { printf("Sigint handler called.\n"); }

        destroy_shared_semaphores();
        destroy_shared_memory();
    } else {
        // Child process
        
        exit(100);
    }   
}
