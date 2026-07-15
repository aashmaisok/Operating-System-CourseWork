#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT      5050
#define BUF_SIZE  1024

static int read_line(int sock, char *buf, size_t bufsize) {
    size_t total = 0;
    while (total < bufsize - 1) {
        char c;
        ssize_t n = recv(sock, &c, 1, 0);
        if (n < 0) return -1;
        if (n == 0) return (total == 0) ? 0 : (int)total;
        if (c == '\n') break;
        buf[total++] = c;
    }
    buf[total] = '\0';
    return (int)total;
}

static void send_line(int sock, const char *msg) {
    char out[BUF_SIZE];
    snprintf(out, sizeof(out), "%s\n", msg);
    send(sock, out, strlen(out), 0);
}

int main(int argc, char *argv[]) {
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); exit(1); }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server address: %s\n", server_ip);
        close(sock);
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }
    printf("Connected to %s:%d\n", server_ip, PORT);

    char line[BUF_SIZE];

    int n = read_line(sock, line, sizeof(line));
    if (n <= 0) { fprintf(stderr, "Server closed connection before auth.\n"); close(sock); exit(1); }
    printf("Server: %s\n", line);

    char username[64], password[64];
    printf("Username: ");
    if (!fgets(username, sizeof(username), stdin)) { close(sock); exit(1); }
    username[strcspn(username, "\n")] = 0;
    printf("Password: ");
    if (!fgets(password, sizeof(password), stdin)) { close(sock); exit(1); }
    password[strcspn(password, "\n")] = 0;

    char authmsg[200];
    snprintf(authmsg, sizeof(authmsg), "AUTH %s %s", username, password);
    send_line(sock, authmsg);

    n = read_line(sock, line, sizeof(line));
    if (n <= 0) { fprintf(stderr, "Connection lost during authentication.\n"); close(sock); exit(1); }
    printf("Server: %s\n", line);

    if (strncmp(line, "OK", 2) != 0) {
        printf("Authentication failed. Exiting.\n");
        close(sock);
        return 1;
    }

    printf("\nCommands: ECHO <text> | TIME | UPPER <text> | QUIT\n");
    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        send_line(sock, line);
        int is_quit = (strncmp(line, "QUIT", 4) == 0);

        n = read_line(sock, line, sizeof(line));
        if (n <= 0) { printf("Server closed the connection.\n"); break; }
        printf("Server: %s\n", line);

        if (is_quit) break;
    }

    close(sock);
    return 0;
}
