all:
	gcc -Wall -Wextra -O3  -fwhole-program -flto=auto -pipe -march=native -mtune=native -o main ./*.c ./*/*.c ./*/*.h -lm

debug:
	gcc -Wall -Wextra -O3  -fwhole-program -flto=auto -pipe -march=native -mtune=native -DDEBUG -o main ./*.c ./*/*.c ./*/*.h -lm

gdb:
	gcc -Wall -Wextra -pipe -g -o main ./*.c ./*/*.c ./*/*.h -lm

.PHONY: all debug gdb
