#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

// Color Definitions
#define RED    "\033[1;31m"
#define WHITE  "\033[1;37m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN   "\033[1;36m"
#define GRAY   "\033[1;30m"
#define PURPLE "\033[1;35m"
#define RESET  "\033[0m"

char APP_PATH[PATH_MAX];
const char* arb64 = "ICQkJCQkJFwgICQkJCQkJCRcICAgJCQkJCQkXCAgICQkJCQkJFwgICQkJCQkJCRcICQkJCQkJFwgCiQkICBfXyQkXCAkJCAgX18kJFwgJCQgIF9fJCRcICQkICBfXyQkXCAkJCAgX18kJFxcXyQkICBffAokJCAvICAkJCB8JCQgfCAgJCQgfCQkIC8gIFxfX3wkJCAvICAkJCB8JCQgfCAgJCQgfCAkJCB8ICAKJCQkJCQkJCQgfCQkJCQkJCQgIHwkJCB8ICAgICAgJCQkJCQkJCQgfCQkJCQkJCQgIHwgJCQgfCAgCiQkICBfXyQkIHwkJCAgX18kJDwgJCQgfCAgICAgICQkICBfXyQkIHwkJCAgX19fXy8gICQkIHwgIAokJCB8ICAkJCB8JCQgfCAgJCQgfCQkIHwgICQkXCAkJCB8ICAkJCB8JCQgfCAgICAgICAkJCB8ICAKJCQgfCAgJCQgfCQkIHwgICQkIHxcJCQkJCQkICB8JCQgfCAgJCQgfCQkIHwgICAgICQkJCQkJFwgClxfX3wgIFxfX3xcX198ICBcX198IFxfX19fX18vIFxfX3wgIFxfX3xcX198ICAgICBcX19fX19ffAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIA==";

void building_logo_animation() {
    FILE *fp;
    char command[2048];
    char line[1024];
    sprintf(command, "echo \"%s\" | base64 -d", arb64);
    fp = popen(command, "r");
    if (!fp) return;
    printf(RED);
    while (fgets(line, sizeof(line), fp) != NULL) {
        printf("%s", line);
        fflush(stdout);
        usleep(30000); // Logo animation speed
    }
    pclose(fp);
    printf(RESET "\n");
}

void loading_animation(const char *msg) {
    const char *frames[] = {"[ ■□□□□□□□□□ ]", "[ ■■□□□□□□□□ ]", "[ ■■■■□□□□□□ ]", "[ ■■■■■■□□□□ ]", "[ ■■■■■■■■□□ ]", "[ ■■■■■■■■■■ ]"};
    printf(WHITE "%s ", msg);
    for (int i = 0; i < 6; i++) {
        printf(CYAN "\r%s %s" RESET, msg, frames[i]);
        fflush(stdout);
        usleep(150000);
    }
    printf(GREEN " OK!" RESET "\n");
}

void compile_and_update() {
    printf(YELLOW "\n[ARC-BUILDER] Starting compilation routine...\n" RESET);
    
    // 1. Enter the source folder
    if (chdir("source") != 0) {
        fprintf(stderr, RED "[ERROR] 'source' folder not found for compilation!\n" RESET);
        return;
    }

    loading_animation("Compiling arc_server     ");
    system("gcc server.c api.c -o arc_server -lssl -lcrypto -pthread -w");

    loading_animation("Updating ARCAPI          ");
    system("gcc core.c -o core -pthread -w");

    printf(YELLOW "[ARC-BUILDER] Moving binaries and applying permissions...\n" RESET);
    system("mv core ../core && mv arc_server ../arc_server");
    
    // Return to root and grant permissions
    chdir("..");
    system("chmod +x core arc_server");

    printf(GREEN "[SUCCESS] All modules updated successfully!\n\n" RESET);
}

void cleanup(int sig) {
    printf("\n" GREEN "[ARCAPI]" RESET WHITE " Cleaning up all ARCAPI services..." RESET "\n");
    system("sudo pkill -f arc_server");
    system("pkill -f script.py");
    system("pkill -f script.js");
    printf(GREEN "[ARCAPI]" RESET WHITE " Services terminated. Goodbye!" RESET "\n");
    exit(0);
}

void program() {
    system("clear");
    building_logo_animation();

    if (getcwd(APP_PATH, sizeof(APP_PATH)) != NULL) {
        printf(GRAY "[PATH] %s" RESET "\n", APP_PATH);
    }

    // Execute compilation before starting services
    compile_and_update();

    printf(GREEN "[ARCAPI]" RESET WHITE " Status: " RESET GREEN "ONLINE" RESET "\n");
    printf(WHITE "--------------------------------------------------" RESET "\n");
    
    char cmd[PATH_MAX + 50];

    // Start the new API server (HTTPS or HTTP based on OPERATION_MODE)
    printf(GREEN "[ARCAPI]" GREEN "[+]" RESET WHITE " Starting new API server..." RESET "\n");
    
    pid_t server_pid = fork();
    if (server_pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    } else if (server_pid == 0) {
        // Child process (arc_server)
        char *args[] = { "./arc_server", NULL };
        if (execv(args[0], args) == -1) {
            perror("Failed to launch arc_server");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process (core)
        // Stays running to catch signals (like SIGINT via CTRL+C) and trigger cleanup
        while (1) {
            pause();
        }
    }
}

int main() {
    // Validate sudo at the start
    if (system("sudo -v") != 0) {
        fprintf(stderr, RED "[ERROR]" RESET WHITE " This program needs to be executed as superuser (sudo)." RESET "\n");
        exit(1);
    }

    signal(SIGINT, cleanup);
    program();
    return 0;
}