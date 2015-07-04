
#EmAnt Standard Makefile

# Tonewheel Organ with Percussion And Reverb
# GUI based on FLTK test

# Configured for Linux AMD64 (Ubuntu 14.04LTS, kernel 3.13.0-xx)

CXX		= g++
CC		= gcc
PG =

#OPTIM		=  -Os -Wall -Wunused -Wno-format-y2k  -fno-exceptions -fno-strict-aliasing -ffunction-sections -fdata-sections
#CFLAGS		= $(OPTIM) -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_THREAD_SAFE -D_REENTRANT   
#CXXFLAGS	= $(OPTIM) -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_THREAD_SAFE -D_REENTRANT 

LDFLAGS		= $(OPTIM)  
LDLIBS		= -lXfixes -lXext -lpthread -ldl -lm  -lX11 
GLDLIBS		= -lGLU -lGL -lXfixes -lXext -lpthread -ldl -lm  -lX11 
LINKFLTK	= ./lib/libfltk.a
LINKFLTKGL	= ./lib/libfltk_gl.a
LINKFLTKFORMS	= ./lib/libfltk_forms.a ./lib/libfltk.a
LINKFLTKIMG	= ./lib/libfltk_images.a ./lib/libfltk.a 

OBJS=	fltk_test.o

EXEC= test
EXE_POSTFIX = _exe				# I add this on Linux, to help .gitignore skip executables

all:	$(OBJS)
	$(CXX) $(CXXFLAGS) -o $(EXEC)$(EXE_POSTFIX) $(OBJS) $(LINKFLTK) $(LDLIBS) $(GLDLIBS)
	
clean:
	-@rm -f *.o $(EXEC)$(EXE_POSTFIX) *~ 

exec: all
	./$(EXEC)$(EXE_POSTFIX)

#DEPS gxx -MM 

