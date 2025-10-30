# Makefile - Build with local cJSON

TARGET = main
SRC = main.c \
	./lib/cJSON/cJSON.c \
	./wifi/wifi_scheduler.c \
	./wifi/fun/wifi_enable.c \
	./wifi/fun/wifi_status.c \
	./wifi/fun/wifi_scan.c

CC = gcc

CFLAGS = -D_GNU_SOURCE \
	-Wall \
	-Wextra \
	-std=c99 \
	-I./lib/cJSON \
	-I/usr/include/libwebsockets\
	-D_GNU_SOURCE \
	-std=c99
	
LDFLAGS = -L./lib/cJSON/build -lwebsockets  -lm -lpthread -lrt

# 默认目标
all: $(TARGET)

# 编译规则
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild
