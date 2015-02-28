CC = gcc
CFLAGS = -Wall -O3
LOADLIBES = -pthread
BINDIR=/usr/sbin
NAME=likana

likana:

install:
	install -d ${DESTDIR}${BINDIR}
	install -m 755 -g root -o root likana ${DESTDIR}${BINDIR}
	install -m 755 -g root -o root ./etc/init.d/likana ${DESTDIR}/etc/init.d/
	install -m 755 -g root -o root ./etc/default/likana ${DESTDIR}/etc/default/likana

uninstall:
	rm ${BINDIR}/likana
	rm /etc/init.d/likana
	rm /etc/default/likana
	rm /etc/rc2.d/S20${NAME}
	rm /etc/rc3.d/S20${NAME}
	rm /etc/rc4.d/S20${NAME}
	rm /etc/rc5.d/S20${NAME}

clean:
	rm ./likana
