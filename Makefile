CC = gcc
CFLAGS = -Wall -O3
LOADLIBES = -pthread
BINDIR=/usr/sbin
NAME=likana

likana:

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 -g root -o root ${NAME} ${DESTDIR}${BINDIR}
	install -m 755 -g root -o root ./etc/init.d/${NAME} ${DESTDIR}/etc/init.d
	install -m 755 -g root -o root ./etc/default/${NAME} ${DESTDIR}/etc/default

uninstall:
	rm ${BINDIR}/${NAME}
	rm /etc/init.d/${NAME}
	rm /etc/default/${NAME}
	rm /etc/rc2.d/S20${NAME}
	rm /etc/rc3.d/S20${NAME}
	rm /etc/rc4.d/S20${NAME}
	rm /etc/rc5.d/S20${NAME}

clean:
	rm ./${NAME}
