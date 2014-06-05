LIBRARIES=-lglfw -framework OpenGL
SOURCES=main.o CPU.o MMU.o Graphics.o GPU.o helper.o
FLAGS=-Os -std=c++11 -Iinclude -Wall
TESTFLAGS= 

all: gbce 
gbce: $(SOURCES)
	clang++ $(FLAGS) -o gbce $(SOURCES) $(LIBRARIES)

debug: TESTFLAGS = -D DEBUG=1 
debug: $(SOURCES)
	clang++ $(FLAGS) -o debug $(SOURCES) $(LIBRARIES)	

test: TESTFLAGS = -D TESTF=1
test: $(SOURCES) Test.o
	clang++ $(FLAGS) -o test $(SOURCES) Test.o $(LIBRARIES)

%.o: src/%.cc
	clang++ $(FLAGS) $(TESTFLAGS) -c $<

clean:
	rm -f *.o
	rm -f log/*.log