#include <argsint.h>

#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct argp_option options[] = {
    {"count", 'c', "int", 0, "Sets the number of philosophers that must be greater than one."},
    {"detect-starvation", 1, 0, OPTION_ARG_OPTIONAL, "If set, the application will detect the starvation and print it out."},
    {"detect-deadlock", 2, 0, OPTION_ARG_OPTIONAL, "If set, the application will detect the deadlock and print it out."},
    {"resolve-deadlock", 3, 0, OPTION_ARG_OPTIONAL, "If set, the application will resolve the deadlock if it is detected."},
    {0}
};

static error_t parse_opt(int key, char* arg, struct argp_state *state) {
    struct application_options *options = state->input;

    switch(key) {
        case 'c':
            int count = atoi(arg);
            if(count <= 1) { return ARGS_INT_ERR_COUNT; }

            options->count = count;
            break;
        case 1:
            options->detect_starvation = true;
            break;
        case 2:
            options->detect_deadlock = true;
            break;
        case 3:
            options->resolve_deadlock = true;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, NULL, NULL};

void int_args(int argc, char *argv[], struct application_options* opt) {
    opt->count = 0;
    opt->detect_starvation = false;
    opt->detect_deadlock = false;
    opt->resolve_deadlock = false;

    error_t parse_result = argp_parse(&argp, argc, argv, 0, 0, opt);
    if(parse_result != 0) {
        switch(parse_result) {
            case ARGP_ERR_UNKNOWN:
                break;
            case ARGS_INT_ERR_COUNT:
                printf("%s: option requires an integer argument greater than one -- 'c'.\n", argv[0]);
                printf("Try '--help' or '--usage' for more information.\n");
                exit(222);
            default:
                printf("An unknown error has ocurred while parsing the arguments.\n");
                exit(111);
        }
    }
}