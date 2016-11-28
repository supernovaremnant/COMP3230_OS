agent: trs.h trs.c agent.c
	gcc -c trs.c
	gcc -c agent.c
	gcc trs.o agent.o -lpthread -o agent
clean: 
	rm -rf *.o
	rm agent
