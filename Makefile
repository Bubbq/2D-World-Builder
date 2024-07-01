# to move in created world
play:
	gcc engine.c -o run -lraylib -lm -Wall 

#  using the editor
edit:
	gcc editor.c -o run -lraylib -lm -Wall

clean:
	rm run
	clear
