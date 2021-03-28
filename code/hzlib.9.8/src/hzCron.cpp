//
//	File:	hzCron.cpp
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

#include <iostream>

#include <sys/time.h>

#include <stdarg.h>

#include "hzChars.h"
#include "hzErrcode.h"
#include "hzCtmpls.h"
#include "hzProcess.h"
#include "hzCron.h"

/*
**	Variables
*/

global	const char*	_hzGlobal_Periodicities	[] =
{
	"Never",
	"Every Day",
	"Mon - Sat",
	"Weekdays only",
	"Every Monday",
	"Every Tuesday",
	"Every Wednesday",
	"Every Thursday",
	"Every Friday",
	"Every Saturday",
	"Every Sunday",
	"Every other Monday",
	"Every other Tuesday",
	"Every other Wednesday",
	"Every other Thursday",
	"Every other Friday",
	"Every other Saturday",
	"Every other Sunday",
	"Monthly",
	"Bi-Monthly (odd)",
	"Bi-Monthly (even)",
	"Quarterly (1)",
	"Quarterly (2)",
	"Quarterly (3)",
	"Half-Yearly (Jan/Jul)",
	"Half-Yearly (Feb/Aug)",
	"Half-Yearly (Mar/Sep)",
	"Half-Yearly (Apr/Oct)",
	"Half-Yearly (May/Nov)",
	"Half-Yearly (Jun/Dec)",
	"Yearly (Jan)",
	"Yearly (Feb)",
	"Yearly (Mar)",
	"Yearly (Apr)",
	"Yearly (May)",
	"Yearly (Jun)",
	"Yearly (Jul)",
	"Yearly (Aug)",
	"Yearly (Sep)",
	"Yearly (Oct)",
	"Yearly (Nov)",
	"Yearly (Dec)",
	"Random (not periodic)",
	""
} ;

global const char*	_hzGlobal_Monthrules	[] =
{
	"Not Applicable",
	"From Era Start Date",
	"First day of month",
	"First weekday of month",
	"First workday of month",
	"Last day of month",
	"Last weekday of month",
	"Last workday of month",
	"1st Monday",
	"1st Tuesday",
	"1st Wednesday",
	"1st Thursday",
	"1st Friday",
	"1st Saturday",
	"1st Sunday",
	"2nd Monday",
	"2nd Tuesday",
	"2nd Wednesday",
	"2nd Thursday",
	"2nd Friday",
	"2nd Saturday",
	"2nd Sunday",
	"3rd Monday",
	"3rd Tuesday",
	"3rd Wednesday",
	"3rd Thursday",
	"3rd Friday",
	"3rd Saturday",
	"3rd Sunday",
	"4th Monday",
	"4th Tuesday",
	"4th Wednesday",
	"4th Thursday",
	"4th Friday",
	"4th Saturday",
	"4th Sunday",
	"Last Monday",
	"Last Tuesday",
	"Last Wednesday",
	"Last Thursday",
	"Last Friday",
	"Last Saturday",
	"Last Sunday",
	""
} ;

global const char*	_hzGlobal_Holidays	[] =
{
	"Newyear",
	"Good Friday",
	"Easter Sunday",
	"Easter Monday",
	"Mayday",
	"Spring",
	"Summer",
	"Christmas Day",
	"Boxing Day",
	"Newyear in lieu",
	"Christmas in lieu",
	"Boxing in lieu",
	"All UK public",
	""
} ;

//	Actual or calculated holidays
global	const uint32_t	HZ_PUBHOL_NEWYEAR		= 0x00000001 ;	//	First working day of the year
global	const uint32_t	HZ_PUBHOL_GOOD_FRIDAY	= 0x00000002 ;	//	Friday before Easter
global	const uint32_t	HZ_PUBHOL_EASTER_SUNDAY	= 0x00000004 ;	//	Monday before Easter
global	const uint32_t	HZ_PUBHOL_EASTER_MONDAY	= 0x00000008 ;	//	Monday before Easter
global	const uint32_t	HZ_PUBHOL_MAYDAY		= 0x00000010 ;	//	First Monday in May
global	const uint32_t	HZ_PUBHOL_SPRING		= 0x00000020 ;	//	Last Monday in May
global	const uint32_t	HZ_PUBHOL_SUMMER		= 0x00000040 ;	//	Last Monday in August
global	const uint32_t	HZ_PUBHOL_XMAS			= 0x00000080 ;	//	Always Dec 25th
global	const uint32_t	HZ_PUBHOL_BOXING		= 0x00000100 ;	//	Always Dec 26th

//	In liue days
global	const uint32_t	HZ_PUBHOL_LUE_NEWYEAR	= 0x00001000 ;	//	First working day of the year
global	const uint32_t	HZ_PUBHOL_LUE_XMAS		= 0x00002000 ;	//	First working day after Dec 25th if this date is not a working day
global	const uint32_t	HZ_PUBHOL_LUE_BOXING	= 0x00004000 ;	//	First working day after Dec 26th if this date is not a working day
global	const uint32_t	HZ_PUBHOL_ALLUK			= 0x000071ff ;	//	All UK public holidays

/*
**	Section 1:	Non-member periodicity/monthrule text conversion functions
*/

hzPeriodicity Str2Periodicity  (const hzString& P)
{
	//	Category:	Config
	//
	//	Convert textual representation of a periodicity (eg from a config file) into a hzPeriodicity enum value.
	//
	//	Arguments:	1)	P	String assumed to be the name of a valid hzPeriodicity value
	//
	//	Returns:	Enum hzPeriodicity

	uint32_t	nIndex ;	//	Periodicity iterator

	if (!P)
		return HZPERIOD_INVALID ;

	for (nIndex = 0 ; _hzGlobal_Periodicities[nIndex] ; nIndex++)
	{
		if (P.Equiv(_hzGlobal_Periodicities[nIndex]))
			return (hzPeriodicity) nIndex ;
	}

	return HZPERIOD_INVALID ;
}

hzMonthrule	Str2Monthrule	(const hzString& R)
{
	//	Category:	Config
	//
	//	Convert textual representation of a month-rule (eg from a config file) into a hzMonthrule enum value.
	//
	//	Arguments:	1)	R	String assumed to be the name of a valid hzMonthrule value
	//
	//	Returns:	Enum hzMonthrule

	uint32_t	nIndex ;	//	Monthrule iterator

	if (!R)
		return HZMONTHRULE_INVALID ;

	for (nIndex = 0 ; _hzGlobal_Monthrules[nIndex] ; nIndex++)
	{
		if (R == _hzGlobal_Monthrules[nIndex])
			return (hzMonthrule) nIndex ;
	}

	return HZMONTHRULE_INVALID ;
}

const char*	Periodicity2Txt	(const hzPeriodicity P)
{
	//	Category:	Diagnostics
	//
	//	Convert hzPeriodicity enum value into text (for diagnostic purposes)
	//
	//	Arguments:	1)	P	The valid hzPeriodicity value
	//
	//	Returns:	Const reference to hzString as value being text form of the periodicity

	if (P < 0 || P > HZPERIOD_INVALID)
		return _hzGlobal_Periodicities[HZPERIOD_INVALID] ;
	return _hzGlobal_Periodicities[P] ;
}

const char*	Monthrule2Txt	(const hzMonthrule mrule)
{
	//	Category:	Diagnostics
	//
	//	Convert hzMonthrule enum value into text (for diagnostic purposes)
	//
	//	Arguments:	1)	mrule	The valid hzMonthrule value
	//
	//	Returns:	Const reference to hzString as value being text form of the month rule

	if (mrule < 0 || mrule > HZMONTHRULE_INVALID)
		return _hzGlobal_Monthrules[HZMONTHRULE_INVALID] ;
	return _hzGlobal_Monthrules[mrule] ;
}

/*
**	Holiday calculator
*/

//	FnGrp:		IsHoliday
//
//	Category:	Timer/Scheduling
//
//	Establish if a given date is a public holiday or a public holiday in lieu. The function returns 0 if the date is not a public holiday or a positive value if
//	it is. The value is bitwise to identify the particularly holiday and so this function depends on there not being too many public holidays being declared.
//
//	The returned value will normally have only one bit set (equate to only one holiday). However if the date is a holiday that could be in lieu but happens not
//	to be on this occasion, then the flags will have two bits set - ie it will represent two holidays, one for the holiday and one for it's lieu equivelent.
//
//	There is a logic to this bizare approach. It allows for applications to differentiate between the two. For example one may have a schedule for weekdays and
//	another for Saturdays and yet another for Sundays and bank holidays. And a completely different schedule for particular holidays such as Christmas day. In
//	this instance the application would need to know the difference between Christmas day falling on a weekend and it's in lieu equivelent.

uint32_t	IsHoliday	(hzSDate& date, uint32_t holFlags)
{
	//	Arguments:	1)	date		Short form date
	//				2)	holFlags	By default, this will select UK public holidays
	//
	//	Returns:	>0	A bitwise value indicating which holiday the given date is
	//				0	If the given date is not a holiday

	_hzfunc(__func__) ;

	static	hzMapM<hzSDate,uint32_t>	hols ;		//	Known public holidays
	static	hzSet<uint32_t>				hol_year ;	//	Holidays calculated for year

	hzXDate		moon ;			//	Date & time of full moon
	hzXDate		limt ;			//	Run-up to full moon date
	hzSDate		test ;			//	Date to test if moon date is Good Friday/Easter Sunday
	uint32_t	Lo ;			//	First possible holiday date
	uint32_t	Hi ;			//	Last possible holiday date
	uint32_t	theday = 0 ;	//	Holiday found
	uint32_t	nIndex ;		//	Holiday iterator

	if (!holFlags)
		holFlags = HZ_PUBHOL_ALLUK ;

	if (!hol_year.Exists(date.Year()))
	{
		//	If we have not yet calculated the holidays for the supplied year, do so now.
		hol_year.Insert(date.Year()) ;

		//	New year
		test.SetDate(date.Year(), 1, 1) ;
		hols.Insert(test, HZ_PUBHOL_NEWYEAR) ;
		if (test.Dow() == 5)
			test += 2 ;
		if (test.Dow() == 6)
			test += 1 ;
		hols.Insert(test, HZ_PUBHOL_LUE_NEWYEAR) ;

		//	Calc Easter. Sunday after full moon after spring Equinox (March 21st)
		moon.SetDateTime("20120109083006") ;
		limt.SetDate(date.Year(), 3, 21) ;
		limt.SetTime(0, 0, 0) ;

		for (; moon <= limt ; moon.altdate(SECOND, 2541298)) ;
		for (; moon.Dow() != 6 ; moon.altdate(DAY, 1)) ; 
		test.SetDate(date.Year(), moon.Month(), moon.Day()) ;
		test -= 2 ; hols.Insert(test, HZ_PUBHOL_GOOD_FRIDAY) ;
		test += 2 ; hols.Insert(test, HZ_PUBHOL_EASTER_SUNDAY) ;
		test += 1 ; hols.Insert(test, HZ_PUBHOL_EASTER_MONDAY) ;

		//	Mayday, First Monday in May
		test.SetDate(date.Year(), 5, 1) ;
		if (test.Dow() == 5)
			test += 2 ;
		if (test.Dow() == 6)
			test += 1 ;
		hols.Insert(test, HZ_PUBHOL_MAYDAY) ;

		//	Last Monday in May
		test.SetDate(date.Year(), 5, 31) ;
		for (; test.Dow() ; test -= 1) ;
		hols.Insert(test, HZ_PUBHOL_SPRING) ;

		//	Last Monday in August
		test.SetDate(date.Year(), 5, 31) ;
		for (; test.Dow() ; test -= 1) ;
		hols.Insert(test, HZ_PUBHOL_SUMMER) ;

		//	Always Dec 25th
		test.SetDate(date.Year(), 12, 25) ;
		hols.Insert(test, HZ_PUBHOL_XMAS) ;
		if (test.Dow() == 5)
			test += 2 ;
		if (test.Dow() == 6)
			test += 1 ;
		hols.Insert(test, HZ_PUBHOL_LUE_XMAS) ;

		//	Boxing day, always Dec 25th
		test.SetDate(date.Year(), 12, 26) ;
		hols.Insert(test, HZ_PUBHOL_BOXING) ;
		if (test.Dow() == 5)
			test += 2 ;
		if (test.Dow() == 6)
			test += 1 ;
		hols.Insert(test, HZ_PUBHOL_LUE_BOXING) ;
	}

	Lo = hols.First(date) ;
	if (Lo < 0)
		return false ;
	Hi = hols.Last(date) ;

	for (nIndex = Lo ; nIndex <= Hi ; nIndex++)
		theday |= hols.GetObj(nIndex) ;

	return theday ;
}

uint32_t	IsHoliday	(hzXDate& date, uint32_t holFlags)
{
	//	Arguments:	1)	date		Full form date
	//				2)	holFlags	By default, this will select UK public holidays
	//
	//	Returns:	>0	A bitwise value indicating which holiday the given date is
	//				0	If the given date is not a holiday

	hzSDate	sd ;	//	Short form date

	sd.SetDate(date.NoDays()) ;
	return IsHoliday(sd, holFlags) ;
}

/*
**	Section 2:	hzCron functions
*/

hzEcode	hzCron::Initialize	(hzSDate& start, hzPeriodicity period, hzMonthrule mrule)
{
	//	hzCron::Initialize
	//
	//	Set up start of era date if Y, M and D are non-zero and if the periodicity requires them.
	//
	//	Arguments:	1)	start	Ths short form date that marks the earliest date for which tasks can be generated
	//				2)	period	The applicable periodicity
	//				3)	mrule	The Monthrule augments the periodicity
	//
	//	Returns:	E_NOINIT	If the settings have not been set up or set up correctly
	//				E_OK		If the periodicity and monthrule settings are a viable combination

	_hzfunc("hzCron::Initialize") ;

	m_Era = start ;

	if (period == HZPERIOD_NEVER)
	{
		m_error = "Periodicity not set" ;
		return E_NOINIT ;
	}

	m_Period = period ;

	return Validate() ;
}

void	hzCron::Exclude	(uint32_t phFlags)
{
	//	Exclude from this hzCron's schedule, one or more public holidays (that are represented as 'HZ_PUBHOL' flags)
	//
	//	Arguments:	1)	phFlags	Public Holiday to be excluded
	//
	//	Returns:	None

	m_exclhols |= phFlags ;
}

void	hzCron::Exclude	(uint32_t month, uint32_t day)
{
	//	Exclude from this hzCron's schedule, dates in the year beyond the supplied month and day
	//
	//	Arguments:	1)	month	The month part of a month/day exclusion
	//				2)	day		The day part
	//
	//	Returns:	None

	m_frMth = 0 ;
	m_frDay = 0 ;
	m_toMth = month ;
	m_toDay = day ;
}

void	hzCron::Exclude	(uint32_t fromMonth, uint32_t fromDay, uint32_t toMonth, uint32_t toDay)
{
	//	Exclude from this hzCron's schedule, dates in the year before the supplied month and day (args 1 and 2) and after the supplied month and
	//	day (args 3 and 4)
	//
	//	Arguments:	1)	fromMonth	The month part of a month/day specifying start of annual exclusion period
	//				2)	fromDay		The day part of the start
	//				3)	toMonth		The month part of a month/day specifying end of the annual exclusion period
	//				4)	toDay		The day part of the end
	//
	//	Returns:	None

	m_frMth = fromMonth ;
	m_frDay = fromDay ;
	m_toMth = toMonth ;
	m_toDay = toDay ;
}

hzEcode	hzCron::Exclude	(const hzString& arg)
{
	//	Exclude from this hzCron's schedule, dates described in the supplied control string. (Can be used directly to interpret config files).
	//
	//	Arguments:	1)	arg		Exclude directive eg "New Year"
	//
	//	Returns:	E_ARGUMENT	If the supplied control string is empty
	//				E_BADVALUE	If the month is not 1-12 or day is not valid for the month
	//				E_OK		If the operation was successful

	_hzfunc("hzCron::Exclude") ;

	hzString	S ;		//	Temp string

	if (!arg)
		return E_ARGUMENT ;

	//	Deal with arg of the form mm:dd
	if (IsDigit(arg[0]))
	{
		if (IsDigit(arg[1]) && arg[1] == CHAR_COLON && IsDigit(arg[1]) && IsDigit(arg[1]))
		{
			m_toMth = (10 * (arg[0] - '0')) + (arg[1] - '0') ;
			m_toDay = (10 * (arg[3] - '0')) + (arg[4] - '0') ;

			if (!m_toMth || m_toMth > 12)		return E_BADVALUE ;
			if (m_toMth == 2 && m_toDay > 29)	return E_BADVALUE ;

			if ((m_toMth == 4 || m_toMth == 6 || m_toMth == 9 || m_toMth == 11) && m_toDay > 30)
				return E_BADVALUE ;
 			if (m_toMth > 31)
				return E_BADVALUE ;

			return E_OK ;
		}

		return E_FORMAT ;
	}

	//	Deal with specific holidays
	S = arg ;
	S.ToLower() ;

	if		(arg == "new year")			m_exclhols |= HZ_PUBHOL_NEWYEAR ;
	else if (arg == "good friday")		m_exclhols |= HZ_PUBHOL_GOOD_FRIDAY ;
	else if (arg == "easter sunday")	m_exclhols |= HZ_PUBHOL_EASTER_SUNDAY ;
	else if (arg == "easter monday")	m_exclhols |= HZ_PUBHOL_EASTER_MONDAY ;
	else if (arg == "mayday")			m_exclhols |= HZ_PUBHOL_MAYDAY ;
	else if (arg == "spring")			m_exclhols |= HZ_PUBHOL_SPRING ;
	else if (arg == "summer")			m_exclhols |= HZ_PUBHOL_SUMMER ;
	else if (arg == "xmas day")			m_exclhols |= HZ_PUBHOL_XMAS ;
	else if	(arg == "boxing day")		m_exclhols |= HZ_PUBHOL_BOXING ;
	else if	(arg == "new year lieu")	m_exclhols |= HZ_PUBHOL_LUE_NEWYEAR ;
	else if	(arg == "xmas lieu")		m_exclhols |= HZ_PUBHOL_LUE_XMAS ;
	else if	(arg == "boxing lieu")		m_exclhols |= HZ_PUBHOL_LUE_BOXING ;
	else if	(arg == "all uk public")	m_exclhols |= HZ_PUBHOL_ALLUK ;
	else
		return E_BADVALUE ;

	return E_OK ;
}

hzEcode	hzCron::Exclude	(const hzString& from, const hzString& to)
{
	//	Exclude from this hzCron's schedule, dates described in the supplied control strings 'from' and 'to'. (Can be used directly to interpret
	//	config files).
	//
	//	Arguments:	1)	from	Early Exclude directive eg "New Year"
	//				2)	to		Later Exclude directive
	//
	//	Returns:	E_ARGUMENT	If the supplied control string is empty
	//				E_BADVALUE	If the month is not 1-12 or day is not valid for the month
	//				E_OK		If the operation was successful

	_hzfunc("hzCron::Exclude") ;

	uint32_t	M ;		//	Month
	uint32_t	D ;		//	Day

	if (!from && !to)
		return E_NODATA ;
	if (!from || !to)
		return E_BADVALUE ;
	if (from.Length() != 5 || to.Length() != 5)
		return E_BADVALUE ;

	if (IsDigit(from[0]) && IsDigit(from[1]) && from[2] == CHAR_COLON && IsDigit(from[3]) && IsDigit(from[4])
		&& IsDigit(to[0]) && IsDigit(to[1]) && to[2] == CHAR_COLON && IsDigit(to[3]) && IsDigit(to[4]))
	{
		//	The 'from' date
		M = (10 * (from[0] - '0')) + (from[1] - '0') ;
		D = (10 * (from[3] - '0')) + (from[4] - '0') ;

		if (!M || M > 12)		return E_BADVALUE ;
		if (M == 2 && D > 29)	return E_BADVALUE ;

		if ((M == 4 || M == 6 || M == 9 || M == 11) && D > 30)
			return E_BADVALUE ;
 		if (D > 31)
			return E_BADVALUE ;

		m_frMth = M ;
		m_frDay = D ;

		//	The 'to' date
		M = (10 * (to[0] - '0')) + (to[1] - '0') ;
		D = (10 * (to[3] - '0')) + (to[4] - '0') ;

		if (!M || M > 12)		return E_BADVALUE ;
		if (M == 2 && D > 29)	return E_BADVALUE ;

		if ((M == 4 || M == 6 || M == 9 || M == 11) && D > 30)
			return E_BADVALUE ;
 		if (D > 31)
			return E_BADVALUE ;

		m_toMth = M ;
		m_toDay = D ;
		return E_OK ;
	}

	return E_FORMAT ;
}

hzEcode	hzCron::Validate	(void)
{
	//	Validate the hzCron settings. The following tests are performed:-
	//
	//		1)	The periodicity must be set.
	//
	//		2)	If the periodicty is every two weeks, then an era start date must be provided. This is because the DoW (day of week) function could
	//			not unambiquously trigger the event.
	//
	//		3)	If the periodicty is monthly or less often, there must be a month-rule to define at which point in the month, the trigger applies.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOINIT	If the settings have not been set up or set up correctly
	//				E_OK		If the settings pass all the tests

	_hzfunc("hzCron::Validate") ;

	hzEcode	rc = E_OK ;		//	Return code

	switch	(m_Period)
	{
	case HZPERIOD_DAY:		//	Every day",
	case HZPERIOD_MONSAT:	//	Mon - Sat",
	case HZPERIOD_WEEKDAY:	//	Weekdays only
	case HZPERIOD_EMON:		//	Every Monday
	case HZPERIOD_ETUE:		//	Every Tuesday
	case HZPERIOD_EWED:		//	Every Wednesday
	case HZPERIOD_ETHR:		//	Every Thursday
	case HZPERIOD_EFRI:		//	Every Friday
	case HZPERIOD_ESAT:		//	Every Saturday
	case HZPERIOD_ESUN:		//	Every Sunday
		break ;

	case HZPERIOD_ALT_MON:	//	Every other Monday
	case HZPERIOD_ALT_TUE:	//	Every other Tuesday
	case HZPERIOD_ALT_WED:	//	Every other Wednesday
	case HZPERIOD_ALT_THR:	//	Every other Thursday
	case HZPERIOD_ALT_FRI:	//	Every other Friday
	case HZPERIOD_ALT_SAT:	//	Every other Saturday
	case HZPERIOD_ALT_SUN:	//	Every other Sunday
		if (!m_Era.IsSet())
		{
			rc = E_NOINIT ;
			m_error = "\tFortnightly invokations must have an era start date otherwise weeks are ambiguous" ;
		}
		break ;

	case HZPERIOD_MONTH:	//	Monthly
	case HZPERIOD_MONTH1:	//	Bi-Monthly (odd)
	case HZPERIOD_MONTH2:	//	Bi-Monthly (even)
	case HZPERIOD_QTR1:		//	Quarterly (1)
	case HZPERIOD_QTR2:		//	Quarterly (2)
	case HZPERIOD_QTR3:		//	Quarterly (3)
	case HZPERIOD_HYEAR1:	//	Half-Yearly (1)
	case HZPERIOD_HYEAR2:	//	Half-Yearly (2)
	case HZPERIOD_HYEAR3:	//	Half-Yearly (3)
	case HZPERIOD_HYEAR4:	//	Half-Yearly (4)
	case HZPERIOD_HYEAR5:	//	Half-Yearly (5)
	case HZPERIOD_HYEAR6:	//	Half-Yearly (6)
	case HZPERIOD_YEAR1:	//	Yearly (Jan)
	case HZPERIOD_YEAR2:	//	Yearly (Feb)
	case HZPERIOD_YEAR3:	//	Yearly (Mar)
	case HZPERIOD_YEAR4:	//	Yearly (Apr)
	case HZPERIOD_YEAR5:	//	Yearly (May)
	case HZPERIOD_YEAR6:	//	Yearly (Jun)
	case HZPERIOD_YEAR7:	//	Yearly (Jul)
	case HZPERIOD_YEAR8:	//	Yearly (Aug)
	case HZPERIOD_YEAR9:	//	Yearly (Sep)
	case HZPERIOD_YEAR10:	//	Yearly (Oct)
	case HZPERIOD_YEAR11:	//	Yearly (Nov)
	case HZPERIOD_YEAR12:	//	Yearly (Dec)
	case HZPERIOD_RANDOM:	//	Completly Random
		if (m_Rule == HZMONTHRULE_INVALID && !m_Era.IsSet())
		{
			rc = E_NOINIT ;
			m_error = "Periods of a month or more must have a valid monthrule" ;
		}
		break ;

	default:
		m_error = "Periodicity not set" ;
		rc = E_NOINIT ;
	}

	if (rc == E_OK)
		m_bActive = true ;

	return rc ;
}

hzEcode	hzCron::TestDate	(hzSDate& D)
{
	//	Apply rules to see if the supplied date is a running day
	//
	//	Arguments:	1)	D	Short form date
	//
	//	Returns:	E_NOINIT	If this hzCron is not initialized
	//				E_RANGE		If the date is out of range
	//				E_OK		If the supplied date is a running day

	_hzfunc("hzCron::TestDate") ;

	hzXDate		tmp ;				//	Current system time and date
	uint32_t	_mon ;				//	First/last Monday
	uint32_t	_tue ;				//	First/last Tuesday
	uint32_t	_wed ;				//	First/last Wednesday
	uint32_t	_thr ;				//	First/last Thursday
	uint32_t	_fri ;				//	First/last Friday
	uint32_t	_sat ;				//	First/last Saturday
	uint32_t	_sun ;				//	First/last Sunday
	uint32_t	dow = D.Dow() ;		//	Day of week of test date
	uint32_t	day = D.Day() ;		//	Day of month of test date
	uint32_t	mon = D.Month() ;	//	Month of year of test date
	uint32_t	mlen ;				//	Length of month

	if (!m_bActive)
	{
		m_error = "Interval not validated/initialized" ;
		return E_NOINIT ;
	}

	switch	(m_Period)
	{
	case HZPERIOD_NEVER:	return E_RANGE ;
	case HZPERIOD_RANDOM:	return E_OK ;
	case HZPERIOD_DAY:		return E_OK ;
	case HZPERIOD_MONSAT:	return dow == 6 ? E_RANGE : E_OK ;
	case HZPERIOD_WEEKDAY:	return dow < 5 ? E_OK : E_RANGE ;
	case HZPERIOD_EMON:		return dow == 0 ? E_OK : E_RANGE ;
	case HZPERIOD_ETUE:		return dow == 1 ? E_OK : E_RANGE ;
	case HZPERIOD_EWED:		return dow == 2 ? E_OK : E_RANGE ;
	case HZPERIOD_ETHR:		return dow == 3 ? E_OK : E_RANGE ;
	case HZPERIOD_EFRI:		return dow == 4 ? E_OK : E_RANGE ;
	case HZPERIOD_ESAT:		return dow == 5 ? E_OK : E_RANGE ;
	case HZPERIOD_ESUN:		return dow == 6 ? E_OK : E_RANGE ;

	case HZPERIOD_ALT_MON:		//	Every other Monday
	case HZPERIOD_ALT_TUE:		//	Every other Tuesday
	case HZPERIOD_ALT_WED:		//	Every other Wednesday
	case HZPERIOD_ALT_THR:		//	Every other Thursday
	case HZPERIOD_ALT_FRI:		//	Every other Friday
	case HZPERIOD_ALT_SAT:		//	Every other Saturday
	case HZPERIOD_ALT_SUN:		//	Every other Sunday
		if (m_Era.IsSet() && !((D.NoDays() - m_Era.NoDays()) % 14))
			return E_OK ;
		return E_RANGE ;
	default:
		break ;
	}

	//	Check if we are in a month in which it will run

	if (m_Period == HZPERIOD_MONTH1 && (!(mon % 2)))	return E_RANGE ;
	if (m_Period == HZPERIOD_MONTH2 && (mon % 2))		return E_RANGE ;
	if (m_Period == HZPERIOD_QTR1 && (mon % 3) != 1)	return E_RANGE ;
	if (m_Period == HZPERIOD_QTR2 && (mon % 3) != 2)	return E_RANGE ;
	if (m_Period == HZPERIOD_QTR3 && (mon % 3) != 0)	return E_RANGE ;
	if (m_Period == HZPERIOD_HYEAR1 && (mon % 6) != 1)	return E_RANGE ;
	if (m_Period == HZPERIOD_HYEAR2 && (mon % 6) != 2)	return E_RANGE ;
	if (m_Period == HZPERIOD_HYEAR3 && (mon % 6) != 3)	return E_RANGE ;
	if (m_Period == HZPERIOD_HYEAR4 && (mon % 6) != 4)	return E_RANGE ;
	if (m_Period == HZPERIOD_HYEAR5 && (mon % 6) != 5)	return E_RANGE ;
	if (m_Period == HZPERIOD_HYEAR6 && (mon % 6) != 0)	return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR1 && mon != 1)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR2 && mon != 2)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR3 && mon != 3)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR4 && mon != 4)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR5 && mon != 5)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR6 && mon != 6)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR7 && mon != 7)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR8 && mon != 8)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR9 && mon != 9)			return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR10 && mon != 10)		return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR11 && mon != 11)		return E_RANGE ;
	if (m_Period == HZPERIOD_YEAR12 && mon != 12)		return E_RANGE ;

	//	We are in a month that it will run so check if rules will allow run on supplied date

	if (m_Rule == HZMONTHRULE_ERA_DERIVE && day != m_Era.Day())	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_FIRST_DAY && day != 1)			return E_RANGE ;

	mlen = monlen(D.Year(), mon) ;

	if (m_Rule == HZMONTHRULE_LAST_DAY && day != mlen)
		return E_RANGE ;

	tmp.SetDate(D.Year(), mon, 1) ;

	switch (tmp.Dow())
	{
	case 0:	_mon = 1 ; _tue = 2 ; _wed = 3 ; _thr = 4 ; _fri = 5 ; _sat = 6 ; _sun = 7 ; break ;
	case 1:	_mon = 7 ; _tue = 1 ; _wed = 2 ; _thr = 3 ; _fri = 4 ; _sat = 5 ; _sun = 6 ; break ;
	case 2:	_mon = 6 ; _tue = 7 ; _wed = 1 ; _thr = 2 ; _fri = 3 ; _sat = 4 ; _sun = 5 ; break ;
	case 3:	_mon = 5 ; _tue = 6 ; _wed = 7 ; _thr = 1 ; _fri = 2 ; _sat = 3 ; _sun = 4 ; break ;
	case 4:	_mon = 4 ; _tue = 5 ; _wed = 6 ; _thr = 7 ; _fri = 1 ; _sat = 2 ; _sun = 3 ; break ;
	case 5:	_mon = 3 ; _tue = 4 ; _wed = 5 ; _thr = 6 ; _fri = 7 ; _sat = 1 ; _sun = 2 ; break ;
	case 6:	_mon = 2 ; _tue = 3 ; _wed = 4 ; _thr = 5 ; _fri = 6 ; _sat = 7 ; _sun = 1 ; break ;
	}

	if (m_Rule == HZMONTHRULE_FIRST_WEEK_DAY && day != _mon)		//	First of month
	if (m_Rule == HZMONTHRULE_FIRST_WORK_DAY && day != _mon)		//	First work day of month (must impliment national holidays)

	if (m_Rule == HZMONTHRULE_1ST_MON && day != _mon)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_1ST_TUE && day != _tue)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_1ST_WED && day != _wed)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_1ST_THR && day != _thr)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_1ST_FRI && day != _fri)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_1ST_SAT && day != _sat)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_1ST_SUN && day != _sun)	return E_RANGE ;

	if (m_Rule == HZMONTHRULE_2ND_MON && day != (_mon+7))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_2ND_TUE && day != (_tue+7))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_2ND_WED && day != (_wed+7))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_2ND_THR && day != (_thr+7))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_2ND_FRI && day != (_fri+7))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_2ND_SAT && day != (_sat+7))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_2ND_SUN && day != (_sun+7))	return E_RANGE ;

	if (m_Rule == HZMONTHRULE_3RD_MON && day != (_mon+14))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_3RD_TUE && day != (_tue+14))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_3RD_WED && day != (_wed+14))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_3RD_THR && day != (_thr+14))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_3RD_FRI && day != (_fri+14))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_3RD_SAT && day != (_sat+14))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_3RD_SUN && day != (_sun+14))	return E_RANGE ;

	if (m_Rule == HZMONTHRULE_4TH_MON && day != (_mon+21))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_4TH_TUE && day != (_tue+21))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_4TH_WED && day != (_wed+21))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_4TH_THR && day != (_thr+21))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_4TH_FRI && day != (_fri+21))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_4TH_SAT && day != (_sat+21))	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_4TH_SUN && day != (_sun+21))	return E_RANGE ;

	tmp.SetDate(D.Year(), mon, monlen(D.Year(), mon)) ;

	switch (tmp.Dow())
	{
	case 0:	_mon = mlen   ; _tue = mlen-6 ; _wed = mlen-5 ; _thr = mlen-4 ; _fri = mlen-3 ; _sat = mlen-2 ; _sun = mlen-1 ; break ;
	case 1:	_mon = mlen-1 ; _tue = mlen   ; _wed = mlen-6 ; _thr = mlen-5 ; _fri = mlen-4 ; _sat = mlen-3 ; _sun = mlen-2 ; break ;
	case 2:	_mon = mlen-2 ; _tue = mlen-1 ; _wed = mlen   ; _thr = mlen-6 ; _fri = mlen-5 ; _sat = mlen-4 ; _sun = mlen-3 ; break ;
	case 3:	_mon = mlen-3 ; _tue = mlen-2 ; _wed = mlen-1 ; _thr = mlen   ; _fri = mlen-6 ; _sat = mlen-5 ; _sun = mlen-4 ; break ;
	case 4:	_mon = mlen-4 ; _tue = mlen-5 ; _wed = mlen-6 ; _thr = mlen-1 ; _fri = mlen   ; _sat = mlen-6 ; _sun = mlen-5 ; break ;
	case 5:	_mon = mlen-5 ; _tue = mlen-4 ; _wed = mlen-5 ; _thr = mlen-6 ; _fri = mlen-1 ; _sat = mlen   ; _sun = mlen-6 ; break ;
	case 6:	_mon = mlen-6 ; _tue = mlen-5 ; _wed = mlen-4 ; _thr = mlen-3 ; _fri = mlen-2 ; _sat = mlen-1 ; _sun = mlen   ; break ;
	}

	//if (m_Rule == HZMONTHRULE_LAST_WEEK_DAY,		//	Last weekday of month
	//if (m_Rule == HZMONTHRULE_LAST_WORK_DAY,		//	Last workday of month

	if (m_Rule == HZMONTHRULE_LAST_MON && day != _mon)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_LAST_TUE && day != _tue)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_LAST_WED && day != _wed)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_LAST_THR && day != _thr)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_LAST_FRI && day != _fri)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_LAST_SAT && day != _sat)	return E_RANGE ;
	if (m_Rule == HZMONTHRULE_LAST_SUN && day != _sun)	return E_RANGE ;

	return E_OK ;
}

hzEcode	hzCron::TestDate	(hzXDate& D)
{
	//	Apply rules to see if date is a running day
	//
	//	Arguments:	1)	D	Short form date
	//
	//	Returns:	E_NOINIT	If this hzCron is not initialized
	//				E_RANGE		If the date is out of range
	//				E_OK		If the supplied date is a running day

	hzXDate	f ;		//	Test date

	f.SetDate(D.Year(), D.Month(), D.Day()) ;
	return TestDate(f) ;
}

hzEcode hzCron::Advance	(hzXDate& date, uint32_t factor)
{
	//	From the supplied date, advance to the Nth next valid date & time for the cron entry, where N is the supplied factor. Skip over
	//	excluded periods or dates.
	//
	//	Arguments:	1)	date	Full date and time. If this is not set it will be set to the current date and time.
	//				2)	factor	No of intervals to advance
	//
	//	Returns:	E_ARGUMENT	If the number of intervals to advance is zero
	//				E_NOINIT	If this hzCron is not active or cannot be advanced
	//				E_OK		If this hzCron set the date of the next occurance

	_hzfunc("hzCron::Advance(hzXDate)") ;

	hzSDate		fr ;		//	From start of exclusion period
	hzSDate		to ;		//	To end of exclusion period
	hzSDate		sd ;		//	System date
	uint32_t	Y ;			//	Year
	uint32_t	M ;			//	Month
	uint32_t	D ;			//	Day of month
	uint32_t	dow ;		//	Day of week
	uint32_t	alt ;		//	Days to advance
	bool		bExcl ;		//	Date is excluded

	if (!m_bActive)						{ date.Clear() ; m_error = "Interval not active" ; return E_RANGE ; }
	if (m_Period == HZPERIOD_NEVER)		{ date.Clear() ; m_error = "Interval has no periodicity" ; return E_RANGE ; }
	if (m_Period == HZPERIOD_INVALID)	{ date.Clear() ; m_error = "Interval has invalid periodicity" ; return E_RANGE ; }
	if (m_Period == HZPERIOD_RANDOM)	{ date.Clear() ; m_error = "Interval is chaotic" ; return E_RANGE ; }

	if (!factor)
		{ m_error = "Factor must be 1 or more" ; return E_ARGUMENT ; }

	if (!date.IsSet())
		date.SysDateTime() ;

	do
	{
		Y = date.Year() ;
		M = date.Month() ;
		D = date.Day() ;
		dow = date.Dow() ;

		//	Every day
		if		(m_Period == HZPERIOD_DAY)		{ alt = 1 ; date.altdate(DAY, alt) ; }

		//  Mon - Sat",
		else if (m_Period == HZPERIOD_MONSAT)	{ alt = dow==5 ? 2 : 1 ; date.altdate(DAY, alt) ; }

		//  Weekdays only
		else if (m_Period == HZPERIOD_WEEKDAY)	{ alt = dow==4 ? 3 : dow==5 ? 2 : 1 ; date.altdate(DAY, alt) ; }

		//	Every [Monday - Sunday]
		else if (m_Period == HZPERIOD_EMON)		{ alt = 7-dow ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ETUE)		{ alt = dow==1 ? 7 : dow<1 ? 1 : 8-dow ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_EWED)		{ alt = dow==2 ? 7 : dow<2 ? 2-dow : 9-dow ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ETHR)		{ alt = dow==3 ? 7 : dow<3 ? 3-dow : 10-dow ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_EFRI)		{ alt = dow==4 ? 7 : dow<4 ? 4-dow : 11-dow ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ESAT)		{ alt = dow==5 ? 7 : dow<5 ? 5-dow : 6 ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ESUN)		{ alt = dow==6 ? 7 : 6-dow ; date.altdate(DAY, alt) ; }

		//	Every other [Monday - Sunday]
		else if (m_Period == HZPERIOD_ALT_MON)	{ alt = 7-dow ; alt += 7 ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ALT_TUE)	{ alt = dow==1 ? 7 : dow<1 ? 1 : 8-dow ; alt += 7 ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ALT_WED)	{ alt = dow==2 ? 7 : dow<2 ? 2-dow : 9-dow ; alt += 7 ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ALT_THR)	{ alt = dow==3 ? 7 : dow<3 ? 3-dow : 10-dow ; alt += 7 ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ALT_FRI)	{ alt = dow==4 ? 7 : dow<4 ? 4-dow : 11-dow ; alt += 7 ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ALT_SAT)	{ alt = dow==5 ? 7 : dow<5 ? 5-dow : 6 ; alt += 7 ; date.altdate(DAY, alt) ; }
		else if (m_Period == HZPERIOD_ALT_SUN)	{ alt = dow==6 ? 7 : 6-dow ; alt += 7 ; date.altdate(DAY, alt) ; }

		else
		{
			//	Periodicities that are multiples of months

			switch	(m_Period)
			{
			//	Monthly
			case HZPERIOD_MONTH:
				M++ ; break ;

			//	Bi-monthly [odd/even]
			case HZPERIOD_MONTH1: case HZPERIOD_MONTH2:
				alt = M % 2 ; M += alt ? alt : 2 ; break ;

			//	Quarterly [month1 - month3]
			case HZPERIOD_QTR1: case HZPERIOD_QTR2: case HZPERIOD_QTR3:
				alt = M % 3 ; M += alt ? alt : 3 ; break ;

			//	Half yearly [month1 - month6]
			case HZPERIOD_HYEAR1: case HZPERIOD_HYEAR2: case HZPERIOD_HYEAR3: case HZPERIOD_HYEAR4: case HZPERIOD_HYEAR5: case HZPERIOD_HYEAR6:
				alt = M % 6 ; M += alt ? alt : 6 ; break ;

			//	Every [Jan - Dec]
			case HZPERIOD_YEAR1: case HZPERIOD_YEAR2: case HZPERIOD_YEAR3: case HZPERIOD_YEAR4: case HZPERIOD_YEAR5: case HZPERIOD_YEAR6:
			case HZPERIOD_YEAR7: case HZPERIOD_YEAR8: case HZPERIOD_YEAR9: case HZPERIOD_YEAR10: case HZPERIOD_YEAR11: case HZPERIOD_YEAR12:
				Y++ ; break ;
			}

			if (M > 12)
				{ Y++ ; M -= 12 ; }
			date.SetDate(Y, M, 1) ;
		}

		//	Have arrived at a possible date. Check if this is not excluded. If it is we do not decrement the factor
		bExcl = false ;

		if (m_frMth)
		{
			//	An exclusion period is in force
			sd.SetDate(date.Year(), date.Month(), date.Day()) ;
			fr.SetDate(date.Year(), m_frMth, m_frDay) ;
			to.SetDate(date.Year(), m_toMth, m_toDay) ;

			if (sd >= fr && sd <= to)
				bExcl = true ;
		}

		if (!m_frMth && m_toMth)
		{
			if (M == m_toMth && D == m_toDay)
				bExcl = true ;
		}

		if (IsHoliday(date, m_exclhols))
			bExcl = true ;

		if (!bExcl)
			factor-- ;
	}
	while (factor) ;

	return E_OK ;
}

hzEcode hzCron::Advance	(hzSDate& d, uint32_t factor)
{
	//	Set the trigger date to the next date according to the periodicity and rules
	//
	//	Arguments:	1)	date	Full date and time
	//				2)	factor	No of intervals to advance
	//
	//	Returns:	E_ARGUMENT	If the number of intervals to advance is zero
	//				E_NOINIT	If this hzCron is not active or cannot be advanced
	//				E_OK		If this hzCron set the date of the next occurance

	_hzfunc("hzCron::Advance(hzSDate)") ;

	hzXDate	fullDate ;		//	Translate the supplied short form date to a date and time (midnight)
	hzEcode	rc ;			//	Return code

	fullDate.SetDate(d.Year(), d.Month(), d.Day()) ;
	rc = Advance(fullDate, factor) ;
	if (rc == E_OK)
		d.SetDate(fullDate.Year(), fullDate.Month(), fullDate.Day()) ;
	return rc ;
}

hzEcode hzCron::Retard	(hzXDate& date, uint32_t factor)
{
	//	Set the trigger date to the previous date according to the periodicity and rules
	//
	//	Arguments:	1)	date	Full date and time
	//				2)	factor	No of intervals to retard
	//
	//	Returns:	E_ARGUMENT	If the number of intervals to retard is zero
	//				E_NOINIT	If this hzCron is not active or cannot be retarded
	//				E_OK		If this hzCron set the date of the previous occurance

	_hzfunc("hzCron::Advance(hzXDate)") ;

	uint32_t	Y ;		//	Year
	uint32_t	M ;		//	Month
	uint32_t	dow ;	//	Day of week
	uint32_t	alt ;	//	No of days to retard

	if (!m_bActive)
	{
		date.Clear() ;
		m_error = "Interval not active" ;
		return E_NOINIT ;
	}

	if (factor < 1)
	{
		m_error = "Factor must be 1 or more" ;
		return E_ARGUMENT ;
	}

	if (!date.IsSet())
		date.SysDateTime() ;

	do
	{
		Y = date.Year() ;
		M = date.Month() ;
		dow = date.Dow() ;

		//	Every day
		if		(m_Period == HZPERIOD_DAY)		{ alt = 1 ; date.altdate(DAY, -alt) ; }

		//  Mon - Sat",
		else if (m_Period == HZPERIOD_MONSAT)	{ alt = dow==0 ? 2 : 1 ; date.altdate(DAY, -alt) ; }

		//  Weekdays only
		else if (m_Period == HZPERIOD_WEEKDAY)	{ alt = dow>4 ? dow - 4 : dow ? 1 : 3 ; date.altdate(DAY, -alt) ; }

		//	Every [Monday - Friday]
		else if (m_Period == HZPERIOD_EMON)		{ alt = dow>0 ? dow : 7 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ETUE)		{ alt = dow>1 ? dow - 1 : dow + 6 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_EWED)		{ alt = dow>2 ? dow - 2 : dow + 5 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ETHR)		{ alt = dow>3 ? dow - 3 : dow + 4 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_EFRI)		{ alt = dow>4 ? dow - 4 : dow + 3 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ESAT)		{ alt = dow>5 ? dow - 5 : dow + 2 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ESUN)		{ alt = dow + 1 ; date.altdate(DAY, -alt) ; }

		//	Every other [Monday - Friday]
		else if (m_Period == HZPERIOD_ALT_MON)	{ alt = dow>0 ? dow : 7 ; alt += 7 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ALT_TUE)	{ alt = dow>1 ? dow - 1 : dow + 6 ; alt += 7 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ALT_WED)	{ alt = dow>2 ? dow - 2 : dow + 5 ; alt += 7 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ALT_THR)	{ alt = dow>3 ? dow - 3 : dow + 4 ; alt += 7 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ALT_FRI)	{ alt = dow>4 ? dow - 4 : dow + 3 ; alt += 7 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ALT_SAT)	{ alt = dow>5 ? dow - 5 : dow + 2 ; alt += 7 ; date.altdate(DAY, -alt) ; }
		else if (m_Period == HZPERIOD_ALT_SUN)	{ alt = dow + 1 ; alt += 7 ; date.altdate(DAY, -alt) ; }

		else
		{
			switch	(m_Period)
			{
			//	Monthly
			case HZPERIOD_MONTH:
				M-- ; break ;

			//	Bi-monthly [odd/even]
			case HZPERIOD_MONTH1:
			case HZPERIOD_MONTH2:
				alt = M % 2 ; M -= alt ? alt : 2 ; break ;

			//	Quarterly [month1 - month3]
			case HZPERIOD_QTR1: case HZPERIOD_QTR2: case HZPERIOD_QTR3:
				alt = M % 3 ; M -= alt ? alt : 3 ; break ;

			//	Half yearly [month1 - month6]
			case HZPERIOD_HYEAR1: case HZPERIOD_HYEAR2: case HZPERIOD_HYEAR3: case HZPERIOD_HYEAR4: case HZPERIOD_HYEAR5: case HZPERIOD_HYEAR6:
				alt = M % 6 ; M -= alt ? alt : 6 ; break ;

			//	Every [Jan - Dec]
			case HZPERIOD_YEAR1: case HZPERIOD_YEAR2: case HZPERIOD_YEAR3: case HZPERIOD_YEAR4: case HZPERIOD_YEAR5: case HZPERIOD_YEAR6:
			case HZPERIOD_YEAR7: case HZPERIOD_YEAR8: case HZPERIOD_YEAR9: case HZPERIOD_YEAR10: case HZPERIOD_YEAR11: case HZPERIOD_YEAR12:
				Y-- ; break ;
			}

			if (M < 1)
				{ Y-- ; M += 12 ; }
			date.SetDate(Y, M, 1) ;
		}

		//	Have arrived at a possible date. Check if this is not excluded. If it is we do not decrement the factor
		if (!IsHoliday(date, m_exclhols))
			factor-- ;
	}
	while (factor) ;

	return E_OK ;
}

hzEcode hzCron::Retard	(hzSDate& d, uint32_t factor)
{
	//	Set the trigger date to the previous date according to the periodicity and rules
	//
	//	Arguments:	1)	date	Short form date
	//				2)	factor	No of intervals to retard
	//
	//	Returns:	E_ARGUMENT	If the number of intervals to retard is zero
	//				E_NOINIT	If this hzCron is not active or cannot be retarded
	//				E_OK		If this hzCron set the date of the previous occurance

	_hzfunc("hzCron::Retard(hzSDate)") ;

	hzXDate	fullDate ;		//	Translate the supplied short form date to a date and time (midnight)
	hzEcode	rc ;			//	Return code

	fullDate.SetDate(d.Year(), d.Month(), d.Day()) ;
	rc = Retard(fullDate, factor) ;
	if (rc == E_OK)
		d.SetDate(fullDate.Year(), fullDate.Month(), fullDate.Day()) ;
	return rc ;
}
