TARGET = csocksd
SRC = main.c log.c ev.c ringbuf.c

CFLAGS += -Wall -Wextra -Werror
CFLAGS += -std=c23 -D_XOPEN_SOURCE=600 -D_OPEN_SOURCE
CFLAGS += -ggdb -g


CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-int-to-pointer-cast
CFLAGS += -Wno-pointer-to-int-cast
CFLAGS += -Wno-unused-variable

OBJ = $(SRC:.c=.o)

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

all: $(TARGET) $(OBJ)

clean:
	$(RM) *.o $(TARGET)

.PHONY: all clean
