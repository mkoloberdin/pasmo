# Makefile for pasmo

CPP=g++
CPPFLAGS=-W -Wall

OBJS=pasmo.o pasmotypes.o token.o \
	asm.o asmfile.o \
	tap.o tzx.o spectrum.o cpc.o

all: pasmo


pasmo: $(OBJS)
	$(CPP) $(CPPFLAGS) -o pasmo $(OBJS)

pasmo.o: pasmo.cpp asm.h
	$(CPP) $(CPPFLAGS) -c pasmo.cpp

pasmotypes.o: pasmotypes.cpp pasmotypes.h
	$(CPP) $(CPPFLAGS) -c pasmotypes.cpp

token.o: token.cpp token.h pasmotypes.h
	$(CPP) $(CPPFLAGS) -c token.cpp

asmfile.o: asmfile.cpp asmfile.h token.h pasmotypes.h
	$(CPP) $(CPPFLAGS) -c asmfile.cpp

asm.o: asm.cpp asm.h token.h pasmotypes.h asmfile.h tap.h tzx.h
	$(CPP) $(CPPFLAGS) -c asm.cpp

tap.o: tap.cpp tap.h pasmotypes.h
	$(CPP) $(CPPFLAGS) -c tap.cpp

tzx.o: tzx.cpp tzx.h pasmotypes.h
	$(CPP) $(CPPFLAGS) -c tzx.cpp

spectrum.o: spectrum.cpp spectrum.h pasmotypes.h
	$(CPP) $(CPPFLAGS) -c spectrum.cpp

cpc.o: cpc.cpp cpc.h pasmotypes.h
	$(CPP) $(CPPFLAGS) -c cpc.cpp


tgz:
	tar czf pasmo.tgz Makefile *.cpp *.h *.asm \
		README NEWS pasmodoc.html

strip: pasmo
	strip pasmo

clean:
	rm -f pasmo
	rm -f *.o

# End.
