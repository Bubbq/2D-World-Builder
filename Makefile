all:
	gcc editor.c animation.c entity.c tile.c worldbuilder.c timer.c -o run -lraylib -lm -Wall -fsanitize=address
new:
	gcc test.c utils.c list.c -lraylib -lm -Wall -fsanitize=address -o test

clean:
	rm run
	clear
