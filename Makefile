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
unstall:
    systemctl stop  ${TARGET} >/dev/null 2>&1
	-@rm -rf ${SYSTEMDPATH}/${TARGET}.service >/dev/null 2>&1
	-@rm -rf ${COPYPATH}/${TARGET} >/dev/null 2>&1
clean:
	-@rm -rf ${TARGET} >/dev/null 2>&1
