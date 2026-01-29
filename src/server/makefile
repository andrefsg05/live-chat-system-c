CC = gcc
CFLAGS = -g
TARGET = server

SRCS = server.c userRegister.c userAuthentication.c groups.c list.c serverMessages.c serverGroups.c serverFile_transfer.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)