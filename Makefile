comp : serveur0-RealSoupTime serveur1-RealSoupTime serveur2-RealSoupTime

client: client.o
	gcc -o client client.o

client.o: client.c
	gcc -o client.o -c client.c
	
serveur0-RealSoupTime: serveur0-RealSoupTime.o
	gcc -o serveur0-RealSoupTime serveur0-RealSoupTime.o

serveur0-RealSoupTime.o: serveur0-RealSoupTime.c
	gcc -o serveur0-RealSoupTime.o -c serveur0-RealSoupTime.c

serveur1-RealSoupTime: serveur1-RealSoupTime.o
	gcc -o serveur1-RealSoupTime serveur1-RealSoupTime.o

serveur1-RealSoupTime.o: serveur1-RealSoupTime.c
	gcc -o serveur1-RealSoupTime.o -c serveur1-RealSoupTime.c

serveur2-RealSoupTime: serveur2-RealSoupTime.o
	gcc -o serveur2-RealSoupTime serveur2-RealSoupTime.o

serveur2-RealSoupTime.o: serveur2-RealSoupTime.c
	gcc -o serveur2-RealSoupTime.o -c serveur2-RealSoupTime.c