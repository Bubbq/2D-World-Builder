all:
	gcc editor.c animation.c entity.c tile.c worldbuilder.c timer.c -o run -lraylib -lm -Wall -fsanitize=address

clean:
	rm run
	clear
