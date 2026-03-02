#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <thread>
#include <cstring>
#include <sys/sysinfo.h>
#include <sys/time.h>

#define PIPE_RD 0x0
#define PIPE_WR 0x1

int
get_num_cpus()
{
    int fds[2];
    static char *const argv[] = {"grep", "-c", "processor", "/proc/cpuinfo", nullptr};
    
    if (pipe(fds) < 0)
        err(EXIT_FAILURE, "pipe");
    if (fork() == 0) {
        close(fds[PIPE_RD]);
        dup2(fds[PIPE_WR], STDOUT_FILENO);
        if (execvp(argv[0], argv) < 0)
            err(EXIT_FAILURE, "execvp");
    }
    char buf[64];
    close(fds[PIPE_WR]);
    read(fds[PIPE_RD], buf, sizeof(buf));
    buf[strlen(buf)] = '\0';
    wait(NULL);
    close(fds[PIPE_RD]);
    return atoi(buf);
}

int
get_cpus_online()
{
    int fd;
    if ((fd = open("/sys/devices/system/cpu/online", O_RDONLY)) < 0)
        err(EXIT_FAILURE, "open");
    char buf[16];
    read(fd, buf, sizeof(buf) - 1);
    buf[strlen(buf)] = '\0';
    close(fd);
    return atoi(buf) + 1;
}

int
main(void)
{
    struct timeval t_start;
    struct timeval t_end;
    int elapsed;
    
    gettimeofday(&t_start, nullptr);
    printf("get_num_cpus: %d\n", get_num_cpus());
    gettimeofday(&t_end, nullptr);
    elapsed = ((t_end.tv_sec * 1000000) + t_end.tv_usec) -
                   ((t_start.tv_sec * 1000000) + t_start.tv_usec);
    printf("%dus\n", elapsed);

    gettimeofday(&t_start, nullptr);
    printf("std::thread::hardware_concurrency(): %d\n", 
           std::thread::hardware_concurrency());
    gettimeofday(&t_end, nullptr);
    elapsed = ((t_end.tv_sec * 1000000) + t_end.tv_usec) -
                   ((t_start.tv_sec * 1000000) + t_start.tv_usec);
    printf("%dus\n", elapsed);

    gettimeofday(&t_start, nullptr);
    printf("get_nprocs: %d\n", get_nprocs());
    gettimeofday(&t_end, nullptr);
    elapsed = ((t_end.tv_sec * 1000000) + t_end.tv_usec) -
                   ((t_start.tv_sec * 1000000) + t_start.tv_usec);
    printf("%dus\n", elapsed);

    gettimeofday(&t_start, nullptr);
    printf("get_cpus_online: %d\n", get_cpus_online());
    gettimeofday(&t_end, nullptr);
    elapsed = ((t_end.tv_sec * 1000000) + t_end.tv_usec) -
                   ((t_start.tv_sec * 1000000) + t_start.tv_usec);
    printf("%dus\n", elapsed);
    _exit(0);
}
