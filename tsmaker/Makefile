DIR = ./src
INCLUDE = -I./include
LD = -lboost_system -lboost_filesystem -luv -lcurl -pthread -lzip

all: core.o process_file.o url.o decompress.o auto_comp.o pid.o tsmaker

core.o: $(DIR)/core.cpp
	g++ $(INCLUDE) $(DIR)/core.cpp -c -o $(DIR)/core.o

pid.o: $(DIR)/pid.cpp
	g++ $(INCLUDE) $(DIR)/pid.cpp -c -o $(DIR)/pid.o

decompress.o: $(DIR)/decompress.cpp
	g++ $(INCLUDE) $(DIR)/decompress.cpp -c -o $(DIR)/decompress.o

url.o:  $(DIR)/url.cpp
	g++ $(INCLUDE) $(DIR)/url.cpp -c -o $(DIR)/url.o

process_file.o: $(DIR)/process_file.cpp
	g++ $(INCLUDE) $(DIR)/process_file.cpp -c -o $(DIR)/process_file.o

auto_comp.o: $(DIR)/auto_comp.cpp
	g++ $(INCLUDE) $(DIR)/auto_comp.cpp -c -o $(DIR)/auto_comp.o

tsmaker: $(DIR)/core.o $(DIR)/pid.o $(DIR)/decompress.o $(DIR)/url.o $(DIR)/process_file.o $(DIR)/auto_comp.o
	g++ -g -O3 -Wall $(DIR)/core.o $(DIR)/pid.o $(DIR)/decompress.o $(DIR)/url.o $(DIR)/process_file.o $(DIR)/auto_comp.o -o tsmaker $(LD)

clean:
	rm $(DIR)/*.o

install:
	cp ./tsmaker /usr/local/bin
 
