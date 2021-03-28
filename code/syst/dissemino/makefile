#
#	Makefile for HadronZoo Dissemino Low Latency Website Engine (Version 1.1)
#
#  Legal Notice: This file is part of the HadronZoo::Dissemino program which in turn depends on the HadronZoo C++ Class Library with which it is shipped.
#
#  Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
#
#  HadronZoo::Dissemino is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free
#  Software Foundation, either version 3 of the License, or any later version.
#
#  The HadronZoo C++ Class Library is also free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation, either version 3 of the License, or any later version.
#
#  HadronZoo::Dissemino is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along with HadronZoo::Dissemino. If not, see http://www.gnu.org/licenses.
#

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
OBJ		= ../../.objs/syst/dissemino
LIB		= /usr/lib
BIN		= $(HZEXEC)/bin
SRC		= .
CCMD	= g++
CFLAGS	= -I$(HZI) -DUNIX -DXMEMORY

#
#	Source and Object lists
#

DISSEMINO_SRC	= dissemino.cpp
DISSEMINO_OBJ	= $(OBJ)/dissemino.o

#
#	Target
#
#
all:	$(BIN)/dissemino

$(BIN)/dissemino:	$(DISSEMINO_OBJ) $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ $(DISSEMINO_OBJ) -lHadronZoo_9.8 -lpthread -lssl -lcrypto -lrt -lz

clean:
	rm -f $(BIN)/dissemino
	rm -f $(OBJ)/*.o

depends:
	mkdir -p $(OBJ)
	makedepend -I$(HZI) -DUNIX -p$(OBJ)/ -fmakedep $(DISSEMINO_SRC) 2>makedeperr ;

#
#	Objects
#

$(OBJ)/dissemino.o:	$(SRC)/dissemino.cpp $(LIB)/libHadronZoo_9.8.a
	$(CCMD) -o $@ -c $(CFLAGS) $(SRC)/dissemino.cpp

include makedep

#
#	End of makefile
#
