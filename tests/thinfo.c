#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>

int flag = 0x0;

void *
start(void *)
{
    while (!flag);
    return NULL;
}

int
main(void)
{
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i)
        pthread_create(threads + i, NULL, start, NULL);
    
    FILE *f;
    int fd;
    char *line = NULL;
    size_t sz = 0;

    char buf[64];
    snprintf(buf, sizeof(buf), "/proc/%d/task/%d/status", getpid(), getpid());

    if ((f = fopen(buf, "r")) == NULL)
        err(EXIT_FAILURE, "fopen");
    if ((fd = open("tmp.txt", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) < 0)
        err(EXIT_FAILURE, "open");
    
    while (getline(&line, &sz, f) > 0)
        write(fd, line, strlen(line));

    flag = 0x1;
    for (int i = 0; i < 4; ++i)
        pthread_join(threads[i], NULL);

    free(line);
    fclose(f);
    close(fd);
    _exit(0);
}
