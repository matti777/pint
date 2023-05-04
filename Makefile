# Makefile,v 1.3 2002/02/10 14:10:59 matti Exp
# Makefile for PINT

OBJFILES = src/pint.c src/curses.c src/formatters.c src/network.c src/cmdline.c

PROGNAME = pint
CC       = gcc

$(PROGNAME): $(OBJFILES)
	$(CC) -lncurses $(OBJFILES) -o $(PROGNAME)

clean:
	rm src/*.o
	rm src/*~
