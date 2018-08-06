#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

void test_plus() {
    assert(1 + 1 == 2);
}

void test_minus() {
    assert(1 - 1 == 2);
}

void run(void (*test_fn)()) {
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

    child_pid = fork();
    if (child_pid == 0) {
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
    waitpid(child_pid, &status, 0);

    printf("Test %s %s!\n", info.dli_sname,
            status == 0 ? "succeded" : "failed");

    // Dump output from stdout of failed processes
    if (status != 0) {
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
                printf("%s", output_buffer);
            }
        }
    }

    close(filedes[0]);
}

int main () {
    puts("Running tests");

    run(&test_plus);
    run(&test_minus);
}
