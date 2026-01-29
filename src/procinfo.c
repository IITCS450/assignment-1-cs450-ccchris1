#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void usage(const char *a) {
    fprintf(stderr,"Usage: %s <pid>\n",a); 
    exit(1);
}

static int isnum(const char*s) {
    for(;*s;s++) if(!isdigit(*s)) return 0; 
    return 1;
}
int main(int c,char**v){
 if(c!=2||!isnum(v[1])) usage(v[0]);
    char path[256];
    char buf[1024];
    FILE *f;

// --- 1. Get State, PPID, and CPU info from /proc/PID/stat ---
    snprintf(path, sizeof(path), "/proc/%s/stat", v[1]);
    f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) fprintf(stderr, "Error: PID %s not found.\n", v[1]);
        else perror("Error opening stat");
        return 1;
    }

    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return 1;
    }
    fclose(f);

    char *last_paren = strrchr(buf, ')');
    char state;
    int ppid, cpu_id;
    unsigned long utime, stime;

    sscanf(last_paren + 2, 
           "%c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %*u %*d %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*d %d", 
           &state, &ppid, &utime, &stime, &cpu_id);

    // Converting clock ticks to seconds
    long ticks = sysconf(_SC_CLK_TCK);
    double total_time = (double)(utime + stime) / ticks;

    // --- 2. Get VmRSS from /proc/PID/status ---
    long rss = 0;
    snprintf(path, sizeof(path), "/proc/%s/status", v[1]);
    f = fopen(path, "r");
    if (f) {
        while (fgets(buf, sizeof(buf), f)) {
            if (strncmp(buf, "VmRSS:", 6) == 0) {
                sscanf(buf, "VmRSS: %ld", &rss);
                break;
            }
        }
        fclose(f);
    }

    // --- 3. Get Command line from /proc/PID/cmdline ---
    char cmd[1024] = "";
    snprintf(path, sizeof(path), "/proc/%s/cmdline", v[1]);
    f = fopen(path, "r");
    if (f) {
        size_t n = fread(cmd, 1, sizeof(cmd) - 1, f);
        if (n > 0) {
            cmd[n] = '\0';
            for (size_t i = 0; i < n - 1; i++) {
                if (cmd[i] == '\0') cmd[i] = ' ';
            }
        } else {
            strncpy(cmd, last_paren - 10, 10); 
        }
        fclose(f);
    }

    printf("PID:%s\n", v[1]);
    printf("State:%c\n", state);
    printf("PPID:%d\n", ppid);
    printf("Cmd:%s\n", cmd);
    printf("CPU:%d %.3f\n", cpu_id, total_time);
    printf("VmRSS:%ld\n", rss);

    return 0;
}
