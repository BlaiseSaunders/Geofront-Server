CC = gcc
CFLAGS = -lm -pthread -Wall -ansi -pedantic -Wextra -g `sdl2-config --cflags --libs` -lSDL2_ttf -pg -D_POSIX_C_SOURCE=200112L
DEPS = header.h
OBJ = main.o entities.o ai.o threads.o
OUT_EXE = geofront_serv

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT_EXE): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o $(OUT_EXE)

install:
	cp ./$(OUT_EXE) /bin

rebuild: clean
	 make
