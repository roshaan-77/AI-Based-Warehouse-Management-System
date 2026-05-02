CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -pthread -lrt
TARGET = warehouse_system
SRCS = main.c admin.c retailer.c supplier.c login.c user.c warehouse.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
