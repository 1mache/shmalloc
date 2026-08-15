gcc src/main.c -o out/main -std=c99 -fsanitize=address -static-libasan -g
echo "Done"