CC = gcc
CFLAGS = -I/opt/homebrew/include -arch arm64
LDFLAGS = -L/opt/homebrew/lib -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -arch arm64

simple_game: simple_game.c
	$(CC) simple_game.c -o simple_game $(CFLAGS) $(LDFLAGS)

run: simple_game
	./simple_game

clean:
	rm -f simple_game

