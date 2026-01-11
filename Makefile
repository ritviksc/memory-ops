CC = gcc
CFLAGS = -Wall -O3 -march=x86-64-v3 -fno-builtin-memcpy

# Test files
SRC1 = largetest.c
SRC2 = testmemcpy.c

# Targets
largetest: $(SRC1)
        $(CC) $(CFLAGS) memcpy1.c $(SRC1) -o p1

smalltest: $(SRC2)
        $(CC) $(CFLAGS) memcpy1.c $(SRC2) -o p2

tests: largetest smalltest

clean:
        rm -f p1 p2

PHONY: largetest smalltest tests clean
