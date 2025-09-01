all:
	gcc editor.c utils.c list.c asset_cache.c -I raylib/src/ raylib/src/libraylib.a -lm -Wall -fsanitize=address -o editor

clean:
	rm editor
	clear
