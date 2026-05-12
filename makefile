OBJECTS = unixds.o inetds.o proto.o
FLAGS = -Wall -g `pkg-config --cflags opencv4`
LIBS = -lpthread `pkg-config --libs opencv4`


help:
	@echo "Utilizare:"
	@echo " make serverds   -- construiește serverul"
	@echo " make inetclient -- construiește clientul de test"
	@echo " make unixclient -- construiește adminul de test"
	@echo " make clean      -- șterge fișierele obiect și executabilele"

serverds: server.cpp ${OBJECTS}
	g++ $(FLAGS) server.cpp ${OBJECTS} -o serverds $(LIBS)

unixds.o: unixds.cpp proto.h
	g++ $(FLAGS) -c unixds.cpp -o unixds.o

inetds.o: inetds.cpp proto.h
	g++ $(FLAGS) -c inetds.cpp -o inetds.o

proto.o: proto.cpp proto.h
	g++ $(FLAGS) -c proto.cpp -o proto.o


inetclient: inetclient.cpp
	g++ -g inetclient.cpp -Wall -o inetclient

unixclient: unixclient.cpp
	g++ -g unixclient.cpp -Wall -o unixclient

clean:
	rm -f ${OBJECTS} serverds inetclient unixclient unixds.o inetds.o proto.o

# NOTICE: use sudo apt-get install gsoap libgsoap-dev
# NOTICE: use sudo apt-get install libncurses5-dev libncursesw5-dev
# NOTICE: use the sample INET client (gcc inetsample.c -o inets; ./inets)
