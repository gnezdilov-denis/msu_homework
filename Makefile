CC = gcc
CFLAGS = -O2 -Wall -std=c99
all: tanker_detector
tanker_detector: main.o lodepng.o
	$(CC) $(CFLAGS) -o tanker_detector main.o lodepng.o -lm
main.o: main.c
	$(CC) $(CFLAGS) -c main.c
lodepng.o: lodepng.c
	$(CC) $(CFLAGS) -c lodepng.c
clean:
	rm -f *.o tanker_detector detections.png
