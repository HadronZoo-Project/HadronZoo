#
#	Makefile for the HadronZoo::Epistula email suite. This comprises:-
#
#		1)	epistula	The main SMTP, POP3 and HTTP server program
#		2)	episMail	Command line mail sender program
#		3)	episCTRL	Command line epistula start/stop controller
#		4)	episImp		System admin tool for importing lagacy email messages
#
#	Legal Notice: This file is part of the HadronZoo::Epistula program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
#
#	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
#
#	HadronZoo::Dissemino is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
#	Software Foundation, either version 3 of the License, or any later version.
#
#	The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
#	as published by the Free Software Foundation, either version 3 of the License, or any later version.
#
#	HadronZoo::Dissemino is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License along with HadronZoo::Dissemino. If not, see http://www.gnu.org/licenses.


ifndef HZBASE
HZBASE=$(HOME)
endif

USERID=$(shell id -u)

ifeq ($(USERID),0)
	HZEXEC=/usr/local
else
	HZEXEC=$(HOME)
endif

HZI		= ../../hzlib.9.8/inc
OBJ		= ../../.objs/syst/epistula
LIB		= /usr/lib
BIN		= $(HZEXEC)/bin
SRC		= .
CCMD	= g++
CFLAGS	= -I$(HZI) -Winline -DUNIX

#	Main Epistula SMTP/POP3/HTTP Server
EPISTULA_SRC =	epData.cpp		\
				epSMTP.cpp		\
				epHTTP.cpp		\
				epPOP3.cpp		\
				epSend.cpp		\
				epistula.cpp
EPISTULA_OBJ =	$(OBJ)/epData.o	\
				$(OBJ)/epSMTP.o	\
				$(OBJ)/epHTTP.o	\
				$(OBJ)/epPOP3.o	\
				$(OBJ)/epSend.o	\
				$(OBJ)/epistula.o

#	Epistula Command Line Mailer
EPISMAIL_SRC =	emailcmd.cpp
EPISMAIL_OBJ =	$(OBJ)/emailcmd.o

# Epistula email importer
EPISIMP_SRC =	epData.cpp		\
				emailimp.cpp
EPISIMP_OBJ =	$(OBJ)/epData.o	\
				$(OBJ)/emailimp.o

#
#	Targets
#

all:		$(BIN)/epistula	\
			$(BIN)/emailcmd	\
			$(BIN)/emailimp

epistula:	$(BIN)/epis
emailcmd:	$(BIN)/emailcmd
emailimp:	$(BIN)/emailimp

$(BIN)/epistula:	$(EPISTULA_OBJ) $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ $(CFLAGS) $(EPISTULA_OBJ) -lHadronZoo_9.8 -lpthread -lresolv -lssl -lcrypto -lrt -lz

$(BIN)/emailcmd:	$(EPISMAIL_OBJ) $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ $(CFLAGS) $(EPISMAIL_OBJ) -lHadronZoo_9.8 -lpthread -lresolv -lssl -lcrypto -lrt -lz

$(BIN)/emailimp:	$(EPISIMP_OBJ) $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ $(CFLAGS) $(EPISIMP_OBJ) -lHadronZoo_9.8 -lpthread -lresolv -lssl -lcrypto -lrt -lz

#
#	Additional targets
#

clean:
	rm -f $(OBJ)/*.o
	rm -f $(BIN)/epistula
	rm -f $(BIN)/emailcmd
	rm -f $(BIN)/emailimp

depends:
	makedepend -I$(HZI) -p$(OBJ)/ -fmakedep *.cpp 2>makedeperr;

#
#	Objects
#

$(OBJ)/epData.o:	$(SRC)/epData.cpp
	$(CCMD) -o $@ -c $(CFLAGS) epData.cpp

$(OBJ)/epSMTP.o:	$(SRC)/epSMTP.cpp
	$(CCMD) -o $@ -c $(CFLAGS) epSMTP.cpp

$(OBJ)/epHTTP.o:	$(SRC)/epHTTP.cpp
	$(CCMD) -o $@ -c $(CFLAGS) epHTTP.cpp

$(OBJ)/epPOP3.o:	$(SRC)/epPOP3.cpp
	$(CCMD) -o $@ -c $(CFLAGS) epPOP3.cpp

$(OBJ)/epSend.o:	$(SRC)/epSend.cpp
	$(CCMD) -o $@ -c $(CFLAGS) epSend.cpp

$(OBJ)/epistula.o:	$(SRC)/epistula.cpp
	$(CCMD) -o $@ -c $(CFLAGS) epistula.cpp

$(OBJ)/emailcmd.o:	$(SRC)/emailcmd.cpp
	$(CCMD) -o $@ -c $(CFLAGS) emailcmd.cpp

$(OBJ)/emailimp.o:		$(SRC)/emailimp.cpp
	$(CCMD) -o $@ -c $(CFLAGS) emailimp.cpp

include makedep

#
#	THE END
#
