CC = gcc
CFILE = ./src/main.c
EXE = ./dist/main
CFLAGS = -Wall

all: $(EXE)
	$(EXE)

$(EXE): $(CFILE)
	$(CC) $(CFLAGS) -o $(EXE) $(CFILE)

clean:
	rm -rf $(EXE)
