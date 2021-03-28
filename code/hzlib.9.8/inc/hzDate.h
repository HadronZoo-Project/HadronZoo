//
//	File:	hzDate.h
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
//	Three HadronZoo data and time package provides a set of classes and methods that promote a 'rational and comprehensive' approach to dates and
//	times. Much of this package is thus asthetic.
//
//	Four classes are provided namely:-
//
//	1)	hzTime	- Based on the number of seconds since midnight (4 bytes)
//	2)	hzSDate	- A short form of date based on the number of days since Jan 1st, 0000 (4 bytes)
//	3)	hzXDate	- A full date and time accurate to microseconds (8 bytes)
//

#ifndef hzDate_h
#define hzDate_h

#include "hzChain.h"

#define DAYS_IN_10K		 3652425		//	days in 10000 years
#define DAYS_IN_2K		  730485		//	days in 2000 years
#define DAYS_IN_4C		  146097		//	days in 400 years
#define DAYS_IN_1C		   36524		//	days in 100 years (in which the 00 year is not a leap)
#define DAYS_IN_LC		   36525		//	days in 100 years (in which the 00 year is a leap)
#define DAYS_IN_4Y		    1461		//	days in 4 years
#define DAYS_IN_1Y		     365		//	days in 1 year
#define DAYS_IN_LY		     366		//	days in 1 leap year
#define DAYS_B4_EPOCH	  719528		//	days between 00000101 and 19700101
#define DAYS_EPOCH_END	  744383		//	days at end of epoch (2030)
#define HOURS_B4_EPOCH	17268672		//	hours between 00000101 and 19700101
#define HOURS_EPOCH_END	17865192		//	hours at end of epoch (rounded to nearest whole day)
#define SECS_IN_DAY		   86400		//	seconds in one day
#define SECS_IN_HOUR	    3600		//	seconds in one day
#define DAYS_EXCEL		  693959		//	Number of days with Dec 31st 1899 being day 1 (Microsoft Excel dates)

#define NULL_DOW    0xFF
#define NULL_TIME	0x80000000
#define NULL_DATE	0x80000000

enum	hzInterval
{
	//	Category:	Scheduling
	//
	//	Represents a standard interval

	DAY,			//	One day
	MONTH,			//	One calender month
	YEAR,			//	One calender year
	HOUR,			//	One hour
	MINUTE,			//	One minute
	SECOND			//	One second
} ;

enum	hzDateFmt
{
	//	Category:	Data Processing
	//
	//	Enumerated HadronZoo date presentation formats.

	FMT_DT_UNKNOWN	= 0x0000,		//	Invalid or uninitialized format

	//	Dates contrl flags
	FMT_DATE_DOW	= 0x0001,		//	Print the dow (this will appear first)
	FMT_DATE_USA	= 0x0002,		//	Where applicable, put day before month
	FMT_DATE_ABBR	= 0x0004,		//	Write words (eg dow and monthname) out in short form
	FMT_DATE_FULL	= 0x0008,		//	Write words (eg dow and monthname) out in full

	//	Date only formats
	FMT_DATE_DFLT	= 0x0010,		//	Default format YYYYMMDD
	FMT_DATE_STD	= 0x0020,		//	Standard format YYYY/MM/DD
	FMT_DATE_NORM	= 0x0040,		//	Normal format DD/MM/YYYY (UK) or MM/DD/YYYY (US)
	FMT_DATE_FORM	= 0x0080,		//	Day_of_month+monthname+YYYY (UK) or monthname+day_of_month+YYYY (US)

	//	Time only formats
	FMT_TIME_DFLT	= 0x0100,		//	Time HHMMSS
	FMT_TIME_STD	= 0x0200,		//	Time HH:MM:SS
	FMT_TIME_USEC	= 0x0400,		//	Time HH:MM:SS.uSec

	//	Timezones (always last)
	FMT_TZ_CODE		= 0x1000,		//	Timezone as code
	FMT_TZ_NUM		= 0x2000,		//	Timezone as number of hours +/- GMT
	FMT_TZ_BOTH		= 0x3000,		//	Timezone as digits plus (code in braces)

	//	Combined Date & Time formats
	FMT_DT_DFLT	= FMT_DATE_DFLT + FMT_TIME_DFLT,	//	YYYYMMDD HHMMSS
	FMT_DT_STD	= FMT_DATE_STD  + FMT_TIME_STD,		//	YYYY/MM/DD HH:MM:SS
	FMT_DT_NORM	= FMT_DATE_NORM + FMT_TIME_STD,		//	DD/MM/YYYY (UK) or MM/DD/YYYY (US) followed by HH:MM:SS
	FMT_DT_USEC	= FMT_DATE_STD  + FMT_TIME_USEC,	//	YYYYMMDD HH:MM:SS.uSec
	FMT_DT_INET	= FMT_DATE_DOW  + FMT_DATE_FULL + FMT_TIME_STD + FMT_TZ_NUM,
									//	Day of Week + Day_of_month+monthname+YYYY or monthname+day_of_month+YYYY + HH:MM:SS + timezone
} ;

#define FMT_DT_CONTROL		0x000F	//	Checks if there are any control settings (dow, long names, american)
#define FMT_DT_MASK_DATES	0x00F0	//	Checks if there are any control settings (dow, long names, american)
#define FMT_DT_MASK_TIMES	0x0F00	//	Checks if there are any control settings (dow, long names, american)
#define FMT_DT_MASK_TZONE	0xF000	//	Checks if there are any timezone settings

/*
**	Date calculation macros
*/

#define isleap(y)	(!(y%400)?1:!(y%100)?0:!(y%4)?1:0)
#define monlen(y,m)	(m==2?isleap(y)?29:28:m==4||m==6||m==9||m==11?30:31)

/*
**	Variables for formal date presentation
*/

extern	const char*	hz_daynames_abrv	[] ;	//	Day names (short)
extern	const char*	hz_daynames_full	[] ;	//	Day names (full)
extern	const char*	hz_monthnames_abrv	[] ;	//	Month names (short)
extern	const char*	hz_monthnames_full	[] ;	//	Month names (full)

/*
**	misc non-member functions
*/

hzEcode	_daysfromdate	(uint32_t& nDays, uint32_t Y, uint32_t M, uint32_t D) ;
void	_datefromdays	(uint32_t& Y, uint32_t& M, uint32_t& D, uint32_t nDays) ;

/*
**	TIME	(HHMMSS)
*/

class	hzTime
{
	//	Category:	Data
	//
	//	hzTime is a data type to represent the time of day. It has allowed range of 00:00:00 (midnight) thru to 23:59:59. The minimum
	//	unit is the second.

private:
	uint32_t	m_secs ;	//	No of seconds since midnight

public:
	hzTime	(void)	{ m_secs = NULL_TIME ; }
	~hzTime	(void)	{}

	void		SysTime	(void) ;
	void		SetTime	(const hzTime& op) ;
	hzEcode		SetTime	(uint32_t h, uint32_t m, uint32_t s) ;
	hzEcode		SetTime	(uint32_t nSecs) ;
	hzEcode		SetTime	(const char* pTimeStr) ;

	void		Clear	(void)	{ m_secs = NULL_TIME ; }

	bool		IsNull	(void) const { return m_secs == NULL_TIME ? true : false ; }
	bool		IsSet	(void) const { return m_secs == NULL_TIME ? false : true ; }
	uint32_t	NoSecs	(void) const { return m_secs ; }
	uint32_t	Hour	(void) const { return m_secs / 3600 ; }
	uint32_t	Min		(void) const { return (m_secs / 60) % 60 ; }
	uint32_t	Sec		(void) const { return m_secs % 60 ; }

	const char*	Txt		(hzRecep16&, hzDateFmt eEType = FMT_TIME_DFLT) const ;
	const char*	Txt		(hzRecep32&, hzDateFmt eEType = FMT_TIME_DFLT) const ;
	hzString	Str		(hzDateFmt eEType = FMT_TIME_DFLT) const ;

	/*
	**	Assignment Operators
	*/

	const hzTime&	operator=	(const hzTime& op)		{ m_secs = op.m_secs ; return *this ; }
	const hzTime&	operator=	(uint32_t nSecs)		{ SetTime(nSecs) ; return *this ; }
	const hzTime&	operator=	(const char* cpTime)	{ SetTime(cpTime) ; return *this ; }
	const hzTime&	operator=	(const hzString& time)	{ SetTime(*time) ; return *this ; }

	/*
	**	Arithmetic Operators
	*/
 
	bool	operator+=	(uint32_t nSecs) { m_secs += nSecs ; return true ; }
	bool	operator-=	(uint32_t nSecs) { m_secs -= nSecs ; return true ; }

	/*
	**	Test Operators
	*/

	bool	operator==	(const hzTime& op) const	{ return (m_secs==op.m_secs) ? true : false ; }
	bool	operator!=	(const hzTime& op) const	{ return (m_secs!=op.m_secs) ? true : false ; }
	bool	operator<	(const hzTime& op) const	{ return (m_secs< op.m_secs) ? true : false ; }
	bool	operator<=	(const hzTime& op) const	{ return (m_secs<=op.m_secs) ? true : false ; }
	bool	operator>	(const hzTime& op) const	{ return (m_secs> op.m_secs) ? true : false ; }
	bool	operator>=	(const hzTime& op) const	{ return (m_secs>=op.m_secs) ? true : false ; }

	bool	operator!	(void) const	{ return m_secs == NULL_TIME ? true : false ; }

	//	Stream operator
	friend	std::ostream&	operator<<	(std::ostream& os, const hzTime& Time) ;
} ;

/*
**	Short date (YYYYMMDD)
*/

class	hzXDate ;

class	hzSDate
{
	//	Category:	Data
	//
	//	hzSDate is a data type capable of representing a date. It comprises a single 32 bit variable whose value is the number of days
	//	since 'the Birth of Christ' or Jan 1st, year 0.

	uint32_t	m_days ;	//	No of days since Jan 1, 0000

public:
	hzSDate		(void)	{ m_days = NULL_DATE ; }
	~hzSDate	(void)	{}

	//	Set functions
	void	SysDate		(void) ;
	hzEcode	SetDate		(const hzSDate& op) ;
	hzEcode	SetDate		(uint32_t Y, uint32_t M, uint32_t D) ;
	hzEcode	SetDate		(uint32_t nDays) ;
	hzEcode	SetDate		(const char* pDateStr) ;
	void	SetByEpoch	(uint32_t nEpochTime) ;

	//	Modify functions
	hzEcode	AltYear		(int32_t x) ;
	hzEcode	AltMonth	(int32_t x) ;
	hzEcode	AltDay		(int32_t x) ;

	//	Get functions
	uint32_t	Year	(void) const ;
	uint32_t	Month	(void) const ;
	uint32_t	Day		(void) const ;
	uint32_t	Dow		(void) const	{ return (m_days + 5) % 7 ; }
	
	void		Clear	(void)			{ m_days = NULL_DATE ; }
	uint32_t	NoDays	(void) const	{ return m_days ; }
	uint32_t	Excel	(void) const	{ return m_days > DAYS_EXCEL ? m_days - DAYS_EXCEL : 0 ; }
	bool		IsNull	(void) const	{ return m_days == NULL_DATE ? true : false ; }
	bool		IsSet	(void) const	{ return m_days == NULL_DATE ? false : true ; }

	const char*	Txt		(hzRecep32&, hzDateFmt eEType = FMT_DATE_DFLT) const ;
	hzString	Str		(hzDateFmt eEType = FMT_DATE_DFLT) const ;

	//	Assignment Operators
	const hzSDate&	operator=	(const hzXDate& op) ;

	const hzSDate&	operator=	(const hzSDate& op)		{ m_days = op.m_days ; return *this ; }
	const hzSDate&	operator=	(uint32_t nDays)			{ SetDate(nDays) ; return *this ; }
	const hzSDate&	operator=	(const char* cpDate)	{ SetDate(cpDate) ; return *this ; }
	const hzSDate&	operator=	(const hzString& S)		{ SetDate(*S) ; return *this ; }

	//	Arithmetic Operators
	const hzSDate&	operator+=	(uint32_t nDays)
	{
		//	Add to this date, the supplied number of days
		m_days += nDays ;
		m_days = m_days<0 ? 0 : m_days>DAYS_IN_10K ? DAYS_IN_10K : m_days ;
		return *this ;
	}
 
	const hzSDate&	operator-=	(uint32_t nDays)
	{
		//	Subtract from this date, the supplied number of days
		m_days -= nDays ;
		m_days = m_days<0 ? 0 : m_days>DAYS_IN_10K ? DAYS_IN_10K : m_days ;
		return *this ;
	}

	//	Test Operators
	bool	operator== (const hzXDate& op) const ;

	bool	operator== (const hzSDate& op) const	{ return (m_days==op.m_days) ? true : false ; }
	bool	operator!= (const hzSDate& op) const	{ return (m_days!=op.m_days) ? true : false ; }
	bool	operator<  (const hzSDate& op) const	{ return (m_days< op.m_days) ? true : false ; }
	bool	operator<= (const hzSDate& op) const	{ return (m_days<=op.m_days) ? true : false ; }
	bool	operator>  (const hzSDate& op) const	{ return (m_days> op.m_days) ? true : false ; }
	bool	operator>= (const hzSDate& op) const	{ return (m_days>=op.m_days) ? true : false ; }

	bool	operator!	(void) const	{ return m_days == NULL_TIME ? true : false ; }

	//	Stream operator
	friend	std::ostream&	operator<<	(std::ostream& os, const hzSDate& d) ;
} ;

/*
**	Full date and time
*/

class	hzXDate
{
	//	Category:	Data
	//
	//	A full date and time accurate to microseconds (8 bytes)

private:
	uint32_t	m_hour ;		//	no of hours since midnight 0000/01/01
	uint32_t	m_usec ;		//	no of microseconds since start of hour

	void	altsec	(int32_t nounits) ;
	void	altmin	(int32_t nounits) ;
	void	althour	(int32_t nounits) ;
	void	altday	(int32_t nounits) ;
	void	altmon	(int32_t nounits) ;
	void	altyear	(int32_t nounits) ;

public:
	hzXDate		(void)	{ m_hour = 0 ; m_usec = 0 ; }
	~hzXDate	(void)	{}

	bool		IsNull	(void) const	{ return !m_hour && !m_usec ? true : false ; }
	bool		IsSet	(void) const	{ return m_hour || m_usec ? true : false ; }

	uint32_t	Year	(void) const ;
	uint32_t	Month	(void) const ;
	uint32_t	Day		(void) const ;
	uint64_t	AsVal	(void) const ;

	uint32_t	Hour	(void) const	{ return m_hour%24 ; }
	uint32_t	Min		(void) const	{ return m_usec/60000000 ; }
	uint32_t	Sec		(void) const	{ return (m_usec/1000000)%60 ; }
	uint32_t	uSec	(void) const	{ return m_usec%1000000 ; }
	uint32_t	Dow		(void) const	{ return ((m_hour/24)+5)%7 ; }

	void	Clear	(void)	{ m_hour = 0 ; m_usec = 0 ; }

	void	SysDateTime	(void) ;

	hzEcode	SetDate		(const hzXDate& op)	{ m_hour = op.m_hour ; m_usec = op.m_usec ; return E_OK ; }
	hzEcode	SetDate		(hzSDate& D)		{ m_hour %= 24 ; m_hour += (D.NoDays() * 24) ; return E_OK ; }

	hzEcode	SetDate		(uint32_t Y, uint32_t M, uint32_t D) ;
	hzEcode	SetDate		(uint32_t nDays) ;
	hzEcode	SetDate		(uint32_t nDays, uint32_t nSecs) ;
	hzEcode	SetDate		(uint64_t xdVal) ;
	hzEcode	SetDate		(const char* pDateStr) ;

	hzEcode	SetTime		(hzTime& T)	{ m_hour -= m_hour%24 ; m_hour += T.Hour() ; m_usec = (T.NoSecs()/24) * 1000000 ; }
	hzEcode	SetTime		(const char* cpTimeStr) ;
	hzEcode	SetTime		(uint32_t h, uint32_t m, uint32_t s) ;
	hzEcode	SetTime		(uint32_t nSecs) ;

	hzEcode	SetDateTime	(const char* pDateStr) ;
	void	SetByEpoch	(uint32_t nEpochTime) ;
	void	SetByEpoch	(uint32_t nEpochTime, uint32_t usec) ;

	void	altdate		(hzInterval unit, int32_t nounits) ;

	uint32_t	DaysInYear	(void) const ;
	uint32_t	NoDays		(void) const	{ return m_hour/24 ; }
	uint32_t	NoSecs		(void) const	{ return ((m_hour%24) * 3600) + (m_usec/1000000) ; }
	uint32_t	Excel		(void) const	{ return (m_hour/24) > DAYS_EXCEL ? (m_hour/24) - DAYS_EXCEL : 0 ; }

	hzSDate		Date	(void) const ;		//	Return the date part only
	hzTime		Time	(void) const ;		//	Return the time part only
	uint32_t	AsEpoch	(void) const ;		//	Return the time & date as an epoch

	hzString	Str		(hzDateFmt eEType = FMT_DT_DFLT) const ;

	//	Assignment Operators
	const hzXDate&	operator=	(const hzXDate& op)	{ m_hour = op.m_hour ; m_usec = op.m_usec ; return *this ; }
	const hzXDate&	operator=	(const hzSDate& op)	{ m_hour = op.Day() * 24 ; m_usec = 0 ; return *this ; }
	const hzXDate&	operator=	(const hzString& S)	{ SetDate(*S) ; return *this ; }
	const hzXDate&	operator=	(const char* s)		{ SetDate(s) ; return *this ; }

	//	Arithmentic Operators
	const hzXDate&	operator+=	(uint32_t nDays)
	{
		//	Add to this date the supplied number of days
		m_hour += (nDays * 24) ;
		m_hour = m_hour < 0 ? 0 : m_hour > (DAYS_IN_10K * 24) ? (DAYS_IN_10K * 24) : m_hour ;
		return *this ;
	}
 
	const hzXDate&	operator-=	(uint32_t nDays)
	{
		//	Subtract from this date the supplied number of days
		m_hour -= (nDays * 24) ;
		m_hour = m_hour < 0 ? 0 : m_hour > (DAYS_IN_10K * 24) ? (DAYS_IN_10K * 24) : m_hour ;
		return *this ;
	}

	//	Test Operators
	bool	operator==	(const hzXDate& op) const ;
	bool	operator!=	(const hzXDate& op) const ;
	bool	operator<	(const hzXDate& op) const ;
	bool	operator<=	(const hzXDate& op) const ;
	bool	operator>	(const hzXDate& op) const ;
	bool	operator>=	(const hzXDate& op) const ;

	bool	operator!	(void) const	{ return !m_hour && !m_usec ? true : false ; }

	//	Static functions
	static	int32_t	datecmp	(hzXDate& a, hzXDate& b) ;

	//	Stream operator
	friend	std::ostream&	operator<<	(std::ostream& os, const hzXDate& Date) ;
} ;

//	Externals
extern	const hzSDate	_hz_null_hzSDate ;		//	Null short date
extern	const hzXDate	_hz_null_hzXDate ;		//	Null full date
extern	const hzTime	_hz_null_hzTime ;		//	Null time

/*
**	Prototypes
*/

hzDateFmt	Str2DateFmt		(const hzString& dtFmt) ;
const char*	DateFmt2Txt		(hzDateFmt dtFmt) ;

//	Return real time in microseconds/nanoseconds
uint64_t	RealtimeMicro	(void) ;
uint64_t	RealtimeNano	(void) ;

//	Following return number of chars consumed if criteria is met
uint32_t	IsTime			(uint32_t& h, uint32_t& m, uint32_t& s, const char* cpStr) ;
uint32_t	IsTime			(uint32_t& h, uint32_t& m, uint32_t& s, hzString& S) ;
uint32_t	IsTime			(uint32_t& h, uint32_t& m, uint32_t& s, hzChain::Iter& ci) ;

uint32_t	IsDate			(uint32_t& Y, uint32_t& M, uint32_t& D, const char* cpStr) ;
uint32_t	IsDate			(uint32_t& Y, uint32_t& M, uint32_t& D, hzString& S) ;

uint32_t	IsDateTime		(uint32_t& Y, uint32_t& M, uint32_t& D, uint32_t& h, uint32_t& m, uint32_t& s, const char* cpStr) ;
uint32_t	IsDateTime		(uint32_t& Y, uint32_t& M, uint32_t& D, uint32_t& h, uint32_t& m, uint32_t& s, hzString& S) ;
uint32_t	IsFormalDate	(hzXDate& date, hzChain::Iter& ci) ;

#endif	//	hzDate_h
