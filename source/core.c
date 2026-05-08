#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>

// Definições de Cores
#define RED    "\033[1;31m"
#define WHITE  "\033[1;37m"
#define GREEN  "\033[1;32m"
#define YELLOW "\033[1;33m"
#define CYAN   "\033[1;36m"
#define GRAY   "\033[1;30m"
#define PURPLE "\033[1;35m"
#define RESET  "\033[0m"

char APP_PATH[PATH_MAX];
const char* arb64 = "ICAvJCQkJCQkICAvJCQgICAgICAgLyQkJCQkJCQgIC8kJCQkJCQgICAgICAgIC8kJCQkJCQgICAvJCQkJCQkICAvJCQkJCQkJCAgLyQkJCQkJCQkCiAvJCRfXyAgJCR8ICQkICAgICAgfCAkJF9fICAkJHxfICAkJF8vICAgICAgIC8kJF9fICAkJCAvJCRfXyAgJCR8ICQkX18gICQkfCAkJF9fX19fLwp8ICQkICBcICQkfCAkJCAgICAgIHwgJCQgIFwgJCQgIHwgJCQgICAgICAgIHwgJCQgIFxfXy98ICQkICBcICQkfCAkJCAgXCAkJHwgJCQgICAgICAKfCAkJCQkJCQkJHwgJCQgICAgICB8ICQkJCQkJCQvICB8ICQkICAgICAgICB8ICQkICAgICAgfCAkJCAgfCAkJHwgJCQkJCQkJC98ICQkJCQkICAgCnwgJCRfXyAgJCR8ICQkICAgICAgfCAkJF9fICAkJCAgfCAkJCAgICAgICAgfCAkJCAgICAgIHwgJCQgIHwgJCR8ICQkX18gICQkfCAkJF9fLyAgIAp8ICQkICB8ICQkfCAkJCAgICAgIHwgJCQgIFwgJCQgIHwgJCQgICAgICAgIHwgJCQgICAgJCR8ICQkICB8ICQkfCAkJCAgXCAkJHwgJCQgICAgICAKfCAkJCAgfCAkJHwgJCQkJCQkJCR8ICQkICB8ICQkIC8kJCQkJCQgICAgICB8ICAkJCQkJCQvfCAgJCQkJCQkL3wgJCQgIHwgJCR8ICQkJCQkJCQkCnxfXy8gIHxfXy98X19fX19fX18vfF9fLyAgfF9fL3xfX19fX18vICAgICAgIFxfX19fX18vICBcX19fX19fLyB8X18vICB8X18vfF9fX19fX19fLwo=";

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
        usleep(30000); // Velocidade da logo
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
    printf(YELLOW "\n[ALRI-BUILDER] Iniciando rotina de compilação...\n" RESET);
    
    // 1. Entrar na pasta source
    if (chdir("source") != 0) {
        fprintf(stderr, RED "[ERROR] Pasta 'source' não encontrada para compilação!\n" RESET);
        return;
    }

    loading_animation("Compilando alri_server   ");
    system("gcc server.c api.c -o alri_server -lssl -lcrypto -pthread -w");

    loading_animation("Atualizando ALRI-CORE    ");
    system("gcc core.c -o core -pthread -w");

    printf(YELLOW "[ALRI-BUILDER] Movendo binários e aplicando permissões...\n" RESET);
    system("mv core ../core && mv alri_server ../alri_server");
    
    // Voltar para a raiz e dar permissão
    chdir("..");
    system("chmod +x core alri_server");

    printf(GREEN "[SUCCESS] Todos os módulos foram atualizados com sucesso!\n\n" RESET);
}

void cleanup(int sig) {
    printf("\n" GREEN "[ALRI-CORE]" RESET WHITE " Cleaning up all ALRI services..." RESET "\n");
    system("sudo pkill -f alri_server");
    system("pkill -f script.py");
    system("pkill -f script.js");
    printf(GREEN "[ALRI-CORE]" RESET WHITE " Services terminated. Goodbye!" RESET "\n");
    exit(0);
}

void program() {
    system("clear");
    building_logo_animation();

    if (getcwd(APP_PATH, sizeof(APP_PATH)) != NULL) {
        printf(GRAY "[PATH] %s" RESET "\n", APP_PATH);
    }

    // Executa a compilação antes de iniciar os serviços
    compile_and_update();

    printf(GREEN "[ALRI-CORE]" RESET WHITE " Status: " RESET GREEN "ONLINE" RESET "\n");
    printf(WHITE "--------------------------------------------------" RESET "\n");
    
    char cmd[PATH_MAX + 50];

    // Iniciar o novo servidor API (HTTPS ou HTTP conforme OPERATION_MODE)
    printf(GREEN "[ALRI-CORE]" GREEN "[+]" RESET WHITE " Starting new API server..." RESET "\n");
    
    pid_t server_pid = fork();
    if (server_pid == -1) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    } else if (server_pid == 0) {
        // Processo filho (alri_server)
        char *args[] = { "./alri_server", NULL };
        if (execv(args[0], args) == -1) {
            perror("Failed to launch alri_server");
            exit(EXIT_FAILURE);
        }
    } else {
        // Processo pai (core)
        // Permanece em execução para capturar sinais (como SIGINT via CTRL+C) e invocar cleanup
        while (1) {
            pause();
        }
    }
}

int main() {
    // Valida o sudo no início
    if (system("sudo -v") != 0) {
        fprintf(stderr, RED "[ERROR]" RESET WHITE " This program needs to be executed as superuser (sudo)." RESET "\n");
        exit(1);
    }

    signal(SIGINT, cleanup);
    program();
    return 0;
}