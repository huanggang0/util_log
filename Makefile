# CHUANG JI Makefile


TARGET = log_test
SRC  = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJ  := $(SRC:%.c=%.o)
CC   = gcc
CFLAGS := -g

all: $(TARGET)

$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $^ -o  $@ -lpthread -lm   
	@echo Compiling $@

%.o : %.c
	$(CC) -g -c $^

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)

