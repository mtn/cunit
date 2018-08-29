#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

#ifndef TESTVERBSOE
#define TESTVERBOSE false
#endif


void test_plus() {
    assert(1 + 1 == 2);
}

void test_minus() {
    if (1 - 1 != 2) {
        exit(1);
    }
}

// Run, expecting a exit failure
void run_return(void (*test_fn)(), int expected_return) {
    pid_t child_pid;
    Dl_info info;

    int filedes[2];
    if (pipe(filedes) == -1) {
        perror("Failed while setting up pipe");
        exit(1);
    }

    int result = dladdr((const void*)test_fn, &info);
    if (result == 0) {
        perror("Failed to find test function name, aborting!");
        exit(1);
    }

    if ((child_pid = fork()) == 0) {
        while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR));
        close(filedes[1]);
        close(filedes[0]);

        test_fn();
        exit(0);
    } else if (child_pid == -1) {
        perror("Failed while forking child to run test");
        exit(1);
    }
    close(filedes[1]);

    int status = 0;
    // Wait for the child process to terminate
    if (waitpid(child_pid, &status, 0) == -1) {
        perror("waitpid() failed");
        exit(EXIT_FAILURE);
    }
    int exit_status;
    if (WIFEXITED(status)) {
        exit_status = WEXITSTATUS(status);
    } else {
        exit_status = 1; // Declare failure if the process didn't exit normally
    }

    printf("Test %s %s!\n", info.dli_sname,
            exit_status == expected_return ? "succeded" : "failed");

    if (exit_status != expected_return || TESTVERBOSE) {
        printf("\tExpected return code %d, got %d\n", expected_return, exit_status);

        char output_buffer[4096];
        while (true) {
            ssize_t count = read(filedes[0], output_buffer, sizeof(output_buffer));
            if (count == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    perror("An error occured while reading from the failed child's stdout");
                    exit(1);
                }
            } else if (count == 0) {
                break;
            } else {
                printf("%s\n", output_buffer);
            }
        }
    }

    close(filedes[0]);
}

void run(void (*test_fn)()) {
    run_return(test_fn, 0);
}

int main () {
    puts("Running tests");

    run(&test_plus);
    run(&test_minus);
    run_return(&test_minus, 1);
}
