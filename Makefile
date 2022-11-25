CC = gcc
CP = cp
LIBS = -lpthread 
TARGET = cpds-agent
FILEPATH = src/*.c
COPYPATH = /usr/bin
SERVICEFILE = config/cpds-agent.service
SYSTEMDPATH = /usr/lib/systemd/system
all:
	$(CC) ${FILEPATH} -o ${TARGET} ${LIBS}
install:
	$(CP) ${TARGET} ${COPYPATH}
	$(CP) ${SERVICEFILE} ${SYSTEMDPATH}
	systemctl daemon-reload
	systemctl enable ${TARGET}
	systemctl restart ${TARGET}
clean:
	-@rm -rf ${TARGET} >/dev/null 2>&1
