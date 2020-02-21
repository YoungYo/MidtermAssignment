//
// Created by supermouse on 2020/2/20.
//

#include "common.h"

static int count;

static void sig_int(int signo) {
    printf("\nreceived %d datagrams\n", count);
    exit(0);
}

void error(int status, int err, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (err)
        fprintf(stderr, ": %s (%d)\n", strerror(err), err);
    if (status)
        exit(status);
}

int main(int argc, char **argv) {
    int listenfd;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERV_PORT);

    int rt1 = bind(listenfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (rt1 < 0) {
        error(1, errno, "bind failed ");
    }

    int rt2 = listen(listenfd, LISTENQ);
    if (rt2 < 0) {
        error(1, errno, "listen failed ");
    }

    signal(SIGINT, sig_int);
    signal(SIGPIPE, SIG_DFL);

    FILE *fstream = NULL;
    while (1) {
        int connfd;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        if ((connfd = accept(listenfd, (struct sockaddr *) &client_addr, &client_len)) < 0) {
            error(1, errno, "bind failed ");
        }

        char message[MAXLINE];
        count = 0;

        for (;;) {
            int n = read(connfd, message, MAXLINE);
            if (n < 0) {
                error(1, errno, "error read");
            } else if (n == 0) {
                printf("Client closed\n");
                close(connfd);
                break;
            }
            message[n] = 0;
            printf("Received command: %s", message); //打印接收到的命令
            count++;

            if (strncmp(message, "cd", 2) == 0){
                message[n - 1] = 0;
//                printf("%s, %ld\n", message + 3, strlen(message + 3));
                chdir(message+3);
                printf("Current working directory: %s\n", getcwd(NULL, NULL));
            } else{
                if(NULL == (fstream = popen(message,"r"))) {
                    fprintf(stderr,"execute command failed: %s",strerror(errno));
                    return -1;
                }

                char send_line[MAXLINE];
                while(NULL != fgets(send_line + strlen(send_line), sizeof(send_line), fstream));
                printf("%s",send_line); //打印命令执行结果
                int write_nc = send(connfd, send_line, strlen(send_line), 0);
                if (write_nc < 0) {
                    error(1, errno, "error write");
                }
                send_line[0] = 0;
                pclose(fstream);
            }
        }
    }
}


