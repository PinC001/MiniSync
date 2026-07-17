CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11
LDFLAGS = -lpthread -lrt

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Objetos compartidos entre ambos binarios
COMMON_OBJS = $(OBJ_DIR)/scanner.o $(OBJ_DIR)/backup.o $(OBJ_DIR)/stats.o \
              $(OBJ_DIR)/ipc.o $(OBJ_DIR)/logger.o

.PHONY: all clean run-scan run-monitor dirs

all: dirs $(BIN_DIR)/scan $(BIN_DIR)/minisyncd

dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR) backup logs

# --- comando scan ---
$(BIN_DIR)/scan: $(OBJ_DIR)/scan.o $(OBJ_DIR)/scanner.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --- daemon monitor (incluye logica de workers y logger, mismo binario) ---
$(BIN_DIR)/minisyncd: $(OBJ_DIR)/monitor.o $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

run-scan: all
	./$(BIN_DIR)/scan .

run-monitor: all
	./$(BIN_DIR)/minisyncd . backup
