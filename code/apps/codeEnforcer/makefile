#
#	Makefile for HadronZoo::CodeEnforcer
#
#	Legal Notice: This file is part of the HadronZoo::CodeEnforcer program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
#
#	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
#
#	HadronZoo::CodeEnforcer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
#	Software Foundation, either version 3 of the License, or any later version.
#
#	The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
#	as published by the Free Software Foundation, either version 3 of the License, or any later version.
#
#	HadronZoo::CodeEnforcer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#	FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License along with HadronZoo::CodeEnforcer. If not, see http://www.gnu.org/licenses.
# 

ifndef HZBASE
HZBASE=$(HOME)
endif

USERID=$(shell id -u)

ifeq ($(USERID),0)
	HZEXEC=/home/rballard
else
	HZEXEC=$(HZBASE)
endif

HZI		= ../../hzlib.9.8/inc
OBJ		= ../../.objs/apps/ce
SLIB	= /lib/x86_64-linux-gnu
LIB		= /usr/lib
BIN		= $(HZEXEC)/bin
SRC		= .
CCMD	= g++
CFLAGS	= -I$(HZI) -g -O3 -rdynamic -Wformat -Wsign-compare -Wunused -Wno-error -DUNIX

#
#	Source and Object lists
#

CE_SRC =	$(SRC)/ceToken.cpp\
			$(SRC)/ceReport.cpp\
			$(SRC)/ceEntity.cpp\
			$(SRC)/ceComment.cpp\
			$(SRC)/cePreproc.cpp\
			$(SRC)/ceXpress.cpp\
			$(SRC)/ceParse.cpp\
			$(SRC)/ce.cpp

CE_OBJ =	$(OBJ)/ceToken.o\
			$(OBJ)/ceReport.o\
			$(OBJ)/ceEntity.o\
			$(OBJ)/ceComment.o\
			$(OBJ)/cePreproc.o\
			$(OBJ)/ceXpress.o\
			$(OBJ)/ceParse.o\
			$(OBJ)/ce.o

#
#	Target
#

$(BIN)/ce:	$(CE_OBJ) $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ $(CE_OBJ) -lHadronZoo_9.8 -lpthread -lssl -lcrypto -lrt -lz

clean:
	rm -f $(BIN)/ce
	rm -f $(OBJ)/*.o

depends:
	makedepend -I$(HZI) -DUNIX -p$(OBJ)/ -fmakedep *.cpp 2>makedeperr ;

#
#	Objects
#

$(OBJ)/ceToken.o:		$(SRC)/ceToken.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) ceToken.cpp

$(OBJ)/ceReport.o:		$(SRC)/ceReport.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) ceReport.cpp

$(OBJ)/ceEntity.o:		$(SRC)/ceEntity.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) ceEntity.cpp

$(OBJ)/ceComment.o:		$(SRC)/ceComment.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) ceComment.cpp

$(OBJ)/cePreproc.o:		$(SRC)/cePreproc.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) cePreproc.cpp

$(OBJ)/ceXpress.o:		$(SRC)/ceXpress.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) ceXpress.cpp

$(OBJ)/ceParse.o:		$(SRC)/ceParse.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) ceParse.cpp

$(OBJ)/ce.o:		$(SRC)/ce.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) ce.cpp

include makedep

#
#	End of makefile
#
