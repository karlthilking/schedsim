#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>
#include <err.h>

int
main(void)
{
    int policies[3] = {SCHED_FIFO, SCHED_RR, SCHED_OTHER};
    char *names[3] = {"SCHED_FIFO", "SCHED_RR", "SCHED_OTHER"};

    for (int i = 0; i < 3; ++i) {
        printf("Policy: %s\n", names[i]);
        printf("Max Scheduling Priority: %d\n", sched_get_priority_max(policies[i]));
        printf("Min Scheduling Priority: %d\n", sched_get_priority_min(policies[i]));
    }
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NICE, &rlim) < 0)
        perror("getrlimit");
    printf("nice limit: %ld\n", 20 - rlim.rlim_cur);

    rlim.rlim_cur = 40;
    if (setrlimit(RLIMIT_NICE, &rlim) < 0)
        perror("setrlimit");

    if (nice(-20) < 0)
        perror("nice(-20)");

    _exit(0);
}
