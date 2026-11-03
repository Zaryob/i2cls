#
# i2c-tools-ng - Modern, smart I2C diagnostic and interrogation suite
# Copyright (C) 2026 i2c-tools-ng contributors
# Licensed under GNU General Public License v3.0 (GPLv3)
#

CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -std=c99
CPPFLAGS += -Iinclude -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
DEPFLAGS = -MMD -MP
LDFLAGS ?=

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

SRCS = src/i2c_types.c \
       src/i2c_bus.c \
       src/i2c_db.c \
       src/i2c_vendor.c \
       src/i2c_sysfs.c \
       src/commands/cmd_ls.c \
       src/commands/cmd_tree.c \
       src/commands/cmd_probe.c \
       src/commands/cmd_bind.c

OBJS = $(SRCS:.c=.o)
MAIN_OBJ = src/main.o

TEST_SRCS = tests/test_suite.c
TEST_OBJS = tests/test_suite.o $(OBJS)

TARGET = bin/i2ctool
TEST_TARGET = bin/test_suite
DB_TEST_TARGET = bin/test_db

SYMLINKS = bin/i2cls bin/i2ctree bin/i2cprobe bin/i2cbind bin/i2cunbind

.PHONY: all clean test check install uninstall symlinks

all: $(TARGET) symlinks

$(TARGET): $(OBJS) $(MAIN_OBJ)
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built: $@"

symlinks: $(TARGET)
	@mkdir -p bin
	@ln -sf i2ctool bin/i2cls
	@ln -sf i2ctool bin/i2ctree
	@ln -sf i2ctool bin/i2cprobe
	@ln -sf i2ctool bin/i2cbind
	@ln -sf i2ctool bin/i2cunbind
	@echo "Created command symlinks: i2cls, i2ctree, i2cprobe, i2cbind, i2cunbind"

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

test: all $(TEST_TARGET) $(DB_TEST_TARGET)
	@echo "Running test suite..."
	@./$(TEST_TARGET)
	@./$(DB_TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DB_TEST_TARGET): tests/test_db.o src/i2c_db.o src/i2c_vendor.o
	@mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

check: test
	python3 tests/test_cli.py

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/i2ctool
	ln -sf i2ctool $(DESTDIR)$(BINDIR)/i2cls
	ln -sf i2ctool $(DESTDIR)$(BINDIR)/i2ctree
	ln -sf i2ctool $(DESTDIR)$(BINDIR)/i2cprobe
	ln -sf i2ctool $(DESTDIR)$(BINDIR)/i2cbind
	ln -sf i2ctool $(DESTDIR)$(BINDIR)/i2cunbind
	@echo "Installed binaries to $(DESTDIR)$(BINDIR)"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/i2ctool
	rm -f $(DESTDIR)$(BINDIR)/i2cls
	rm -f $(DESTDIR)$(BINDIR)/i2ctree
	rm -f $(DESTDIR)$(BINDIR)/i2cprobe
	rm -f $(DESTDIR)$(BINDIR)/i2cbind
	rm -f $(DESTDIR)$(BINDIR)/i2cunbind
	@echo "Uninstalled binaries from $(DESTDIR)$(BINDIR)"

clean:
	rm -f $(OBJS) $(MAIN_OBJ) $(TEST_OBJS) tests/test_db.o $(OBJS:.o=.d) $(MAIN_OBJ:.o=.d) tests/test_suite.d tests/test_db.d
	rm -rf bin
	@echo "Cleaned build artifacts."

-include $(OBJS:.o=.d) $(MAIN_OBJ:.o=.d) tests/test_suite.d tests/test_db.d
