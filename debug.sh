gcc src/main.c -o out/main -std=c99 -O0 -fno-omit-frame-pointer -fsanitize=address -static-libasan -g -Wall -Werror -Wno-unused
echo "Done"
