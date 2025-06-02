CC = gcc
CFLAGS = -MMD -Wall -Wextra -pedantic -g

SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)
DEP = $(OBJ:%.o=%.d)

EXE = out

$(EXE) : $(OBJ)
	$(CC) $^ -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(DEP)

clean:
	rm -f $(OBJ) $(DEP) $(EXE)