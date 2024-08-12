all:
	gcc editor.c animation.c entity.c tile.c worldbuilder.c timer.c -o run -lraylib -lm -Wall 

clean:
	rm run
	clear
