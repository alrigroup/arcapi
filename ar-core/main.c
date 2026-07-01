#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>
#include <stdarg.h>

// Color Definitions
#define RED    "\033[1;31m"
#define WHITE  "\033[1;37m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN   "\033[1;36m"
#define GRAY   "\033[1;30m"
#define PURPLE "\033[1;35m"
#define RESET  "\033[0m"

pid_t global_server_pid = -1;
volatile sig_atomic_t pending_action = 0; // Prevenção a Condição de Corrida (Async-Signal-Safe)

char APP_PATH[PATH_MAX];
const char* arb64 = "ICAvJCQkJCQkICAvJCQkJCQkJCAgICAgICAgIC8kJCQkJCQgICAvJCQkJCQkICAvJCQkJCQkJCAgLyQkJCQkJCQkCiAvJCRfXyAgJCR8ICQkX18gICQkICAgICAgIC8kJF9fICAkJCAvJCRfXyAgJCR8ICQkX18gICQkfCAkJF9fX19fLwp8ICQkICBcICQkfCAkJCAgXCAkJCAgICAgIHwgJCQgIFxfXy98ICQkICBcICQkfCAkJCAgXCAkJHwgJCQgICAgICAKfCAkJCQkJCQkJHwgJCQkJCQkJC8gICAgICB8ICQkICAgICAgfCAkJCAgfCAkJHwgJCQkJCQkJC98ICQkJCQkICAgCnwgJCRfXyAgJCR8ICQkX18gICQkICAgICAgfCAkJCAgICAgIHwgJCQgIHwgJCR8ICQkX18gICQkfCAkJF9fLyAgIAp8ICQkICB8ICQkfCAkJCAgXCAkJCAgICAgIHwgJCQgICAgJCR8ICQkICB8ICQkfCAkJCAgXCAkJHwgJCQgICAgICAKfCAkJCAgfCAkJHwgJCQgIHwgJCQgICAgICB8ICAkJCQkJCQvfCAgJCQkJCQkL3wgJCQgIHwgJCR8ICQkJCQkJCQkCnxfXy8gIHxfXy98X18vICB8X18vICAgICAgIFxfX19fX18vICBcX19fX19fLyB8X18vICB8X18vfF9fX19fX19fLwo=";
const char* arb64_tty = "ICAvJCQkJCQkICAvJCQkJCQkJCAgICAgICAgIC8kJCQkJCQgICAvJCQkJCQkICAvJCQkJCQkJCAgLyQkJCQkJCQkCiAvJCRfXyAgJCR8ICQkX18gICQkICAgICAgIC8kJF9fICAkJCAvJCRfXyAgJCR8ICQkX18gICQkfCAkJF9fX19fLwp8ICQkICBcICQkfCAkJCAgXCAkJCAgICAgIHwgJCQgIFxfXy98ICQkICBcICQkfCAkJCAgXCAkJHwgJCQgICAgICAKfCAkJCQkJCQkJHwgJCQkJCQkJC8gICAgICB8ICQkICAgICAgfCAkJCAgfCAkJHwgJCQkJCQkJC98ICQkJCQkICAgCnwgJCRfXyAgJCR8ICQkX18gICQkICAgICAgfCAkJCAgICAgIHwgJCQgIHwgJCR8ICQkX18gICQkfCAkJF9fLyAgIAp8ICQkICB8ICQkfCAkJCAgXCAkJCAgICAgIHwgJCQgICAgJCR8ICQkICB8ICQkfCAkJCAgXCAkJHwgJCQgICAgICAKfCAkJCAgfCAkJHwgJCQgIHwgJCQgICAgICB8ICAkJCQkJCQvfCAgJCQkJCQkL3wgJCQgIHwgJCR8ICQkJCQkJCQkCnxfXy8gIHxfXy98X18vICB8X18vICAgICAgIFxfX19fX18vICBcX19fX19fLyB8X18vICB8X18vfF9fX19fX19fLwo=";

void core_print(const char *format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("%s", buffer);
    fflush(stdout);
    FILE *tty = fopen("/dev/tty1", "w");
    if (tty) {
        fprintf(tty, "%s", buffer);
        fflush(tty);
        fclose(tty);
    }
}

void building_logo_animation() {
    FILE *fp_stdout;
    FILE *fp_tty = NULL;
    FILE *tty_out = fopen("/dev/tty1", "w");
    
    char cmd_stdout[2048];
    char cmd_tty[2048];
    
    sprintf(cmd_stdout, "echo \"%s\" | base64 -d", arb64);
    fp_stdout = popen(cmd_stdout, "r");
    if (!fp_stdout) {
        if (tty_out) fclose(tty_out);
        return;
    }
    
    if (tty_out) {
        sprintf(cmd_tty, "echo \"%s\" | base64 -d", arb64_tty);
        fp_tty = popen(cmd_tty, "r");
    }

    printf(RED);
    if (tty_out) {
        fprintf(tty_out, RED);
    }
    
    char line_stdout[1024];
    char line_tty[1024];
    int more_stdout = 1, more_tty = 1;
    
    while (more_stdout || more_tty) {
        if (more_stdout) {
            if (fgets(line_stdout, sizeof(line_stdout), fp_stdout) != NULL) {
                printf("%s", line_stdout);
                fflush(stdout);
            } else {
                more_stdout = 0;
            }
        }
        
        if (more_tty && tty_out && fp_tty) {
            if (fgets(line_tty, sizeof(line_tty), fp_tty) != NULL) {
                fprintf(tty_out, "%s", line_tty);
                fflush(tty_out);
            } else {
                more_tty = 0;
            }
        }
        
        if (more_stdout || more_tty) {
            usleep(30000); // Logo animation speed
        }
    }
    
    pclose(fp_stdout);
    if (fp_tty) pclose(fp_tty);
    
    printf(RESET "\n");
    if (tty_out) {
        fprintf(tty_out, RESET "\n");
        fclose(tty_out);
    }
}

void loading_animation(const char *msg) {
    core_print(WHITE " ⚙  " RESET "%-30s ", msg);
    for (int i = 1; i <= 10; i++) {
        core_print("\r" WHITE " ⚙  " RESET "%-30s " CYAN "[", msg);
        for (int j = 0; j < 10; j++) {
            if (j < i) core_print("■");
            else core_print(GRAY "□" CYAN);
        }
        core_print("]" RESET);
        usleep(80000); // 80ms step for smooth effect
    }
    core_print(GREEN " ✓ DONE\n" RESET);
}

void compile_and_update() {
    core_print("\n" PURPLE " ┌──────────────────────────────────────────────┐\n" RESET);
    core_print(PURPLE   " │ " YELLOW "ARC-BUILDER" WHITE " Initialization Sequence          " PURPLE "│\n" RESET);
    core_print(PURPLE   " └──────────────────────────────────────────────┘\n\n" RESET);
    
    // 1. We are already at root, no need to chdir
    loading_animation("Compiling Server Module");
    system("make -C ar-ws");

    loading_animation("Updating Core Orchestrator");
    system("gcc ar-core/main.c -o core -pthread -w");

    core_print(GRAY "\n ⮑  Configuring permissions...\n" RESET);
    system("chmod +x core arc_server");
    core_print(GREEN " ✨ All modules successfully updated!\n\n" RESET);
}

void cleanup(int sig) {
    core_print("\n\n" RED " ⚠ " WHITE "Interrupt received. Terminating AR-BEMF...\n" RESET);
    if (global_server_pid > 0) {
        kill(global_server_pid, SIGTERM);
    }
    system("pkill -f script.py > /dev/null 2>&1");
    system("pkill -f script.js > /dev/null 2>&1");
    core_print(GREEN " ✓ Services gracefully stopped. Goodbye!\n" RESET);
    exit(0);
}

// Handlers curtos e seguros: Apenas modificam a flag atômica
void handle_sigusr1(int sig) {
    pending_action = 1;
}

void handle_sigusr2(int sig) {
    pending_action = 2;
}

// Substitui o "system" perigoso por fork/exec controlado e com timeout (Fix 3.1)
int safe_compile(const char *cmd[], const char *work_dir) {
    pid_t pid = fork();
    if (pid == 0) {
        chdir(work_dir);
        execvp(cmd[0], (char * const*)cmd);
        exit(1);
    } else if (pid > 0) {
        int status;
        int timeout = 150; // Timeout máximo de 15 segundos para o GCC
        while (timeout > 0) {
            pid_t res = waitpid(pid, &status, WNOHANG);
            if (res == pid) return WEXITSTATUS(status);
            usleep(100000);
            timeout--;
        }
        kill(pid, SIGKILL); // Destrói compiler zumbi
        waitpid(pid, &status, 0);
        return -1;
    }
    return -1;
}

void program() {
    core_print("\033[H\033[J"); // Clears both stdout and TTY1 synchronously
    building_logo_animation();

    if (getcwd(APP_PATH, sizeof(APP_PATH)) != NULL) {
        core_print(CYAN " ⌂ Workspace: " GRAY "%s\n" RESET, APP_PATH);
    }

    // Execute compilation before starting services
    compile_and_update();

    core_print(GREEN " ╔══════════════════════════════════════════════╗\n" RESET);
    core_print(GREEN " ║  " WHITE "AR-WS Status: " GREEN "ONLINE                  ║\n" RESET);
    core_print(GREEN " ╚══════════════════════════════════════════════╝\n\n" RESET);
    
    char cmd[PATH_MAX + 50];

    // Start the new API server (HTTPS or HTTP based on OPERATION_MODE)
    core_print(CYAN " [⟳] Starting background services...\n" RESET);
    
    global_server_pid = fork();
    if (global_server_pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    } else if (global_server_pid == 0) {
        // Child process (arc_server)
        char *args[] = { "./arc_server", NULL };
        if (execv(args[0], args) == -1) {
            perror("Failed to launch arc_server");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process (core)
        // Loop seguro fora dos tratadores de sinal (Resolve CWE-479)
        while (1) {
            if (pending_action == 1) {
                pending_action = 0;
                core_print(YELLOW "\n ⟳ [CORE] HOT-RELOAD SIGNAL RECEIVED: API MODULE\n" RESET);
                const char *cmd[] = {"make", "-C", "ar-ws", NULL};
                int r = safe_compile(cmd, ".");
                if (r != 0) core_print(RED " ✖ COMPILATION FAILED! Check logs above.\n" RESET);
                
                if (global_server_pid > 0) { kill(global_server_pid, SIGTERM); waitpid(global_server_pid, NULL, 0); }
                global_server_pid = fork();
                if (global_server_pid == 0) {
                    char *args[] = { "./arc_server", NULL };
                    execv(args[0], args); exit(1);
                }
            } else if (pending_action == 2) {
                pending_action = 0;
                core_print(RED "\n 🔥 [CORE] FULL SYSTEM RESTART SIGNAL RECEIVED! 🔥\n" RESET);
                const char *cmd_srv[] = {"make", "-C", "ar-ws", NULL};
                safe_compile(cmd_srv, ".");
                const char *cmd_core[] = {"gcc", "ar-core/main.c", "-o", "core", "-pthread", "-w", NULL};
                int r = safe_compile(cmd_core, ".");
                if (r != 0) core_print(RED " ✖ CORE COMPILATION FAILED!\n" RESET);
                
                if (global_server_pid > 0) { kill(global_server_pid, SIGTERM); waitpid(global_server_pid, NULL, 0); }
                core_print(CYAN " [⟳] Re-executing Core...\n" RESET);
                char *args[] = { "./core", NULL };
                execv(args[0], args); exit(1);
            }
            usleep(100000); // Polling leve ao invés do bloqueante pause()
        }
    }
}

int main() {
    // Validate sudo at the start
    if (geteuid() != 0) {
        core_print(RED " ✖ [ERROR]" RESET WHITE " This program needs to be executed as superuser (root)." RESET "\n");
        exit(1);
    }

    signal(SIGINT, cleanup);
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGUSR2, handle_sigusr2);
    program();
    return 0;
}