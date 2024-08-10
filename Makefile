all:
	gcc editor.c animation.c entity.c tile.c worldbuilder.c -o run -lraylib -lm -Wall 

clean:
	rm run
	clear
