# to move in created world, NOTE: you need to have a spawn.txt
all:
	gcc game.c -o run -lraylib -lm -Wall 

#  using the editor
editor:
	gcc main.c -o run -lraylib -lm -Wall

clean:
	rm run
	clear