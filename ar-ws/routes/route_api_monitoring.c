#include "../routes.h"
#include "../core/logs.h"
#include "../core/user_manager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>



void api_metrics_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); return; }

    cJSON *resp = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(resp, "metrics");
    
    pthread_mutex_lock(&stats_mutex);
    for(int i=0; i < route_stat_count; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "path", route_stats[i].path);
        cJSON_AddNumberToObject(item, "count", route_stats[i].count);
        cJSON_AddItemToArray(arr, item);
    }
    pthread_mutex_unlock(&stats_mutex);
    
    server_send_json(conn, 200, resp);
}

void api_ips_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); return; }

    cJSON *resp = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(resp, "ips");
    
    pthread_mutex_lock(&access_mutex);
    int start = access_count < MAX_ACCESS_LOGS ? 0 : access_head;
    for(int i=0; i < access_count; i++) {
        int idx = (start + i) % MAX_ACCESS_LOGS;
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "ip", access_logs[idx].ip);
        cJSON_AddNumberToObject(item, "timestamp", (double)access_logs[idx].ts);
        cJSON_AddStringToObject(item, "path", access_logs[idx].path);
        cJSON_AddNumberToObject(item, "status", access_logs[idx].status);
        cJSON_AddStringToObject(item, "anon_id", access_logs[idx].anon_id);
        cJSON_AddItemToArray(arr, item);
    }
    pthread_mutex_unlock(&access_mutex);
    
    server_send_json(conn, 200, resp);
}

void api_logs_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 1) { server_send_response(conn, 403, "application/json", "{\"error\": \"Forbidden.\"}"); return; }

    cJSON *resp = cJSON_CreateObject();
    cJSON *arr = cJSON_AddArrayToObject(resp, "logs");
    
    pthread_mutex_lock(&log_mutex);
    int start = log_count < MAX_LOG_ENTRIES ? 0 : log_head;
    for(int i=0; i < log_count; i++) {
        int idx = (start + i) % MAX_LOG_ENTRIES;
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "timestamp", (double)server_logs[idx].ts);
        cJSON_AddStringToObject(item, "level", server_logs[idx].level);
        cJSON_AddStringToObject(item, "message", server_logs[idx].message);
        cJSON_AddItemToArray(arr, item);
    }
    pthread_mutex_unlock(&log_mutex);
    
    server_send_json(conn, 200, resp);
}



static unsigned long read_val(const char *buf, const char *key) {
    const char *p = strstr(buf, key);
    if (!p) return 0;
    p += strlen(key);
    while (*p && (*p == ' ' || *p == '\t')) p++;
    return strtoul(p, NULL, 10);
}

void api_system_info_handler(ClientConnection *conn, HttpRequest *req) {
    AuthResult auth = authenticate_request(req, conn);
    if (auth.valid != 1) { send_auth_error(conn, &auth); return; }
    if (auth.role > 0) { server_send_response(conn, 403, "application/json", "{\"error\": \"ROOT only.\"}"); return; }

    cJSON *resp = cJSON_CreateObject();
    char errbuf[512] = {0};
    size_t errpos = 0;
    double upt = 0;

#define LOG_ERR(fmt, ...) do { \
    int n = snprintf(errbuf + errpos, sizeof(errbuf) - errpos, fmt, ##__VA_ARGS__); \
    if (n > 0) errpos += (size_t)n; \
    if (errpos > sizeof(errbuf)) errpos = sizeof(errbuf); \
} while(0)

    {
        char buf[256] = {0};
        if (gethostname(buf, sizeof(buf)) == 0) cJSON_AddStringToObject(resp, "hostname", buf);
        else { cJSON_AddStringToObject(resp, "hostname", "unknown"); LOG_ERR("gethostname:%s ", strerror(errno)); }
    }

    {
        struct utsname u;
        if (uname(&u) == 0) {
            char os[128];
            snprintf(os, sizeof(os), "%s %s", u.sysname, u.release);
            cJSON_AddStringToObject(resp, "os", os);
        } else {
            cJSON_AddStringToObject(resp, "os", "unknown");
            LOG_ERR("uname:%s ", strerror(errno));
        }
    }

    {
        FILE *fu = fopen("/proc/uptime", "r");
        if (fu) {
            if (fscanf(fu, "%lf", &upt) != 1) upt = 0;
            fclose(fu);
        } else LOG_ERR("uptime:%s ", strerror(errno));
        cJSON_AddNumberToObject(resp, "uptime", upt);
    }

    {
        cJSON *cpu = cJSON_CreateObject();
        FILE *f = fopen("/proc/stat", "r");
        if (f) {
            unsigned long long u = 0, n = 0, s = 0, i = 0;
            if (fscanf(f, "cpu %llu %llu %llu %llu", &u, &n, &s, &i) >= 4) {
                unsigned long long total = u + n + s + i;
                unsigned long long idle = i;
                int cores = sysconf(_SC_NPROCESSORS_ONLN);
                cJSON_AddNumberToObject(cpu, "cores", cores > 0 ? cores : 1);

                static unsigned long long prev_total = 0, prev_idle = 0;
                if (prev_total > 0 && prev_idle > 0) {
                    unsigned long long dtotal = total - prev_total;
                    unsigned long long didle = idle - prev_idle;
                    double pct = dtotal > 0 ? 100.0 * (double)(dtotal - didle) / (double)dtotal : 0;
                    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
                    cJSON_AddNumberToObject(cpu, "usage_percent", pct);
                } else {
                    cJSON_AddNumberToObject(cpu, "usage_percent", 0);
                }
                prev_total = total;
                prev_idle = idle;

                double l1 = 0, l5 = 0, l15 = 0;
                fclose(f);
                f = fopen("/proc/loadavg", "r");
                if (f) {
                    if (fscanf(f, "%lf %lf %lf", &l1, &l5, &l15) >= 1) {
                        cJSON_AddNumberToObject(cpu, "load_1m", l1);
                        cJSON_AddNumberToObject(cpu, "load_5m", l5);
                        cJSON_AddNumberToObject(cpu, "load_15m", l15);
                    }
                    fclose(f);
                } else LOG_ERR("loadavg:%s ", strerror(errno));
            } else {
                LOG_ERR("stat_parse_fail ");
                fclose(f);
            }
        } else LOG_ERR("stat:%s ", strerror(errno));
        cJSON_AddItemToObject(resp, "cpu", cpu);
    }

    {
        cJSON *mem = cJSON_CreateObject();
        FILE *f = fopen("/proc/meminfo", "r");
        if (f) {
            char line[256];
            unsigned long mt = 0, ma = 0, st = 0, sf = 0;
            while (fgets(line, sizeof(line), f)) {
                if (!mt) mt = read_val(line, "MemTotal:");
                if (!ma) ma = read_val(line, "MemAvailable:");
                if (!st) st = read_val(line, "SwapTotal:");
                if (!sf) sf = read_val(line, "SwapFree:");
            }
            fclose(f);
            mt /= 1024; ma /= 1024; st /= 1024; sf /= 1024;
            unsigned long mu = mt > ma ? mt - ma : 0;
            unsigned long su = st > sf ? st - sf : 0;
            int mp = mt > 0 ? (int)((mu * 100) / mt) : 0;
            int sp = st > 0 ? (int)((su * 100) / st) : 0;
            cJSON_AddNumberToObject(mem, "total_mb", mt);
            cJSON_AddNumberToObject(mem, "used_mb", mu);
            cJSON_AddNumberToObject(mem, "available_mb", ma);
            cJSON_AddNumberToObject(mem, "percent", mp);
            cJSON *swap = cJSON_CreateObject();
            cJSON_AddNumberToObject(swap, "total_mb", st);
            cJSON_AddNumberToObject(swap, "used_mb", su);
            cJSON_AddNumberToObject(swap, "percent", sp);
            cJSON_AddItemToObject(mem, "swap", swap);
        } else LOG_ERR("meminfo:%s ", strerror(errno));
        cJSON_AddItemToObject(resp, "memory", mem);
    }

    {
        cJSON *disk = cJSON_CreateArray();
        struct statvfs vfs;
        if (statvfs("/", &vfs) == 0) {
            unsigned long total = (unsigned long)(vfs.f_frsize * vfs.f_blocks) / (1024ULL * 1024 * 1024);
            unsigned long free_ = (unsigned long)(vfs.f_frsize * vfs.f_bfree) / (1024ULL * 1024 * 1024);
            unsigned long used = total > free_ ? total - free_ : 0;
            int pct = total > 0 ? (int)((used * 100) / total) : 0;
            cJSON *item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "mount", "/");
            cJSON_AddNumberToObject(item, "total_gb", total);
            cJSON_AddNumberToObject(item, "used_gb", used);
            cJSON_AddNumberToObject(item, "free_gb", free_);
            cJSON_AddNumberToObject(item, "percent", pct);
            cJSON_AddItemToArray(disk, item);
        } else LOG_ERR("statvfs:%s ", strerror(errno));
        cJSON_AddItemToObject(resp, "disk", disk);
    }

    {
        cJSON *net = cJSON_CreateArray();
        FILE *f = fopen("/proc/net/dev", "r");
        if (f) {
            char line[512];
            if (fgets(line, sizeof(line), f) && fgets(line, sizeof(line), f)) {
                while (fgets(line, sizeof(line), f)) {
                    char iface[64] = {0};
                    unsigned long long rx = 0, tx = 0;
                    if (sscanf(line, "%63s %llu %*u %*u %*u %*u %*u %*u %*u %llu", iface, &rx, &tx) >= 2) {
                        char *colon = strchr(iface, ':');
                        if (colon) *colon = '\0';
                        if (iface[0] && strcmp(iface, "lo") != 0) {
                            cJSON *item = cJSON_CreateObject();
                            cJSON_AddStringToObject(item, "name", iface);
                            cJSON_AddNumberToObject(item, "rx_bytes", rx);
                            cJSON_AddNumberToObject(item, "tx_bytes", tx);
                            cJSON_AddItemToArray(net, item);
                        }
                    }
                }
            }
            fclose(f);
        } else LOG_ERR("net_dev:%s ", strerror(errno));
        cJSON_AddItemToObject(resp, "network", net);
    }

    {
        cJSON *procs = cJSON_CreateArray();
        DIR *dp = opendir("/proc");
        if (dp) {
            struct dirent *entry;
            int count = 0;
            int scanned = 0;
            while ((entry = readdir(dp)) != NULL && scanned < 500 && count < 50) {
                if (entry->d_type != DT_DIR) continue;
                int is_num = 1;
                for (const char *p = entry->d_name; *p; p++) { if (!isdigit(*p)) { is_num = 0; break; } }
                if (!is_num) continue;
                long pid = atol(entry->d_name);
                if (pid == getpid() || pid == 1) continue;
                scanned++;

                char spath[64];
                snprintf(spath, sizeof(spath), "/proc/%s/status", entry->d_name);
                FILE *sf = fopen(spath, "r");
                if (!sf) continue;
                char pname[64] = {0};
                char state[8] = {0};
                unsigned long rss = 0;
                unsigned int uid = 0;
                char ln[256];
                while (fgets(ln, sizeof(ln), sf)) {
                    if (!pname[0]) sscanf(ln, "Name:\t%63s", pname);
                    if (!state[0]) sscanf(ln, "State:\t%7s", state);
                    if (uid == 0) sscanf(ln, "Uid:\t%u", &uid);
                    unsigned long v = 0;
                    if (sscanf(ln, "VmRSS:\t%lu kB", &v) == 1) rss = v;
                }
                fclose(sf);
                if (!pname[0]) continue;
                // skip kernel threads (no memory)
                if (rss == 0) continue;

                // CPU from /proc/pid/stat
                double cpu_pct = 0;
                char statpath[64];
                snprintf(statpath, sizeof(statpath), "/proc/%s/stat", entry->d_name);
                FILE *sf2 = fopen(statpath, "r");
                if (sf2) {
                    if (fgets(ln, sizeof(ln), sf2)) {
                        char *cp = strrchr(ln, ')');
                        if (cp) {
                            unsigned long utime = 0, stime = 0;
                            if (sscanf(cp + 1, " %*c %*d %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %lu %lu", &utime, &stime) >= 2) {
                                double ticks = (double)(utime + stime) / (double)sysconf(_SC_CLK_TCK);
                                cpu_pct = upt > 0 ? (ticks / upt * 100.0) : 0;
                                if (cpu_pct > 1000) cpu_pct = 1000;
                            }
                        }
                    }
                    fclose(sf2);
                }

                // exe path (try exe symlink, fallback cmdline)
                char expath[256] = {0};
                char exelink[64];
                snprintf(exelink, sizeof(exelink), "/proc/%s/exe", entry->d_name);
                ssize_t exlen = readlink(exelink, expath, sizeof(expath) - 1);
                if (exlen > 0) { expath[exlen] = '\0'; }
                else {
                    // fallback: first token of /proc/pid/cmdline
                    snprintf(exelink, sizeof(exelink), "/proc/%s/cmdline", entry->d_name);
                    FILE *fcmd = fopen(exelink, "r");
                    if (fcmd) {
                        if (fgets(expath, sizeof(expath) - 1, fcmd)) {
                            size_t el = strlen(expath);
                            if (el > 0 && expath[el-1] == '\n') expath[el-1] = '\0';
                        }
                        fclose(fcmd);
                    }
                }

                // owner
                char owner[32] = "?";
                struct passwd *pw = getpwuid(uid);
                if (pw) { strncpy(owner, pw->pw_name, sizeof(owner) - 1); }
                else { snprintf(owner, sizeof(owner), "%u", uid); }

                cJSON *item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "name", pname);
                cJSON_AddNumberToObject(item, "pid", pid);
                cJSON_AddStringToObject(item, "state", state);
                cJSON_AddNumberToObject(item, "memory_kb", rss);
                cJSON_AddNumberToObject(item, "cpu_percent", cpu_pct);
                cJSON_AddStringToObject(item, "path", expath[0] ? expath : "?");
                cJSON_AddStringToObject(item, "owner", owner);
                cJSON_AddItemToArray(procs, item);
                count++;
            }
            closedir(dp);
        } else LOG_ERR("proc_dir:%s ", strerror(errno));
        cJSON_AddItemToObject(resp, "processes", procs);
    }

    if (errpos > 0) {
        cJSON_AddStringToObject(resp, "_debug", errbuf);
    }

    server_send_json(conn, 200, resp);
}

#undef LOG_ERR
