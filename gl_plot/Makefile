CXX = g++

CFLAGS := -g -I/usr/include/SDL -O2 -pipe -Wall -ansi
LDFLAGS := -lrfftw -lfftw -lm -lGLEW -lGL -lGLU -lSDL_image -lSDL_ttf `sdl-config --cflags --libs`

PROGRAM = gl_plot

SRC=\
	serial.cpp\
	font.cpp\
	main.cpp\
	debug.cpp\
	pong.cpp\
	p300.cpp\

OBJ = $(SRC:%.cpp=%.o)

.cpp.o :
	$(CXX) -c $< $(CFLAGS) -o $@

all:	$(PROGRAM)

$(PROGRAM):	$(OBJ)
	$(CXX) -o $(PROGRAM) $(OBJ) $(LDFLAGS)

.PHONY : clean
clean:
	rm -f $(OBJ)
