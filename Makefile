CC = gcc
CP = cp
LIBS = -lpthread 
TARGET = cpds-agent
FILEPATH = src/*.c
COPYPATH = /usr/bin
all:
	$(CC) ${FILEPATH} -o ${TARGET} ${LIBS}
install:
	$(CP) ${TARGET} ${COPYPATH}
clean:
	-@rm -rf ${TARGET} >/dev/null 2>&1
