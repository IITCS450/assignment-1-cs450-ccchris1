#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *a){fprintf(stderr,"Usage: %s <cmd> [args]\n",a); exit(1);}
static double d(struct timespec a, struct timespec b){
 return (b.tv_sec-a.tv_sec)+(b.tv_nsec-a.tv_nsec)/1e9;}
int main(int c,char**v){
    if (c < 2) usage(v[0]);
    struct timespec start, end;
    int status;

    // 2. Start the wall-clock timer
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        return 1;
    }

    // 3. Fork the process
    pid_t pid = fork();

    if (pid < 0) {
        // Fork failed
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child Process
        execvp(v[1], &v[1]);
        perror("execvp");
        exit(1);
    } else {
        // Parent Process
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        // 4. Stop the timer immediately after the child finishes
        clock_gettime(CLOCK_MONOTONIC, &end);

        // 5. Exit code
        int exit_val = 0;
        if (WIFEXITED(status)) {
            exit_val = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            exit_val = WTERMSIG(status);
        }

        // 6. Print the results in the required format
        printf("pid=%d elapsed=%.3f exit=%d\n", pid, d(start, end), exit_val);
    }


 return 0;
}
