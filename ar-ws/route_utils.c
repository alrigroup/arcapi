#include "routes.h"
#include <unistd.h>
#include <stdio.h>

void sendpage(ClientConnection *conn, const char *folder_name) {
    char full_path[512];
    
    // Try index.html directly in folder
    snprintf(full_path, sizeof(full_path), "ar-ws/web/%s/index.html", folder_name);
    if (access(full_path, F_OK) != 0) {
        // If not found, try main.html
        snprintf(full_path, sizeof(full_path), "ar-ws/web/%s/main.html", folder_name);
        if (access(full_path, F_OK) != 0) {
            // Fallback for SPA projects like Vite that use dist/ folder
            snprintf(full_path, sizeof(full_path), "ar-ws/web/%s/dist/index.html", folder_name);
        }
    }
    
    server_serve_file(conn, full_path, "text/html");
}
