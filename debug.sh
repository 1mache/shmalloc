gcc src/main.c src/shmalloc.c src/shmalloc.h -o out/main -std=c99 -fsanitize=address -static-libasan -g
echo "Done"