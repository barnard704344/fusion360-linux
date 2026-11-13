/* hostrunner <outfile> <donefile> <cmd> [args...]
 * Forks: child redirects stdout to outfile and execs cmd.
 * Parent waits for the child, then creates donefile and writes the exit
 * status into it. The marker is the signal the shim polls for.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
int main(int argc, char **argv)
{
    if (argc < 4) return 2;

    pid_t pid = fork();
    if (pid < 0) return 3;

    if (pid == 0) {
        int fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if (fd < 0) _exit(3);
        dup2(fd, 1);
        close(fd);
        /* Proton prepends its own libs to LD_LIBRARY_PATH; host Qt apps
         * (kdialog) segfault in QXkbCommon with them. Restore the host value. */
        const char *orig = getenv("ORIG_LD_LIBRARY_PATH");
        if (orig && *orig) setenv("LD_LIBRARY_PATH", orig, 1);
        else unsetenv("LD_LIBRARY_PATH");
        execvp(argv[3], &argv[3]);
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) { /* retry on EINTR */ }

    int df = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if (df >= 0) {
        char buf[32];
        int n = snprintf(buf, sizeof buf, "%d", WIFEXITED(status) ? WEXITSTATUS(status) : -1);
        if (write(df, buf, (size_t)n) < 0) { /* best effort */ }
        close(df);
    }
    return 0;
}
