CXXFLAGS = -I./include $(shell pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0)
OBJS = ytclient.o gtkabout.o main.o
CXX = g++
FLAG = -c -o
TARGET = youtube

all: ytclient.o gtkabout.o main.o $(TARGET)


ytclient.o: ./src/ytclient.cpp
	$(CXX) $(CXXFLAGS) ./src/ytclient.cpp $(FLAG) ./ytclient.o

gtkabout.o: ./src/gtkabout.cpp
	$(CXX) $(CXXFLAGS) ./src/gtkabout.cpp $(FLAG) ./gtkabout.o
main.o:  ./src/main.cpp
	$(CXX) $(CXXFLAGS) ./src/main.cpp $(FLAG) ./main.o

youtube: $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(CXXFLAGS)

clean:
	rm $(OBJS)
	rm $(TARGET)

run:
	./youtube
