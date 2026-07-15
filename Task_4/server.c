/* ============================================================================
 * Client-Server IPC Demo — SERVER
 * ST5004CEM - Operating Systems and Security
 * Task 4: Network Programming and IPC
 *
 * Features implemented (per assignment brief):
 *   1. TCP socket-based server
 *   2. Simple text line protocol (see PROTOCOL.md)
 *   3. Multiple concurrent clients via fork() per connection
 *   4. Basic security: username/password authentication + input validation
 *   5. Error handling and clean connection teardown
 * ==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT        5050
#define BACKLOG     10
#define BUF_SIZE    1024
#define MAX_LINE    1024

/* ---------- hardcoded credential store ----------
 * For a production system this would live in a proper user database with
 * salted/hashed passwords (see Task 3). Kept simple here so Task 4 can
 * focus on the networking/IPC requirements. */
typedef struct { const char *user; const char *pass; } Credential;
static const Credential CREDENTIALS[] = {
    { "alice", "pass123" },
    { "bob",   "hunter2" }
};
#define NUM_CREDS (int)(sizeof(CREDENTIALS) / sizeof(CREDENTIALS[0]))

static int authenticate(const char *user, const char *pass) {
    for (int i = 0; i < NUM_CREDS; i++) {
        if (strcmp(CREDENTIALS[i].user, user) == 0 &&
            strcmp(CREDENTIALS[i].pass, pass) == 0) {
            return 1;
        }
    }
    return 0;
}

static void log_line(const char *fmt, ...) {
    char ts[32];
    time_t now = time(NULL);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("[%s] ", ts);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

/* Reads a single '\n'-terminated line from the socket into buf.
 * Returns bytes read, 0 on clean disconnect, -1 on error. */
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

/* Basic input validation: printable ASCII only, bounded length. Rejects
 * anything that looks like it is trying to break the line protocol. */
static int is_valid_input(const char *s) {
    size_t len = strlen(s);
    if (len == 0 || len >= MAX_LINE) return 0;
    for (size_t i = 0; i < len; i++) {
        if (s[i] < 32 && s[i] != '\t') return 0; /* reject control chars */
    }
    return 1;
}

static void to_upper_str(char *s) {
    for (; *s; s++) *s = (char)toupper((unsigned char)*s);
}

static void handle_client(int client_fd, struct sockaddr_in *addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, client_ip, sizeof(client_ip));
    log_line("Connection from %s:%d (pid %d)", client_ip, ntohs(addr->sin_port), getpid());

    char line[MAX_LINE];
    int n;

    /* ---- authentication phase ---- */
    send_line(client_fd, "AUTH_REQUIRED");
    n = read_line(client_fd, line, sizeof(line));
    if (n <= 0) { log_line("Client %s disconnected before authenticating", client_ip); close(client_fd); return; }

    char cmd[32], user[64], pass[64];
    int authenticated = 0;
    if (sscanf(line, "%31s %63s %63s", cmd, user, pass) == 3 && strcmp(cmd, "AUTH") == 0) {
        if (authenticate(user, pass)) {
            authenticated = 1;
            send_line(client_fd, "OK AUTH_SUCCESS");
            log_line("User '%s' authenticated from %s", user, client_ip);
        } else {
            send_line(client_fd, "ERR AUTH_FAILED");
            log_line("Failed auth attempt for user '%s' from %s", user, client_ip);
        }
    } else {
        send_line(client_fd, "ERR BAD_PROTOCOL");
        log_line("Malformed auth line from %s", client_ip);
    }

    if (!authenticated) { close(client_fd); return; }

    /* ---- command phase ---- */
    while ((n = read_line(client_fd, line, sizeof(line))) > 0) {
        if (!is_valid_input(line)) {
            send_line(client_fd, "ERR INVALID_INPUT");
            log_line("Rejected invalid input from %s (user %s)", client_ip, user);
            continue;
        }

        char verb[32], arg[MAX_LINE];
        arg[0] = '\0';
        int matched = sscanf(line, "%31s %[^\n]", verb, arg);

        if (matched >= 1 && strcmp(verb, "QUIT") == 0) {
            send_line(client_fd, "OK BYE");
            log_line("User '%s' quit from %s", user, client_ip);
            break;
        } else if (matched >= 1 && strcmp(verb, "TIME") == 0) {
            char ts[64];
            time_t now = time(NULL);
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
            char resp[128];
            snprintf(resp, sizeof(resp), "OK %s", ts);
            send_line(client_fd, resp);
        } else if (matched == 2 && strcmp(verb, "ECHO") == 0) {
            char resp[MAX_LINE + 8];
            snprintf(resp, sizeof(resp), "OK %s", arg);
            send_line(client_fd, resp);
        } else if (matched == 2 && strcmp(verb, "UPPER") == 0) {
            to_upper_str(arg);
            char resp[MAX_LINE + 8];
            snprintf(resp, sizeof(resp), "OK %s", arg);
            send_line(client_fd, resp);
        } else {
            send_line(client_fd, "ERR UNKNOWN_COMMAND");
        }
    }

    if (n < 0) log_line("Connection error with %s", client_ip);
    else if (n == 0) log_line("Client %s (user %s) disconnected", client_ip, user);

    close(client_fd);
}

/* Reap finished child processes so they don't become zombies. */
static void reap_children(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0) { }
}

int main(void) {
    signal(SIGCHLD, reap_children);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); close(server_fd); exit(1);
    }
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen"); close(server_fd); exit(1);
    }

    log_line("Server listening on port %d (pid %d)", PORT, getpid());

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            if (errno == EINTR) continue; /* interrupted by SIGCHLD, retry */
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
        } else if (pid == 0) {
            /* child: handles exactly one client, then exits */
            close(server_fd);
            handle_client(client_fd, &client_addr);
            close(client_fd);
            exit(0);
        } else {
            /* parent: keep accepting new connections */
            close(client_fd);
        }
    }

    close(server_fd);
    return 0;
}
