TARGET = csocksd
SRC = main.c log.c ev.c ringbuf.c tcp-listener.c session.c csocks.c protocol.c
SRCDIR = ./source

SRC := $(addprefix $(SRCDIR)/,$(SRC))

CFLAGS += -Wall -Wextra -Werror
CFLAGS += -std=c23 -D_GNU_SOURCE
CFLAGS += -ggdb -g
CFLAGS += -Iinclude

CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-int-to-pointer-cast
CFLAGS += -Wno-pointer-to-int-cast
CFLAGS += -Wno-unused-variable

OBJ = $(notdir $(SRC:.c=.o))

%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $(CFLAGS) $<

$(TARGET): $(OBJ)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

all: $(TARGET) $(OBJ)

clean:
	$(RM) *.o $(TARGET)

.PHONY: all clean
