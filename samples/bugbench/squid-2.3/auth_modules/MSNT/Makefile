# Makefile for smb_auth

INSTALLBIN = /usr/local/squid/bin

OBJECTS = md4.o rfcnb-io.o rfcnb-util.o session.o smbauth.o smbdes.o \
smbencrypt.o smblib-util.o smblib.o valid.o denyusers.o

CC      = gcc
CFLAGS  = -s -O2

msntauth: $(OBJECTS)
	$(CC) $(CFLAGS) -o msntauth $(OBJECTS)

install: msntauth
	install -m 0111 msntauth $(INSTALLBIN)

clean:
	rm -f msntauth libsmbvalid.a $(OBJECTS)
