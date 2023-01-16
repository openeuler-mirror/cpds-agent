CC = gcc
CP = cp
MKDIR = mkdir -p
DIRECTORY = bin
LIBS = -lpthread -ldl -lm 
LIBS += -Wl,-Bstatic -L"libs/sqlite3" -lsqlite3 -Wl,-Bdynamic
LIBS += -Wl,-Bstatic -L"libs/zlog" -lzlog -Wl,-Bdynamic
LIBS +=  -g -O0 -lglib-2.0 -I /usr/include/glib-2.0/ `pkg-config --cflags glib-2.0`
TARGET = cpds-agent
FILEPATH = src/*.c
COPYPATH = /usr/bin
SERVICEFILE = config/cpds-agent.service
SYSTEMDPATH = /usr/lib/systemd/system
LOGCONFILE = config/cpds_log.conf
LOGCONFTARGET = /etc/cpds/cpds-agent
all:
	$(CC) ${FILEPATH} -o ${TARGET} ${LIBS}
	$(MKDIR)  $(DIRECTORY)
	mv ${TARGET} $(DIRECTORY)
install:
	$(MKDIR) ${LOGCONFTARGET}
	$(CP) ${LOGCONFILE} ${LOGCONFTARGET}
	$(CP) ${DIRECTORY}/${TARGET} ${COPYPATH}
	$(CP) ${SERVICEFILE} ${SYSTEMDPATH}
	systemctl daemon-reload
	systemctl enable ${TARGET}
	systemctl restart ${TARGET}
uninstall:
	systemctl stop  ${TARGET} >/dev/null 2>&1
	-@rm -rf ${SYSTEMDPATH}/${TARGET}.service >/dev/null 2>&1
	-@rm -rf ${COPYPATH}/${TARGET} >/dev/null 2>&1
	-@rm -rf ${LOGCONFTARGET} >/dev/null 2>&1
clean:
	-@rm -rf ${TARGET} >/dev/null 2>&1
