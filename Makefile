CXX      = g++
CXXFLAGS = -Wall -Wextra -g

# Default target: build everything
all: client serverM serverA serverP serverR

# --- Executables ---

client: client.o
	$(CXX) $(CXXFLAGS) -o client client.o

serverM: serverM.o
	$(CXX) $(CXXFLAGS) -o serverM serverM.o

serverA: serverA.o
	$(CXX) $(CXXFLAGS) -o serverA serverA.o

serverP: serverP.o
	$(CXX) $(CXXFLAGS) -o serverP serverP.o

serverR: serverR.o
	$(CXX) $(CXXFLAGS) -o serverR serverR.o

# --- Object files ---

# Generic rule: build .o from .cpp and its matching .h
%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $<

# --- Utility targets ---

clean:
	rm -f client serverM serverA serverP serverR *.o
