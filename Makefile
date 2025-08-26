all:
	gcc editor.c animation.c entity.c tile.c worldbuilder.c timer.c -o run -lraylib -lm -Wall -fsanitize=address
new:
	gcc test.c list.c -lraylib -lm -Wall -o test

clean:
	rm run
	clear
