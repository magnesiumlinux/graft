CC=gcc
CFLAGS=-Wall
LDFLAGS=
PROGS=graft_group graft_passwd

all: ${PROGS}

graft_passwd: graft_passwd.o
graft_group: graft_group.o

test: .PHONY
	cd test; ./test_passwd.sh
	cd test; ./test_group.sh

clean: .PHONY
	rm -rf *.o ${PROGS}

.PHONY: 
