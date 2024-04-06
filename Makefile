all:
	gcc main.c -o run -lraylib -lm
clean:
	rm run
	rm spawn.txt