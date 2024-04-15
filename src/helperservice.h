#include <semaphore.h>
#include <stdbool.h>

#define PS_NONE 0
#define PS_WAITING 1
#define PS_EATING 2

typedef struct {
    int id;
    int state;
} philosopher_state;

void init_helper_service(bool _verbose);

void execute_helper_service(bool _detect_deadlock, bool _detect_starvation);

void add_philosopher(int _id, int _state);

int get_helper_service_pid();

sem_t* get_synchronization_semaphore();

void destroy_helper_service_resources();