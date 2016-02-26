EXE=test-shunting-yard
SRC=$(EXE).cpp shunting-yard.cpp
OBJ=$(SRC:.cpp=.o)

CXX=g++
CFLAGS=-g -Wall -pedantic #-DDEBUG
all: $(EXE)

$(EXE): $(OBJ); $(CXX) $(CFLAGS) $(OBJ) -o $(EXE)
%.o: %.cpp %.h; $(CXX) $(CFLAGS) -c $< -o $@ $(DEBUG)

again: clean all

run: $(EXE); ./$(EXE)

clean: ; rm -f $(EXE) $(OBJ)
