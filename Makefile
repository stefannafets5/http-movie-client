CC = gcc
CPP = g++
CFLAGS = -std=c11 -Wall -Wextra -g
CPPFLAGS = -std=c++11 -Wall -Wextra -g

TARGET = client

SOURCES = client.cpp buffer.c helpers.c requests.c
OBJECTS = client.o buffer.o helpers.o requests.o

$(TARGET): $(OBJECTS)
	$(CPP) $(OBJECTS) -o $(TARGET)

# Compile C files
buffer.o: buffer.c
	$(CC) $(CFLAGS) -c buffer.c -o buffer.o

helpers.o: helpers.c
	$(CC) $(CFLAGS) -c helpers.c -o helpers.o

requests.o: requests.c
	$(CC) $(CFLAGS) -c requests.c -o requests.o

# Compile C++ file
client.o: client.cpp
	$(CPP) $(CPPFLAGS) -c client.cpp -o client.o

# Clean up
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean