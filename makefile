CC=gcc
CFLAGS=-Wall -Wextra --std c11
TARGET=grabPixelColor

compile: 
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c -lX11 -lpng

