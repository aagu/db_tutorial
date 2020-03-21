SOURCES = src/table.c src/btree.c src/pager.c src/utils.c src/db.c

CC = gcc
CFLAGS = 
INCLUDES = include

db: ${SOURCES}
	${CC} ${CFLAGS} -I ${INCLUDES} -o $@ ${SOURCES}

run: db
	./db mydb.db

clean:
	rm db