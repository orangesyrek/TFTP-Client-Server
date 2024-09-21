CC = gcc

LDFLAGS = -lm -Wall
LIBS = 

CLIENT_SOURCES = src/tftp-client.c src/tftp-functions.c
CLIENT_OBJECTS = $(CLIENT_SOURCES:.c=.o)

SERVER_SOURCES = src/tftp-server.c src/tftp-functions.c
SERVER_OBJECTS = $(SERVER_SOURCES:.c=.o)

all: tftp-client tftp-server

tftp-client: $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) -o tftp-client $(CLIENT_OBJECTS) $(LDFLAGS) $(LIBS)

tftp-server: $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) -o tftp-server $(SERVER_OBJECTS) $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(PACKET_OBJECTS)
	rm -f tftp-client
	rm -f tftp-server

.PHONY: clean
