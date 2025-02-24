#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

volatile sig_atomic_t quit;

void handle_term_int(int signo) {
    syslog(LOG_INFO, "Caught signal, exiting\n");
    quit = 1;
}

static ssize_t recv_packet(int fd) {
    char buf[512];
    int flag = 1;
    ssize_t size;

    memset(buf, 0, sizeof(buf));
    FILE *f = fopen("/var/tmp/aesdsocketdata", "a");

    while(flag) {
        errno = 0;
        size = recv(fd, buf, sizeof(buf), 0);
        if (size < 0) {
            if (errno == EINTR) {
                continue;
            }
            fclose(f);
            return -1;
        }

        if (size == 0) {
            break;
        }

        for(int i = 0; i < size; i++) {
            fputc(buf[i], f);
            if (buf[i] == '\n') {
                flag = 0;
                break;
            }
        }
    }

    fclose(f);
    return size;
}

static ssize_t send_packet(int fd) {
    FILE *f = fopen("/var/tmp/aesdsocketdata", "r");
    char buf[512];
    memset(buf, 0, sizeof(buf));

    while(fgets(buf, sizeof(buf), f) != 0) {
        int size = strlen(buf);
        char *ptr = buf;

        while(size > 0) {
            errno = 0;
            int ret = send(fd, ptr, size, 0);
            if (ret < 0) {
                if (errno == EINTR) {
                    continue;
                }
                fclose(f);
                return -1;
            }
            ptr += ret;
            size -= ret;
        }
    }

    fclose(f);
    return 0;
}

int main(int argc, char* argv[]) {
    bool is_daemon = false;
    int opt;
    while((opt = getopt(argc, argv, "d")) != -1) {
        if (opt == 'd') {
            is_daemon = true;
            break;
        }
    }
    sigset_t set, old_set;
    if (sigfillset(&set) < 0) {
        return -1;
    }

    if (sigdelset(&set, SIGINT | SIGTERM) < 0) {
        return -1;
    }

    // if (sigprocmask(SIG_BLOCK, &set, &old_set) < 0) { 
    //     return -1;
    // }

    if (sigprocmask(SIG_SETMASK, NULL, &old_set) < 0) { 
        return -1;
    }

    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = handle_term_int;
    act.sa_mask = set;

    if (sigaction(SIGTERM, &act, NULL) < 0) {
        return -1;
    }

    if (sigaction(SIGINT, &act, NULL) < 0) {
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(9000),
        .sin_addr = {
            .s_addr = htonl(INADDR_ANY),
        }
    };

    int enable_port_reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
               &enable_port_reuse, sizeof(enable_port_reuse)) < 0)
    {
        return -1;
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        return -1;
    }

    if (listen(fd, 5)) {
        return -1;
    }

    if (is_daemon) {
        if (daemon(1, 0) < 0) {
            return -1;
        }
    }

    struct sockaddr_in peer;
    memset(&peer, 0, sizeof(struct sockaddr_in));
    socklen_t addrlen = sizeof(struct sockaddr_in);

    while(!quit) {
        int peerfd;
        if ((peerfd = accept(fd, (struct sockaddr *)&peer, &addrlen)) < 0) {
            if (quit) {
                break;
            }
            return -1;
        }
        
        char dst[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, (void *)&peer.sin_addr, dst, INET_ADDRSTRLEN) == NULL) {
            return -1;
        }

        // printf("Accepted connection from %s\n", dst);
        syslog(LOG_INFO, "Accepted connection from %s\n", dst);

        while(1) {
            int ret = recv_packet(peerfd);
            if (ret < 0) {
                return -1;
            }
            if (ret == 0) {
                break;
            }
            ret = send_packet(peerfd);
            if (ret < 0) {
                return -1;
            }
            if (quit) {
                break;
            }
        }

        // printf("Closed connection from %s\n", dst);
        syslog(LOG_INFO, "Closed connection from %s\n", dst);
        close(peerfd);
    }

    close(fd);
    unlink("/var/tmp/aesdsocketdata");
    if (sigprocmask(SIG_SETMASK, &old_set, NULL) < 0) { 
        return -1;
    }

    return 0;
}
