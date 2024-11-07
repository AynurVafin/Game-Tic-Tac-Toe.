CC = g++
CFLAGS = -Wall -Wextra
CLIENT_CPP = client.cpp
SERVER_CPP = server.cpp
CLIENT = client
SERVER = server


all: Game

Game:
	$(CC) $(CFLAGS) $(SERVER_CPP) -o $(SERVER)
	$(CC) $(CFLAGS) $(CLIENT_CPP) -o $(CLIENT)
	

clean:
	rm -rf *.o *.a *.gcno *.gcda *.info *.txt report test

rebuild: clean all
