#include <helperservice.h>

#include <stdbool.h>
#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/mman.h>

#define HELPER_SERVICE_SHARED_MEMORY_NAME "/helper_serviceW_shared_memory"

void helper_service_shared_memory_setup();
void create_helper_service_shared_memory();
void enlarge_helper_service_shared_memory_length(off_t _length);

void synchronization_semaphore_setup();
void map_synchronization_semaphore();
void init_synchronization_semaphore();

void destroy_helper_service_resources();
void destory_synchronization_semaphore();
void destroy_synchronization_shared_memory();

philosopher_state* philosophers_state = NULL;
sem_t* synchronization_semaphore = NULL;
off_t helper_service_shared_memory_length = 0;
int synchronization_fd;
int philosophers_count;
int pid;
bool verbose;

bool __initialized;

void init_helper_service(bool _verbose) {
    verbose = _verbose;
    helper_service_shared_memory_setup();
    synchronization_semaphore_setup();
    __initialized = true;
}

void execute_helper_service(bool _detect_deadlock, bool _detect_starvation) {
    if(!__initialized) {
        perror("Helper service not initialized");
        exit(EXIT_FAILURE);
    }

    if(verbose) { printf("Executing helper service..\n"); }

    pid = getpid();

    if(verbose) { printf("Starting synchronization semaphore..\n"); }

    // Start the synchronization semaphore for all the philosophers
    for(int i = 0; i < philosophers_count; i++) {
        printf("SEMAPHORE [%d] | STATE [%d]\n", philosophers_state[i].id, philosophers_state[i].state);
        sem_post(synchronization_semaphore);
    }

    if(verbose) { printf("Synchronization semaphore started.\n"); }

    while(true) {
        //TODO execute philosophers checks
    }
}

void add_philosopher(int _id, int _state) {
    if(verbose) { printf("Adding philosopher [%d].\n", _id); }

    off_t offset = helper_service_shared_memory_length;

    enlarge_helper_service_shared_memory_length(sizeof(philosopher_state));

    printf("offset: %d\n", offset);

    philosophers_state = (philosopher_state *)mmap(NULL, sizeof(philosopher_state), PROT_READ | PROT_WRITE, MAP_SHARED, synchronization_fd, offset);
    
    philosophers_state[philosophers_count].id = _id;
    philosophers_state[philosophers_count].state = _state;

    philosophers_count++;

    if(verbose) { printf("Philosopher [%d] added.\n", _id); }
}

int get_helper_service_pid() {
    return pid;
}

sem_t* get_synchronization_semaphore() {
    return synchronization_semaphore;
}

// Setup Section

void helper_service_shared_memory_setup() {
    if(verbose) { printf("Synchronization shared memory setup started.\n"); }

    create_helper_service_shared_memory();

    if(verbose) { printf("Synchronization shared memory setup completed.\n"); }
}

void create_helper_service_shared_memory() {
    if(verbose) { printf("Creating shared memory.\n"); }

    synchronization_fd = shm_open(HELPER_SERVICE_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0777);
    if(synchronization_fd < 0) {
        perror("shm_open");
        exit(6);
    }

    if(verbose) { printf("Shared memory created.\n"); }
}

void enlarge_helper_service_shared_memory_length(off_t _length) {
    if(verbose) { printf("Setting helper service shared memory size.\n"); }
    printf("length before: %d\n", helper_service_shared_memory_length);

    if (ftruncate(synchronization_fd, helper_service_shared_memory_length + _length) == -1) {
        perror("ftruncate");
        exit(7);
    }

    helper_service_shared_memory_length += _length;

    printf("length after: %d\n", helper_service_shared_memory_length);

    if(verbose) { printf("Helper service shared memory size set.\n"); }
}

void synchronization_semaphore_setup() {
    if(verbose) { printf("Synchronization semaphore setup started.\n"); }

    map_synchronization_semaphore();
    init_synchronization_semaphore();

    if(verbose) { printf("Synchronization semaphore setup started.\n"); }
}

void map_synchronization_semaphore() {
    if(verbose) { printf("Synchronization semaphore mapping started.\n"); }

    enlarge_helper_service_shared_memory_length(sizeof(sem_t));

    synchronization_semaphore = (sem_t *)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, synchronization_fd, 0);
    if(synchronization_semaphore == MAP_FAILED) {
        perror("mmap");
        exit(600);
    }
    
    if(verbose) { printf("Synchronization semaphore mapping completed.\n"); }
}

void init_synchronization_semaphore() {
    if(verbose) { printf("Synchronization semaphore initialization started.\n"); }

    if(sem_init(synchronization_semaphore, 1, 0) < 0) {
        perror("sem_init");
        exit(9);
    }

    if(verbose) { printf("Synchronization semaphore initialization completed.\n"); }
}

void destroy_helper_service_resources() {
    if(verbose) { printf("Destroying helper service resources..\n"); }
    destory_synchronization_semaphore();
    destroy_synchronization_shared_memory();
    if(verbose) { printf("Helper service resources destroyed.\n"); }
}

void destory_synchronization_semaphore() {
    if(verbose) { printf("Destroying shared semaphores.\n"); }

    if(sem_destroy(synchronization_semaphore) < 0) {
        perror("sem_destroy");
        exit(5);
    }

    if(verbose) { printf("Shared semaphores destroyed.\n"); }
}

void destroy_synchronization_shared_memory() {
    if(verbose) { printf("Destroying synchronization shared memory.\n"); }

    // Unmap all semaphores from the shared memory
    if(verbose) { printf("Unmapping synchronization semaphore..\n"); }
    if(munmap(synchronization_semaphore, sizeof(sem_t)) < 0) {
        perror("munmap");
        exit(10);
    }
    if(verbose) { printf("Synchronization semaphore unmapped.\n"); }

    // Close the shared memory File Descriptor
    if(verbose) { printf("Closing synchronization shared memory file descriptor.\n"); }
    if(close(synchronization_fd) < 0) {
        perror("close");
        exit(11);
    }
    if(verbose) { printf("Synchronization shared memory file descriptor closed.\n"); }

    // Unlink and remove the shared memory from the system
    if(verbose) { printf("Unlinking synchronization shared memory.\n"); }
    if(shm_unlink(HELPER_SERVICE_SHARED_MEMORY_NAME) < 0) {
        perror("shm_unlink");
        exit(12);
    }
    if(verbose) { printf("Synchronization shared memory unlinked.\n"); }
    
    if(verbose) { printf("Synchronization shared memory destroyed.\n"); }
}
