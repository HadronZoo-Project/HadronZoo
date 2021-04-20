#
#	makefile for HadronZoo C++ Class Library (Version 9.8)
#
#	Legal Notice: This file is part of the HadronZoo C++ Class Library.
#
#	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
#
#	The HadronZoo C++ Class Library is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
#	as published by the Free Software Foundation, either version 3 of the License, or any later version.
#
#	The HadronZoo C++ Class Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License along with the HadronZoo C++ Class Library. If not, see
#	<http://www.gnu.org/licenses/>.
#

#	ifndef HZBASE
#	HZBASE=$(HOME)
#	endif

#	USERID=$(shell id -u)

#	ifeq ($(USERID),0)
#		HZEXEC=/usr/local
#	else
#		HZEXEC=$(HZBASE)
#	endif

SRC		= .
INC		= ../inc
OBJ		= ../../.objs/hzlib
LIB		= /usr/lib
CCMD	= g++
CFLAGS	= -c -I$(INC) -g -Os -Wunused -Wformat -Wsign-compare -Wno-error -DUNIX

HADRONZOO_SRC =	hdbBinRepos.cpp		\
				hdbClass.cpp		\
				hdbIdset.cpp		\
				hdbIndex.cpp		\
				hdbIsamfile.cpp		\
				hdbObject.cpp		\
				hdbObjCache.cpp		\
				hdbObjStore.cpp		\
				hdbObjRepos.cpp		\
				hdsCore.cpp			\
				hdsConfig.cpp		\
				hdsExec.cpp			\
				hdsGenerate.cpp		\
				hdsGraph.cpp		\
				hdsMaster.cpp		\
				hdsResource.cpp		\
				hdsScript.cpp		\
				hdsSystem.cpp		\
				hdsTree.cpp			\
				hzAtom.cpp			\
				hzChain.cpp			\
				hzCodec.cpp			\
				hzCron.cpp			\
				hzDate.cpp			\
				hzDelta.cpp			\
				hzDirectory.cpp		\
				hzDNS.cpp			\
				hzDomain.cpp		\
				hzDocHtml.cpp		\
				hzDocXml.cpp		\
				hzEmaddr.cpp		\
				hzError.cpp			\
				hzFtpClient.cpp		\
				hzHttpClient.cpp	\
				hzHttpServer.cpp	\
				hzIpaddr.cpp		\
				hzIdxCh.cpp			\
				hzIsam.cpp			\
				hzIsamT.cpp			\
				hzIpServer.cpp		\
				hzLock.cpp			\
				hzLogger.cpp		\
				hzMailer.cpp		\
				hzMemory.cpp		\
				hzNumexp.cpp		\
				hzPop3.cpp			\
				hzProcess.cpp		\
				hzRegex.cpp			\
				hzString.cpp		\
				hzStrRepos.cpp		\
				hzTcpClient.cpp		\
				hzTextproc.cpp		\
				hzTokens.cpp		\
				hzTree.cpp			\
				hzTypes.cpp			\
				hzUdpClient.cpp		\
				hzUrl.cpp			\
				hzUnixacc.cpp

HADRONZOO_OBJ =	$(OBJ)/hdbBinRepos.o	\
				$(OBJ)/hdbClass.o		\
				$(OBJ)/hdbIdset.o		\
				$(OBJ)/hdbIndex.o		\
				$(OBJ)/hdbIsamfile.o	\
				$(OBJ)/hdbObject.o		\
				$(OBJ)/hdbObjCache.o	\
				$(OBJ)/hdbObjStore.o	\
				$(OBJ)/hdbObjRepos.o	\
				$(OBJ)/hdsCore.o		\
				$(OBJ)/hdsConfig.o		\
				$(OBJ)/hdsExec.o		\
				$(OBJ)/hdsGenerate.o	\
				$(OBJ)/hdsGraph.o		\
				$(OBJ)/hdsMaster.o		\
				$(OBJ)/hdsResource.o	\
				$(OBJ)/hdsScript.o		\
				$(OBJ)/hdsSystem.o		\
				$(OBJ)/hdsTree.o		\
				$(OBJ)/hzAtom.o			\
				$(OBJ)/hzChain.o		\
				$(OBJ)/hzCron.o			\
				$(OBJ)/hzCodec.o		\
				$(OBJ)/hzDate.o			\
				$(OBJ)/hzDelta.o		\
				$(OBJ)/hzDirectory.o	\
				$(OBJ)/hzDocHtml.o		\
				$(OBJ)/hzDocXml.o		\
				$(OBJ)/hzDomain.o		\
				$(OBJ)/hzDNS.o			\
				$(OBJ)/hzEmaddr.o		\
				$(OBJ)/hzError.o		\
				$(OBJ)/hzFtpClient.o	\
				$(OBJ)/hzHttpClient.o	\
				$(OBJ)/hzHttpServer.o	\
				$(OBJ)/hzIpaddr.o		\
				$(OBJ)/hzIdxCh.o		\
				$(OBJ)/hzIsam.o			\
				$(OBJ)/hzIsamT.o		\
				$(OBJ)/hzIpServer.o		\
				$(OBJ)/hzLock.o			\
				$(OBJ)/hzLogger.o		\
				$(OBJ)/hzMailer.o		\
				$(OBJ)/hzMemory.o		\
				$(OBJ)/hzNumexp.o		\
				$(OBJ)/hzPop3.o			\
				$(OBJ)/hzProcess.o		\
				$(OBJ)/hzRegex.o		\
				$(OBJ)/hzString.o		\
				$(OBJ)/hzStrRepos.o		\
				$(OBJ)/hzTcpClient.o	\
				$(OBJ)/hzTextproc.o		\
				$(OBJ)/hzTokens.o		\
				$(OBJ)/hzTree.o			\
				$(OBJ)/hzTypes.o		\
				$(OBJ)/hzUdpClient.o	\
				$(OBJ)/hzUrl.o			\
				$(OBJ)/hzUnixacc.o

#
#	Targets
#

$(LIB)/libHadronZoo_9.8.a:	$(HADRONZOO_OBJ)
	ar r $(LIB)/libHadronZoo_9.8.a $(HADRONZOO_OBJ)

clean:
	rm -f $(LIB)/libHadronZoo_9.8.a
	rm -f $(OBJ)/*.o

depends:
	makedepend -I$(INC) -DUNIX -p$(OBJ)/ -fmakedep $(HADRONZOO_SRC) 2>makedeperr ;

#
#	Objects
#

$(OBJ)/hdbBinRepos.o:		$(SRC)/hdbBinRepos.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbBinRepos.cpp

$(OBJ)/hdbClass.o:			$(SRC)/hdbClass.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbClass.cpp

$(OBJ)/hdbIdset.o:			$(SRC)/hdbIdset.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbIdset.cpp

$(OBJ)/hdbIndex.o:			$(SRC)/hdbIndex.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbIndex.cpp

$(OBJ)/hdbIsamfile.o:		$(SRC)/hdbIsamfile.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbIsamfile.cpp

$(OBJ)/hdbObject.o:			$(SRC)/hdbObject.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbObject.cpp

$(OBJ)/hdbObjCache.o:		$(SRC)/hdbObjCache.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbObjCache.cpp

$(OBJ)/hdbObjStore.o:		$(SRC)/hdbObjStore.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbObjStore.cpp

$(OBJ)/hdbObjRepos.o:		$(SRC)/hdbObjRepos.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdbObjRepos.cpp

$(OBJ)/hdsCore.o:			$(SRC)/hdsCore.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsCore.cpp

$(OBJ)/hdsConfig.o:			$(SRC)/hdsConfig.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsConfig.cpp

$(OBJ)/hdsExec.o:			$(SRC)/hdsExec.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsExec.cpp

$(OBJ)/hdsGenerate.o:		$(SRC)/hdsGenerate.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsGenerate.cpp

$(OBJ)/hdsGraph.o:			$(SRC)/hdsGraph.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsGraph.cpp

$(OBJ)/hdsMaster.o:			$(SRC)/hdsMaster.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsMaster.cpp

$(OBJ)/hdsResource.o:		$(SRC)/hdsResource.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsResource.cpp

$(OBJ)/hdsScript.o:			$(SRC)/hdsScript.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsScript.cpp

$(OBJ)/hdsSystem.o:			$(SRC)/hdsSystem.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsSystem.cpp

$(OBJ)/hdsTree.o:			$(SRC)/hdsTree.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hdsTree.cpp

$(OBJ)/hzAtom.o:			$(SRC)/hzAtom.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzAtom.cpp

$(OBJ)/hzChain.o:			$(SRC)/hzChain.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzChain.cpp

$(OBJ)/hzCodec.o:			$(SRC)/hzCodec.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzCodec.cpp

$(OBJ)/hzCron.o:			$(SRC)/hzCron.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzCron.cpp

$(OBJ)/hzDate.o:			$(SRC)/hzDate.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzDate.cpp

$(OBJ)/hzDelta.o:			$(SRC)/hzDelta.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzDelta.cpp

$(OBJ)/hzDirectory.o:		$(SRC)/hzDirectory.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzDirectory.cpp

$(OBJ)/hzDocHtml.o:			$(SRC)/hzDocHtml.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzDocHtml.cpp

$(OBJ)/hzDocXml.o:			$(SRC)/hzDocXml.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzDocXml.cpp

$(OBJ)/hzDomain.o:			$(SRC)/hzDomain.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzDomain.cpp

$(OBJ)/hzDNS.o:				$(SRC)/hzDNS.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzDNS.cpp

$(OBJ)/hzEmaddr.o:			$(SRC)/hzEmaddr.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzEmaddr.cpp

$(OBJ)/hzError.o:			$(SRC)/hzError.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzError.cpp

$(OBJ)/hzFtpClient.o:		$(SRC)/hzFtpClient.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzFtpClient.cpp

$(OBJ)/hzHttpClient.o:		$(SRC)/hzHttpClient.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzHttpClient.cpp

$(OBJ)/hzHttpServer.o:		$(SRC)/hzHttpServer.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzHttpServer.cpp

$(OBJ)/hzIpaddr.o:			$(SRC)/hzIpaddr.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzIpaddr.cpp

$(OBJ)/hzIdxCh.o:			$(SRC)/hzIdxCh.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzIdxCh.cpp

$(OBJ)/hzIsam.o:			$(SRC)/hzIsam.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzIsam.cpp

$(OBJ)/hzIsamT.o:			$(SRC)/hzIsamT.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzIsamT.cpp

$(OBJ)/hzIpServer.o:		$(SRC)/hzIpServer.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzIpServer.cpp

$(OBJ)/hzLock.o:			$(SRC)/hzLock.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzLock.cpp

$(OBJ)/hzLogger.o:			$(SRC)/hzLogger.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzLogger.cpp

$(OBJ)/hzMailer.o:			$(SRC)/hzMailer.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzMailer.cpp

$(OBJ)/hzMemory.o:			$(SRC)/hzMemory.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzMemory.cpp

$(OBJ)/hzNumexp.o:			$(SRC)/hzNumexp.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzNumexp.cpp

$(OBJ)/hzPop3.o:			$(SRC)/hzPop3.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzPop3.cpp

$(OBJ)/hzProcess.o:			$(SRC)/hzProcess.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzProcess.cpp

$(OBJ)/hzRegex.o:			$(SRC)/hzRegex.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzRegex.cpp

$(OBJ)/hzString.o:			$(SRC)/hzString.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzString.cpp

$(OBJ)/hzStrRepos.o:		$(SRC)/hzStrRepos.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzStrRepos.cpp

$(OBJ)/hzTcpClient.o:		$(SRC)/hzTcpClient.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzTcpClient.cpp

$(OBJ)/hzTextproc.o:		$(SRC)/hzTextproc.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzTextproc.cpp

$(OBJ)/hzTokens.o:			$(SRC)/hzTokens.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzTokens.cpp

$(OBJ)/hzTree.o:			$(SRC)/hzTree.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzTree.cpp

$(OBJ)/hzTypes.o:			$(SRC)/hzTypes.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzTypes.cpp

$(OBJ)/hzUdpClient.o:		$(SRC)/hzUdpClient.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzUdpClient.cpp

$(OBJ)/hzUrl.o:				$(SRC)/hzUrl.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzUrl.cpp

$(OBJ)/hzUnixacc.o:			$(SRC)/hzUnixacc.cpp
	$(CCMD) -o $@ $(CFLAGS) $(SRC)/hzUnixacc.cpp

include makedep

#
#	The end
#
