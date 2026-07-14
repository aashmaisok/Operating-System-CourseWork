/* ============================================================================
 * Secure File Management System
 * ST5004CEM - Operating Systems and Security
 * Task 3: File System Operations and Security
 *
 * Features implemented (per assignment brief):
 *   1. File creation, reading, writing, and deletion operations
 *   2. User authentication mechanism (register / login)
 *   3. File permission system (read, write, execute for owner/group/others)
 *   4. Encryption/decryption capability for sensitive files
 *   5. Audit logging to track file access and modifications
 *
 * All managed files live under ./vault/ so the demo never touches files
 * outside its own sandbox. Two metadata stores are used:
 *   users.dat       -> "username:group:password_hash"
 *   filesystem.meta -> "filename:owner:group:permissions:encrypted"
 * ==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#define VAULT_DIR      "vault"
#define USERS_FILE     "users.dat"
#define META_FILE      "filesystem.meta"
#define AUDIT_FILE     "audit.log"
#define MAX_LINE       512
#define MAX_NAME       64

/* ---------- data structures ---------- */

typedef struct {
    char username[MAX_NAME];
    char group[MAX_NAME];
} Session;

typedef struct {
    char filename[MAX_NAME];
    char owner[MAX_NAME];
    char group[MAX_NAME];
    char perm[10];      /* e.g. "rwxr-x---" : owner/group/other */
    int  encrypted;      /* 0 or 1 */
} FileMeta;

static Session current = { "", "" };

/* ---------- security note ----------
 * Password hashing: this project uses a simple FNV-1a hash purely to
 * avoid storing plaintext passwords on disk. It is NOT a cryptographically
 * secure hash (no salting, fast to brute force) and would not be suitable
 * for a production system - see SECURITY_ANALYSIS.md for discussion.
 * ------------------------------------ */
static unsigned long hash_password(const char *pw) {
    unsigned long hash = 2166136261UL;
    for (const char *p = pw; *p; p++) {
        hash ^= (unsigned char)(*p);
        hash *= 16777619UL;
    }
    return hash;
}

/* ---------- audit logging ---------- */
static void log_action(const char *username, const char *action,
                        const char *target, const char *result) {
    FILE *f = fopen(AUDIT_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(f, "[%s] user=%-10s action=%-10s target=%-20s result=%s\n",
            ts, username[0] ? username : "-", action, target, result);
    fclose(f);
}

/* ---------- user management ---------- */

static int user_exists(const char *username) {
    FILE *f = fopen(USERS_FILE, "r");
    if (!f) return 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char name[MAX_NAME];
        sscanf(line, "%63[^:]", name);
        if (strcmp(name, username) == 0) { fclose(f); return 1; }
    }
    fclose(f);
    return 0;
}

static void register_user(void) {
    char username[MAX_NAME], group[MAX_NAME], password[MAX_NAME];
    printf("New username: ");
    scanf("%63s", username);
    if (user_exists(username)) {
        printf("That username already exists.\n");
        log_action(username, "REGISTER", "-", "FAIL:duplicate");
        return;
    }
    printf("Group name (e.g. staff, students): ");
    scanf("%63s", group);
    printf("Password: ");
    scanf("%63s", password);

    FILE *f = fopen(USERS_FILE, "a");
    if (!f) { printf("Could not access user store.\n"); return; }
    fprintf(f, "%s:%s:%lu\n", username, group, hash_password(password));
    fclose(f);
    printf("User '%s' registered.\n", username);
    log_action(username, "REGISTER", "-", "SUCCESS");
}

static int login_user(void) {
    char username[MAX_NAME], password[MAX_NAME];
    printf("Username: ");
    scanf("%63s", username);
    printf("Password: ");
    scanf("%63s", password);

    FILE *f = fopen(USERS_FILE, "r");
    if (!f) { printf("No registered users yet.\n"); return 0; }

    char line[MAX_LINE];
    unsigned long given = hash_password(password);
    while (fgets(line, sizeof(line), f)) {
        char name[MAX_NAME], group[MAX_NAME];
        unsigned long stored_hash;
        if (sscanf(line, "%63[^:]:%63[^:]:%lu", name, group, &stored_hash) == 3) {
            if (strcmp(name, username) == 0 && stored_hash == given) {
                fclose(f);
                strncpy(current.username, username, MAX_NAME);
                strncpy(current.group, group, MAX_NAME);
                printf("Login successful. Welcome, %s.\n", username);
                log_action(username, "LOGIN", "-", "SUCCESS");
                return 1;
            }
        }
    }
    fclose(f);
    printf("Invalid username or password.\n");
    log_action(username, "LOGIN", "-", "FAIL:bad_credentials");
    return 0;
}

/* ---------- file metadata helpers ---------- */

static int find_meta(const char *filename, FileMeta *out) {
    FILE *f = fopen(META_FILE, "r");
    if (!f) return 0;
    char line[MAX_LINE];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        FileMeta m;
        int enc;
        if (sscanf(line, "%63[^:]:%63[^:]:%63[^:]:%9[^:]:%d",
                   m.filename, m.owner, m.group, m.perm, &enc) == 5) {
            m.encrypted = enc;
            if (strcmp(m.filename, filename) == 0) { *out = m; found = 1; break; }
        }
    }
    fclose(f);
    return found;
}

static void rewrite_meta_without(const char *filename) {
    FILE *in = fopen(META_FILE, "r");
    if (!in) return;
    FILE *out = fopen("filesystem.meta.tmp", "w");
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), in)) {
        char name[MAX_NAME];
        sscanf(line, "%63[^:]", name);
        if (strcmp(name, filename) != 0) fputs(line, out);
    }
    fclose(in); fclose(out);
    remove(META_FILE);
    rename("filesystem.meta.tmp", META_FILE);
}

static void append_meta(FileMeta *m) {
    FILE *f = fopen(META_FILE, "a");
    if (!f) return;
    fprintf(f, "%s:%s:%s:%s:%d\n", m->filename, m->owner, m->group,
            m->perm, m->encrypted);
    fclose(f);
}

/* Check whether current user has the requested permission ('r','w','x')
 * on a file, using the classic owner/group/other model. */
static int has_permission(FileMeta *m, char action) {
    int offset;
    if (strcmp(current.username, m->owner) == 0)      offset = 0;      /* owner bits */
    else if (strcmp(current.group, m->group) == 0)    offset = 3;      /* group bits */
    else                                               offset = 6;      /* other bits */

    char required;
    if (action == 'r') required = 'r';
    else if (action == 'w') required = 'w';
    else required = 'x';

    return m->perm[offset + (required == 'r' ? 0 : required == 'w' ? 1 : 2)] == required;
}

/* ---------- simple XOR cipher for the encryption requirement ----------
 * NOTE: XOR-with-key is used here to satisfy the brief's requirement for
 * an "encryption/decryption capability" while keeping the coursework
 * scope reasonable. It is reversible with the same key but is NOT
 * cryptographically strong (see SECURITY_ANALYSIS.md).
 * ------------------------------------------------------------------- */
static void xor_cipher(char *data, size_t len, const char *key) {
    size_t klen = strlen(key);
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key[i % klen];
    }
}

/* ---------- core file operations ---------- */

static void ensure_vault(void) {
    mkdir(VAULT_DIR, 0700);
}

static void create_file_op(void) {
    char filename[MAX_NAME], perm[10] = "rw-r-----";
    char content[4096], key[MAX_NAME] = "";
    int encrypt_choice;

    printf("New file name: ");
    scanf("%63s", filename);

    FileMeta existing;
    if (find_meta(filename, &existing)) {
        printf("A file with that name already exists.\n");
        log_action(current.username, "CREATE", filename, "FAIL:exists");
        return;
    }

    printf("Encrypt this file? (1=yes, 0=no): ");
    scanf("%d", &encrypt_choice);
    if (encrypt_choice) {
        printf("Encryption key: ");
        scanf("%63s", key);
    }

    printf("Enter file content (single line): ");
    getchar();
    fgets(content, sizeof(content), stdin);
    content[strcspn(content, "\n")] = 0;

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", VAULT_DIR, filename);
    FILE *f = fopen(path, "wb");
    if (!f) { printf("Could not create file.\n"); return; }

    size_t len = strlen(content);
    if (encrypt_choice) xor_cipher(content, len, key);
    fwrite(content, 1, len, f);
    fclose(f);

    FileMeta m;
    strncpy(m.filename, filename, MAX_NAME);
    strncpy(m.owner, current.username, MAX_NAME);
    strncpy(m.group, current.group, MAX_NAME);
    strncpy(m.perm, perm, sizeof(m.perm));
    m.encrypted = encrypt_choice;
    append_meta(&m);

    printf("File '%s' created%s.\n", filename, encrypt_choice ? " (encrypted)" : "");
    log_action(current.username, "CREATE", filename, "SUCCESS");
}

static void read_file_op(void) {
    char filename[MAX_NAME];
    printf("File name to read: ");
    scanf("%63s", filename);

    FileMeta m;
    if (!find_meta(filename, &m)) {
        printf("File not found.\n");
        log_action(current.username, "READ", filename, "FAIL:not_found");
        return;
    }
    if (!has_permission(&m, 'r')) {
        printf("Permission denied: you do not have read access.\n");
        log_action(current.username, "READ", filename, "FAIL:denied");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", VAULT_DIR, filename);
    FILE *f = fopen(path, "rb");
    if (!f) { printf("Could not open file.\n"); return; }

    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = 0;
    fclose(f);

    if (m.encrypted) {
        char key[MAX_NAME];
        printf("This file is encrypted. Enter key: ");
        scanf("%63s", key);
        xor_cipher(buf, n, key);
    }

    printf("--- Content of '%s' ---\n%s\n------------------------\n", filename, buf);
    log_action(current.username, "READ", filename, "SUCCESS");
}

static void write_file_op(void) {
    char filename[MAX_NAME], content[4096];
    printf("File name to write/overwrite: ");
    scanf("%63s", filename);

    FileMeta m;
    if (!find_meta(filename, &m)) {
        printf("File not found.\n");
        log_action(current.username, "WRITE", filename, "FAIL:not_found");
        return;
    }
    if (!has_permission(&m, 'w')) {
        printf("Permission denied: you do not have write access.\n");
        log_action(current.username, "WRITE", filename, "FAIL:denied");
        return;
    }

    printf("New content (single line): ");
    getchar();
    fgets(content, sizeof(content), stdin);
    content[strcspn(content, "\n")] = 0;

    char key[MAX_NAME] = "";
    size_t len = strlen(content);
    if (m.encrypted) {
        printf("This file is encrypted. Enter key: ");
        scanf("%63s", key);
        xor_cipher(content, len, key);
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", VAULT_DIR, filename);
    FILE *f = fopen(path, "wb");
    if (!f) { printf("Could not write file.\n"); return; }
    fwrite(content, 1, len, f);
    fclose(f);

    printf("File '%s' updated.\n", filename);
    log_action(current.username, "WRITE", filename, "SUCCESS");
}

static void delete_file_op(void) {
    char filename[MAX_NAME];
    printf("File name to delete: ");
    scanf("%63s", filename);

    FileMeta m;
    if (!find_meta(filename, &m)) {
        printf("File not found.\n");
        log_action(current.username, "DELETE", filename, "FAIL:not_found");
        return;
    }
    /* Only the owner may delete, regardless of write bit - stricter rule
     * documented in the security analysis. */
    if (strcmp(current.username, m.owner) != 0) {
        printf("Permission denied: only the owner may delete this file.\n");
        log_action(current.username, "DELETE", filename, "FAIL:denied");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", VAULT_DIR, filename);
    remove(path);
    rewrite_meta_without(filename);

    printf("File '%s' deleted.\n", filename);
    log_action(current.username, "DELETE", filename, "SUCCESS");
}

static void chmod_op(void) {
    char filename[MAX_NAME], newperm[10];
    printf("File name to change permissions: ");
    scanf("%63s", filename);

    FileMeta m;
    if (!find_meta(filename, &m)) {
        printf("File not found.\n");
        return;
    }
    if (strcmp(current.username, m.owner) != 0) {
        printf("Permission denied: only the owner may change permissions.\n");
        log_action(current.username, "CHMOD", filename, "FAIL:denied");
        return;
    }

    printf("Current permissions: %s\n", m.perm);
    printf("Enter new 9-char permission string (e.g. rw-r-----): ");
    scanf("%9s", newperm);
    if (strlen(newperm) != 9) {
        printf("Invalid format.\n");
        return;
    }

    rewrite_meta_without(filename);
    strncpy(m.perm, newperm, sizeof(m.perm));
    append_meta(&m);

    printf("Permissions updated.\n");
    log_action(current.username, "CHMOD", filename, "SUCCESS");
}

static void list_files_op(void) {
    FILE *f = fopen(META_FILE, "r");
    if (!f) { printf("No files yet.\n"); return; }
    char line[MAX_LINE];
    printf("%-20s %-12s %-10s %-10s %s\n", "FILENAME", "OWNER", "GROUP", "PERM", "ENCRYPTED");
    while (fgets(line, sizeof(line), f)) {
        FileMeta m; int enc;
        if (sscanf(line, "%63[^:]:%63[^:]:%63[^:]:%9[^:]:%d",
                   m.filename, m.owner, m.group, m.perm, &enc) == 5) {
            printf("%-20s %-12s %-10s %-10s %s\n", m.filename, m.owner, m.group,
                   m.perm, enc ? "yes" : "no");
        }
    }
    fclose(f);
}

static void view_audit_log(void) {
    FILE *f = fopen(AUDIT_FILE, "r");
    if (!f) { printf("No audit entries yet.\n"); return; }
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) printf("%s", line);
    fclose(f);
}

/* ---------- menus ---------- */

static void logged_in_menu(void) {
    int choice;
    do {
        printf("\n=== Secure File Management System (user: %s) ===\n", current.username);
        printf("1. Create file\n2. Read file\n3. Write/overwrite file\n");
        printf("4. Delete file\n5. Change file permissions\n6. List files\n");
        printf("7. View audit log\n8. Logout\n> ");
        if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); choice = -1; }

        switch (choice) {
            case 1: create_file_op(); break;
            case 2: read_file_op(); break;
            case 3: write_file_op(); break;
            case 4: delete_file_op(); break;
            case 5: chmod_op(); break;
            case 6: list_files_op(); break;
            case 7: view_audit_log(); break;
            case 8:
                log_action(current.username, "LOGOUT", "-", "SUCCESS");
                current.username[0] = 0;
                printf("Logged out.\n");
                break;
            default: printf("Invalid option.\n");
        }
    } while (choice != 8);
}

int main(void) {
    ensure_vault();
    int choice;
    printf("Secure File Management System - ST5004CEM Task 3\n");
    do {
        printf("\n1. Register\n2. Login\n3. Exit\n> ");
        if (scanf("%d", &choice) != 1) { while (getchar() != '\n'); choice = -1; }

        switch (choice) {
            case 1: register_user(); break;
            case 2: if (login_user()) logged_in_menu(); break;
            case 3: printf("Goodbye.\n"); break;
            default: printf("Invalid option.\n");
        }
    } while (choice != 3);

    return 0;
}
