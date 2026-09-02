CXX = g++
CXXFLAGS = -O3 -Wall -pthread -std=c++17

TARGET = varredor
SRCS = varredor.cpp collatz.cpp
OBJS = $(SRCS:.cpp=.o)
HEADERS = collatz.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) parcial_*.txt
