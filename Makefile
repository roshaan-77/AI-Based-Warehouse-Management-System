CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -pthread -lrt
TARGET = warehouse_system
UI_TARGET = dashboard
WEB_TARGET = warehouse_web
SRCS = main.c admin.c retailer.c supplier.c login.c user.c warehouse.c ai.c ui.c
OBJS = $(SRCS:.c=.o)
WEB_SRCS = web_ui.c retailer.c supplier.c login.c user.c warehouse.c ai.c
WEB_OBJS = $(WEB_SRCS:.c=.web.o)

all: $(TARGET) $(UI_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -lncurses -o $@

$(UI_TARGET): dashboard.c ai.c
	$(CC) $(CFLAGS) dashboard.c ai.c $(LDFLAGS) -lncurses -o $@

$(WEB_TARGET): $(WEB_OBJS)
	$(CC) $(CFLAGS) $(WEB_OBJS) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.web.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(WEB_OBJS) $(TARGET) $(UI_TARGET) $(WEB_TARGET)

.PHONY: all clean
