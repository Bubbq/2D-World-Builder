# to move in created world
all:
	gcc engine.c -o run -lraylib -lm -Wall 

#  using the editor
editor:
	gcc editor.c -o run -lraylib -lm -Wall

clean:
	rm run
	clear