# Makefile para mpasmo

CPPFLAGS=-W -Wall

all: pasmo

pasmo: pasmo.o asm.o token.o
	g++ $(CPPFLAGS) -o pasmo pasmo.o asm.o token.o

token.o: token.cpp token.h
	g++ $(CPPFLAGS) -c token.cpp

asm.o: asm.cpp asm.h token.h
	g++ $(CPPFLAGS) -c asm.cpp

pasmo.o: pasmo.cpp asm.h
	g++ $(CPPFLAGS) -c pasmo.cpp

tgz:
	tar czf pasmo.tgz Makefile *.cpp *.h *.asm README NEWS

strip: pasmo
	strip pasmo

# Fin.
