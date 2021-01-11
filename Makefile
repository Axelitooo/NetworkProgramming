comp : serveur1-RealSoupTime serveur2-RealSoupTime serveur3-RealSoupTime

client: client.o
	gcc -o client client.o

client.o: client.c
	gcc -o client.o -c client.c

serveur1-RealSoupTime: serveur1-RealSoupTime.o
	gcc -o serveur1-RealSoupTime serveur1-RealSoupTime.o

serveur1-RealSoupTime.o: serveur1-RealSoupTime.c
	gcc -o serveur1-RealSoupTime.o -c serveur1-RealSoupTime.c

serveur2-RealSoupTime: serveur2-RealSoupTime.o
	gcc -o serveur2-RealSoupTime serveur2-RealSoupTime.o

serveur2-RealSoupTime.o: serveur2-RealSoupTime.c
	gcc -o serveur2-RealSoupTime.o -c serveur2-RealSoupTime.c
	
serveur3-RealSoupTime: serveur3-RealSoupTime.o
	gcc -o serveur3-RealSoupTime serveur3-RealSoupTime.o

serveur3-RealSoupTime.o: serveur3-RealSoupTime.c
	gcc -o serveur3-RealSoupTime.o -c serveur3-RealSoupTime.c