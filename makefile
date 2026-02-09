TARGET = csocksd
SRC = main.c

CFLAGS += -Wall -Wextra -Werror
CFLAGS += -std=c99 -D_XOPEN_SOURCE

OBJ = $(SRC:.c=.o)

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

all: $(TARGET) $(OBJ)

clean:
	$(RM) *.o $(TARGET)

.PHONY: all clean
