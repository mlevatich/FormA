CC     = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall
LIBS   = -lSDL2 -lSDL2_mixer
DEPS   = headers/constants.h
OBJ    = main.o
SRC    = src

%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

CSEE6863: $(OBJ)
	$(CC) $(LIBS) -o $@ $^ $(CFLAGS)
	rm -f *.o
