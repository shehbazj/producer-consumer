CFLAGS = -pthread -g

all: prod_con

prod_con: prod_con.c
	gcc prod_con.c -o prod_con.o $(CFLAGS)

clean:
	rm -rf *.o
