#include <errno.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <syslog.h>
#include <sys/wait.h>
#include <unistd.h>

#include "systemcalls.h"

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    // command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    pid_t pid = fork();
    if (pid == -1) {
        syslog(LOG_ERR, "fork() failed");
        return false;
    }
    if (pid == 0) {
        execv(command[0], command);
        syslog(LOG_ERR, "execv() failed");
        exit(-1);
    }

    int status = 0;
    printf("hello\n");
    wait(&status);
    printf("%d %d\n", WIFEXITED(status), WEXITSTATUS(status));
    if (WIFEXITED(status) && WEXITSTATUS(status) == 255) {
        printf("In here\n");
        return false;
    }

    va_end(args);

    return true;
}

int main(){
    printf("Final result: %d\n", do_exec(3, "echo", "hello", "$PATH"));
}