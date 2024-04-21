# to move in created world
all:
	gcc game.c -o run -lraylib -lm -Wall 

#  using the editor
editor:
	gcc main.c -o run -lraylib -lm -Wall

clean:
	rm run
	clear