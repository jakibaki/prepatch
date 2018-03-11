build:
	gcc -O3 -Wall -ldl -fPIC -shared -o libprepatch.so prepatch.c
