CC = gcc
CFLAGS = -pthread
LDFLAGS = -pthread
TARGET = client

SRCS = 	client.c client_authentication.c client_messages.c client_terminal.c client_threads.c list.c client_groups.c clientFile_transfer.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)