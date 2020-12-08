CXX = g++
CXXFLAGS = -Wall 
LDFLAGS = -static -static-libgcc -static-libstdc++
INCLUDE = -I include 
LBLIBS = -L lib -lvisa64
SRC = n6705b.cpp
OBJ = $(SRC:.cc=.o)
EXEC = n6705b.exe
RES = n6705b.res

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC) -o $@ $(INCLUDE) $(LBLIBS) $(RES)

clean:
	rm -rf $(%.o) $(EXEC)