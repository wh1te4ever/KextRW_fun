SRCS = $(wildcard *.c) $(wildcard *.m)
CC = clang
CFLAGS = -Wall -Wextra -I../include
FRAMEWORKS = -framework IOKit -framework CoreFoundation
BUILD_DIR = ../build

all:
	mkdir -p $(BUILD_DIR)/bin
	$(CC) $(CFLAGS) $(FRAMEWORKS) $(SRCS) -o $(BUILD_DIR)/bin/kerntest_uaf; \
	ldid -Sentitlements.plist $(BUILD_DIR)/bin/kerntest_uaf; \

clean:
	rm -rf $(BUILD_DIR)/bin
