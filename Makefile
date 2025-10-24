ARGS=-std=c99 -g -Wall -Wextra -pedantic 
#ARGS=-std=c99 -g -Wall -Wextra -pedantic -DNDEBUG
#ARGS=-std=c99 -g -Wall -Wextra -pedantic -D__APPLE__
ARGS=-std=c99 -g -Wall -pedantic
OBJS=main.o deck.o hand.o card.o player.o user.o computer.o state.o printf.o
BIN=gin
DEP=globals.h Makefile

$(BIN): $(OBJS)
	$(CC) $(OBJS) -lm -o $(BIN)

main.o: main.c $(DEP)
	$(CC) $(ARGS) -c main.c

deck.o: deck.c $(DEP)
	$(CC) $(ARGS) -c deck.c

hand.o: hand.c $(DEP)
	$(CC) $(ARGS) -c hand.c

card.o: card.c $(DEP)
	$(CC) $(ARGS) -c card.c

player.o: player.c $(DEP)
	$(CC) $(ARGS) -c player.c

user.o: user.c $(DEP)
	$(CC) $(ARGS) -c user.c

computer.o: computer.c $(DEP)
	$(CC) $(ARGS) -c computer.c

state.o: state.c $(DEP)
	$(CC) $(ARGS) -c state.c

printf.o: printf.c $(DEP)
	$(CC) $(ARGS) -c printf.c

clean:
	rm -r -f $(BIN) $(OBJS) $(BIN2) *dSYM
