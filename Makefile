OBJECTS = geometry.o vector_maths.o physics.o game_loop.o
CCFLAGS = -Iinclude -O3 -Wall -Werror -pedantic -std=c++17 -Wno-error=unused-function
LIBRARIES = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -pthread

all: $(OBJECTS) airhockey_server airhockey_client

$(OBJECTS): %.o: %.cpp include/%.hpp
	g++ $(CCFLAGS) -c $< -o $@

airhockey_server: airhockey_server.cpp $(OBJECTS)
	g++ $(CCFLAGS) airhockey_server.cpp $(OBJECTS) $(LIBRARIES) -o airhockey_server

airhockey_client: airhockey_client.cpp $(OBJECTS)
	g++ $(CCFLAGS) airhockey_client.cpp $(OBJECTS) $(LIBRARIES) -o airhockey_client

client: client.cpp $(OBJECTS)
	g++ $(CCFLAGS) client.cpp $(OBJECTS) $(LIBRARIES) -o client

server: server.cpp $(OBJECTS)
	g++ $(CCFLAGS) server.cpp $(OBJECTS) $(LIBRARIES) -o server

mouse_throughput: mouse_throughput.cpp
	g++ $(CCFLAGS) mouse_throughput.cpp $(LIBRARIES) -lX11 -o mouse_throughput

clean:
	rm -rf airhockey_server airhockey_client $(OBJECTS)
