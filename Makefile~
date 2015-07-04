
#EmAnt Standard Makefile

# Tonewheel Organ with Percussion And Reverb

CC = g++
PG =
LDFLAGS= librtmidi.a librtaudio.a -lpthread -lasound $(PG)
CXXFLAGS= -Wall -O3 -std=c++11 -O3 $(PG)

OBJS=	tonewheel.o\
		sc_reverb.o\
		adsr.o\
		perc_envelope.o
		
EXEC= tonewheel


all:	$(OBJS)
	$(CC) -o $(EXEC) $(OBJS) $(LDFLAGS) 
	
clean:
	-@rm -f *.o $(EXEC) *~ 

exec: all
	./$(EXEC)

#DEPS gxx -MM 

tonewheel.o:	tonewheel.cpp sc_reverb.h
sc_reverb.o:	sc_reverb.c sc_reverb.h 
adsr.o :	adsr.cpp adsr.h
perc_envelope.o : adsr.h perc_envelope.h perc_envelope.cpp 

