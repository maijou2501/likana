CC = gcc
CFLAGS = -Wall -O3
LOADLIBES = -pthread
BINDIR=/usr/sbin
NAME=likana

${NAME}:

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 -g root -o root ${NAME} ${DESTDIR}${BINDIR}
	install -m 755 -g root -o root ./etc/init.d/${NAME} ${DESTDIR}/etc/init.d
	install -m 755 -g root -o root ./etc/default/${NAME} ${DESTDIR}/etc/default

uninstall:
	rm -f ${BINDIR}/${NAME}
	rm -f /etc/init.d/${NAME}
	rm -f /etc/default/${NAME}
	rm -f /etc/rc*.d/*${NAME}

clean:
	rm -f ./${NAME}
