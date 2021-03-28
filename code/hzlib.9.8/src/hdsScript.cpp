//
//	File:	hdsScript.cpp
//
//	Legal Notice: This file is part of the HadronZoo C++ Class Library.
//
//	Copyright 1998, 2020 HadronZoo Project (http://www.hadronzoo.com)
//
//	The HadronZoo C++ Class Library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
//	as published by the Free Software Foundation, either version 3 of the License, or any later version.
//
//	The HadronZoo C++ Class Library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
//	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License along with the HadronZoo C++ Class Library. If not, see
//	http://www.gnu.org/licenses.

//
//	Dissemino HTML Generation
//

#include <iostream>
#include <fstream>

#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#include <netdb.h>
#include <signal.h>
#include <sys/stat.h>
#include <openssl/ssl.h>

#include "hzChars.h"
#include "hzString.h"
#include "hzChain.h"
#include "hzCtmpls.h"
#include "hzCodec.h"
#include "hzDate.h"
#include "hzDatabase.h"
#include "hzDocument.h"
#include "hzTextproc.h"
#include "hzHttpServer.h"
#include "hzMailer.h"
#include "hzProcess.h"
#include "hzDissemino.h"

using namespace std ;

/*
**	Standard Scripts
*/

//	JavaScript:	_dsmScript_gwp
//
//	The function gwp() or "Get Window Parameters". This is deployed by onshowpage and onresize events in the <body> tag.

hzString	_dsmScript_gwp =
	"function gwp()\n"
	"{\n"
	"\tvar w = window.innerWidth || document.documentElement.clientWidth || document.body.clientWidth;\n"
	"\tvar h = window.innerHeight || document.documentElement.clientHeight || document.body.clientHeight;\n"
	"\tvar txt;\n"
	"\th-=80;\n"
	"\ttxt=h+\"px\";\n"
	"\tdocument.getElementById(\"navlhs\").style.height=txt;\n"
	"\th-=74;\n"
	"\ttxt=h+\"px\";\n"
	"\tdocument.getElementById(\"articlecontent\").style.height=txt;\n"
	"}\n" ;

//	JavaScript:	_dsmScript_ckEmail
//
//	The function ckEmail checks if a submitted string amounts to a valid email address. Dissemino will automatically include this script in a page if it has a
//	form with a feild of type EMADDR

hzString	_dsmScript_ckEmail =
	"function ckEmaddr(str)\n"
	"{\n"
	"\tvar at=\"@\"\n"
	"\tvar dot=\".\"\n"
	"\tvar lat=str.indexOf(at)\n"
	"\tvar lstr=str.length\n"
	"\tvar ldot=str.indexOf(dot)\n\n"
	"\tif (str.indexOf(at)==-1)\t{ alert(\"Invalid E-mail No @ sign\"); return false }\n"
	"\tif (str.indexOf(at)==0)\t\t{ alert(\"Invalid E-mail Nothing to the left of the @ sign\"); return false }\n"
	"\tif (str.indexOf(at)==lstr)\t{ alert(\"Invalid E-mail Nothing to the right of the @ sign\"); return false }\n"
	"\tif (str.indexOf(dot)==-1)\t{ alert(\"Invalid E-mail No period\"); return false }\n"
	"\tif (str.indexOf(dot)==0)\t{ alert(\"Invalid E-mail Nothing to the left of the period\"); return false }\n"
	"\tif (str.indexOf(dot)==lstr)\t{ alert(\"Invalid E-mail Nothing to the right of the period\"); return false }\n\n"
	"\tif (str.indexOf(at,(lat+1))!=-1)\t\t{ alert(\"Invalid E-mail case 1\"); return false }\n"
	"\tif (str.substring(lat-1,lat)==dot)\t\t{ alert(\"Invalid E-mail case 2\"); return false }\n"
	"\tif (str.substring(lat+1,lat+2)==dot)\t{ alert(\"Invalid E-mail case 3\"); return false }\n"
	"\tif (str.indexOf(dot,(lat+2))==-1)\t\t{ alert(\"Invalid E-mail case 4\"); return false }\n"
	"\tif (str.indexOf(\" \")!=-1)\t\t\t\t{ alert(\"Invalid E-mail: No spaces permitted\"); return false }\n\n"
	"\treturn true\n"
	"}\n" ;

//	JavaScript:	_dsmScript_ckUrl
//
//	The function ckURL checks if a submitted string amounts to a valid URL. Dissemino will automatically include this script in a page if it has a form with a
//	feild of type URL

hzString	_dsmScript_ckUrl =
	"function ckUrl(url)\n"
	"{\n"
	"\tvar a= /https:\\/\\/[A-Za-z0-9\\.-]{3,}\\.[A-Za-z]{3}/""\n"
	"\tvar b= /http:\\/\\/[A-Za-z0-9\\.-]{3,}\\.[A-Za-z]{3}/""\n"
	"\tvar c= /[A-Za-z0-9\\.-]{3,}\\.[A-Za-z]{3}/""\n\n"
	"\tif (a.test(url) || b.test(url) || c.test(url))\n"
	"\t\treturn true\n"
	"\treturn false\n"
	"}\n" ;

//	JavaScript:	_dsmScript_navbarMenu2
//
//	This script sets positioning variables and contains the functions of:-
//
//		1) makeMenu		Make (compile) the menu from a data array using the document.write method
//		2) hideLast		If there is a last element selected, set it to be hidden
//		3) IEBum		Set properties of elemnet to visible/hidden for IE and other browsers
//		4) doSub		Make the submenu (drop down menu) become visible
//		5) ckmaus		Check current mouse position

hzString	_dsmScript_navbarMenu2 =
	"var lastId=-1;\n"
	"var curX;\n"
	"var curY;\n"
	"function makeMenu()\n"
	"{\n"
	"	var str=\"\";\n"
	"	var url;\n"
	"	var txt;\n"
	"	var x,y;\n"
	"	str += \"<table cellspacing='0' cellpadding='0' border='0' style='background-color:#000000;'>\";\n"
	"	str += \"<tr height='20'><td>&nbsp;</td><td>\";\n"
	"	for(x=0;x<pdmX.length;x++)\n"
	"	{\n"
	"		if(pdmX[x][2]==\"\")\n"
	"			str+=\"<span id='Pdm\" +x+ \"' style='position:relative;'>"
	"<a href='#' class='nbar' onmouseover='doSub(\" +x+ \")' onclick='return false;'>\" +pdmX[x][1]+ \" &nbsp; &nbsp;</a></span>\";\n"
	"		else\n"
	"			str+=\"<span id='Pdm\" +x+ \"'>"
	"<a href='\" + pdmX[x][2] + \"' class='nbar' onmouseover='hideLast()'>\" +pdmX[x][1]+ \" &nbsp; &nbsp;</a></span>\";\n"
	"	}\n"
	"	str += \"</td></tr></table>\";\n"
	"	for(x=0;x<pdmX.length;x++)\n"
	"	{\n"
	"		if (pdmX[x][2]==\"\")\n"
	"		{\n"
	"			str+=\"<div id='Sub\" +x+ \"' style='visibility:hidden;position:absolute;' onmouseover='IEBum(0,\" +x+ \")' onmouseout='IEBum(1,\" +x+ \")'>\";\n"
	"			str+=\"<table border='0' cellspacing='0' cellpadding='0' width='200' style='background-color:#000000;'>\"; \n"
	"			for(y=0;y<subX.length;y++)\n"
	"			{\n"
	"				if(subX[y][0] == x)\n"
	"					{str += \"<tr height='18'><td>&nbsp;<a href='\" +subX[y][2]+ \"' class='nbar' onmouseover='IEBum(0,\" +x+ \")'>\" +subX[y][1]+ \"</a></td></tr>\";}\n"
	"			}\n"
	"			str +=\"</table></div>\";\n"
	"		}\n"
	"	}\n"
	"	document.write(str);\n"
	"	str=\"\";\n"
	"}\n"
	"function hideLast()\n"
	"{\n"
	"	if (lastId!=-1)\n"
	"		document.getElementById('Sub'+lastId).style.visibility='hidden';\n"
	"	lastId=-1;\n"
	"}\n"
	"function IEBum(act,num)\n" 
	"{\n"
	"	if (act==0) { lastId=num; document.getElementById('Sub'+num).style.visibility='visible'; }\n"
	"	if (act==1) { lastId=-1; document.getElementById('Sub'+num).style.visibility='hidden'; }\n"
	"}\n"
	"function doSub(id)\n"
	"{\n"
	"	hideLast();\n"
	"	if(id==-1)\n"
	"		return\n"
	"	curY = document.getElementById('Pdm'+id).offsetTop+20;\n"
	"	curX = document.getElementById('Pdm'+id).offsetLeft;\n"
	"	document.getElementById('Sub'+id).style.top=curY+'px';\n"
	"	document.getElementById('Sub'+id).style.left=curX+'px';\n"
	"	document.getElementById('Sub'+id).style.visibility='visible';\n"
	"	lastId = id;\n"
	"}\n"
	"function ckmaus(e)\n"
	"{\n"
	"	var xx;\n"
	"	var yy;\n"
	"	if (document.all)\n"
	"		{ xx = window.event.clientX; yy = window.event.clientY; }\n"
	"	else\n"
	"		{ xx=e.pageX; yy=e.pageY; }\n"
	"	if ((xx > (curX+220)) || (xx<curX))\n"
	"		hideLast();\n"
	"}\n"
	"makeMenu();\n"
	"if (document.all)\n"
	"	document.body.onmousemove=ckmaus;\n"
	"else\n"
	"	{ window.captureEvents(Event.MOUSEMOVE); window.onmousemove=ckmaus;}\n" ;

//	JavaScript:	_dsmScript_navbarMenu3
//
//	This script sets positioning variables and contains the functions of:-
//
//		1) makeMenu		Make (compile) the menu from a data array using the document.write method
//		2) hideLast		If there is a last element selected, set it to be hidden
//		3) IEBum		Set properties of elemnet to visible/hidden for IE and other browsers
//		4) doSub		Make the submenu (drop down menu) become visible
//		5) ckmaus		Check current mouse position

hzString	_dsmScript_navbarMenu3 =
	"var lastId=-1;\n"
	"var curX;\n"
	"var curY;\n"
	"function makeMenu()\n"
	"{\n"
	"	var str=\"\";\n"
	"	var url;\n"
	"	var txt;\n"
	"	var x,y;\n"
	"	str += \"<table cellspacing='0' cellpadding='0' border='0' style='background-color:#000000;'>\";\n"
	"	str += \"<tr height='20'><td>&nbsp;</td><td>\";\n"
	"	for(x=0;x<pdmX.length;x++)\n"
	"	{\n"
	"		if(pdmX[x][2]==\"\")\n"
	"			str+=\"<span id='Pdm\" +x+ \"' style='position:relative;'>"
	"<a href='#' class='nbar' onmouseover='doSub(\" +x+ \")' onclick='return false;'>\" +pdmX[x][1]+ \" &nbsp; &nbsp;</a></span>\";\n"
	"		else\n"
	"			str+=\"<span id='Pdm\" +x+ \"'>"
	"<a href='\" + pdmX[x][2] + \"' class='nbar' onmouseover='hideLast()'>\" +pdmX[x][1]+ \" &nbsp; &nbsp;</a></span>\";\n"
	"	}\n"
	"	str += \"</td></tr></table>\";\n"
	"	for(x=0;x<pdmX.length;x++)\n"
	"	{\n"
	"		if (pdmX[x][2]==\"\")\n"
	"		{\n"
	"			str+=\"<div id='Sub\" +x+ \"' style='visibility:hidden;position:absolute;' onmouseover='IEBum(0,\" +x+ \")' onmouseout='IEBum(1,\" +x+ \")'>\";\n"
	"			str+=\"<table border='0' cellspacing='0' cellpadding='0' width='200' style='background-color:#000000;'>\"; \n"
	"			for(y=0;y<subX.length;y++)\n"
	"			{\n"
	"				if(subX[y][0] == x)\n"
	"					{str += \"<tr height='18'><td>&nbsp;<a href='\" +subX[y][2]+ \"' class='nbar' onmouseover='IEBum(0,\" +x+ \")'>\" +subX[y][1]+ \"</a></td></tr>\";}\n"
	"			}\n"
	"			str +=\"</table></div>\";\n"
	"		}\n"
	"	}\n"
	"	document.write(str);\n"
	"	str=\"\";\n"
	"}\n"
	"function hideLast()\n"
	"{\n"
	"	if (lastId!=-1)\n"
	"		document.getElementById('Sub'+lastId).style.visibility='hidden';\n"
	"	lastId=-1;\n"
	"}\n"
	"function IEBum(act,num)\n" 
	"{\n"
	"	if (act==0) { lastId=num; document.getElementById('Sub'+num).style.visibility='visible'; }\n"
	"	if (act==1) { lastId=-1; document.getElementById('Sub'+num).style.visibility='hidden'; }\n"
	"}\n"
	"function doSub(id)\n"
	"{\n"
	"	hideLast();\n"
	"	if(id==-1)\n"
	"		return\n"
	"	curY = document.getElementById('Pdm'+id).offsetTop+20;\n"
	"	curX = document.getElementById('Pdm'+id).offsetLeft;\n"
	"	document.getElementById('Sub'+id).style.top=curY+'px';\n"
	"	document.getElementById('Sub'+id).style.left=curX+'px';\n"
	"	document.getElementById('Sub'+id).style.visibility='visible';\n"
	"	lastId = id;\n"
	"}\n"
	"function ckmaus(e)\n"
	"{\n"
	"	var xx;\n"
	"	var yy;\n"
	"	if (document.all)\n"
	"		{ xx = window.event.clientX; yy = window.event.clientY; }\n"
	"	else\n"
	"		{ xx=e.pageX; yy=e.pageY; }\n"
	"	if ((xx > (curX+220)) || (xx<curX))\n"
	"		hideLast();\n"
	"}\n"
	"makeMenu();\n"
	"if (document.all)\n"
	"	document.body.onmousemove=ckmaus;\n"
	"else\n"
	"	{ window.captureEvents(Event.MOUSEMOVE); window.onmousemove=ckmaus;}\n" ;

//	JavaScript:	_dsmScript_FileMime
//
//	This script ????

const hzString	_dsmScript_FileMime =
	"var _validFileExtensions = [\".jpg\", \".jpeg\", \".bmp\", \".gif\", \".png\"];\n"
	"function Validate(oForm)\n"
	"{\n"
	"	var arrInputs = oForm.getElementsByTagName(\"input\");\n"
	"	for (var i = 0; i < arrInputs.length; i++)\n"
	"	{\n"
	"		var oInput = arrInputs[i];\n"
	"		if (oInput.type == \"file\")\n"
	"		{\n"
	"			var sFileName = oInput.value;\n"
	"			if (sFileName.length > 0)\n"
	"			{\n"
	"				var blnValid = false;\n"
	"				for (var j = 0; j < _validFileExtensions.length; j++)\n"
	"				{\n"
	"					var sCurExtension = _validFileExtensions[j];\n"
	"					if (sFileName.substr(sFileName.length - sCurExtension.length, sCurExtension.length).toLowerCase() == sCurExtension.toLowerCase())\n"
	"					{\n"
	"						blnValid = true;\n"
	"						break;\n"
	"					}\n"
	"				}\n"
	"				if (!blnValid)\n"
	"				{\n"
	"					alert(\"Sorry, \" + sFileName + \" is invalid, allowed extensions are: \" + _validFileExtensions.join(\", \"));\n"
	"					return false;\n"
	"				}\n"
	"			}\n"
	"		}\n"
	"	}\n"
	"	return true;\n"
	"}\n" ;

//	JavaScript:	_dsmScript_tog
//
//	This script is part of the navtree regime and toggles between the open and closed book images

hzString	_dsmScript_tog =
	"function tog(id,imageNode)\n"
	"{\n"
	"	var folder = document.getElementById(id);\n"
	"	if (folder == null)\n"
	"		return;\n"
	"	if (folder.style.display==\"block\")\n"
	"	{\n"
	"		if (imageNode!=null)\n"
	"			imageNode.src=\"/img/cb.png\";\n"
	"		folder.style.display=\"none\";\n"
	"	}\n"
	"	else\n"
	"	{\n"
	"		if (imageNode!=null)\n"
	"			imageNode.src=\"/img/ob.png\";\n"
	"		folder.style.display=\"block\";\n"
	"	}\n"
	"}\n" ;

//	JavaScript:	_dsmScript_loadArticle
//
//	This script is part of the Dissemino AJAX regime and performs the background server request to change the article being displayed in the host page.

hzString	_dsmScript_loadArticle =
	"function loadArticle(url)\n"
	"{\n"
	"	if (!url)\n"
	"		return false ;\n"
	"	var req;\n"
	"	var res=false;\n"
	"	if (window.XMLHttpRequest)\n"
	"		req=new XMLHttpRequest();\n"
	"	else\n"
	"		req=new ActiveXObject(\"Microsoft.XMLHTTP\");\n"
	"	req.onreadystatechange=function()\n"
	"	{\n"
	"		if (req.readyState==4)\n"
	"		{\n"
	"			if (req.status == 0 || (req.status >= 200 && req.status < 400))\n"
	"			{\n"
	"				document.getElementById('articletitle').innerHTML=req.getResponseHeader('x-title');\n"
	"				document.getElementById('articlecontent').innerHTML=req.responseText;\n"
	"				res=true;\n"
	"			}\n"
	"		}\n"
	"	}\n"
	"	req.open(\"GET\",url,false);\n"
	"	req.send(null);\n"
	"	return res;\n"
	"}\n" ;

hzString	_dsmScript_loadArticleNew =
	"function loadArticle(url,grp,art)\n"
	"{\n"
	"	if (!url)\n"
	"		return false ;\n"
	"	var req;\n"
	"	var res=false;\n"
	"	var seq=url+'?'+grp+'='+art;\n"
	"	if (window.XMLHttpRequest)\n"
	"		req=new XMLHttpRequest();\n"
	"	else\n"
	"		req=new ActiveXObject(\"Microsoft.XMLHTTP\");\n"
	"	req.onreadystatechange=function()\n"
	"	{\n"
	"		if (req.readyState==4)\n"
	"		{\n"
	"			if (req.status == 0 || (req.status >= 200 && req.status < 400))\n"
	"			{\n"
	"				document.getElementById('articletitle').innerHTML=req.getResponseHeader('x-title');\n"
	"				document.getElementById('articlecontent').innerHTML=req.responseText;\n"
	"				res=true;\n"
	"			}\n"
	"		}\n"
	"	}\n"
	"	req.open(\"GET\",seq,false);\n"
	"	req.send(null);\n"
	"	return res;\n"
	"}\n" ;

//	JavaScript:	_dsmScript_ckExists
//
//	This script is part of the Dissemino AJAX regime and performs the background server request to check if a particular value (e.g. a proposed username) is
//	already in use.

hzString	_dsmScript_ckExists =
	"function ckExists(cls,mem,val)\n"
	"{\n"
	"	var req;\n"
	"	var url;\n"
	"	var res=false;\n"
	"	url = \"ck-\" + cls + \".\" + mem + \".\" + val;\n"
	"	if (window.XMLHttpRequest)\n"
	"		req=new XMLHttpRequest();\n"
	"	else\n"
	"		req=new ActiveXObject(\"Microsoft.XMLHTTP\");\n"
	"	req.onreadystatechange=function()\n"
	"	{\n"
	"		if (req.readyState==4 && req.status==200)\n"
	"			res=true;\n"
	"	}\n"
	"	req.open(\"GET\",url,false);\n"
	"	req.send(null);\n"
	"	return res;\n"
	"}\n" ;

//	JavaScript:	_dsmScript_navtree
//
//	The 'makeTree' function renders the navigation tree visible in a page. makeTree assumes a JavaScript data array called 'ntX', with elements of five fields as follows:-
//
//		1)	Indentation level of the item in the tree.
//
//		2)	Entry type, 1 for folder and 0 for document.
//
//		3)	Default Display mode, 1 for open, 0 for closed.
//
//		4)	Item tital as it will appear in the tree.
//
//		5)	Item URL. Note that this can be blank if the tree item does not point to an article. A blank in the case of a folder is quite common but for a document rather silly. It
//			is nonetheless legal for field 2 to be a 0 (document) and for field 4 to be blank.
//
//	For example:-
//
//	<xscript name="my_manual.js">
//	var ntX=new Array();
//	ntX=[
//		[1,1,1,"My Manual","myman.0"],
//			[2,1,1,"Section 1:",""],
//				[3,0,0,"Document 1.1","myman.1.1"],
//				[3,0,0,"Document 1.2","myman.1.2"],
//			[2,1,1,"Section 2:",""],
//				[3,0,0,"Document 2.1","myman.2.1"],
//				[3,0,0,"Document 2.2","myman.2.2"],
//		];
//	</xscript>
//
//	Note that in the above, the items named 'myman' are all pre-existing articles defined with an <xtreeItem> tag. The indentation appears only for
//	human readability and has no bearing on the resulting tree. Each item has four fields as follows:-
//
//
//	The <xtreeitem> tag has two attributes of id and title and all <xtreeitem> tags are direct child tags of <xartgrp> which has attributes of name and
//	page. The name is the name of the group of articles (after all no point having a tree if there is only one article). And it is this group name
//	that is passed to makeTree().
//
//	The makeTree() function itself works by string appendage as per normal. The whole is encased in a <div> called 'f1' while all folders give rise to
//	a <div> of 'f' with an ascending number. Items within a folder, including sub-folders are held within a <ul> with each within a <li>. So a folder
//	which must contain at least one document or folder, will give an opening <div>, and because the item level is always 1 greater than that of its
//	parent folder, a <ul>, plus the <li> to hold the folder title and if applicable, the folder article reference. The <li> is then closed. The first
//	document within a folder gives an opening <ul> and has the document title and article reference within a <li> tag and anti-tag. The </ul> antitag
//	will only be generated if the level drops back. The </div> antitag will only be generated when a new folder is encountered or when levels drop
//	at the end of the whole list.

hzString	_dsmScript_navtree =
	"function makeTree(gname)\n"
	"{\n"
	"	var str=\"\";\n"
	"	var x=0;\n"
	"	var uid=1;\n"
	"	var lev=0;\n"
	"\n"
	"	str += \"<div id='f1'>\";\n"
	"	for(x=0;x<ntX.length;x++)\n"
	"	{\n"
	"		for (;ntX[x][0]<lev;lev--)\n"
	"			{ str += \"</ul></div>\"; }\n"
	"		lev=ntX[x][0];\n"
	"\n"
	"		if (ntX[x][1]==0)\n"
	"		{\n"
	"			str += \"<li class='nfld'><a href='#' class='nfld' onclick=\\\"loadArticle('?\" + gname + \"=\" + ntX[x][4] + \"');\\\">\"\n"
	"			str += \"<img src='/img/doc.png'>\" + ntX[x][3] + \"</a></li>\";\n"
	"		}\n"
	"		else\n"
	"		{\n"
	"			uid+=1;\n"
	"			if (ntX[x][2]==0)\n"
	"				str += \"<li class='nfld'><img src='/img/cb.png' onclick=\\\"tog('f\" + uid + \"',this);\\\"/>\"\n"
	"			else\n"
	"				str += \"<li class='nfld'><img src='/img/ob.png' onclick=\\\"tog('f\" + uid + \"',this);\\\"/>\"\n"
	"			if (ntX[x][4]==\"\")\n"
	"				str += \"<span class='nfld'>\" + ntX[x][3] + \"</span></li>\";\n"
	"			else\n"
	"			{\n"
	"				str += \"<a href='#' class='nfld' onclick=\\\"loadArticle('?\" + gname + \"=\" + ntX[x][4] + \"');\\\">\" + ntX[x][3] + \"</a></li>\";\n"
	"			}\n"
	"			if (ntX[x][2]==0)\n"
	"				str += \"<div id='f\" + uid + \"' style='display:none;'><ul>\";\n"
	"			else\n"
	"				str += \"<div id='f\" + uid + \"' style='display:block;'><ul>\";\n"
	"		}\n"
	"	}\n"
	"\n"
	"	for (;lev>0;lev--)\n"
	"		{ str += \"</ul></div>\"; }\n"
	"	str += \"</div>\";\n"
	"	document.write(str);\n"
	"}\n" ;

//	JavaScript:	_dsmScript_CliSess
//
//	Experimetal script to test web storage

hzString	_dsmScript_CliSess =
	"function clickCounter()\n"
	"{\n"
	"	if (typeof(Storage) !== \"undefined\")\n"
	"	{\n"
	"		if (sessionStorage.clickcount)\n"
	"			{ sessionStorage.clickcount = Number(sessionStorage.clickcount)+1; }\n"
	"		else\n"
	"			{ sessionStorage.clickcount = 1; }\n"
	"		document.getElementById(\"result\").innerHTML = \"You have clicked the button \" + sessionStorage.clickcount + \" time(s) in this session.\";\n"
	"	}\n"
	"	else\n"
	"	{\n"
	"		document.getElementById(\"result\").innerHTML = \"Sorry, your browser does not support web storage...\";\n"
	"	}\n"
	"}\n" ;

/*
**	Pull down menu driver
*/

hzEcode	hdsApp::MakeNavbarJS	(hzChain& Z, hdsLang* pLang, uint32_t access)
{
	//	Category:	HTML Generation
	//
	//	Produce a JavaScript array for the purpose of populating a navbar (navigation drop-down menu). The elements comprise the level (of the item in the menu), the page title and
	//	the page url. This format is assumed by the standard script _dsmScript_navbar which converts the array into a visible navbar. For the navbar to appear in a page, both array
	//	and script must appear within the output HML for the page, either within a <script> tag OR as a referred resources within tags of the form <script src="">.
	//
	//	This function is used by the hdsApp::SetupScripts initialization function to prepare several scripts ahead of their use in pages. There are efficiency savings in doing this
	//	but it is imperative the navbar must not contain links to pages the visitor is not permitted to access. To this end, hdsApp::SetupScripts calls this function is called once
	//	for access level PUBLIC, once for access level of ADMIN and once for each of the user classes. This produces several variant navbar scripts and the
	//	page generation process selects the script to include in the page according to the user status.
	//
	//
	//	and the resulting script stored. The page generation process then simply includes the stored script.
	//
	//
	//	Arguments:	1)	Z		The chain that will be populated with a script
	//				2)	pLang	Currently applicable language
	//				3)	access	The access level to which the script pertains
	//
	//	Returns:	E_OK	If the navigation bar script is created

	_hzfunc("hdsApp::MakeNavbarJS") ;

	hdsSubject*		pSubj ;			//	Subject
	hdsPage*		pPage ;			//	Page within subject
	uint32_t*		counts ;		//	Counts of valid pages for each subject
	hzString		langSubj ;		//	Language string (page subject)
	hzString		langTitl ;		//	Language string (page title)
	hzString		S ;				//	Temp string
	uint32_t		x ;				//	Subhect iterator
	uint32_t		y ;				//	Page iterator (within subject)
	bool			doComma ;		//	Comma control

	if (!pLang)
		pLang = m_pDfltLang ;
	if (!pLang)
		Fatal("%s. No language supplied\n", *_fn) ;
	Z.Clear() ;

	counts = new uint32_t[m_vecPgSubjects.Count()] ;

	//	Go thru the subjects and the pages under the subjects and make a copy depending on access rights of pages.
	for (x = 0 ; x < m_vecPgSubjects.Count() ; x++)
	{
		pSubj = m_vecPgSubjects[x] ;

		m_pLog->Out("%s. NOTE: Subject [%s] has %d pages\n", *_fn, *pSubj->subject, pSubj->pglist.Count()) ;

		counts[x] = 0 ;
		if (!pSubj->pglist.Count())
			continue ;

		for (y = 0 ; y < pSubj->pglist.Count() ; y++)
		{
			pPage = pSubj->pglist[y] ;

			//	Only count if the page has correct access
			if (pPage->m_Access == ACCESS_PUBLIC || access == ACCESS_ADMIN
					|| (pPage->m_Access == ACCESS_NOBODY && !access)
					|| ((access & ACCESS_MASK) & pPage->m_Access))
			{
				m_pLog->Out("For access=%08x: Page %s (subj %s), acc=%08x\n", access, *pPage->m_Title, *pPage->m_Subj, pPage->m_Access) ;
				counts[x] += 1 ;
			}
		}
	}

	Z.Clear() ;

	//	Prepare headings
	Z <<
	"var pdmX = new Array();\n"
	"pdmX = [" ;

	doComma = false ;
	for (x = 0 ; x < m_vecPgSubjects.Count() ; x++)
	{
		if (!counts[x])
			continue ;

		pSubj = m_vecPgSubjects[x] ;

		if (!pSubj->pglist.Count())
			continue ;

		//	Find value to print for subject
		if		(pLang->m_LangStrings.Exists(pSubj->m_USL))			langSubj = pLang->m_LangStrings[pSubj->m_USL] ;
		else if (m_pDfltLang->m_LangStrings.Exists(pSubj->m_USL))	langSubj = m_pDfltLang->m_LangStrings[pSubj->m_USL] ;
		else
			langSubj = pSubj->subject ;

		//	Find value to print for subject's first page
		pPage = pSubj->pglist[0] ;

		if		(pLang->m_LangStrings.Exists(pPage->m_USL))			langTitl = pLang->m_LangStrings[pPage->m_USL] ;
		else if (m_pDfltLang->m_LangStrings.Exists(pPage->m_USL))	langTitl = m_pDfltLang->m_LangStrings[pPage->m_USL] ;
		else
			langTitl = pSubj->first ;

		//	Write entry to script
		if (doComma)
			Z << ",\n" ;
			//Z.AddByte(CHAR_COMMA) ;

		if (pSubj->pglist.Count() == 1 && pSubj->first)
			Z.Printf("[%d,\"%s\",\"%s\"]", x, *langSubj, *langTitl) ;
		else
			Z.Printf("[%d,\"%s\",\"\"]", x, *langSubj) ;
		doComma = true ;
	}
	Z << "];\n" ;

	//	Prepare sub-menus
	Z <<
	"var subX = new Array();\n"
	"subX = [" ;

	doComma = false ;
	for (x = 0 ; x < m_vecPgSubjects.Count() ; x++)
	{
		if (!counts[x])
			continue ;

		pSubj = m_vecPgSubjects[x] ;

		if (!pSubj->pglist.Count())
			continue ;

		for (y = 0 ; y < pSubj->pglist.Count() ; y++)
		{
			pPage = pSubj->pglist[y] ;

			if		(pLang->m_LangStrings.Exists(pPage->m_USL))			langTitl = pLang->m_LangStrings[pPage->m_USL] ;
			else if (m_pDfltLang->m_LangStrings.Exists(pPage->m_USL))	langTitl = m_pDfltLang->m_LangStrings[pPage->m_USL] ;
			else
				langTitl = pPage->m_Title ;

			if (pPage->m_Access == ACCESS_PUBLIC || access == ACCESS_ADMIN
					|| (pPage->m_Access == ACCESS_NOBODY && !access)
					|| ((access & ACCESS_MASK) & pPage->m_Access))
			{
				if (doComma)
					Z << ",\n" ;
					//Z.AddByte(CHAR_COMMA) ;
				Z.Printf("[%d,\"%s\",\"%s\"]", x, *langTitl, *pPage->m_Url) ;
				doComma = true ;
			}
		}
	}
	Z << "];\n" ;

	delete [] counts ;
	return E_OK ;
}

void	hdsApp::SetupScripts	(void)
{
	//	This is run as part of the hdsApp initislization sequence. It adds the navbar handler script and the user dependent navbar menu scripts.
	//	These are places in m_rawScripts (the unzipped versions) and m_zipScripts (the zipped versions).
	//
	//	Note. As the navigation bar is multi-lingual, this function must be called after the language files have been imported.
	//
	//	Arguments:	None
	//	Returns:	None

	_hzfunc("hdsApp::setupScripts") ;

	hzChain		Z ;			//	Script chain
	hzChain		X ;			//	Zipped version
	hdsLang*	pLang ;		//	Language
	hdsTree*	pAG ;		//	Article group
	//hdsUsertype	ut ;		//	User types
	hzString	name ;		//	Name of script
	hzString	tmpStr ;	//	Temp string
	uint32_t	n ;			//	User type iterator
	uint32_t	x ;			//	User type iterator

	if (!this)
		hzerr(_fn, HZ_ERROR, E_CORRUPT, "No instance") ;

	/*
	**	Navigation Bars
	*/

	//	General script
	name = "navhdl.js" ;
	Z = _dsmScript_navbarMenu2 ;
	Gzip(X, Z) ;
	tmpStr = X ;
	m_rawScripts.Insert(name, _dsmScript_navbarMenu2) ;
	m_zipScripts.Insert(name, tmpStr) ;
	m_pLog->Out("%s. Created script %s of sizes %d and %d\n", *_fn, *name, Z.Size(), X.Size()) ;

	name = "navhdl3.js" ;
	Z = _dsmScript_navbarMenu3 ;
	Gzip(X, Z) ;
	tmpStr = X ;
	m_rawScripts.Insert(name, _dsmScript_navbarMenu3) ;
	m_zipScripts.Insert(name, tmpStr) ;
	m_pLog->Out("%s. Created script %s of sizes %d and %d\n", *_fn, *name, Z.Size(), X.Size()) ;

	//	Language dependent data scripts
	for (n = 0 ; n < m_Languages.Count() ; n++)
	{
		pLang = m_Languages.GetObj(n) ;

		name = "nav-public.js" ;
		MakeNavbarJS(Z, pLang, ACCESS_PUBLIC) ;
		tmpStr = Z ;
		pLang->m_rawScripts.Insert(name, tmpStr) ;
		Gzip(X, Z) ;
		tmpStr = X ;
		pLang->m_zipScripts.Insert(name, tmpStr) ;

		//	m_pLog->Out("%s. Created script %s->%s of sizes %d and %d\n[", *_fn, *pLang->m_code, *name, Z.Size(), X.Size()) ;
		//	m_pLog->Out(Z) ;
		//	m_pLog->Out("]\n") ;

		name = "nav-admin.js" ;
		MakeNavbarJS(Z, pLang, ACCESS_ADMIN) ;
		tmpStr = Z ;
		pLang->m_rawScripts.Insert(name, tmpStr) ;
		Gzip(X, Z) ;
		tmpStr = X ;
		pLang->m_zipScripts.Insert(name, tmpStr) ;

		//	m_pLog->Out("%s. Created script %s->%s of sizes %d and %d\n[", *_fn, *pLang->m_code, *name, Z.Size(), X.Size()) ;
		//	m_pLog->Out(Z) ;
		//	m_pLog->Out("]\n") ;

		for (x = 0 ; x < m_UserTypes.Count() ; x++)
		{
			tmpStr = m_UserTypes.GetKey(x) ;

			name = "nav-" + tmpStr + ".js" ;

			MakeNavbarJS(Z, pLang, m_UserTypes.GetObj(x)) ;	//ut.m_Access) ;
			tmpStr = Z ;
			pLang->m_rawScripts.Insert(name, tmpStr) ;
			Gzip(X, Z) ;
			tmpStr = X ;
			pLang->m_zipScripts.Insert(name, tmpStr) ;

			//	m_pLog->Out("%s. Created script %s->%s of sizes %d and %d\n[", *_fn, *pLang->m_code, *name, Z.Size(), X.Size()) ;
			//	m_pLog->Out(Z) ;
			//	m_pLog->Out("]\n") ;

		}
	}

	//	LANGUAGE SELECTOR

	//	Email checker
	name = "ckExists.js" ;
	Z = _dsmScript_ckExists ;
	Gzip(X, Z) ;
	tmpStr = X ;
	m_rawScripts.Insert(name, _dsmScript_ckExists) ;
	m_zipScripts.Insert(name, tmpStr) ;
	m_pLog->Out("%s. Created script ckExists.js of sizes %d and %d\n", *_fn, Z.Size(), X.Size()) ;

	/*
	**	NAVIGATION TREES
	*/

	name = "tog.js" ;
	Z = _dsmScript_tog ;
	Gzip(X, Z) ;
	tmpStr = X ;
	m_rawScripts.Insert(name, _dsmScript_tog) ;
	m_zipScripts.Insert(name, tmpStr) ;
	m_pLog->Out("%s. Created script tog.js of sizes %d and %d\n", *_fn, Z.Size(), X.Size()) ;

	name = "loadArticle.js" ;
	Z = _dsmScript_loadArticle ;
	Gzip(X, Z) ;
	tmpStr = X ;
	m_rawScripts.Insert(name, _dsmScript_loadArticle) ;
	m_zipScripts.Insert(name, tmpStr) ;
	m_pLog->Out("%s. Created script loadArticle.js of sizes %d and %d\n", *_fn, Z.Size(), X.Size()) ;

	for (n = 0 ; n < m_ArticleGroups.Count() ; n++)
	{
		pAG = m_ArticleGroups.GetObj(n) ;
		if (!pAG)
			continue ;

		name = "navtree-" ;
		name += pAG->m_Groupname ;
		name += ".js" ;
		//name = "navtree-public.js" ;
		Z = _dsmScript_navtree ;
		Z.Printf("makeTree('%s');\n", *pAG->m_Groupname) ;
		tmpStr = Z ;
		m_rawScripts.Insert(name, tmpStr) ;
		Gzip(X, Z) ;
		tmpStr = X ;
		m_zipScripts.Insert(name, tmpStr) ;
		m_pLog->Out("%s. Created script %s of sizes %d and %d\n", *_fn, *name, Z.Size(), X.Size()) ;
	}

	//	Do the admin navtree
	name = "navtree-master_dme.js" ;
	Z = _dsmScript_navtree ;
	Z << "makeTree(\"adm_dme\");\n" ;
	tmpStr = Z ;
	m_rawScripts.Insert(name, tmpStr) ;
	Gzip(X, Z) ;
	tmpStr = X ;
	m_zipScripts.Insert(name, tmpStr) ;

	//	Report
	m_pLog->Out("%s. Script Summary\n", *_fn) ;
	for (n = 0 ; n < m_Languages.Count() ; n++)
	{
		pLang = m_Languages.GetObj(n) ;

		for (x = 0 ; x < pLang->m_rawScripts.Count() ; x++)
		{
			name = pLang->m_rawScripts.GetKey(x) ;
			m_pLog->Out("%s. Lang %s: Script %s\n", *_fn, *pLang->m_code, *name) ;

			if (pLang->m_rawScripts.Exists(name))
				m_pLog->Out("%s. Lang %s: Script %s exists\n", *_fn, *pLang->m_code, *name) ;
			else
				m_pLog->Out("%s. Lang %s: Script %s not-found\n", *_fn, *pLang->m_code, *name) ;
		}
	}
}
