all:
	g++ generation.cpp editor.cpp  -o run -lraylib
clean:
	rm run
	rm spawn.txt