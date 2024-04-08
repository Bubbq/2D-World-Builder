all:
	gcc main.c -o run -lraylib -lm
old:
	g++ editor.cpp generation.cpp -o run -lraylib -lm
clean:
	rm run
	rm spawn.txt