//
//	File:	hzDate.cpp
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

//
//	Implimentation of Dates
//

#include <iostream>

#include <sys/time.h>
#include <stdarg.h>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzErrcode.h"
#include "hzProcess.h"
#include "hzDate.h"

using namespace std ;

/*
**	Global Variables
*/

global	const char*	hz_daynames_abrv	[] =
{
	//	Abreviated day names (Default language English)
	"Mon","Tue","Wed","Thu","Fri","Sat","Sun",""
} ;

global	const char*	hz_daynames_full	[] =
{
	//	Full day names (Default language English)
	"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday",""
} ;

global	const char*	hz_monthnames_abrv	[] =
{
	//	Abreviated month names (Default language English)
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec",""
} ;

global	const char*	hz_monthnames_full	[] =
{
	//	Full month names (Default language English)
	"January","February","March","April","May","June","July","August","September","October","November","December",""
} ;

global	const	hzXDate	_hz_null_hzXDate ;		//	Null full date
global	const	hzSDate	_hz_null_hzSDate ;		//	Null short date
global	const	hzTime	_hz_null_hzTime ;		//	Null time

struct	hzTimezone
{
	//	Category:	Time
	//
	//	International Timezone. Maps timezone code to GMT offset

	const char*	code ;		//	Timezone code
	int32_t		hour ;		//	Hours ahead of GMT
	int32_t		min ;		//	Minutes ahead of GMT

	void	clear	(void)	{ code = 0 ; hour = min = 0 ; }
} ;

global	hzTimezone	_hzGlobal_Timezones	[]	=
{
	{ "ADT",	-3,	 0,	},	//	Atlantic Daylight Time			North America	UTC - 3 hours
	{ "AFT",	 4,	 0,	},	//	Afghanistan Time				Asia			UTC + 4:30 hours
	{ "AKDT",	-8,	 0,	},	//	Alaska Daylight Time			North America	UTC - 8 hours
	{ "AKST",	-9,	 0,	},	//	Alaska Std Time					North America	UTC - 9 hours
	{ "ALMT",	 6,	 0,	},	//	Alma-Ata Time					Asia			UTC + 6 hours
	{ "AMST",	 5,	 0,	},	//	Armenia Summer Time				Asia			UTC + 5 hours
	{ "AMST",	-3,	 0,	},	//	Amazon Summer Time				South America	UTC - 3 hours
	{ "AMT",	 4,	 0,	},	//	Armenia Time					Asia			UTC + 4 hours
	{ "AMT",	-4,	 0,	},	//	Amazon Time						South America	UTC - 4 hours
	{ "ANAST",	12,	 0,	},	//	Anadyr Summer Time				Asia			UTC + 12 hours
	{ "ANAT",	12,	 0,	},	//	Anadyr Time						Asia			UTC + 12 hours
	{ "AQTT",	 5,	 0,	},	//	Aqtobe Time						Asia			UTC + 5 hours
	{ "ART",	-3,	 0,	},	//	Argentina Time					South America	UTC - 3 hours
	{ "AST",	 3,	 0,	},	//	Arabia Std Time					Asia			UTC + 3 hours
	{ "AST",	-4,	 0,	},	//	Atlantic Std Time				Atlantic		UTC - 4 hours
	{ "AST",	-4,	 0,	},	//	Atlantic Std Time				Caribbean		UTC - 4 hours
	{ "AST",	-4,	 0,	},	//	Atlantic Std Time				North America	UTC - 4 hours
	{ "AZOST",	 4,	 0,	},	//	Azores Summer Time				Atlantic		UTC
	{ "AZOT",	-1,	 0,	},	//	Azores Time						Atlantic		UTC - 1 hour
	{ "AZST",	 5,	 0,	},	//	Azerbaijan Summer Time			Asia			UTC + 5 hours
	{ "AZT",	 4,	 0,	},	//	Azerbaijan Time					Asia			UTC + 4 hours
	{ "BNT",	 8,	 0,	},	//	Brunei Darussalam Time			Asia			UTC + 8 hours
	{ "BOT",	-4,	 0,	},	//	Bolivia Time					South America	UTC - 4 hours
	{ "BRST",	-2,	 0,	},	//	Brasilia Summer Time			South America	UTC - 2 hours
	{ "BRT",	-3,	 0,	},	//	Brasía time						South America	UTC - 3 hours
	{ "BST",	 6,	 0,	},	//	Bangladesh Std Time				Asia			UTC + 6 hours
	{ "BST",	 1,	 0,	},	//	British Summer Time				Europe			UTC + 1 hour
	{ "BTT",	 6,	 0,	},	//	Bhutan Time						Asia			UTC + 6 hours
	{ "CAST",	 8,	 0,	},	//	Casey Time						Antarctica		UTC + 8 hours
	{ "CAT",	 2,	 0,	},	//	Central Africa Time				Africa			UTC + 2 hours
	{ "CCT",	 6,	 0,	},	//	Cocos Islands Time				Indian Ocean	UTC + 6:30 hours
	{ "CDT",	10,	 0,	},	//	Central Daylight Time			Australia		UTC + 10:30 hours
	{ "CDT",	-4,	 0,	},	//	Cuba Daylight Time				Caribbean		UTC - 4 hours
	{ "CDT",	-5,	 0,	},	//	Central Daylight Time			North America	UTC - 5 hours
	{ "CEST",	 2,	 0,	},	//	Central European Summer Time	Europe			UTC + 2 hours
	{ "CET",	 1,	 0,	},	//	Central European Time			Africa			UTC + 1 hour
	{ "CET",	 1,	 0,	},	//	Central European Time			Europe			UTC + 1 hour
	{ "CHADT",	13,	45,	},	//	Chatham Island Daylight Time	Pacific			UTC + 13:45 hours
	{ "CHAST",	12,	45,	},	//	Chatham Island Std Time			Pacific			UTC + 12:45 hours
	{ "CKT",	-10, 0,	},	//	Cook Island Time				Pacific			UTC - 10 hours
	{ "CLST",	-3,	 0,	},	//	Chile Summer Time				South America	UTC - 3 hours
	{ "CLT",	-4,	 0,	},	//	Chile Std Time					South America	UTC - 4 hours
	{ "COT",	-5,	 0,	},	//	Colombia Time					South America	UTC - 5 hours
	{ "CST",	 8,	 0,	},	//	China Std Time					Asia			UTC + 8 hours
	{ "CST",	 9,	30,	},	//	Central Std Time				Australia		UTC + 9:30 hours
	{ "CST",	-6,	0,	},	//	Central Std Time				Central America	UTC - 6 hours
	{ "CST",	-5,	0,	},	//	Cuba Std Time					Caribbean		UTC - 5 hours
	{ "CST",	-6,	0,	},	//	Central Std Time				North America	UTC - 6 hours
	{ "CVT",	-1,	0,	},	//	Cape Verde Time					Africa			UTC - 1 hour
	{ "CXT",	 7,	0,	},	//	Christmas Island Time			Australia		UTC + 7 hours
	{ "ChST",	10,	0,	},	//	Chamorro Std Time				Pacific			UTC + 10 hours
	{ "DAVT",	 7,	0,	},	//	Davis Time						Antarctica		UTC + 7 hours
	{ "EASST",	-5,	0,	},	//	Easter Island Summer Time		Pacific			UTC - 5 hours
	{ "EAST",	-6,	0,	},	//	Easter Island Std Time			Pacific			UTC - 6 hours
	{ "EAT",	 3,	0,	},	//	Eastern Africa Time				Africa			UTC + 3 hours
	{ "EAT",	 3,	0,	},	//	East Africa Time				Indian Ocean	UTC + 3 hours
	{ "ECT",	-5,	0,	},	//	Ecuador Time					South America	UTC - 5 hours
	{ "EDT",	11,	0,	},	//	Eastern Daylight Time			Australia		UTC + 11 hours
	{ "EDT",	-4,	0,	},	//	Eastern Daylight Time			Caribbean		UTC - 4 hours
	{ "EDT",	-4,	0,	},	//	Eastern Daylight Time			North America	UTC - 4 hours
	{ "EDT",	11,	0,	},	//	Eastern Daylight Time			Pacific			UTC + 11 hours
	{ "EEST",	 3,	0,	},	//	Eastern European Summer Time	Africa			UTC + 3 hours
	{ "EEST",	 3,	0,	},	//	Eastern European Summer Time	Asia			UTC + 3 hours
	{ "EEST",	 3,	0,	},	//	Eastern European Summer Time	Europe			UTC + 3 hours
	{ "EET",	 2,	0,	},	//	Eastern European Time			Africa			UTC + 2 hours
	{ "EET",	 2,	0,	},	//	Eastern European Time			Asia			UTC + 2 hours
	{ "EET",	 2,	0,	},	//	Eastern European Time			Europe			UTC + 2 hours
	{ "EGST",	 0,	0,	},	//	East Greenland Summer Time		North America	UTC
	{ "EGT",	-1,	0,	},	//	East Greenland Time				North America	UTC - 1 hour
	{ "EST",	10,	0,	},	//	Eastern Std Time				Australia		UTC + 10 hours
	{ "EST",	-5,	0,	},	//	Eastern Std Time				Central America	UTC - 5 hours
	{ "EST",	-5,	0,	},	//	Eastern Std Time				Caribbean		UTC - 5 hours
	{ "EST",	-5,	0,	},	//	Eastern Standard Time			North America	UTC - 5 hours
	{ "ET",		-5,	0,	},	//	Tiempo del Este					Central America	UTC - 5 hours
	{ "ET",		-5,	0,	},	//	Tiempo del Este					Caribbean		UTC - 5 hours
	{ "ET",		-5,	0,	},	//	Tiempo Del Este 				North America	UTC - 5 hours
	{ "FJST",	13,	0,	},	//	Fiji Summer Time				Pacific			UTC + 13 hours
	{ "FJT",	12,	0,	},	//	Fiji Time						Pacific			UTC + 12 hours
	{ "FKST",	-3,	0,	},	//	Falkland Islands Summer Time	South America	UTC - 3 hours
	{ "FKT",	-4,	0,	},	//	Falkland Island Time			South America	UTC - 4 hours
	{ "FNT",	-2,	0,	},	//	Fernando de Noronha Time		South America	UTC - 2 hours
	{ "GALT",	-6,	0,	},	//	Galapagos Time					Pacific			UTC - 6 hours
	{ "GAMT",	-9,	0,	},	//	Gambier Time					Pacific			UTC - 9 hours
	{ "GET",	 4,	0,	},	//	Georgia Std Time				Asia			UTC + 4 hours
	{ "GFT",	0,	0,	},	//	French Guiana Time				South America	UTC - 3 hours
	{ "GILT",	0,	0,	},	//	Gilbert Island Time				Pacific			UTC + 12 hours
	{ "GMT",	0,	0,	},	//	Greenwich Mean Time				Africa			UTC
	{ "GMT",	0,	0,	},	//	Greenwich Mean Time				Europe			UTC
	{ "GST",	0,	0,	},	//	Gulf Std Time					Asia			UTC + 4 hours
	{ "GYT",	0,	0,	},	//	Guyana Time						South America	UTC - 4 hours
	{ "HAA",	0,	0,	},	//	Heure Avanc	l'Atlantique		Atlantic		UTC - 3 hours
	{ "HAA",	0,	0,	},	//	Heure Avanc l'Atlantique		North America	UTC - 3 hours
	{ "HAC",	0,	0,	},	//	Heure Avanc Centre				North America	UTC - 5 hours
	{ "HADT",	0,	0,	},	//	Hawaii-Aleutian Daylight Time	North America	UTC - 9 hours
	{ "HAE",	0,	0,	},	//	Heure Avanc l'Est 				Caribbean		UTC - 4 hours
	{ "HAE",	0,	0,	},	//	Heure Avanc l'Est 				North America	UTC - 4 hours
	{ "HAP",	0,	0,	},	//	Heure Avanc Pacifique			North America	UTC - 7 hours
	{ "HAR",	0,	0,	},	//	Heure Avanc Rocheuses			North America	UTC - 6 hours
	{ "HAST",	0,	0,	},	//	Hawaii-Aleutian Std Time		North America	UTC - 10 hours
	{ "HAT",	0,	0,	},	//	Heure Avanc Terre-Neuve			North America	UTC - 2:30 hours
	{ "HAY",	0,	0,	},	//	Heure Avanc Yukon				North America	UTC - 8 hours
	{ "HKT",	0,	0,	},	//	Hong Kong Time					Asia			UTC + 8 hours
	{ "HLV",	0,	0,	},	//	Hora Legal de Venezuela			South America	UTC - 4:30 hours
	{ "HNA",	0,	0,	},	//	Heure Normale de l'Atlantique	Atlantic		UTC - 4 hours
	{ "HNA",	0,	0,	},	//	Heure Normale de l'Atlantique	Caribbean		UTC - 4 hours
	{ "HNA",	0,	0,	},	//	Heure Normale de l'Atlantique	North America	UTC - 4 hours
	{ "HNC",	0,	0,	},	//	Heure Normale du Centre			Central America	UTC - 6 hours
	{ "HNC",	0,	0,	},	//	Heure Normale du Centre			North America	UTC - 6 hours
	{ "HNE",	0,	0,	},	//	Heure Normale de l'Est			Central America	UTC - 5 hours
	{ "HNE",	0,	0,	},	//	Heure Normale de l'Est			Caribbean		UTC - 5 hours
	{ "HNE",	0,	0,	},	//	Heure Normale de l'Est			North America	UTC - 5 hours
	{ "HNP",	0,	0,	},	//	Heure Normale du Pacifique		North America	UTC - 8 hours
	{ "HNR",	0,	0,	},	//	Heure Normale des Rocheuses		North America	UTC - 7 hours
	{ "HNT",	0,	0,	},	//	Heure Normale de Terre-Neuve	North America	UTC - 3:30 hours
	{ "HNY",	0,	0,	},	//	Heure Normale du Yukon			North America	UTC - 9 hours
	{ "HOVT",	0,	0,	},	//	Hovd Time						Asia			UTC + 7 hours
	{ "ICT",	0,	0,	},	//	Indochina Time					Asia			UTC + 7 hours
	{ "IDT",	0,	0,	},	//	Israel Daylight Time			Asia			UTC + 3 hours
	{ "IOT",	0,	0,	},	//	Indian Chagos Time				Indian Ocean	UTC + 6 hours
	{ "IRDT",	0,	0,	},	//	Iran Daylight Time				Asia			UTC + 4:30 hours
	{ "IRKST",	0,	0,	},	//	Irkutsk Summer Time				Asia			UTC + 9 hours
	{ "IRKT",	0,	0,	},	//	Irkutsk Time					Asia			UTC + 8 hours
	{ "IRST",	0,	0,	},	//	Iran Std Time					Asia			UTC + 3:30 hours
	{ "IST",	0,	0,	},	//	Israel Std Time					Asia			UTC + 2 hours
	{ "IST",	0,	0,	},	//	India Std Time					Asia			UTC + 5:30 hours
	{ "IST",	0,	0,	},	//	Irish Std Time					Europe			UTC + 1 hour
	{ "JST",	0,	0,	},	//	Japan Std Time					Asia			UTC + 9 hours
	{ "KGT",	0,	0,	},	//	Kyrgyzstan Time					Asia			UTC + 6 hours
	{ "KRAST",	0,	0,	},	//	Krasnoyarsk Summer Time			Asia			UTC + 8 hours
	{ "KRAT",	0,	0,	},	//	Krasnoyarsk Time				Asia			UTC + 7 hours
	{ "KST",	0,	0,	},	//	Korea Std Time					Asia			UTC + 9 hours
	{ "KUYT",	0,	0,	},	//	Kuybyshev Time					Europe			UTC + 4 hours
	{ "LHDT",	0,	0,	},	//	Lord Howe Daylight Time			Australia		UTC + 11 hours
	{ "LHST",	0,	0,	},	//	Lord Howe Std Time				Australia		UTC + 10:30 hours
	{ "LINT",	0,	0,	},	//	Line Islands Time				Pacific			UTC + 14 hours
	{ "MAGST",	0,	0,	},	//	Magadan Summer Time				Asia			UTC + 12 hours
	{ "MAGT",	0,	0,	},	//	Magadan Time					Asia			UTC + 11 hours
	{ "MART",	0,	0,	},	//	Marquesas Time					Pacific			UTC - 9:30 hours
	{ "MAWT",	0,	0,	},	//	Mawson Time						Antarctica		UTC + 5 hours
	{ "MDT",	0,	0,	},	//	Mountain Daylight Time			North America	UTC - 6 hours
	{ "MHT",	0,	0,	},	//	Marshall Islands Time			Pacific			UTC + 12 hours
	{ "MMT",	0,	0,	},	//	Myanmar Time					Asia			UTC + 6:30 hours
	{ "MSD",	0,	0,	},	//	Moscow Daylight Time			Europe			UTC + 4 hours
	{ "MSK",	0,	0,	},	//	Moscow Std Time					Europe			UTC + 3 hours
	{ "MST",	0,	0,	},	//	Mountain Std Time				North America	UTC - 7 hours
	{ "MUT",	0,	0,	},	//	Mauritius Time					Africa			UTC + 4 hours
	{ "MVT",	0,	0,	},	//	Maldives Time					Asia			UTC + 5 hours
	{ "MYT",	0,	0,	},	//	Malaysia Time					Asia			UTC + 8 hours
	{ "NCT",	0,	0,	},	//	New Caledonia Time				Pacific			UTC + 11 hours
	{ "NDT",	0,	0,	},	//	Newfoundland Daylight Time		North America	UTC - 2:30 hours
	{ "NFT",	0,	0,	},	//	Norfolk Time					Australia		UTC + 11:30 hours
	{ "NOVST",	0,	0,	},	//	Novosibirsk Summer Time			Asia			UTC + 7 hours
	{ "NOVT",	0,	0,	},	//	Novosibirsk Time				Asia			UTC + 6 hours
	{ "NPT",	0,	0,	},	//	Nepal Time 						Asia			UTC + 5:45 hours
	{ "NST",	0,	0,	},	//	Newfoundland Std Time			North America	UTC - 3:30 hours
	{ "NUT",	0,	0,	},	//	Niue Time						Pacific			UTC - 11 hours
	{ "NZDT",	0,	0,	},	//	New Zealand Daylight Time		Antarctica		UTC + 13 hours
	{ "NZDT",	0,	0,	},	//	New Zealand Daylight Time		Pacific			UTC + 13 hours
	{ "NZST",	0,	0,	},	//	New Zealand Std Time			Antarctica		UTC + 12 hours
	{ "NZST",	0,	0,	},	//	New Zealand Std Time			Pacific			UTC + 12 hours
	{ "OMSST",	0,	0,	},	//	Omsk Summer Time				Asia			UTC + 7 hours
	{ "OMST",	0,	0,	},	//	Omsk Std Time					Asia			UTC + 6 hours
	{ "PDT",	0,	0,	},	//	Pacific Daylight Time			North America	UTC - 7 hours
	{ "PET",	0,	0,	},	//	Peru Time						South America	UTC - 5 hours
	{ "PETST",	0,	0,	},	//	Kamchatka Summer Time			Asia			UTC + 12 hours
	{ "PETT",	0,	0,	},	//	Kamchatka Time					Asia			UTC + 12 hours
	{ "PGT",	0,	0,	},	//	Papua New Guinea Time			Pacific			UTC + 10 hours
	{ "PHOT",	0,	0,	},	//	Phoenix Island Time				Pacific			UTC + 13 hours
	{ "PHT",	0,	0,	},	//	Philippine Time					Asia			UTC + 8 hours
	{ "PKT",	0,	0,	},	//	Pakistan Std Time				Asia			UTC + 5 hours
	{ "PMDT",	0,	0,	},	//	Pierre & Miquelon Daylight Time	North America	UTC - 2 hours
	{ "PMST",	0,	0,	},	//	Pierre & Miquelon Std Time		North America	UTC - 3 hours
	{ "PONT",	0,	0,	},	//	Pohnpei Std Time				Pacific			UTC + 11 hours
	{ "PST",	0,	0,	},	//	Pacific Std Time				North America	UTC - 8 hours
	{ "PST",	0,	0,	},	//	Pitcairn Std Time				Pacific			UTC - 8 hours
	{ "PT",		0,	0,	},	//	Tiempo del Pac					North America	UTC - 8 hours
	{ "PWT",	0,	0,	},	//	Palau Time						Pacific			UTC + 9 hours
	{ "PYST",	0,	0,	},	//	Paraguay Summer Time			South America	UTC - 3 hours
	{ "PYT",	0,	0,	},	//	Paraguay Time					South America	UTC - 4 hours
	{ "RET",	0,	0,	},	//	Reunion Time					Africa			UTC + 4 hours
	{ "SAMT",	0,	0,	},	//	Samara Time						Europe			UTC + 4 hours
	{ "SAST",	0,	0,	},	//	South Africa Std Time			Africa			UTC + 2 hours
	{ "SBT",	0,	0,	},	//	Solomon IslandsTime				Pacific			UTC + 11 hours
	{ "SCT",	0,	0,	},	//	Seychelles Time					Africa			UTC + 4 hours
	{ "SGT",	0,	0,	},	//	Singapore Time					Asia			UTC + 8 hours
	{ "SRT",	0,	0,	},	//	Suriname Time					South America	UTC - 3 hours
	{ "SST",	0,	0,	},	//	Samoa Std Time					Pacific			UTC - 11 hours
	{ "TAHT",	0,	0,	},	//	Tahiti Time						Pacific			UTC - 10 hours
	{ "TFT",	0,	0,	},	//	French Southern & Antarctic		Indian Ocean	UTC + 5 hours
	{ "TJT",	0,	0,	},	//	Tajikistan Time					Asia			UTC + 5 hours
	{ "TKT",	0,	0,	},	//	Tokelau Time					Pacific			UTC - 10 hours
	{ "TLT",	0,	0,	},	//	East Timor Time					Asia			UTC + 9 hours
	{ "TMT",	0,	0,	},	//	Turkmenistan Time				Asia			UTC + 5 hours
	{ "TVT",	0,	0,	},	//	Tuvalu Time						Pacific			UTC + 12 hours
	{ "ULAT",	0,	0,	},	//	Ulaanbaatar Time				Asia			UTC + 8 hours
	{ "UYST",	0,	0,	},	//	Uruguay Summer Time				South America	UTC - 2 hours
	{ "UYT",	0,	0,	},	//	Uruguay Time					South America	UTC - 3 hours
	{ "UZT",	0,	0,	},	//	Uzbekistan Time					Asia			UTC + 5 hours
	{ "VET",	0,	0,	},	//	Venezuelan Std Time				South America	UTC - 4:30 hours
	{ "VLAST",	0,	0,	},	//	Vladivostok Summer Time			Asia			UTC + 11 hours
	{ "VLAT",	0,	0,	},	//	Vladivostok Time				Asia			UTC + 10 hours
	{ "VUT",	0,	0,	},	//	Vanuatu Time					Pacific			UTC + 11 hours
	{ "WAST",	0,	0,	},	//	West Africa Summer Time			Africa			UTC + 2 hours
	{ "WAT",	0,	0,	},	//	West Africa Time				Africa			UTC + 1 hour
	{ "WDT",	0,	0,	},	//	Western Daylight Time			Australia		UTC + 9 hours
	{ "WEST",	0,	0,	},	//	Western European Summer Time	Africa			UTC + 1 hour
	{ "WEST",	0,	0,	},	//	Western European Summer Time	Europe			UTC + 1 hour
	{ "WET",	0,	0,	},	//	Western European Time			Africa			UTC
	{ "WET",	0,	0,	},	//	Western European Time			Europe			UTC
	{ "WFT",	0,	0,	},	//	Wallis and Futuna Time			Pacific			UTC + 12 hours
	{ "WGST",	0,	0,	},	//	Western Greenland Summer Time	North America	UTC - 2 hours
	{ "WGT",	0,	0,	},	//	West Greenland Time				North America	UTC - 3 hours
	{ "WIB",	0,	0,	},	//	Western Indonesian Time			Asia			UTC + 7 hours
	{ "WIT",	0,	0,	},	//	Eastern Indonesian Time			Asia			UTC + 9 hours
	{ "WITA",	0,	0,	},	//	Central Indonesian Time			Asia			UTC + 8 hours
	{ "WST",	0,	0,	},	//	Western Sahara Summer Time		Africa			UTC + 1 hour
	{ "WST",	0,	0,	},	//	Western Std Time				Australia		UTC + 8 hours
	{ "WST",	0,	0,	},	//	West Samoa Time					Pacific			UTC - 11 hours
	{ "WT",		0,	0,	},	//	Western Sahara Std Time			Africa			UTC
	{ "YAKST",	0,	0,	},	//	Yakutsk Summer Time				Asia			UTC + 10 hours
	{ "YAKT",	0,	0,	},	//	Yakutsk Time					Asia			UTC + 9 hours
	{ "YAPT",	0,	0,	},	//	Yap Time						Pacific			UTC + 10 hours
	{ "YEKST",	0,	0,	},	//	Yekaterinburg Summer Time		Asia			UTC + 6 hours
	{ "YEKT",	0,	0,	},	//	Yekaterinburg Time				Asia			UTC + 5 hours
	{ "",		0,	0,	}	//	Null timezone
} ;

/*
**	Functions applicable to all date classes
*/

static	uint32_t	_isdow	(const char* cpStr)
{
	//	Determine if current string amounts to a day-name
	//
	//	Arguments:	1)	cpStr	The string to be tested for a valid day-name
	//
	//	Returns:	0+	If the supplied cstr is a dayname. 0xff if not.

	int32_t	nIndex ;	//	Day names iterator

	for (nIndex = 0 ; nIndex < 7 ; nIndex++)
	{
		if (!CstrCompareI(cpStr, hz_daynames_abrv[nIndex]))
			return nIndex ;
		if (!CstrCompareI(cpStr, hz_daynames_full[nIndex]))
			return nIndex ;
	}
	return NULL_DOW ;
}

static	uint32_t	_ismonth	(const char* cpStr)
{
	//	Determine if current string amounts to a month-name
	//
	//	Arguments:	1)	cpStr	The string to be tested for a valid month-name
	//
	//	Returns:	>0	If the supplied cstr is a month name
	//				0	Otherwise

	int32_t	nIndex ;	//	Month names iterator

	for (nIndex = 0 ; nIndex < 12 ; nIndex++)
	{
		if (!CstrCompareI(cpStr, hz_monthnames_abrv[nIndex]))
			return nIndex + 1 ;
		if (!CstrCompareI(cpStr, hz_monthnames_full[nIndex]))
			return nIndex + 1 ;
	}
	return 0 ;
}

//	FnGrp:		IsTime
//	Category:	Text Processing
//
//	Determines if the supplied string value constitues a time and if so, set the supplied hour, minute and seconds references
//
//	Arguments:	1)	h		Hour reference
//				2)	m		Minute reference
//				3)	s		Seconds reference
//				4)	str		String value
//
//	Returns:	>0	The number of chars used in forming the time if the supplied string amounts to one.
//				0	otherwise.

uint32_t	IsTime	(uint32_t& h, uint32_t& m, uint32_t& s, const char* cpStr)
{
	//	Argument:	4)	cpStr	String value

	const char*	i = cpStr ;		//	Input string iterator

	if (!i || i[0] == 0)
		return 0 ;

	if (IsDigit(i[0]) && IsDigit(i[1]) && IsDigit(i[2]) && IsDigit(i[3]) && IsDigit(i[4]) && IsDigit(i[5]) && i[6] <= CHAR_SPACE)
	{
		h = (10 * (i[0] - '0')) + (i[1] - '0') ;
		m = (10 * (i[2] - '0')) + (i[3] - '0') ;
		s = (10 * (i[4] - '0')) + (i[5] - '0') ;
		return (h < 24 && m < 60 && s < 60) ? 6 : 0 ;
	}

	if (IsDigit(i[0]) && IsDigit(i[1]) && i[2] == CHAR_COLON
		&& IsDigit(i[3]) && IsDigit(i[4]) && i[5] == CHAR_COLON
		&& IsDigit(i[6]) && IsDigit(i[7]) && i[8] <= CHAR_SPACE)
	{
		h = (10 * (i[0] - '0')) + (i[1] - '0') ;
		m = (10 * (i[3] - '0')) + (i[4] - '0') ;
		s = (10 * (i[6] - '0')) + (i[7] - '0') ;
		return (h < 24 && m < 60 && s < 60) ? 8 : 0 ;
	}

	return 0 ;
}

uint32_t	IsTime	(uint32_t& Y, uint32_t& M, uint32_t& D, hzString& S)
{
	//	Argument:	4)	S	String value

	return IsTime(Y, M, D, *S) ;
}

uint32_t	IsTime	(uint32_t& h, uint32_t& m, uint32_t& s, hzChain::Iter& ci)
{
	//	Argument:	4)	ci	String value

	hzChain::Iter	xi ;	//	Iterator for input chain

	uint32_t	nCount ;	//	Length of time entity
	char		buf	[12] ;	//	Test buffer

	if (ci.eof())		return 0 ;
	if (!IsDigit(*ci))	return 0 ;

	xi = ci ;

	for (nCount = 0 ; !xi.eof() && (IsDigit(*xi) || *xi == CHAR_COLON) && nCount < 8 ; nCount++, xi++)
		buf[nCount] = *xi ;
	buf[nCount] = 0 ;

	return IsTime(h, m, s, buf) ? 8 : 0 ;
}

//	FnGrp:		IsDate
//	Category:	Text Processing
//
//	Determine if the supplied string amounts to a date.
//
//	Arguments:	1)	Y		Year reference
//				2)	M		Month reference
//				3)	D		Day reference
//				4)	cpStr	The test charachter string
//
//	Returns:	>0	The number of chars used in forming the date if the supplied string does amount to one.
//				0	If the supplied string does not amount to to a valid date.

uint32_t	IsDate	(uint32_t& Y, uint32_t& M, uint32_t& D, const char* cpStr)
{
	const		char*	i = cpStr ;		//	String iterator
	uint32_t	nDigit = 0 ;			//	Number of digits encountered
	uint32_t	nSlash = 0 ;			//	Number of slashes encountered
	uint32_t	nAlpha = 0 ;			//	Number of alphas encountered
	uint32_t	nSpace = 0 ;			//	Number of spaces encountered
	uint32_t	nComma = 0 ;			//	Number of commas encountered
	uint32_t	dow = NULL_DOW ;		//	Day of week
	uint32_t	len = 0 ;				//	Test string lenght

	Y = M = D = 0 ;

	if (!i || i[0] == 0)
		return 0 ;

	//	Check standard formats first
	if (IsDigit(i[0]) && IsDigit(i[1]))
	{
		if (IsDigit(i[2]) && IsDigit(i[3]))
		{
			if (i[4] == CHAR_FWSLASH && IsDigit(i[5]) && IsDigit(i[6]) && i[7] == CHAR_FWSLASH && IsDigit(i[8]) && IsDigit(i[9])) 
			{
				Y = ((i[0]-'0')*1000) + ((i[1]-'0')*100) + ((i[2]-'0')*10) + (i[3]-'0') ;
				M = ((i[5]-'0')*10) + (i[6]-'0') ;
				D = ((i[8]-'0')*10) + (i[9]-'0') ;
				return (Y > 9999 || !M || M > 12 || !D || D > monlen(Y, M)) ? 0 : 10 ;
			}

			if (IsDigit(i[4]) && IsDigit(i[5]) && IsDigit(i[6]) && IsDigit(i[7]))
			{
				Y = ((i[0]-'0')*1000) + ((i[1]-'0')*100) + ((i[2]-'0')*10) + (i[3]-'0') ;
				M = ((i[4]-'0')*10) + (i[5]-'0') ;
				D = ((i[6]-'0')*10) + (i[7]-'0') ;
				return (Y > 9999 || !M || M > 12 || !D || D > monlen(Y, M)) ? 0 : 8 ;
			}
		}
	}

	//	Check more elaborate formats
	for (i = cpStr ; *i ; len++, i++)
	{
		if		(IsDigit(*i))			nDigit++ ;
		else if (*i == CHAR_FWSLASH)	nSlash++ ;
		else if (IsAlpha(*i))			nAlpha++ ;
		else if (*i <= ' ')				nSpace++ ;
		else if (*i <= ',')				nComma++ ;
		else
			return 0 ;
	}

	if (nDigit > 4 && nAlpha > 2 && nSpace > 1)
	{
		//	Could have a date

		for (i = cpStr ; *i ; len++, i++)
		{
			if (IsAlpha(*i))
			{
				if (dow == NULL_DOW)
					dow = _isdow(i) ;
				if (M <= 0)
					M = _ismonth(i) ;
				for (; IsAlpha(*i) ; i++) ;
			}

			if (IsDigit(*i))
			{
				if (!D)
					for (; IsDigit(*i) ; i++)	{ D *= 10 ; D += (*i - '0') ; }
				if (!Y)
					for (; IsDigit(*i) ; i++)	{ Y *= 10 ; Y += (*i - '0') ; }
			}

			if (Y && M > 0 && D > 0)
				break ;
		}
	}
	else
	{
		/*
		**	Check numeric format
		*/

		nDigit = 0 ;
		nSlash = 0 ;
		for (i = cpStr ; *i ; len++, i++)
		{
			if (*i == CHAR_FWSLASH)
				nSlash++ ;
			else if (IsDigit(*i))
				nDigit++ ;
			else
				break ;
		}

		if (*i > CHAR_SPACE && *i != CHAR_MINUS)
			return 0 ;
		i = cpStr ;

		if (nSlash == 0)
		{
			if (nDigit == 8)
			{
				Y = ((i[0]-'0')*1000) + ((i[1]-'0')*100) + ((i[2]-'0')*10) + (i[3]-'0') ;
				M = ((i[4]-'0')*10) + (i[5]-'0') ;
				D = ((i[6]-'0')*10) + (i[7]-'0') ;
			}
		}

		if (nSlash == 2)
		{
			for (i = cpStr ; IsDigit(*i) ; len++, i++)	{ D *= 10 ; D += (*i - '0') ; }
			for (i++ ; IsDigit(*i) ; len++, i++)		{ M *= 10 ; M += (*i - '0') ; }
			for (i++ ; IsDigit(*i) ; len++, i++)		{ Y *= 10 ; Y += (*i - '0') ; }
		}
	}

	return (Y > 9999 || !M || M > 12 || !D || D > monlen(Y, M)) ? 0 : len ;
}

uint32_t	IsDate	(uint32_t& Y, uint32_t& M, uint32_t& D, hzString& S)	{ return IsDate(Y, M, D, *S) ; }

//	FnGrp:		IsDateTime
//	Category:	Text Processing
//
//	Determine if the supplied string amounts to a date and time (does not have to be null terminated)
//
//	Arguments:	1)	Y	Year reference
//				2)	M	Month reference
//				3)	D	Day reference
//				4)	h	Hour reference
//				5)	m	Minute reference
//				6)	s	Seconds reference
//				7)	str	The test charachter string
//
//	Returns:	Number of chars used in forming the time if the supplied string is at the start of a time or 0 otherwise.

uint32_t	IsDateTime	(uint32_t& Y, uint32_t& M, uint32_t& D, uint32_t& h, uint32_t& m, uint32_t& s, const char* str)
{
	const char*	i = str ;		//	String iterator

	uint32_t	lenD = 0 ;		//	Length of date part
	uint32_t	lenT = 0 ;		//	Length of time part

	if (i)
	{
		lenD = IsDate(Y, M, D, i) ;
		if (lenD)
		{
			i += lenD ;
			if (*i == CHAR_SPACE || *i == CHAR_MINUS)
				i++ ;

			lenT = IsTime(h, m, s, i) ;
			if (lenT)
				return lenD + lenT + 1 ;
		}
	}

	return 0 ;
}

uint32_t	IsDateTime	(uint32_t& Y, uint32_t& M, uint32_t& D, uint32_t& h, uint32_t& m, uint32_t& s, hzString& S)
	{ return IsDateTime(Y, M, D, h, m, s, *S) ; }

hzEcode	_daysfromdate	(uint32_t& nDays, uint32_t Y, uint32_t M, uint32_t D)
{
	//	Convert calenda date into the number of days since of days since 00000101.
	//
	//	Arguments:	1)	nDays	Number of days (set by this operation)
	//				2)	Y		The given year
	//				3)	M		The given month
	//				4)	D		The given day
	//
	//	Returns:	E_RANGE	If the supplied arguments Y, M and D do not amount to a valid date
	//				E_OK	If the supplied arguments Y, M and D do form a valid date

	uint32_t	y = Y ;			//	Set to stated year and then decremented to 0
	uint32_t	m = M ;			//	Start at 1 and count up to month before M (to increment days see below)
	uint32_t	days = 0 ;		//	Days since start of calenda (0000/01/01 = 0)
	bool		leap = true ;	//	Leap year indicator

	nDays = NULL_DATE ;
	if (Y < 0 || Y > 9999)			return E_RANGE ;
	if (M < 1 || M > 12)			return E_RANGE ;
	if (D < 1 || D > monlen(Y, M))	return E_RANGE ;

	//	Consider 400 year blocks
	for (; y >= 400 ; days += DAYS_IN_4C, y -= 400) ;

	//	Consider centuries (1st of which will have extra leap year)
	if (y >= 100)
		{ days += DAYS_IN_LC ; y -= 100 ; leap = false ; }
	if (y >= 100)
	{
		for (; y >= 100 ; days += DAYS_IN_1C, y -= 100) ;
		leap = false ;
	}

	//	Consider 4 year blocks
	if (!leap)
	{
		if (y >= 4)
			{ days += 1460 ; y -= 4 ; leap = true ; }
	}
	if (y >= 4)
	{
		for (; y >= 4 ; days += DAYS_IN_4Y, y -= 4) ;
		leap = true ;
	}

	//	Consider trailing years
	if (y && leap)
		{ y-- ; days += 366 ; }
	for (; y ; days += 365, y--) ;

	//	Consider month and day
	for (m = 1 ; m < M ; m++)
		days += monlen(Y, m) ;
	days += (D - 1) ;

	nDays = days ;
	return E_OK ;
}

void	_datefromdays	(uint32_t& Y, uint32_t& M, uint32_t& D, uint32_t nDays)
{
	//	Convert the number of days since the start of the calenda, into Year, Month and Day. This is achieved by the following process
	//	1)	First divide the day number by the no of days in four centuries
	//
	//	Arguments:	1)	Y		The derived year
	//				2)	M		The derived month
	//				3)	D		The derived day
	//				4)	nDays	The number of days since the start of the calenda
	//
	//	Returns:	None

	uint32_t	d = nDays ;		//	Used to calculate date by incremental reduction of the day number
	uint32_t	len = 0 ;		//	Month length
	bool		leap ;			//	Is year derived so far a leap year?

	Y = M = D = 0 ;
	if (nDays <= 0)
		return ;
	leap = true ;

	//	Count 400 year periods
	for (d = nDays ; d >= DAYS_IN_4C ; Y += 400, d -= DAYS_IN_4C) ;

	//	Count 100 year periods (1st will have 36525 days including 25 leap years but the rest will have only 36524)
	if (d >= DAYS_IN_LC)
		{ Y += 100 ; d -= DAYS_IN_LC ; leap = false ; }
	if (d >= DAYS_IN_1C)
	{
		for (; d >= DAYS_IN_1C ; Y += 100, d -= DAYS_IN_1C) ;
		leap = false ;
	}

	//	Count 4 year periods. Note that if single centries were added the year so far will not be a leap year.
	if (!leap)
	{
		if (d >= 1460)
			{ Y += 4 ; d -= 1460 ; leap = true ; }
	}
	if (d >= DAYS_IN_4Y)
	{
		for (; d >= DAYS_IN_4Y ; Y += 4, d -= DAYS_IN_4Y) ;
		leap = true ;
	}

	//	Count 1 year periods. If year so far is a leap the first period is 366 days.
	if (!leap)
		for (; d >= 365 ; Y++, d -= 365) ;
	else
	{
		if (d >= 366)
		{
			Y++ ;
			d -= 366 ;
			//leap = false ;
			for (; d >= 365 ; Y++, d -= 365) ;
		}
	}

	//	Now sort month & days
	M = D = 1 ;
	for (;;)
	{
		len = monlen(Y, M) ;
		if (d < len)
			break ;
		d -= len ;
		M += 1 ;
	}

	D += d ;
}

uint64_t	RealtimeMicro	(void)
{
	//	Category:	Timer/Scheduling
	//
	//	Arguments:	None
	//	Returns:	Value of real time in microseconds

	struct timeval	tv ;	//	Recepticle for system time

	uint64_t	usecs ;		//	Real time in microseconds

	gettimeofday(&tv, 0) ;
	usecs = tv.tv_sec ;
	usecs *= 1000000 ;
	usecs += tv.tv_usec ;

	return usecs ;
}

uint64_t	RealtimeNano	(void)
{
	//	Category:	Timer/Scheduling
	//
	//	Arguments:	None
	//	Returns:	Value of real time in nanoseconds

	struct timespec	tx ;	//	Recepticle for system time

	uint64_t	nano ;		//	Real time in nanoseconds

	clock_gettime(CLOCK_MONOTONIC, &tx) ;
	nano = tx.tv_sec ;
	nano *= 1000000000 ;	//L ;
	nano += tx.tv_nsec ;

	return nano ;
}

/*
**	Section 2:	Time Functions
*/

void	hzTime::SysTime	(void)
{
	//	Set this hzTime instance to the system clock
	//
	//	Arguments:	None
	//	Returns:	None

	time_t		pt ;	//	Recepticle for system time
	struct tm*	t ;		//	Converted system time

	pt = time(&pt) ;
	t = localtime(&pt) ;

	m_secs = (t->tm_hour * 3600) + (t->tm_min * 60) + t->tm_sec ;
}

void	hzTime::SetTime	(const hzTime& op)
{
	//	Set this hzTime instance to the supplied hzTime value
	//
	//	Arguments:	1)	op	The hzTime operand
	//	Returns:	None

	m_secs = op.m_secs ;
}

hzEcode	hzTime::SetTime	(uint32_t h, uint32_t m, uint32_t s)
{
	//	Set this hzTime instance to time supplied as hours, minutes and seconds
	//
	//	Arguments:	1)	h	The hour component
	//				2)	m	The minute component
	//				3)	s	The seconds component
	//
	//	Returns:	E_RANGE	If the supplied hour, minute and seconds are not a valid time
	//				E_OK	If the time has been set

	if (h < 0 || h > 23)	return E_RANGE ;
	if (m < 0 || m > 59)	return E_RANGE ;
	if (s < 0 || s > 59)	return E_RANGE ;

	m_secs = (h * 3600) + (m * 60) + s ;
	return E_OK ;
}

hzEcode	hzTime::SetTime	(uint32_t nSecs)
{
	//	Set this hzTime instance using the number of seconds since midnight.
	//
	//	Arguments:	1)	nSecs	Number of seconds since midnight
	//
	//	Returns:	E_RANGE	If the supplied number of seconds exceeds the number in a day
	//				E_OK	If the time has been set

	if (nSecs < 0 || nSecs > 86399)
		return E_RANGE ;
	m_secs = nSecs ;
	return E_OK ;
}

hzEcode	hzTime::SetTime	(const char* cpTimeStr)
{
	//	Set this hzTime using a string representation of the time. Allowed formats are hh:mm:ss, hh:mm and hhmmss
	//
	//	Arguments:	1)	cpTimeStr	Time char string
	//
	//	Returns:	E_ARGUMENT	If the time cstr is not supplied or blank
	//				E_BADVALUE	If the supplied cstr is not a valid time
	//				E_OK		If the time is set

	const char*	i = cpTimeStr ;		//	Time string iterator
	uint32_t	h ;					//	Hours
	uint32_t	m ;					//	Minutes
	uint32_t	s ;					//	Seconds

	if (!i || !i[0])
		return E_ARGUMENT ;

	if (!strcmp(cpTimeStr, "Not set"))
	{
		m_secs = NULL_TIME ;
		return E_OK ;
	}

	if (i[2] == ':' && i[5] == ':')
	{
		h  = ((i[0] - CHAR_0) * 10) ;
		h +=  (i[1] - CHAR_0) ;
		m  = ((i[3] - CHAR_0) * 10) ;
		m +=  (i[4] - CHAR_0) ;
		s  = ((i[6] - CHAR_0) * 10) ;
		s +=  (i[7] - CHAR_0) ;
	}
	else if (i[2] == ':' && i[5] == 0)
	{
		h  = ((i[0] - CHAR_0) * 10) ;
		h +=  (i[1] - CHAR_0) ;
		m  = ((i[3] - CHAR_0) * 10) ;
		m +=  (i[4] - CHAR_0) ;
		s = 0 ;
	}
	else
	{
		h  = ((i[0] - CHAR_0) * 10) ;
		h +=  (i[1] - CHAR_0) ;
		m  = ((i[2] - CHAR_0) * 10) ;
		m +=  (i[3] - CHAR_0) ;
		s  = ((i[4] - CHAR_0) * 10) ;
		s +=  (i[5] - CHAR_0) ;
	}

	if (h < 0 || h > 23)	return E_BADVALUE ;
	if (m < 0 || m > 59)	return E_BADVALUE ;
	if (s < 0 || s > 59)	return E_BADVALUE ;

	m_secs = (h * 3600) + (m * 60) + s ;
	return E_OK ;
}

hzString  hzTime::Str  (hzDateFmt eFmt) const
{
	//	Create a hzString and populate it as a string of the supplied format. Return the hzString by value.
	//
	//	Arguments:	1)	eFmt	Requested format of output string
	//
	//	Returns:	Instance of hzString by value being the time in text form.

	hzString	v ;				//	Output string (Time)
	char		buf	[32] ;		//	Working buffer

	if (m_secs == NULL_TIME)
		strcpy(buf, "Not set") ;
	else
	{
		if (eFmt & FMT_TIME_DFLT)
			sprintf(buf, "%02d%02d%02d", Hour(), Min(), Sec()) ;
		else if (eFmt & FMT_TIME_STD)
			sprintf(buf, "%02d:%02d:%02d", Hour(), Min(), Sec()) ;
		else
			strcpy(buf, "Invalid Time Format") ;
	}

	v = buf ;
	return v ;
}

const char*  hzTime::Txt  (hzRecep16& r, hzDateFmt eFmt) const
{
	//	Create a hzString and populate it as a string of the supplied format. Return the hzString by value.
	//
	//	Arguments:	1)	eFmt	Requested format of output string
	//
	//	Returns:	Instance of hzString by value being the time in text form.

	if (m_secs == NULL_TIME)
		strcpy(r.m_buf, "Not set") ;
	else
	{
		if (eFmt & FMT_TIME_DFLT)
			sprintf(r.m_buf, "%02d%02d%02d", Hour(), Min(), Sec()) ;
		else if (eFmt & FMT_TIME_STD)
			sprintf(r.m_buf, "%02d:%02d:%02d", Hour(), Min(), Sec()) ;
		else
			strcpy(r.m_buf, "Invalid Time") ;
	}

	return r.m_buf ;
}

const char*  hzTime::Txt  (hzRecep32& r, hzDateFmt eFmt) const
{
	//	Create a hzString and populate it as a string of the supplied format. Return the hzString by value.
	//
	//	Arguments:	1)	eFmt	Requested format of output string
	//
	//	Returns:	Instance of hzString by value being the time in text form.

	if (m_secs == NULL_TIME)
		strcpy(r.m_buf, "Not set") ;
	else
	{
		if (eFmt & FMT_TIME_DFLT)
			sprintf(r.m_buf, "%02d%02d%02d", Hour(), Min(), Sec()) ;
		else if (eFmt & FMT_TIME_STD)
			sprintf(r.m_buf, "%02d:%02d:%02d", Hour(), Min(), Sec()) ;
		else
			strcpy(r.m_buf, "Invalid Time") ;
	}

	return r.m_buf ;
}

std::ostream&	operator<<	(std::ostream& os, const hzTime& t)
{
	//	Category:	Data Output
	//
	//	Append an outgoing stream with the time formatted as hh:mm:ss
	//
	//	Arguments:	1)	os	The output stream
	//				2)	t	The hzTime (const)

	char	buf	[12] ;	//	Working buffer

	sprintf(buf, "%02d:%02d:%02d", t.Hour(), t.Min(), t.Sec()) ;
	os << buf ;
	return os ;
}

/*
**	Section 3:	Short Date Functions
*/

void	hzSDate::SysDate	(void)
{
	//	Set a short date by the system clock
	//
	//	Arguments:	None
	//	Returns:	None

	time_t		pt ;	//	Recepticle for system time
	struct tm*	t ;		//	Converted system time

	pt = time(&pt) ;
	t = localtime(&pt) ;

	_daysfromdate(m_days, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday) ;
}

hzEcode	hzSDate::SetDate	(const hzSDate& op)
{
	//	Set the short date to the supplied short date operand
	//
	//	Arguments:	1)	op	The operant short form date
	//
	//	Returns:	E_OK

	m_days = op.m_days ;
	return E_OK ;
}

hzEcode	hzSDate::SetDate	(uint32_t Y, uint32_t M, uint32_t D)
{
	//	Set the short date by three supplied integers of year, month and day
	//
	//	Arguments:	1)	Y	Year
	//				2)	M	Month
	//				3)	D	Day
	//
	//	Returns:	E_RANGE	If the supplied arguments Y, M and D do not amount to a valid date
	//				E_OK	If the supplied arguments Y, M and D do form a valid date

	return _daysfromdate(m_days, Y, M, D) ;
}

hzEcode	hzSDate::SetDate	(uint32_t nDays)
{
	//	Set the short date by the number of days since the start of the calenda (Jan 1st 0000)
	//
	//	Arguments:	1)	nDays	Number of days
	//
	//	Returns:	E_RANGE	If the supplied arguments Y, M and D do not amount to a valid date
	//				E_OK	If the supplied arguments Y, M and D do form a valid date

	if (nDays < 0 || nDays > DAYS_IN_10K)
		return E_RANGE ;
	m_days = nDays ;
	return E_OK ;
}

hzEcode	hzSDate::SetDate	(const char* cpDateStr)
{
	//	Set the short date by a text string of an acceptable format
	//
	//	Arguments:	1)	cpDateStr	Date as string
	//
	//	Returns:	E_RANGE	If the supplied arguments Y, M and D do not amount to a valid date
	//				E_OK	If the supplied arguments Y, M and D do form a valid date

	uint32_t	Y ;		//	Year part
	uint32_t	M ;		//	Month part
	uint32_t	D ;		//	Day part
	hzEcode		rc ;	//	Return code

	if (!cpDateStr || !cpDateStr[0])
		{ m_days = NULL_DATE ; return E_OK ; }

	if (!IsDate(Y, M, D, cpDateStr))
	{
		m_days = NULL_DATE ;
		return E_FORMAT ;
	}

	return _daysfromdate(m_days, Y, M, D) ;
}

void	hzSDate::SetByEpoch	(uint32_t nEpochTime)
{
	//	Set the short date using a epoch time
	//
	//	Arguments:	1)	nEpochTime	Epoch time
	//	Returns:	None

	time_t		pt ;	//	Recepticle for system time
	struct tm*	t ;		//	Converted system time

	pt = nEpochTime ;
	t = localtime(&pt) ;

	_daysfromdate(m_days, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday) ;
}

uint32_t	hzSDate::Year	(void) const
{
	//	Provide the year value of this date
	//
	//	Arguments:	None
	//	Returns:	Number being the year value of this date

	uint32_t	Y ;		//	Year part
	uint32_t	M ;		//	Month part
	uint32_t	D ;		//	Day part

	_datefromdays(Y, M, D, m_days) ;
	return Y ;
}

uint32_t	hzSDate::Month	(void) const
{
	//	Provide the month value of this date
	//
	//	Arguments:	None
	//	Returns:	Number being the month value of this date

	uint32_t	Y ;		//	Year part
	uint32_t	M ;		//	Month part
	uint32_t	D ;		//	Day part

	_datefromdays(Y, M, D, m_days) ;
	return M ;
}

uint32_t	hzSDate::Day	(void) const
{
	//	Provide the day value of this date
	//
	//	Arguments:	None
	//	Returns:	Number being the day value of this date

	uint32_t	Y ;		//	Year part
	uint32_t	M ;		//	Month part
	uint32_t	D ;		//	Day part

	_datefromdays(Y, M, D, m_days) ;
	return D ;
}

hzString  hzSDate::Str  (hzDateFmt eFmt) const
{
	//	Produce text form of short form date in one of the following formats:-
	//
	//		1)	As YYYYMMDD
	//		2)	As dayname, monthname day year (American)
	//		3)	As dayname, day monthname year (International)
	//
	//	Arguments:	1)	eFmt	Output format
	//
	//	Returns:	Instance of hzString by value being text form of date.

	hzString	v ;			//	Result string
	uint32_t	Y ;			//	Year part
	uint32_t	M ;			//	Month part
	uint32_t	D ;			//	Day part
	char		buf[32] ;	//	Working buffer

	if (m_days == NULL_DATE)
		strcpy(buf, "Not set") ;
	else
	{
		_datefromdays(Y, M, D, m_days) ;

		switch	(eFmt)
		{
		case FMT_DATE_DFLT:	//	yyyy/mm/dd
			sprintf(buf, "%04d%02d%02d", Y, M, D) ;
			break ;

		case FMT_DATE_STD:	//	Dow, day month year
			if (eFmt & FMT_DATE_USA)
				sprintf(buf, "%s %s %d %04d", hz_daynames_abrv[Dow()], hz_monthnames_abrv[M - 1], D, Y) ;
			else
				sprintf(buf, "%s %d %s %04d", hz_daynames_abrv[Dow()], D, hz_monthnames_abrv[M - 1], Y) ;
			break ;

		default:
			sprintf(buf, "Format unavailable") ;
			break ;
		}
	}

	v = buf ;
	return v ;
}

const char*	hzSDate::Txt  (hzRecep32& r, hzDateFmt eFmt) const
{
	//	Produce text form of short form date in one of the following formats:-
	//
	//		1)	As YYYYMMDD
	//		2)	As dayname, monthname day year (American)
	//		3)	As dayname, day monthname year (International)
	//
	//	Arguments:	1)	eFmt	Output format
	//
	//	Returns:	Instance of hzString by value being text form of date.

	uint32_t	Y ;			//	Year part
	uint32_t	M ;			//	Month part
	uint32_t	D ;			//	Day part

	if (m_days == NULL_DATE)
		strcpy(r.m_buf, "Not set") ;
	else
	{
		_datefromdays(Y, M, D, m_days) ;

		switch	(eFmt)
		{
		case FMT_DATE_DFLT:	//	yyyy/mm/dd
			sprintf(r.m_buf, "%04d%02d%02d", Y, M, D) ;
			break ;

		case FMT_DATE_STD:	//	Dow, day month year
			if (eFmt & FMT_DATE_USA)
				sprintf(r.m_buf, "%s %s %d %04d", hz_daynames_abrv[Dow()], hz_monthnames_abrv[M - 1], D, Y) ;
			else
				sprintf(r.m_buf, "%s %d %s %04d", hz_daynames_abrv[Dow()], D, hz_monthnames_abrv[M - 1], Y) ;
			break ;

		default:
			sprintf(r.m_buf, "Format unavailable") ;
			break ;
		}
	}

	return r.m_buf ;
}

hzEcode	hzSDate::AltDay		(int32_t interval)
{
	//	Alter this short date by a number of days into future/past. The changed value will not be assumed if it amounts to an out of range date.
	//
	//	Arguments:	1)	interval	Number of days into future/past
	//
	//	Returns:	E_RANGE	If the supplied interval rendered this date invalid
	//				E_OK	If the supplied interval was applied

	if (interval < 0 && (m_days + interval) < 0)
		return E_RANGE ;
	if ((m_days + interval) > DAYS_IN_10K)
		return E_RANGE ;

	m_days += interval ;
	return E_OK ;
}

hzEcode	hzSDate::AltMonth	(int32_t interval)
{
	//	Alter this short date by a number of months into future/past. The changed value will not be assumed if it amounts to an out of range
	//	date.
	//
	//	Arguments:	1)	interval	Number of months into future/past
	//
	//	Returns:	E_RANGE	If the supplied interval rendered this date invalid
	//				E_OK	If the supplied interval was applied

	uint32_t	Y ;			//	Year part
	uint32_t	M ;			//	Month part
	uint32_t	D ;			//	Day part
	int32_t		months ;	//	Total months

	if (!interval)
		return E_OK ;

	_datefromdays(Y, M, D, m_days) ;

	months = (Y * 12) + M ;
	months += interval ;

	if (months < 0 || months >= 120000)
		return E_RANGE ;

	Y = months / 12 ;
	M = months % 12 ;

	return _daysfromdate(m_days, Y, M, D) ;
}

hzEcode	hzSDate::AltYear	(int32_t interval)
{
	//	Alter this short date by a number of years into future/past. The changed value will not be assumed if it amounts to an out of range
	//	date.
	//
	//	Arguments:	1)	x	Number of years into future/past
	//
	//	Returns:	E_RANGE	If the supplied interval rendered this date invalid
	//				E_OK	If the supplied interval was applied

	uint32_t	Y ;			//	Year part
	uint32_t	M ;			//	Month part
	uint32_t	D ;			//	Day part

	if (interval < 0 && (m_days + interval) < 0)
		return E_RANGE ;
	if ((m_days + interval) > DAYS_IN_10K)
		return E_RANGE ;

	_datefromdays(Y, M, D, m_days) ;
	Y += interval ;
	return _daysfromdate(m_days, Y, M, D) ;
}

const hzSDate&	hzSDate::operator=  (const hzXDate& op)
{
	//	Set a short date to the date component of a full date/time.
	//
	//	Arguments:	1)	op	Operand date
	//
	//	Returns:	Reference to this hzSDate instance

	m_days = op.NoDays() ;
	return *this ;
}

bool	hzSDate::operator==	(const hzXDate& op) const
{
	//	Test if a short date is 'equal' to a long date (ignoring the time in the long date)
	//
	//	Arguments:	1)	op	Operand date
	//
	//	Returns:	True	If this short form date is equal to the date part of the supplied full date
	//				False	Otherwise

	return m_days == op.NoDays() ? true : false ;
}

std::ostream &	operator<<	(std::ostream &os, const hzSDate &d)
{
	//	Category:	Data Output
	//
	//	Stream out the textual manefestation of a short date to s stream
	//
	//	Arguments:	1)	os	Output stream
	//				2)	d	Short form date to output
	//
	//	Returns:	Reference to the supplied output stream

	char	buf[12] ;	//	Date formation buffer

	sprintf(buf, "%04d/%02d/%02d", d.Year(), d.Month(), d.Day()) ;
	os << buf ;
	return os ;
}

/*
**	Section 4:	Full Date Functions
*/

void	hzXDate::altsec		(int32_t units)
{
	//	Advance or retard a hzXDate by a number of seconds. This will alter the day date if the alteration is sufficient to cross one or more
	//	midnight boundaries.
	//
	//	Arguments:	1)	nounits	Number of units (seconds) into future/past
	//
	//	Returns:	None

	int64_t	S ;		//	No of full seconds
	int64_t	X ;		//	No of micro-seconds

	m_hour += (units/3600) ;
	units %= 3600 ;
	S = m_usec ;
	X = units ;
	X *= 1000000 ;
	S += X ;
	if (S > 3600000000)
		{ S -= 3600000000 ; m_hour++ ; }
	m_usec = (uint32_t) S ;
}

void	hzXDate::altmin	(int32_t nounits)
{
	//	Advance or retard a hzXDate by a number of minutes. This will alter the day date if the alteration is sufficient to cross one or more
	//	midnight boundaries.
	//
	//	Arguments:	1)	nounits	Number of units (minutes) into future/past
	//	Returns:	None

	altsec(60 * nounits) ;
}

void	hzXDate::althour	(int32_t nounits)
{
	//	Advance or retard a hzXDate by a number of hours. This will alter the day date if the alteration is sufficient to cross one or more
	//	midnight boundaries.
	//
	//	Arguments:	1)	nounits	Number of units (hours) into future/past
	//	Returns:	None

	altsec(3600 * nounits) ;
}

void	hzXDate::altday	(int32_t nounits)
{
	//	Advance or retard a hzXDate by a number of days.
	//
	//	Arguments:	1)	nounits	Number of units (days) into future/past
	//	Returns:	None

	m_hour += (nounits * 24) ;
}

void	hzXDate::altyear	(int32_t nounits)
{
	//	Advance or retard a hzXDate by a number of years.
	//
	//	Arguments:	1)	nounits	Number of units (years) into future/past
	//	Returns:	None

	uint32_t	days ;		//	Used by date - no of days conversion
	uint32_t	Y ;			//	Year part
	uint32_t	M ;			//	Month part
	uint32_t	D ;			//	Day part

	days = m_hour/24 ;

	_datefromdays(Y, M, D, days) ;
	Y += nounits ;
	_daysfromdate(days, Y, M, D) ;

	days *= 24 ;
	days += (m_hour%24) ;
	m_hour = days ;
}

void	hzXDate::altmon	(int32_t nounits)
{
	//	Advance or retard a hzXDate by a number of months.
	//
	//	Arguments:	1)	nounits	Number of units (months) into future/past
	//	Returns:	None

	uint32_t	days ;		//	Used by date - no of days conversion
	uint32_t	Y ;			//	Year part
	uint32_t	M ;			//	Month part
	uint32_t	D ;			//	Day part

	days = m_hour/24 ;
	_datefromdays(Y, M, D, days) ;

	if (nounits > 0)
	{
		for (; nounits > 0 ; nounits--)
		{
			if (nounits > 12)
				{ Y++ ; nounits -= 11 ; continue ; }

			M = M == 12 ? 1 : M + 1 ;
		}

		if (D > monlen(Y, M))
			D = monlen(Y, M) ;
	}

	if (nounits < 0)
	{
		for (; nounits < 0 ; nounits++)
		{
			if (nounits < -12)
				{ Y-- ; nounits += 11 ; continue ; }

			M = M == 1 ? 12 : M - 1 ;
		}

		if (D > monlen(Y, M))
			D = monlen(Y, M) ;
	}
	_daysfromdate(days, Y, M, D) ;

	days *= 24 ;
	days += (m_hour%24) ;
	m_hour = days ;
}

//	public functions

void	hzXDate::SysDateTime	(void)
{
	//	Set this hzXDate instance by the system clock
	//
	//	Arguments:	None
	//	Returns:	None

	struct timeval	tv ;		//	Recepticle for system time
	struct tm*		t ;			//	Converted system time
	time_t			pt ;		//	Time recepticle
	uint32_t		days ;		//	Used by date - no of days conversion

	gettimeofday(&tv, 0) ;
	pt = tv.tv_sec ;
	t = localtime(&pt) ;

	_daysfromdate(days, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday) ;

	m_hour = (days * 24) + t->tm_hour ;
	m_usec = (t->tm_min * 60) + t->tm_sec ;
	m_usec *= 1000000 ;
	m_usec += tv.tv_usec ;
}

hzEcode	hzXDate::SetDate	(const char* cpDateStr)
{
	//	Set the date component of this hzXDate instance by a text string of an acceptable format
	//
	//	Arguments:	1)	cpDateStr	Date as string
	//
	//	Returns:	E_FORMAT	If the supplied cstr does not amount to a full date and time
	//				E_OK		If this full date and time was set

	uint32_t	days ;		//	Used by date - no of days conversion
	uint32_t	Y ;			//	Year part
	uint32_t	M ;			//	Month part
	uint32_t	D ;			//	Day part
	hzEcode		rc ;		//	Return code

	if (!cpDateStr || !cpDateStr[0])
		{ m_hour = 0 ; m_usec = 0 ; return E_OK ; }

	if (!strcmp(cpDateStr, "Not set"))
		{ m_hour = 0 ; m_usec = 0 ; return E_OK ; }

	if (!IsDate(Y, M, D, cpDateStr))
		return E_FORMAT ;

	rc = _daysfromdate(days, Y, M, D) ;
	if (rc != E_OK)
		return rc ;

	//Y = m_hour % 24 ;
	m_hour = (days * 24) ;
	//m_hour += Y ;
	return E_OK ;
}

hzEcode	hzXDate::SetDateTime	(const char* i)
{
	//	Set both the date and time components of this hzXDate instance by a text string of an acceptable format
	//
	//	Arguments:	1)	cpDateStr	Time and date as string
	//
	//	Returns:	E_FORMAT	If the supplied cstr does not amount to a full date and time
	//				E_OK		If this full date and time was set

	hzTimezone	tz ;				//	Timezone
	uint32_t	dow = NULL_DOW ;	//	Doy of week
	uint32_t	len = 0 ;			//	Length of examined text
	uint32_t	Y = 0 ;				//	Year
	uint32_t	M = 0 ;				//	Month
	uint32_t	D = 0 ;				//	Day
	uint32_t	h = 0 ;				//	Hour
	uint32_t	m = 0 ;				//	Minute
	uint32_t	s = 0 ;				//	Second
	uint32_t	n ;					//	Counter
	uint32_t	hOset = 0 ;			//	Hours in timezone offset to GMT
	uint32_t	mOset = 0 ;			//	Minutes in timezone offset to GMT
	char		buf	[4] ;			//	For reading numbers

	if (!i || !i[0])
		{ m_hour = 0 ; m_usec = 0 ; return E_OK ; }
	tz.clear() ;

	n = IsDateTime(Y, M, D, h, m, s, i) ;
	if (!n)
	{
		Y = M = D = 0 ;

		for (; *i ;)
		{
			if (*i < CHAR_SPACE)
				break ;

			if (*i == CHAR_SPACE || *i == CHAR_COMMA || *i == CHAR_PAROPEN || *i == CHAR_PARCLOSE)
				{ len++ ; i++ ; continue ; }

			if (IsDigit(*i))
			{
				//	We are looking for a day of the month, a year or the start of a time sequence
				n = IsTime(h, m, s, i) ;
				if (n)
					i += n ;
				else
				{
					if (IsPosint(n, i))
					{
						if		(n < 32 && D == 0)		D = n ;
						else if (n < 10000 && Y == 0)	Y = n ;
						else
							return E_FORMAT ;

						for (; IsDigit(*i) ; i++) ;
					}
				}
			}

			if (IsAlpha(*i))
			{
				//	We are looking for a day name, a month name or a timezone code

				if (dow == NULL_DOW)
				{
					for (n = 0 ; n < 7 ; n++)
						if (!CstrCompareI(i, hz_daynames_full[n]))
							{ dow = n ; break ; }

					if (dow == NULL_DOW)
					{
						for (n = 0 ; n < 7 ; n++)
							if (!CstrCompareI(i, hz_daynames_abrv[n]))
								{ dow = n ; break ; }
					}

					if (dow != 8)
						{ for (; IsAlpha(*i) ; i++) ; continue ; }
				}

				if (M == 0)
				{
					for (n = 0 ; n < 12 ; n++)
						if (!CstrCompareI(i, hz_monthnames_full[n]))
							{ M = n + 1 ; break ; }

					if (M == 0)
					{
						for (n = 0 ; n < 12 ; n++)
							if (!CstrCompareI(i, hz_monthnames_abrv[n]))
								{ M = n + 1 ; break ; }
					}

					if (M >= 0)
						{ for (; IsAlpha(*i) ; i++) ; continue ; }
				}

				if (!tz.code)
				{
					for (n = 0 ; _hzGlobal_Timezones[n].code ; n++)
					{
						if (!CstrCompareI(i, _hzGlobal_Timezones[n].code))
							{ tz = _hzGlobal_Timezones[n] ; break ; }
					}

					if (tz.code)
						{ for (; IsAlpha(*i) ; i++) ; continue ; }
				}

				return E_FORMAT ;
			}

			if (*i == CHAR_PLUS || *i == CHAR_MINUS)
			{
				//	We are looking for a timezone offset to GMT (of the form +/-dddd)

				i++ ;	buf[0] = *i ;
				i++ ;	buf[1] = *i ;
				i++ ;	buf[2] = *i ;
				i++ ;	buf[3] = *i ;
	
				if (IsDigit(buf[0]) && IsDigit(buf[1]) && IsDigit(buf[2]) && IsDigit(buf[3]))
				{
					hOset = ((buf[0] - '0') * 10) + (buf[1] - '0') ;
					mOset = ((buf[2] - '0') * 10) + (buf[3] - '0') ;

					if (hOset > 23 || mOset > 59)
						return E_FORMAT ;
					i++ ;
					continue ;
				}

				return E_FORMAT ;
			}
		}
	}

	if (SetDate(Y, M, D) != E_OK)	return E_FORMAT ;
	if (SetTime(h, m, s) != E_OK)	return E_FORMAT ;

	return E_OK ;
}

hzEcode	hzXDate::SetDate	(uint32_t Y, uint32_t M, uint32_t D)
{
	//	Set the date component of this hzXDate instance by three numeric arguments of Y, M and D
	//
	//	Arguments:	1)	Y	Year
	//				2)	M	Month
	//				3)	D	Day
	//
	//	Returns:	E_RANGE	If the supplied year, month and day does not amount to a valid date
	//				E_OK	If this date was set

	uint32_t	hour ;		//	Current hour
	uint32_t	days ;		//	No of days since 0000/01/01
	hzEcode		rc ;		//	Return code

	if (m_hour == 0)
		hour = 0 ;
	else
		hour = m_hour % 24 ;

	rc = _daysfromdate(days, Y, M, D) ;
	if (rc != E_OK)
		{ m_hour = 0 ; m_usec = 0 ; return rc ; }

	m_hour = (days * 24) + hour ;
	return E_OK ;
}

hzEcode	hzXDate::SetDate	(uint32_t nDays, uint32_t nSecs)
{
	//	Set the date and time components of this hzXDate as two numeric arguments, the number of days since Jan 1 0000 and the number of seconds
	//	since midnight.
	//
	//	Arguments:	1)	nDays	Total days since Jan 1 0000
	//				2)	nSecs	Seconds since midnight
	//
	//	Returns:	E_RANGE	If the supplied day count equals or exceeds the number of days in 10,000 years or the seconds exceed number in a day
	//				E_OK	If the time is set

	if (nDays < 0 || nDays > DAYS_IN_10K)	return E_RANGE ;
	if (nSecs < 0 || nSecs > SECS_IN_DAY)	return E_RANGE ;

	m_hour = (nDays * 24) + (nSecs/3600) ;
	m_usec = (nSecs%3600) * 1000000 ;
	return E_OK ;
}

hzEcode	hzXDate::SetDate	(uint64_t xdVal)
{
	//	Set the date and time components of this hzXDate by a single 64-bit value. This function is provided in order to retrieve dates from repositories in the
	//	HadronZoo Database Suite which stores full dates as 64 bit unsigned numbers.
	//
	//	Argument:	The date as a 64 bit value
	//
	//	Returns:	E_RANGE	If the supplied day count equals or exceeds the number of days in 10,000 years or the seconds exceed number in a day
	//				E_OK	If the time is set

	m_usec = xdVal & 0xffffffff ;
	m_hour = xdVal << 32 ;

	//	if (nDays < 0 || nDays > DAYS_IN_10K)	return E_RANGE ;
	//	if (nSecs < 0 || nSecs > SECS_IN_DAY)	return E_RANGE ;

	//	m_hour = (nDays * 24) + (nSecs/3600) ;
	//	m_usec = (nSecs%3600) * 1000000 ;
	return E_OK ;
}

hzEcode	hzXDate::SetDate	(uint32_t nDays)
{
	//	Set the date component of this hzXDate instance by number of days since Jan 1 0000
	//
	//	Arguments:	1)	nDays	Total days since Jan 1 0000
	//
	//	Returns:	E_RANGE	If the supplied day count equals or exceeds the number of days in 10,000 years.
	//				E_OK	If the time is set

	if (nDays >= DAYS_IN_10K)
		return E_RANGE ;

	uint32_t	hour ;		//	Current hour

	hour = m_hour % 24 ;
	m_hour = nDays * 24 ;
	m_hour += hour ;
	return E_OK ;
}

hzEcode	hzXDate::SetTime	(const char* i)
{
	//	Set the time component of this hzXDate using a string representation of the time. Allowed formats are hh:mm:ss, hh:mm and hhmmss
	//
	//	Arguments:	1)	nSecs	Seconds since midnight
	//
	//	Returns:	E_RANGE	If the supplied cstr does not amount to a legal time
	//				E_OK	If the time is set

	uint32_t	h ;		//	Hours
	uint32_t	m ;		//	Minutes
	uint32_t	s ;		//	Seconds

	if (!i)
		return E_OK ;

	if (i[2] == ':' && i[5] == ':')
	{
		h  = ((i[0] - CHAR_0) * 10) ;
		h +=  (i[1] - CHAR_0) ;
		m  = ((i[3] - CHAR_0) * 10) ;
		m +=  (i[4] - CHAR_0) ;
		s  = ((i[6] - CHAR_0) * 10) ;
		s +=  (i[7] - CHAR_0) ;
	}
	else
	{
		h  = ((i[0] - CHAR_0) * 10) ;
		h +=  (i[1] - CHAR_0) ;
		m  = ((i[2] - CHAR_0) * 10) ;
		m +=  (i[3] - CHAR_0) ;
		s  = ((i[4] - CHAR_0) * 10) ;
		s +=  (i[5] - CHAR_0) ;
	}

	if (h < 0 || h > 23)	return E_RANGE ;
	if (m < 0 || m > 59)	return E_RANGE ;
	if (s < 0 || s > 59)	return E_RANGE ;

	m_hour -= (m_hour % 24) ;
	m_hour += h ;
	m_usec = ((m * 60) + s) * 1000000 ;
	return E_OK ;
}

hzEcode	hzXDate::SetTime	(uint32_t h, uint32_t m, uint32_t s)
{
	//	Set the time component of this hzXDate to time supplied as hours, minutes and seconds
	//
	//	Arguments:	1)	h	Hour
	//				2)	m	Minutes
	//				3)	s	Seconds
	//
	//	Returns:	E_RANGE	If the supplied hour, minute and seconds do not amount to a legal time
	//				E_OK	If the time is set

	if (h < 0 || h > 23)	return E_RANGE ;
	if (m < 0 || m > 59)	return E_RANGE ;
	if (s < 0 || s > 59)	return E_RANGE ;

	m_hour -= (m_hour % 24) ;
	m_hour += h ;
	m_usec = ((m * 60) + s) * 1000000 ;
	return E_OK ;
}

hzEcode	hzXDate::SetTime	(uint32_t nSecs)
{
	//	Set the time component of this hzXDate to time supplied as number of seconds since midnight
	//
	//	Arguments:	1)	nEpochTime	The date supplied as an epoch value
	//
	//	Returns:	E_RANGE	If the supplied number of seconds exceed the number in a day
	//				E_OK	If the time is set

	if (nSecs > SECS_IN_DAY)
		return E_RANGE ;

	m_hour -= (m_hour % 24) ;
	m_hour += (nSecs / 3600) ;
	m_usec = (nSecs % 3600) * 1000000 ;
	return E_OK ;
}

void	hzXDate::SetByEpoch	(uint32_t nEpochTime)
{
	//	Set the date and time components of this hzXDate using the epoch time (number of seconds since midnight , Jan 1 1970)
	//
	//	Arguments:	1)	nEpochTime	The date supplied as an epoch value
	//	Returns:	None

	struct tm*	t ;			//	Converted system time
	time_t		pt ;		//	Time recepticle
	uint32_t	days ;		//	Used by date - no of days conversion

	pt = nEpochTime ;
	t = localtime(&pt) ;

	_daysfromdate(days, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday) ;
	m_hour = (days * 24) + t->tm_hour ;
	m_usec = ((t->tm_min * 60) + t->tm_sec) * 1000000 ;
}

void	hzXDate::SetByEpoch	(uint32_t nEpochTime, uint32_t usec)
{
	//	Set the date and time components of this hzXDate using the epoch time (number of seconds since midnight , Jan 1 1970). Also set the
	//	number of microseconds
	//
	//	Arguments:	1)	nEpochTime	The date supplied as an epoch value
	//				2)	usec		Number of microseconds
	//	Returns:	None

	struct tm*	t ;			//	Converted system time
	time_t		pt ;		//	Time recepticle
	uint32_t	days ;		//	Used by date - no of days conversion

	pt = nEpochTime ;
	t = localtime(&pt) ;

	_daysfromdate(days, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday) ;
	m_hour = (days * 24) + t->tm_hour ;
	m_usec = ((t->tm_min * 60) + t->tm_sec) * 1000000 ;
	m_usec += usec ;
}

void	hzXDate::altdate	(hzInterval unit, int32_t nounits)
{
	//	Alter this hzXDate by a number of units which may be either days, months, years, hours, minutes or seconds
	//
	//	Arguments:	1)	unit	Interval duration
	//				2)	nounits	Number of interval durations
	//
	//	Returns:	None

	switch (unit)
	{
	case DAY:		altday(nounits) ;		break ;
	case MONTH:		altmon(nounits) ;		break ;
	case YEAR:		altyear(nounits) ;		break ;
	case HOUR:		althour(nounits) ;		break ;
	case MINUTE:	altmin(nounits) ;		break ;
	case SECOND:	altsec(nounits) ;		break ;
	}
}

uint32_t	hzXDate::Year	(void) const
{
	//	Return the year part from this hzXDate
	//
	//	Arguments:	None
	//	Returns:	Number being year part of this date

	uint32_t	Y ;		//	Year
	uint32_t	M ;		//	Month
	uint32_t	D ;		//	Day of month

	_datefromdays(Y, M, D, m_hour/24) ;
	return Y ;
}

uint32_t	hzXDate::Month	(void) const
{
	//	Return the month part from this hzXDate
	//
	//	Arguments:	None
	//	Returns:	Number being month of year

	uint32_t	Y ;		//	Year
	uint32_t	M ;		//	Month
	uint32_t	D ;		//	Day of month

	_datefromdays(Y, M, D, m_hour/24) ;
	return M ;
}

uint32_t	hzXDate::Day	(void) const
{
	//	Return the day of month part from this hzXDate
	//
	//	Arguments:	None
	//	Returns:	Number being day of month

	uint32_t	Y ;		//	Year
	uint32_t	M ;		//	Month
	uint32_t	D ;		//	Day of month

	_datefromdays(Y, M, D, m_hour/24) ;
	return D ;
}

uint64_t	hzXDate::AsVal	(void) const
{
	//	Return the day of month part from this hzXDate
	//
	//	Arguments:	None
	//	Returns:	Number being day of month

	uint64_t	v ;

	//v = (m_hour << 32) + m_usec ;
	v = m_hour ;
	v <<= 32 ;
	v |= m_usec ;

	return v ;
}

uint32_t	hzXDate::DaysInYear	(void) const
{
	//	Arguments:	None
	//	Returns:	Number of days since Jan 1

	uint32_t	d ;		//	Days since 0000/01/01

	d = m_hour / 24 ;
	for (; d >= DAYS_IN_4C ; d -= DAYS_IN_4C) ;
	for (; d >= DAYS_IN_1C ; d -= DAYS_IN_1C) ;
	for (; d >= DAYS_IN_4Y ; d -= DAYS_IN_4Y) ;
	if (d >= DAYS_IN_LY)
	{
		d -= DAYS_IN_LY ;
		for (; d >= DAYS_IN_1Y ; d -= DAYS_IN_1Y) ;
	}

	return d ;
}

hzSDate	hzXDate::Date	(void) const
{
	//	Extract the date part of the full dte and time as hzSDate (short form date) instance
	//
	//	Arguments:	None
	//	Returns:	Instance of hzSDate as the date part of the full date

	hzSDate	d ;		//	hzTime instance

	d.SetDate(NoDays()) ;
	return d ;
}

hzTime	hzXDate::Time	(void) const
{
	//	Extract the time part of the full data and time as a hzTime instance
	//
	//	Arguments:	None
	//	Returns:	Instance of hzTime as the time part of the full date

	hzTime	t ;		//	hzTime instance

	t.SetTime(NoSecs()) ;
	return t ;
}

uint32_t	hzXDate::AsEpoch	(void) const
{
	//	Return the time & date as an epoch
	//
	//	Arguments:	None
	//	Returns:	Number of seconds passed since epoch start

	uint32_t	epoch ;		//	Target epoch

	if (m_hour < HOURS_B4_EPOCH || m_hour > HOURS_EPOCH_END)
		return 0 ;

	epoch = m_hour - HOURS_B4_EPOCH ;
	epoch *= SECS_IN_HOUR ;
	epoch += (m_usec/1000000) ;
	return epoch ;
}

hzString  hzXDate::Str  (hzDateFmt eFmt) const
{
	//	Create a hzString instance and populate it with the date and/or time. Return the hzString by value.
	//
	//	Arguments:	1)	eFmt	Output format of returned string
	//
	//	Returns:	Instance of hzString by value being the text form of the date

	hzChain		Z ;					//	Chain for building date
	hzString	v ;					//	String to be populated by Z and returned
	bool		bUS ;				//	True if American format required
	bool		bFN ;				//	True if full names are required
	bool		bStarted = false ;	//	Set when something is printed. This tells the next stage to first print a space.

	if (m_hour & 0x80000000)
		{ v = "Not set" ; return v ; }

	bUS = eFmt & FMT_DATE_USA ? true : false ;
	bFN = eFmt & FMT_DATE_FULL ? true : false ;

	//	Do we have the full blown internet format?
	//if ((eFmt & FMT_DT_INET) == FMT_DATE_ABBR)
	if (eFmt == FMT_DT_INET)
	{
		if (bUS)
			Z.Printf("%s %s %d %04d %02d:%02d:%02d +0000 (GMT)",
				hz_daynames_abrv[Dow()], hz_monthnames_abrv[Month() - 1], Day(), Year(), Hour(), Min(), Sec()) ;
		else
			Z.Printf("%s %d %s %04d %02d:%02d:%02d +0000 (GMT)",
				hz_daynames_abrv[Dow()], Day(), hz_monthnames_abrv[Month() - 1], Year(), Hour(), Min(), Sec()) ;
		v = Z ;
		return v ;
	}

	//	Do we have a dow? if so print this first
	if (eFmt & FMT_DATE_DOW)
	{
		if (bFN)
			Z << hz_daynames_full[Dow()] ;
		else
			Z << hz_daynames_abrv[Dow()] ;
		bStarted = true ;
	}

	//	Now print date
	if (eFmt & FMT_DT_MASK_DATES)
	{
		if (bStarted)
			Z.AddByte(CHAR_SPACE) ;

		switch (eFmt & FMT_DT_MASK_DATES)
		{
		case FMT_DATE_DFLT:	//	YYYYMMDD
							Z.Printf("%04d%02d%02d", Year(), Month(), Day());
							break ;

    	case FMT_DATE_STD:	//	YYYY/MM/DD
							Z.Printf("%04d/%02d/%02d", Year(), Month(), Day());
							break ;

    	case FMT_DATE_NORM:	//	DD/MM/YYYY (UK) or MM/DD/YYYY (US)
							if (bUS)
								Z.Printf("%02d/%02d/%04d", Month(), Day(), Year()) ;
							else
								Z.Printf("%02d/%02d/%04d", Day(), Month(), Year()) ;
							break ;

    	case FMT_DATE_FULL:	//	Day_of_month+monthname+YYYY (UK) or monthname+day_of_month+YYYY (US)
							if (bUS)
								Z.Printf("%s %d %04d", hz_monthnames_abrv[Month() - 1], Day(), Year()) ;
							else
								Z.Printf("%d %s %04d", Day(), hz_monthnames_abrv[Month() - 1], Year()) ;
							break ;
		}
		bStarted = true ;
	}

	//	Now print time
	if (eFmt & FMT_DT_MASK_TIMES)
	{
		if (bStarted)
			Z.AddByte(CHAR_MINUS) ;

		switch (eFmt & FMT_DT_MASK_TIMES)
		{
		case FMT_TIME_DFLT:	//  Time HHMMSS
							Z.Printf("%02d%02d%02d", Hour(), Min(), Sec()) ;
							break ;

		case FMT_TIME_STD:	//  Time HH:MM:SS
							Z.Printf("%02d:%02d:%02d", Hour(), Min(), Sec()) ;
							break ;

		case FMT_TIME_USEC:	//  Time HH:MM:SS.uSec
							Z.Printf("%02d:%02d:%02d.%06d", Hour(), Min(), Sec(), uSec()) ;
							break ;
		}
		bStarted = true ;
	}

	//	Now print timezone if applicable
	if (eFmt & FMT_DT_MASK_TZONE)
	{
		if (bStarted)
			Z.AddByte(CHAR_SPACE) ;

		switch (eFmt & FMT_DT_MASK_TIMES)
		{
		case FMT_TZ_CODE:	Z << "GMT" ;			break ;
		case FMT_TZ_NUM:	Z << "+0000" ;			break ;
		case FMT_TZ_BOTH:	Z << "+0000 (GMT)" ;	break ;
		}
		bStarted = true ;
	}

	v = Z ;
	return v ;
}

bool	hzXDate::operator==	(const hzXDate& op) const
{
	//	Return true if the operand hzXDate is the same as this
	//
	//	Arguments:	1)	op	Operand date

	return (m_hour == op.m_hour && m_usec == op.m_usec) ;
}

bool	hzXDate::operator!=	(const hzXDate& op) const
{
	//	Return true if the operand hzXDate is not the same as this
	//
	//	Arguments:	1)	op	Operand date

	return (m_hour != op.m_hour || m_usec != op.m_usec) ;
}

bool	hzXDate::operator<	(const hzXDate& op) const
{
	//	Return true if this hzXDate is earlier than the operand
	//
	//	Arguments:	1)	op	Operand date

	return (m_hour < op.m_hour || (m_hour == op.m_hour && m_usec < op.m_usec)) ;
}

bool	hzXDate::operator<=	(const hzXDate& op) const
{
	//	Return true if this hzXDate is earlier than or the same as the operand
	//
	//	Arguments:	1)	op	Operand date

	return ((m_hour == op.m_hour && m_usec == op.m_usec)
		|| (m_hour < op.m_hour || (m_hour == op.m_hour && m_usec < op.m_usec))) ;
}

bool	hzXDate::operator>	(const hzXDate& op) const
{
	//	Return true if this hzXDate is later than the operand
	//
	//	Arguments:	1)	op	Operand date

	return (m_hour > op.m_hour || (m_hour == op.m_hour && m_usec > op.m_usec)) ;
}

bool	hzXDate::operator>=	(const hzXDate& op) const
{
	//	Return true if this hzXDate is later than or the same as the operand
	//
	//	Arguments:	1)	op	Operand date

	return ((m_hour == op.m_hour && m_usec == op.m_usec) || (m_hour > op.m_hour || (m_hour == op.m_hour && m_usec > op.m_usec))) ;
}

//	static functions (may get rid of these)

int32_t	hzXDate::datecmp	(hzXDate& a, hzXDate& b)
{
	//	Compare two hzXDate instances, A and B. Return 1 if A>B, -1 if A<B and 0 if A and B are equal
	//
	//	Arguments:	1)	a	First date
	//				2)	b	Second date
	//
	//	Returns:	+1	If a > b
	//				-1	If a < b
	//				0	If a and b are equal

	return	a.m_hour > b.m_hour ?  1 : a.m_hour < b.m_hour ? -1 : a.m_usec > b.m_usec ?  1 : a.m_usec < b.m_usec ? -1 : 0 ;
}

std::ostream &	operator<<	(std::ostream &os, const hzXDate &d)
{
	//	Category:	Data Output
	//
	//	Stream out the textual manefestation of a hzXDate to s stream
	//
	//	Arguments:	1)	os	Output stream
	//				2)	d	Date/time to output
	//
	//	Returns: 	Reference to the supplied output stream.

	char	buf	[24] ;	//	Sprintf buffer

	sprintf(buf, "%04d/%02d/%02d %02d:%02d:%02d.%06d",
		d.Year(), d.Month(), d.Day(), d.Hour(), d.Min(), d.Sec(), d.uSec()) ;
	os << buf ;
	return os ;
}

uint32_t	IsFormalDate	(hzXDate& date, hzChain::Iter& ci)
{
	//	Category:	Text Processing
	//
	//	Determine if the supplied chain iterator is at the begining of a legal date and/or time. If it is then the supplied hzXDate reference is populated. The
	//	lenth of the date/time sequence is returned so that the calling process can choose to advance the iterator by this length. The iterator is not advanced
	//	by this function
	//
	//	Expects dates of the form "day_name, dd month_name yyyy hh:mm:ss Time-Zone"
	//
	//	Arguments:	1)	date	The full date
	//				2)	ci		Chain iterator to be tested
	//
	//	Returns:	Number being length of the date string if found
 
	chIter		zi ;				//	Internal chain iterator
	hzTimezone	tz ;				//	Timezone
	uint32_t	n ;					//	Counter
	uint32_t	dow = NULL_DOW ;	//	Day of week
	uint32_t	len = 0 ;			//	Length of examined text
	uint32_t	Y = 0 ;				//	Year
	uint32_t	M = 0 ;				//	Month
	uint32_t	D = 0 ;				//	Day
	uint32_t	h = 0 ;				//	Hour
	uint32_t	m = 0 ;				//	Minute
	uint32_t	s = 0 ;				//	Second
	uint32_t	hOset = 0 ;			//	Hours in timezone offset to GMT
	uint32_t	mOset = 0 ;			//	Minutes in timezone offset to GMT
	char		numBuf[4] ;			//	For reading numbers

	zi = ci ;
	tz.clear() ;

	for (; !zi.eof() ;)
	{
		if (*zi < CHAR_SPACE)
			break ;

		if (*zi == CHAR_SPACE || *zi == CHAR_COMMA || *zi == CHAR_PAROPEN || *zi == CHAR_PARCLOSE)
			{ len++ ; zi++ ; continue ; }

		if (IsDigit(*zi))
		{
			//	We are looking for a day of the month, a year or the start of a time sequence
			n = IsTime(h, m, s, zi) ;
			if (n)
				zi += n ;
			else
			{
				for (n = 0 ; IsDigit(*zi) ; zi++)
					{ n *= 10 ; n += (*zi - '0') ; }

				if		(n < 32 && D == 0)		D = n ;
				else if (n < 10000 && Y == 0)	Y = n ;
				else
					return 0 ;
			}
		}

		if (IsAlpha(*zi))
		{
			//	We are looking for a day name, a month name or a timezone code

			if (dow == NULL_DOW)
			{
				for (n = 0 ; n < 7 ; n++)
					if (zi == hz_daynames_full[n])
						{ dow = n ; break ; }

				if (dow == NULL_DOW)
				{
					for (n = 0 ; n < 7 ; n++)
						if (zi == hz_daynames_abrv[n])
							{ dow = n ; break ; }
				}

				if (dow != 8)
					{ for (; IsAlpha(*zi) ; zi++) ; continue ; }
			}

			if (M == 0)
			{
				for (n = 0 ; n < 12 ; n++)
					if (zi == hz_monthnames_full[n])
						{ M = n + 1 ; break ; }

				if (M == 0)
				{
					for (n = 0 ; n < 12 ; n++)
						if (zi == hz_monthnames_abrv[n])
							{ M = n + 1 ; break ; }
				}

				if (M >= 0)
					{ for (; IsAlpha(*zi) ; zi++) ; continue ; }
			}

			if (!tz.code)
			{
				for (n = 0 ;; n++)
				{
					if (zi == _hzGlobal_Timezones[n].code)
						{ tz = _hzGlobal_Timezones[n] ; break ; }
				}

				if (tz.code)
					{ for (; IsAlpha(*zi) ; zi++) ; continue ; }
			}

			return 0 ;
		}

		if (*zi == CHAR_PLUS || *zi == CHAR_MINUS)
		{
			//	We are looking for a timezone offset to GMT (of the form +/-dddd)

			zi++ ;	numBuf[0] = *zi ;
			zi++ ;	numBuf[1] = *zi ;
			zi++ ;	numBuf[2] = *zi ;
			zi++ ;	numBuf[3] = *zi ;

			if (IsDigit(numBuf[0]) && IsDigit(numBuf[1]) && IsDigit(numBuf[2]) && IsDigit(numBuf[3]))
			{
				hOset = ((numBuf[0] - '0') * 10) + (numBuf[1] - '0') ;
				mOset = ((numBuf[2] - '0') * 10) + (numBuf[3] - '0') ;

				if (hOset > 23 || mOset > 59)
					return 0 ;
				zi++ ;
				continue ;
			}

			return 0 ;
		}
	}

	if (date.SetDate(Y, M, D) != E_OK)	return 0 ;
	if (date.SetTime(h, m, s) != E_OK)	return 0 ;

	ci = zi ;
	return len ;
}

/*
**	Date Formats
*/

const char*	s_dt_types[] =
{
	"FMT_DT_UNKNOWN",	//	Invalid or uninitialized type

	//  Dates contrl flags
	"FMT_DATE_DOW",		//	Print the dow (this will appear first)
	"FMT_DATE_USA",		//	Where applicable, put day before month
	"FMT_DATE_ABBR",	//	Write words (eg dow and monthname) out in short form
	"FMT_DATE_FULL",	//	Write words (eg dow and monthname) out in full

	//  Date only formats
	"FMT_DATE_DFLT",	//	YYYYMMDD
	"FMT_DATE_STD",		//	YYYY/MM/DD
	"FMT_DATE_NORM",	//	DD/MM/YYYY (UK) or MM/DD/YYYY (US)
	"FMT_DATE_FULL",	//	Day_of_month+monthname+YYYY (UK) or monthname+day_of_month+YYYY (US)

	//  Time only formats
	"FMT_TIME_DFLT",	//	Time HHMMSS
	"FMT_TIME_STD",		//	Time HH:MM:SS
	"FMT_TIME_USEC",	//	Time HH:MM:SS.uSec

	//  Timezones (always last)
	"FMT_TZ_CODE",		//	Timezone as code
	"FMT_TZ_DIGITS",	//	Timezone as digits
	"FMT_TZ_BOTH",		//	Timezone as digits plus (code in braces)
} ;

const hzString	DateFmt2Str	(hzDateFmt fmt)
{
	//	Category:	Diagnostics
	//
	//	Convert a HadroZoo Format to its text name for diagnostic purposes
	//
	//	Arguments:	1)	fmt		Enumerated HadronZoo text format
	//
	//	Returns:	Instance of hzString by value being format/layout description

	hzChain		Z ;					//	Output chain
	hzString	S ;					//	Target hzString instance

	//  Dates contrl flags
	if (fmt & FMT_DATE_DOW)		{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[1] ; }
	if (fmt & FMT_DATE_USA)		{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[2] ; }
	if (fmt & FMT_DATE_ABBR)	{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[3] ; }
	if (fmt & FMT_DATE_FULL)	{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[4] ; }

	//  Date only formats
	if (fmt & FMT_DATE_DFLT)	{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[5] ; }
	if (fmt & FMT_DATE_STD)		{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[6] ; }
	if (fmt & FMT_DATE_NORM)	{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[7] ; }
	if (fmt & FMT_DATE_FULL)	{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[8] ; }

	//  Time only formats
	if (fmt & FMT_TIME_DFLT)	{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[9] ; }
	if (fmt & FMT_TIME_STD)		{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[10] ; }
	if (fmt & FMT_TIME_USEC)	{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[11] ; }

	//  Timezones (always last)
	if (fmt & FMT_TZ_CODE)		{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[12] ; }
	if (fmt & FMT_TZ_NUM)		{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[13] ; }
	if (fmt & FMT_TZ_BOTH)		{ Z.AddByte(CHAR_PLUS) ; Z << s_dt_types[14] ; }

	if (Z.Size())
		S = Z ;
	else
		S = s_dt_types[FMT_DT_UNKNOWN] ;

	return S ;
}

hzDateFmt	Str2DateFmt	(const hzString& S)
{
	//	Category:	Config
	//
	//	Convert the name of a HadronZoo Format to the enum
	//
	//	Arguments:	1)	S	String presumed to indicate an enumerated HadronZoo text format
	//
	//	Returns:	Enum value being the format/layout matching supplied description

	int32_t	x = 0 ;		//	Format flagset

	//  Dates contrl flags
	if (S.Contains("FMT_DATE_DOW"))		x |= FMT_DATE_DOW ;
	if (S.Contains("FMT_DATE_USA"))		x |= FMT_DATE_USA ;
	if (S.Contains("FMT_DATE_ABBR"))	x |= FMT_DATE_ABBR ;
	if (S.Contains("FMT_DATE_FULL"))	x |= FMT_DATE_FULL ;

	//  Date only formats
	if (S.Contains("FMT_DATE_DFLT"))	x |= FMT_DATE_DFLT ;
	if (S.Contains("FMT_DATE_STD"))		x |= FMT_DATE_STD ;
	if (S.Contains("FMT_DATE_NORM"))	x |= FMT_DATE_NORM ;
	if (S.Contains("FMT_DATE_FORM"))	x |= FMT_DATE_FORM ;

	//  Time only formats
	if (S.Contains("FMT_TIME_DFLT"))	x |= FMT_TIME_DFLT ;
	if (S.Contains("FMT_TIME_STD"))		x |= FMT_TIME_STD ;
	if (S.Contains("FMT_TIME_USEC"))	x |= FMT_TIME_USEC ;

	//  Timezones (always last)
	if (S.Contains("FMT_TZ_CODE"))		x |= FMT_TZ_CODE ;
	if (S.Contains("FMT_TZ_DIGITS"))	x |= FMT_TZ_NUM ;
	if (S.Contains("FMT_TZ_BOTH"))		x |= FMT_TZ_BOTH ;

	return (hzDateFmt) x ;
}
