TARGET = main
CXX = g++
CXXFLAGS = -O2 -std=c++11 -Wall -pipe
INCLUDES = -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LIBS = -lboost_filesystem -lboost_system -lboost_thread -pthread -Wl,--no-as-needed
SRCS = main.cpp
OBJS := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) $(LIBS) -o $(TARGET)

clean:
	rm -f *.o *~ $(PROGRAM)
