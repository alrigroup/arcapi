CC = gcc
CFLAGS = -Wall -g -pthread -w -I. -I$(SHARED_DIR) -I$(BEMF_DIR)
LIBS = -lssl -lcrypto -lpq

# Directories
BEMF_DIR = ../ar-bemf
CORE_DIR = core
ROUTES_DIR = routes
SHARED_DIR = ../shared

# Source Files
SRCS = main.c \
       endpoints.c \
       $(CORE_DIR)/database.c \
       $(CORE_DIR)/logs.c \
       $(CORE_DIR)/settings.c \
       $(CORE_DIR)/sync_engine.c \
       $(CORE_DIR)/tty.c \
       $(CORE_DIR)/update.c \
       $(CORE_DIR)/user_manager.c \
       $(CORE_DIR)/utils.c \
       $(ROUTES_DIR)/route_api_admin.c \
       $(ROUTES_DIR)/route_api_auth.c \
       $(ROUTES_DIR)/route_api_component.c \
       $(ROUTES_DIR)/route_api_monitoring.c \
       $(ROUTES_DIR)/route_api_system.c \
       $(ROUTES_DIR)/route_api_tty.c \
       $(ROUTES_DIR)/route_static.c \
       $(ROUTES_DIR)/route_home.c \
       $(ROUTES_DIR)/route_manager_dashboard.c \
       $(ROUTES_DIR)/route_manager_login.c \
       route_api.c \
       route_utils.c \
       $(BEMF_DIR)/server.c \
       $(BEMF_DIR)/router.c \
       $(BEMF_DIR)/ratelimit.c \
       $(SHARED_DIR)/cJSON.c

OBJS = $(SRCS:.c=.o)
TARGET = ../arc_server

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
