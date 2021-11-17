CC     = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall
LIBS   = -lSDL2 -lSDL2_mixer -lm
DEPS   = headers/constants.h
OBJ    = main.o
SRC    = src

%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

FormA: $(OBJ)
	$(CC) $(LIBS) -o $@ $^ $(CFLAGS)
	rm -f *.o
