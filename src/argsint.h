#include <stdbool.h>

/**
 * The "count" argument is not greater than zero.
*/
#define ARGS_INT_ERR_COUNT 11

struct application_options {
    int count;
    bool detect_starvation;
    bool detect_deadlock;
    bool resolve_deadlock;
};

/**
 * Interprets the given arguments.
 * 
 * Interprets the given arguments and checks if they are correct.
 * 
 * This interpreter only checks for the next arguments:
 * --help / -h 
 * --count [int]
 * --detect-starvation
 * --detect-deadlock
 * --resolve-deadlock
 * 
 * If the help command is called then the function will print the help informations and shuts down the application.
 * If the function encounters an error while reading the args, it will print the error informations and shuts down the application.
 * 
 * @param argc The number of elements in the argv array.
 * @param argv The given arguments,
 * @param opt The compiled application options.
*/
void int_args(int argc, char *argv[], struct application_options* opt);