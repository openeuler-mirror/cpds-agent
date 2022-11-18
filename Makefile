all:
	gcc src/*.c -o cpds-agent -lpthread
clean:
	-@rm -rf ${TARGET} >/dev/null 2>&1
