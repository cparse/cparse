EXE = test-shunting-yard
SRC = $(EXE).cpp shunting-yard.cpp packToken.cpp functions.cpp operations.cpp objects.cpp catch.cpp
OBJ = $(SRC:.cpp=.o)

CXX ?= g++
CFLAGS = -std=c++11 -g -Wall -pedantic #-DDEBUG
all: $(EXE)

$(EXE): $(OBJ); $(CXX) $(CFLAGS) $(OBJ) -o $(EXE)
%.o: %.cpp *.h; $(CXX) $(CFLAGS) -c $< -o $@ $(DEBUG)

again: clean all

test: $(EXE); ./$(EXE) $(args)

check: $(EXE); valgrind --leak-check=full ./$(EXE) $(args)

clean: ; rm -f $(EXE) $(OBJ)
