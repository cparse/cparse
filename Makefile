EXE = test-shunting-yard
CORE_SRC = shunting-yard.cpp packToken.cpp functions.cpp containers.cpp
SRC = $(EXE).cpp $(CORE_SRC) builtin-features.cpp catch.cpp
OBJ = $(SRC:.cpp=.o)

LD ?= ld
CXX ?= g++
CFLAGS = -std=c++11 -Wall -pedantic
DEBUG = -g #-DDEBUG

all: $(EXE) release

$(EXE): $(OBJ); $(CXX) $(CFLAGS) $(DEBUG) $(OBJ) -o $(EXE)
%.o: %.cpp *.h %/*; $(CXX) $(CFLAGS) $(DEBUG) -c $< -o $@ $(DEBUG)
%.o: %.cpp *.h; $(CXX) $(CFLAGS) $(DEBUG) -c $< -o $@ $(DEBUG)

release: $(CORE_SRC) builtin-features.cpp;
	$(CXX) -c -O3 $(CFLAGS) builtin-features.cpp
	$(LD) -r -O1 $(CORE_SRC:.cpp=.o) -o core-shunting-yard.o

again: clean all

test: $(EXE); ./$(EXE) $(args)

check: $(EXE); valgrind --leak-check=full ./$(EXE) $(args)

simul: $(EXE); cgdb --args ./$(EXE) $(args)

clean: ; rm -f $(EXE) $(OBJ) core-shunting-yard.o full-shunting-yard.o
