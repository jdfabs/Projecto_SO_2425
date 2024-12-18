# Compiler and flags
CC = gcc -pthread
CFLAGS = -g -I./include #debug flag + auto includes
LDFLAGS = -lrt

# Directories
SRC_DIR = ./src
CLIENT_DIR = $(SRC_DIR)/client
SERVER_DIR = $(SRC_DIR)/server
SHARED_DIR = $(SRC_DIR)/common
EXTERNAL_SRC = $(SRC_DIR)/.external

# Executables
CLIENT_EXEC = client
SERVER_EXEC = server

# Source files
CLIENT_SOURCES = $(CLIENT_DIR)/client.c $(SHARED_DIR)/file_handler.c $(EXTERNAL_SRC)/cJSON.c $(SHARED_DIR)/config_loader.c $(SHARED_DIR)/json_utils.c  $(SERVER_DIR)/sudoku_utils.c
SERVER_SOURCES = $(SERVER_DIR)/server.c $(SERVER_DIR)/sudoku_utils.c $(SHARED_DIR)/file_handler.c $(EXTERNAL_SRC)/cJSON.c $(SHARED_DIR)/json_utils.c $(SHARED_DIR)/config_loader.c
# Object files
CLIENT_OBJS = $(CLIENT_SOURCES:.c=.o)  #files .o
SERVER_OBJS = $(SERVER_SOURCES:.c=.o)

# Targets
all: $(CLIENT_EXEC) $(SERVER_EXEC)

# Client executable
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Server executable
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile client source files to object files
$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile server source files to object files
$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile shared source files to object files
$(SHARED_DIR)/%.o: $(SHARED_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executables
clean:
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS) $(CLIENT_EXEC) $(SERVER_EXEC)

# Phony targets to avoid conflicts with files named clean or all
.PHONY: all clean
