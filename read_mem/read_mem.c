#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <errno.h>

unsigned char buf[1024];

int main(int argc, unsigned char **argv)
{
	pid_t pid;
	unsigned long addr1, addr2;
	int buf;

	if (argc < 4) {
		fprintf(stderr, "Not enough args\n");
		return (1);
	}
	pid = strtol(argv[1], NULL, 0);
	addr1 = (unsigned long)strtoll(argv[2], NULL, 0);
	addr2 = (unsigned long)strtoll(argv[3], NULL, 0);
	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL)) {
		perror("PTRACE_ATTACH");
		return (1);
	}
	for (; addr1 < addr2; addr1 += sizeof(int)) {
		errno = 0;
		if (((buf =
		      ptrace(PTRACE_PEEKDATA, pid, (void *)addr1, NULL)) == -1)
		    && errno) {
			perror("PTRACE_PEEKDATA");
			if (ptrace(PTRACE_DETACH, pid, NULL, NULL))
				perror("PTRACE_DETACH");
			return (1);
		}
		if (write(1, &buf, sizeof(buf)) != 4) {
			perror("write");
			if (ptrace(PTRACE_DETACH, pid, NULL, NULL))
				perror("PTRACE_DETACH");
			return (1);
		}
	}
	if (ptrace(PTRACE_DETACH, pid, NULL, NULL)) {
		perror("PTRACE_DETACH");
		return (1);
	}
	return (0);
}
