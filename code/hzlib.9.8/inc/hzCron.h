//
//	File:	hzCron.h
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
//	The hzCron class serves as an extension to the HadronZoo Date classes for mainstream scheduling purposes. It applies rules to schedule periodic tasks in 'real world' dates such
//	as the last Sunday of the month. Once initialized a hzCron instance can test if the associated task would run on the supplied date and can be wound back and forth to establish
//	past and future dates for the task.
//
//	Initialization is both a matter of defining the dates upon which the task can run and the time(s) the task will run at on those dates. To enable schedules that differ according
//	to day of week and official holidays etc, multiple hzCron instances can be used. Where multiple sets of date and time rules apply, the effect of the rules are superimposed. For
//	example, if you want a task such as a system backup, to run midnight every day plus every hour during work time Monday to Friday plus four times on Saturday and bank holidays
//	and twice on Sunday - you would perameterize as follows:-
//
//	<datetime:rule period="Every Day">
//		<runtime absolute="00:00:00">
//	</datetime:rule>
//	<datetime:rule period="Monday - Friday">
//		<datetime:execpt>Bank Holidays</datetime:execpt>
//		<runtime freq="Every hour" start="08:30:00" stop="18:30:00"/>
//	</datetime:rule>
//	<datetime:rule period="Every Saturday">
//		<datetime:execpt>25:12<</datetime:execpt>
//		<runtime freq="10800" start="11:30:00" stop="20:30:00"/>
//	</datetime:rule>
//	<datetime:rule period="Every Sunday">
//		<datetime:execpt>25:12<</datetime:execpt>
//		<runtime time="11:30:00"/>
//		<runtime time="17:30:00"/>
//	</datetime:rule>
//
//	Dates are specified by a hzPeriodicity enum and where applicable, a hzMonthrule enum - together with an optional set of exceptions. The associated times may be absolute times 
//	(eg 06.30.00) or a frequency (eg every hour) together with a start and stop time.
//

#ifndef hzCron_h
#define hzCron_h

#include "hzDate.h"

/*
**	Public Holidays
*/

extern	const uint32_t	HZ_PUBHOL_NEWYEAR ;			//	Always Jan 1st
extern	const uint32_t	HZ_PUBHOL_GOOD_FRIDAY ;		//	Friday before Easter
extern	const uint32_t	HZ_PUBHOL_EASTER_SUNDAY ;	//	Easter Sunday
extern	const uint32_t	HZ_PUBHOL_EASTERMONDAY ;	//	Monday before Easter
extern	const uint32_t	HZ_PUBHOL_MAYDAY ;			//	First Monday in May
extern	const uint32_t	HZ_PUBHOL_SPRING ;			//	Last Monday in May
extern	const uint32_t	HZ_PUBHOL_SUMMER ;			//	Last Monday in August
extern	const uint32_t	HZ_PUBHOL_XMAS ;			//	Always Dec 25th
extern	const uint32_t	HZ_PUBHOL_BOXING ;			//	Always Dec 25th

extern	const uint32_t	HZ_PUBHOL_LUE_NEWYEAR ;		//	First working day of the year
extern	const uint32_t	HZ_PUBHOL_LUE_XMAS ;		//	Either Dec 25th or first working day after that date
extern	const uint32_t	HZ_PUBHOL_LUE_BOXING ;		//	Either Dec 26th or first working day after xmas day in lieu
extern	const uint32_t	HZ_PUBHOL_ALLUK ;			//	All UK public holidays

/*
**	Enum definitions
*/

enum	hzPeriodicity
{
	//	Category:	Scheduling
	//
	//	Used to define the periodicity of a periodic task in standard calenda terms. Note that where the periodicity is a month or more, further qualification with a hzMonthrule is
	//	required.

	HZPERIOD_NEVER,			//	Never runs

	//	Not dependent on monthrules or reference date
	HZPERIOD_DAY,			//	Every day
	HZPERIOD_MONSAT,		//	Mon - Sat
	HZPERIOD_WEEKDAY,		//	Weekdays only
	HZPERIOD_EMON,			//	Every Monday
	HZPERIOD_ETUE,			//	Every Tuesday
	HZPERIOD_EWED,			//	Every Wednesday
	HZPERIOD_ETHR,			//	Every Thursday
	HZPERIOD_EFRI,			//	Every Friday
	HZPERIOD_ESAT,			//	Every Saturday
	HZPERIOD_ESUN,			//	Every Sunday

	//	Need reference date to detemine which week is valid
	HZPERIOD_ALT_MON,		//	Every other Monday
	HZPERIOD_ALT_TUE,		//	Every other Tuesday
	HZPERIOD_ALT_WED,		//	Every other Wednesday
	HZPERIOD_ALT_THR,		//	Every other Thursday
	HZPERIOD_ALT_FRI,		//	Every other Friday
	HZPERIOD_ALT_SAT,		//	Every other Saturday
	HZPERIOD_ALT_SUN,		//	Every other Sunday

	//	Dependent on monthrules to determine day of month to run
	HZPERIOD_MONTH,			//	Monthly
	HZPERIOD_MONTH1,		//	Bi-Monthly (odd)
	HZPERIOD_MONTH2,		//	Bi-Monthly (even)
	HZPERIOD_QTR1,			//	Quarterly (1)
	HZPERIOD_QTR2,			//	Quarterly (2)
	HZPERIOD_QTR3,			//	Quarterly (3)
	HZPERIOD_HYEAR1,		//	Half-Yearly (1)
	HZPERIOD_HYEAR2,		//	Half-Yearly (2)
	HZPERIOD_HYEAR3,		//	Half-Yearly (3)
	HZPERIOD_HYEAR4,		//	Half-Yearly (4)
	HZPERIOD_HYEAR5,		//	Half-Yearly (5)
	HZPERIOD_HYEAR6,		//	Half-Yearly (6)
	HZPERIOD_YEAR1,			//	Yearly (Jan)
	HZPERIOD_YEAR2,			//	Yearly (Feb)
	HZPERIOD_YEAR3,			//	Yearly (Mar)
	HZPERIOD_YEAR4,			//	Yearly (Apr)
	HZPERIOD_YEAR5,			//	Yearly (May)
	HZPERIOD_YEAR6,			//	Yearly (Jun)
	HZPERIOD_YEAR7,			//	Yearly (Jul)
	HZPERIOD_YEAR8,			//	Yearly (Aug)
	HZPERIOD_YEAR9,			//	Yearly (Sep)
	HZPERIOD_YEAR10,		//	Yearly (Oct)
	HZPERIOD_YEAR11,		//	Yearly (Nov)
	HZPERIOD_YEAR12,		//	Yearly (Dec)
	HZPERIOD_RANDOM,		//	Completly Random
	HZPERIOD_INVALID		//	Invalid
} ;

enum	hzMonthrule
{
	//	Category:	Scheduling
	//
	//	Used to specify which day in a month, the task should run

	HZMONTHRULE_NULL,				//	Not applicable (default)
	HZMONTHRULE_ERA_DERIVE,			//	From Start Date
	HZMONTHRULE_FIRST_DAY,			//	First of month
	HZMONTHRULE_FIRST_WEEK_DAY,		//	First of month
	HZMONTHRULE_FIRST_WORK_DAY,		//	First of month
	HZMONTHRULE_LAST_DAY,			//	Last of month
	HZMONTHRULE_LAST_WEEK_DAY,		//	Last of month
	HZMONTHRULE_LAST_WORK_DAY,		//	Last of month
	HZMONTHRULE_1ST_MON,			//	1st Monday
	HZMONTHRULE_1ST_TUE,			//	1st Tuesday
	HZMONTHRULE_1ST_WED,			//	1st Wednesday
	HZMONTHRULE_1ST_THR,			//	1st Thursday
	HZMONTHRULE_1ST_FRI,			//	1st Friday
	HZMONTHRULE_1ST_SAT,			//	1st Saturday
	HZMONTHRULE_1ST_SUN,			//	1st Sunday
	HZMONTHRULE_2ND_MON,			//	2nd Monday
	HZMONTHRULE_2ND_TUE,			//	2nd Tuesday
	HZMONTHRULE_2ND_WED,			//	2nd Wednesday
	HZMONTHRULE_2ND_THR,			//	2nd Thursday
	HZMONTHRULE_2ND_FRI,			//	2nd Friday
	HZMONTHRULE_2ND_SAT,			//	2nd Saturday
	HZMONTHRULE_2ND_SUN,			//	2nd Sunday
	HZMONTHRULE_3RD_MON,			//	3rd Monday
	HZMONTHRULE_3RD_TUE,			//	3rd Tuesday
	HZMONTHRULE_3RD_WED,			//	3rd Wednesday
	HZMONTHRULE_3RD_THR,			//	3rd Thursday
	HZMONTHRULE_3RD_FRI,			//	3rd Friday
	HZMONTHRULE_3RD_SAT,			//	3rd Saturday
	HZMONTHRULE_3RD_SUN,			//	3rd Sunday
	HZMONTHRULE_4TH_MON,			//	4th Monday
	HZMONTHRULE_4TH_TUE,			//	4th Tuesday
	HZMONTHRULE_4TH_WED,			//	4th Wednesday
	HZMONTHRULE_4TH_THR,			//	4th Thursday
	HZMONTHRULE_4TH_FRI,			//	4th Friday
	HZMONTHRULE_4TH_SAT,			//	4th Saturday
	HZMONTHRULE_4TH_SUN,			//	4th Sunday
	HZMONTHRULE_LAST_MON,			//	Last Monday
	HZMONTHRULE_LAST_TUE,			//	Last Tuesday
	HZMONTHRULE_LAST_WED,			//	Last Wednesday
	HZMONTHRULE_LAST_THR,			//	Last Thursday
	HZMONTHRULE_LAST_FRI,			//	Last Friday
	HZMONTHRULE_LAST_SAT,			//	Last Saturday
	HZMONTHRULE_LAST_SUN,			//	Last Sunday
	HZMONTHRULE_INVALID				//	Invalid
} ;

/*
**	Non-member prototypes of functions to convert periodicities/monthrules to/from strings
*/

hzPeriodicity	Str2Periodicity	(const hzString& P) ;
hzMonthrule		Str2Monthrule	(const hzString& R) ;
const char*		Periodicity2Str	(const hzPeriodicity P) ;
const char*		Monthrule2Str	(const hzMonthrule mrule) ;

/*
**	Class definitions
*/

class	hzCron
{
	//	Category:	Scheduling
	//
	//	The hzCron class controls when a task may run. There is both a 'run day' component which determines what days a task can be run (e.g. Weekdays only) and
	//	a time component (e.g. every hour from 6.30am to 6.30pm inclusive). This aims to cope with calenda intervals more in keeping with 'real life' scheduling
	//	requirements as well as fixed intervals such as every five minutes or every six hours.
	//
	//	The run day component is implimented as an enumerated periodicity (hzPeriodicity), which if daily or weekly stands on its own. For 14 day periodicities,
	//	specified as 'every other' of whatever day, an 'Era start' date is also needed in order to determine if the current week applies. Periodicities that are
	//	Monthly or longer require an enumerated monthrule.
	//
	//	Specifying run days by the periodicity on its own or in conjunction with either an era start date or a month rule, does not cater for public holidays or
	//	any other days that might be deemed an exception. This is currently dealt with by a 32 bit flag which selects which public holidays are to be observed.
	//	This approach has been depricated and awaits replacement. The intended new approach will keep the per hzCron instance flag but will test this against a
	//	map of dates to flag values. If the current day is in the map and the flags clash the task will not be queued up for that day. Populating the map will
	//	be the responsibility of the application. Note also that there is currently no means to specify what if anything should happen in lieu of excluded run
	//	days.

	hzString		m_error ;		//	Error reports
	hzSDate			m_Era ;			//	First valid date in series
	hzTime			m_Wake ;		//	First time timer will trigger on the first valid date
	hzPeriodicity	m_Period ;		//	Periodicity (frequency)
	hzMonthrule		m_Rule ;		//	When to run in month (for month based events)
	uint32_t		m_ItemOffset ;	//	Start value for items (for the era; eg issue=1000 starts on Jan 1st, 1875)
	uint32_t		m_exclhols ;	//	Exclude [holiday] flags
	uchar			m_frMth ;		//	Exclude from month/day
	uchar			m_frDay ;		//	Exclude from month/day
	uchar			m_toMth ;		//	Exclude to month/day
	uchar			m_toDay ;		//	Exclude to month/day
	bool			m_bActive ;		//	Set by validation

public:
	hzCron	(void)
	{
		m_Period = HZPERIOD_NEVER ;
		m_Rule = HZMONTHRULE_NULL ;
		m_ItemOffset = 0 ;
		m_exclhols = 0 ;
		m_frMth = 0 ;
		m_frDay = 0 ;
		m_toMth = 0 ;
		m_toDay = 0 ;
		m_bActive = false ;
	}

	~hzCron	(void)
	{
	}

	//	Set functions
	void	SetEra		(const hzSDate& era)					{ m_bActive = false ; m_Era = era ; }
	void	SetEra		(const hzSDate& era, uint32_t nOset)	{ m_bActive = false ; m_Era = era ; m_ItemOffset = nOset ; }
	void	SetPeriod	(const hzPeriodicity period)			{ m_bActive = false ; m_Period = period ; }
	void	SetRule		(const hzMonthrule mrule)				{ m_bActive = false ; m_Rule = mrule ; }

	hzEcode	SetEraDate	(uint32_t Y, uint32_t M, uint32_t D)	{ m_bActive = false ; return m_Era.SetDate(Y, M, D) ; }
	hzEcode	SetEraDate	(const hzString& dstr)					{ m_bActive = false ; return m_Era.SetDate(*dstr) ; }
	hzEcode	SetEraTime	(uint32_t h, uint32_t m, uint32_t s)	{ m_bActive = false ; return m_Wake.SetTime(h, m, s) ; }
	hzEcode	SetEraTime	(const hzString& dstr)					{ m_bActive = false ; return m_Wake.SetTime(*dstr) ; }

	void	SetPeriod	(const hzString& pstr)		{ m_bActive = false ; m_Period = Str2Periodicity(pstr) ; }
	void	SetRule		(const hzString& rstr)		{ m_bActive = false ; m_Rule = Str2Monthrule(rstr) ; }

	//	Get functions
	const hzSDate		Era		(void) const	{ return m_Era ; }
	const hzPeriodicity	Period	(void) const	{ return m_Period ; }
	const hzMonthrule	Rule	(void) const	{ return m_Rule ; }
	const hzTime		Time	(void) const	{ return m_Wake ; }

	hzString&	Error	(void)	{ return m_error ; }

	//	Initialize in one go
	hzEcode	Initialize	(hzSDate& start, hzPeriodicity P, hzMonthrule mrule) ;
	void	Exclude		(uint32_t phFlags) ;
	void	Exclude		(uint32_t month, uint32_t day) ;
	void	Exclude		(uint32_t fromMonth, uint32_t fromDay, uint32_t toMonth, uint32_t toDay) ;
	hzEcode	Exclude		(const hzString& arg) ;
	hzEcode	Exclude		(const hzString& from, const hzString& to) ;

	//	Initialize after setting values individually
	hzEcode	Validate	(void) ;

	//	Test if a date meets periodicity and monthrules
	hzEcode	TestDate	(hzSDate& d) ;
	hzEcode	TestDate	(hzXDate& d) ;

	hzEcode	Advance		(hzXDate& date, uint32_t factor = 1) ;
	hzEcode	Advance		(hzSDate& date, uint32_t factor = 1) ;
	hzEcode	Retard		(hzXDate& date, uint32_t factor = 1) ;
	hzEcode	Retard		(hzSDate& date, uint32_t factor = 1) ;

	bool	operator==	(hzCron& op)	{ return m_Era == op.m_Era && m_Period == op.m_Period && m_Rule == op.m_Rule ? true : false ; }
	bool	operator!=	(hzCron& op)	{ return m_Era == op.m_Era && m_Period == op.m_Period && m_Rule == op.m_Rule ? false : true ; }
} ;

/*
**	Prototypes
*/

uint32_t	IsHoliday	(hzSDate& date, uint32_t holFlags = 0) ;
uint32_t	IsHoliday	(hzXDate& date, uint32_t holFlags = 0) ;

#endif	//	hzCron_h
