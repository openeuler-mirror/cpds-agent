CC=gcc
INCLUDE=src/*.c
TAGET=cpds-agent
all:
	$(CC) ${INCLUDE} -o ${TAGET} -lpthread
clean:
	-@rm -rf ${TAGET} >/dev/null 2>&1