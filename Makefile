CC=gcc
CFLAGS=-Wall -Wextra -pthread

SRCDIR = src
BINDIR = bin

SERVER_EXEC = server
CLIENT_EXEC = client

SERVER_SRC = $(SRCDIR)/server.c
CLIENT_SRC = $(SRCDIR)/client.c

all: $(BINDIR) $(CLIENT_EXEC) $(SERVER_EXEC)

$(BINDIR):
	mkdir -p $(BINDIR)

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $<

$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(BINDIR)/$@ $<

clean:
	rm -f $(SRCDIR)/*.o
	rm -rf $(BINDIR)

.PHONY: all clean