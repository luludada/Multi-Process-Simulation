all: simulation

simulation: simulation.o list.o 
	gcc -pthread -o simulation simulation.o list.o
	
simulation.o: simulation.c
	gcc -g -c simulation.c

clean:
	rm -f main *.o && clear
