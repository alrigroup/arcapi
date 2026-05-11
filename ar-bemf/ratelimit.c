#include "ratelimit.h"
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_IPS 1000

typedef struct {
    char ip[64];
    int login_count;
    time_t login_window;
    int general_count;
    time_t general_window;
} RateLimitEntry;

static RateLimitEntry rl_table[MAX_IPS];
static pthread_mutex_t rl_mutex = PTHREAD_MUTEX_INITIALIZER;

void ratelimit_init() {
    pthread_mutex_lock(&rl_mutex);
    memset(rl_table, 0, sizeof(rl_table));
    pthread_mutex_unlock(&rl_mutex);
}

int ratelimit_check(const char *ip, const char *path) {
    if (!ip || !path) return 1;

    pthread_mutex_lock(&rl_mutex);
    time_t now = time(NULL);
    int is_login = (strncmp(path, "/manager/api/login", 18) == 0);
    int allowed = 1;
    
    int entry_idx = -1;
    int empty_idx = -1;
    int oldest_idx = 0;
    time_t oldest_time = now;

    for (int i = 0; i < MAX_IPS; i++) {
        if (rl_table[i].ip[0] == '\0') {
            if (empty_idx == -1) empty_idx = i;
        } else if (strcmp(rl_table[i].ip, ip) == 0) {
            entry_idx = i;
            break;
        } else {
            // LRU Tracker para substituição caso a tabela encha
            if (rl_table[i].general_window < oldest_time) {
                oldest_time = rl_table[i].general_window;
                oldest_idx = i;
            }
        }
    }

    if (entry_idx == -1) {
        entry_idx = (empty_idx != -1) ? empty_idx : oldest_idx;
        strncpy(rl_table[entry_idx].ip, ip, 63);
        rl_table[entry_idx].ip[63] = '\0';
        rl_table[entry_idx].login_count = 0;
        rl_table[entry_idx].login_window = now;
        rl_table[entry_idx].general_count = 0;
        rl_table[entry_idx].general_window = now;
    }

    if (is_login) {
        if (now - rl_table[entry_idx].login_window >= 60) {
            rl_table[entry_idx].login_window = now;
            rl_table[entry_idx].login_count = 1;
        } else if (++rl_table[entry_idx].login_count > 5) {
            allowed = 0;
        }
    } else {
        if (now - rl_table[entry_idx].general_window >= 60) {
            rl_table[entry_idx].general_window = now;
            rl_table[entry_idx].general_count = 1;
        } else if (++rl_table[entry_idx].general_count > 100) {
            allowed = 0;
        }
    }

    pthread_mutex_unlock(&rl_mutex);
    return allowed;
}