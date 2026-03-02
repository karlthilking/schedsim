#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

static int recvsig = 0;
void sighandler(int sig)
{
	recvsig = 1;
	return;
}

int
main(void)
{
	pid_t cpid;
	if ((cpid = fork()) < 0) 
		err(EXIT_FAILURE, "fork");
	else if (cpid == 0) {
		int fd;
		fd = open("tmp.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
		if (fd < 0)
			err(EXIT_FAILURE, "open");
		for (;;) {
			int errsv;
			write(fd, (void *)&errno, sizeof(errno));
			read(fd, (void *)&errno, sizeof(errno));
			if (recvsig)
				break;
		}
		close(fd);
		unlink("tmp.txt");
		_exit(0);
	} else {
		char buf[64];
		snprintf(buf, sizeof(buf), "/proc/%d/task/%d/status", cpid, cpid);
		for (int i = 0; i < 10; ++i) {
			FILE *f;
			if ((f = fopen(buf, "r")) < 0)
				err(EXIT_FAILURE, "fopen");
			char *line = NULL;
			size_t sz = 0;
			char state = 'I';
			while (getline(&line, &sz, f) > 0) {
				if (!strncmp(line, "State:", strlen("State:"))) {
					sscanf(line, "%*s %c\n", &state);
					break;
				}
			}
			switch (state) {
			case 'R':
				printf("%d: running (%c)\n", cpid, state);
				break;
			case 'S':
				printf("%d: sleeping (%c)\n", cpid, state);
				break;
			case 'T':
				printf("%d: stopped (%c)\n", cpid, state);
				break;
			default:
				break;
			}
			free(line);
			fclose(f);
			usleep(2500);
		}
		int wstat;
		kill(cpid, SIGKILL);
		waitpid(cpid, &wstat, 0);
		if (WIFEXITED(wstat))
			printf("%d: exited (%d)\n", cpid, WEXITSTATUS(wstat));
		else if (WIFSTOPPED(wstat))
			printf("%d: stopped (%d)\n", cpid, WSTOPSIG(wstat));
		else if (WIFSIGNALED(wstat))
			printf("%d: signaled (%d)\n", cpid, WTERMSIG(wstat));
		_exit(0);
	}
}
