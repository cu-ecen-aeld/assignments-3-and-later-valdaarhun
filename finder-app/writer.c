#include <stdio.h>
#include <sys/syslog.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        syslog(LOG_ERR, "Only two arguments should be passed.");
        return 1;
    }

    char *filename = argv[1];
    FILE *f = fopen(filename, "w");
    if (!f) {
        syslog(LOG_ERR, "Failed to create %s", filename);
        return 1;
    }

    char *string = argv[2];
    syslog(LOG_DEBUG, "Writing %s to %s", string, filename);
    if (fputs(string, f) < 0) {
        syslog(LOG_ERR, "Failed to create %s", filename);
        return 1;  
    }
}