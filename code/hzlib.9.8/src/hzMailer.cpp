//
//	File:	hzMailer.cpp
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
//	Implimentation of automatic emailing from within an application.
//

#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>

#include "hzChars.h"
#include "hzTextproc.h"
#include "hzMimetype.h"
#include "hzTcpClient.h"
#include "hzIpServer.h"
#include "hzCodec.h"
#include "hzDNS.h"
#include "hzDirectory.h"
#include "hzMailer.h"
#include "hzProcess.h"

using namespace std ;

/*
**	Global Data
*/

global	hzSet<hzString>	_hzGlobal_EmsgHdrs ;

void	HadronZooInitMessageHdrs	(void)
{
	//	Category:	Data Initialization
	//
	//	List of possible email message headers as supplied by IANA. Note the following denotations E-experimental, S-Standard and O-Obsolete

	if (_hzGlobal_EmsgHdrs.Count())
		return ;

	hzString	S ;

	S = "Accept-Language" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Alternate-Recipient" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "ARC-Authentication-Results" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//	E	[RFC8617]
	S = "ARC-Message-Signature" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	E	[RFC8617]
	S = "ARC-Seal" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	E	[RFC8617]
	S = "Archived-At" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5064]
	S = "Authentication-Results	" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC8601]
	S = "Auto-Submitted" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC3834 section 5]
	S = "Autoforwarded" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Autosubmitted" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Bcc" ;										_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Cc" ;										_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Comments" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Content-Identifier		" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Content-Return" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Conversion" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Conversion-With-Loss" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "DL-Expansion-History" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Date" ;									_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Deferred-Delivery" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Delivery-Date" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Discarded-X400-IPMS-Extensions" ;			_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Discarded-X400-MTS-Extensions" ;			_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Disclose-Recipients" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Disposition-Notification-Options" ;		_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Disposition-Notification-To" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "DKIM-Signature" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC6376]
	S = "Downgraded-Bcc" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Cc" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Disposition-Notification-To" ;	_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Final-Recipient" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC6857 Section 3.1.10]
	S = "Downgraded-From" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857 Section 3.1.10]
	S = "Downgraded-In-Reply-To" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC6857 Section 3.1.10]
	S = "Downgraded-Mail-From" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857 Section 3.1.10]
	S = "Downgraded-Message-Id" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC6857 Section 3.1.10]
	S = "Downgraded-Original-Recipient" ;			_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC6857 Section 3.1.10]
	S = "Downgraded-Rcpt-To" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-References" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC6857 Section 3.1.10]
	S = "Downgraded-Reply-To" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Resent-Bcc" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Resent-Cc" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Resent-From" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Resent-Reply-To" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Resent-Sender" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Resent-To" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Return-Path" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-Sender" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Downgraded-To" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5504][RFC6857]
	S = "Encoding" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Encrypted" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Expires" ;									_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Expiry-Date" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "From" ;									_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322][RFC6854]
	S = "Generate-Delivery-Report" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Importance" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "In-Reply-To" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Incomplete-Copy" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Keywords" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Language" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Latest-Delivery-Time" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-Archive" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-Help" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-ID" ;									_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-Owner" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-Post" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-Subscribe" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-Unsubscribe" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "List-Unsubscribe-Post" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC8058]
	S = "Message-Context" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Message-ID" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Message-Type" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "MMHS-Exempted-Address" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.1 and Appendix B.105]
	S = "MMHS-Extended-Authorisation-Info" ;		_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.2 and Appendix B.106]
	S = "MMHS-Subject-Indicator-Codes" ;			_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.3 and Appendix B.107]
	S = "MMHS-Handling-Instructions" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.4 and Appendix B.108]
	S = "MMHS-Message-Instructions" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.5 and Appendix B.109]
	S = "MMHS-Codress-Message-Indicator" ;			_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.6 and Appendix B.110]
	S = "MMHS-Originator-Reference" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.7 and Appendix B.111]
	S = "MMHS-Primary-Precedence" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.8 and Appendix B.101]
	S = "MMHS-Copy-Precedence" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.9 and Appendix B.102]
	S = "MMHS-Message-Type" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.10 and Appendix B.103]
	S = "MMHS-Other-Recipients-Indicator-To" ;		_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.12 and Appendix B.113]
	S = "MMHS-Other-Recipients-Indicator-CC" ;		_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.12 and Appendix B.113]
	S = "MMHS-Acp127-Message-Identifier" ;			_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.14 and Appendix B.116]
	S = "MMHS-Originator-PLAD" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC6477][ACP123 Appendix A1.15 and Appendix B.117]
	S = "MT-Priority" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC6758]
	S = "Obsoletes" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Organization" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	informational	[RFC7681]
	S = "Original-Encoded-Information-Types" ;		_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Original-From" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5703]
	S = "Original-Message-ID" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Original-Recipient" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC3798][RFC5337]
	S = "Originator-Return-Address" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Original-Subject" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5703]
	S = "PICS-Label" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Prevent-NonDelivery-Report" ;				_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Priority" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Received" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322][RFC5321]
	S = "Received-SPF" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC7208]
	S = "References" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Reply-By" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Reply-To" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Require-Recipient-Valid-Since" ;			_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC7293]
	S = "Resent-Bcc" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Resent-Cc" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Resent-Date" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Resent-From" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322][RFC6854]
	S = "Resent-Message-ID" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Resent-Reply-To" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	O	[RFC5322]
	S = "Resent-Sender" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322][RFC6854]
	S = "Resent-To" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Return-Path" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Sender" ;									_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322][RFC6854]
	S = "Sensitivity" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "Solicitation" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC3865]
	S = "Subject" ;									_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "Supersedes" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "TLS-Report-Domain" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC8460]
	S = "TLS-Report-Submitter" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC8460]
	S = "TLS-Required" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC8689]
	S = "To" ;										_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5322]
	S = "VBR-Info" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//	S	[RFC5518]
	S = "X400-Content-Identifier" ;					_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "X400-Content-Return" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "X400-Content-Type" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "X400-MTS-Identifier" ;						_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "X400-Originator" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "X400-Received" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "X400-Recipients" ;							_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
	S = "X400-Trace" ;								_hzGlobal_EmsgHdrs.Insert(S) ;		//		[RFC4021]
}

/*
**	SMTP Codes
*/

static	SMTPCode	_getSmtpCode	(const char* cpBuf)
{
	//	Lift the SMTP return code from a SMTP server response
	//
	//	Arguments:	1)	ptr	Pointer into buffer at point where the HTTP return code is expected.
	//
	//	Returns:	Enum STMP code

	uint32_t	smtpCode ;		//	SMTP return code

	if (!cpBuf || !cpBuf[0])
		return SMTP_INVALID ;

	if (IsDigit(cpBuf[0]) && IsDigit(cpBuf[1]) && IsDigit(cpBuf[2]) && !IsDigit(cpBuf[3]))
		smtpCode = (100 * (cpBuf[0] - '0')) + (10 * (cpBuf[1] - '0')) + (cpBuf[2] - '0') ;
	else
		return SMTP_INVALID ;

	return (SMTPCode) smtpCode ;
}

const char*	ReportErrorStatus	(SMTPCode eSmtpCode)
{
	//	Category:	Diagnostics
	//
	//	Convert SMTP return code to textual equivelent for diagnostic purposes
	//
	//	Arguments:	1)	eSmtpCode	The SMTP return code
	//
	//	Returns:	Pointer to SMTP return code description as cstr

	static	hzString	smtp_code_text	[] =
	{
		"0   SMTP return code indechicperable",
		"101 The server is unable to connect",
		"111 Connection refused or inability to open an SMTP stream",
		"200 Non standard system status message or help reply",
		"211 System status, or system help reply",
		"214 Help message (this reply is useful only to the human user)",
		"220 Service ready",
		"221 Service closing transmission channel",
		"235 Tells client to go ahead",
		"250 Requested mail action okay, completed",
		"251 User not local; will forward to",
		"252 Cannot VRFY user, but will accept message and attempt delivery",
		"334 Login info, either username or password",
		"354 Start mail input; end with <CRLF>.<CRLF>",
		"420 General connection timeout issue",
		"421 Service not available",
		"422 The recipient’s mailbox has exceeded its storage limit",
		"431 Not enough space on the disk - Expected to be temporary",
		"432 Recipient’s incoming mail queue has been stopped",
		"441 The recipient’s server is not responding",
		"442 The connection was dropped during the transmission.",
		"446 The maximum hop count was exceeded for the message",
		"447 Some SMTP servers will send this if they have decided to time out the client (for being too slow).",
		"449 Routing error",
		"450 Requested mail action not taken: mailbox unavailable (E.g., mailbox busy)",
		"451 Requested action aborted: local error in processing",
		"452 Action not taken: no space left",
		"471 Some SMTP servers return this in respose to errors on the part of the connecting client (as they see it).",
		"500 Syntax error, command unrecognized (This may include errors such as cmd line too long)",
		"501 Syntax error in parameters or arguments",
		"502 Command not implemented",
		"503 Bad sequence of commands",
		"504 Command parameter not implemented",
		"510 Bad email address",
		"511 Bad email address",
		"512 Host server for the recipient’s domain name cannot be found in DNS",
		"513 Address type is incorrect",
		"523 Size of your mail exceeds the server limits",
		"530 Authentication problem",
		"541 The recipient address rejected your message",
		"550 Non-existent email address",
		"551 User not local or address invalid.",
		"552 Requested mail action aborted: exceeded storage allocation",
		"553 Mailbox name not allowed (E.g., mailbox syntax incorrect)",
		"554 Transaction failed (Or in the case of a connection-opening response, No SMTP service)",
		"999 Not a 3 digit entity"
	} ;

	switch	(eSmtpCode)
	{
	case SMTP_NOOP:				return *smtp_code_text[0] ;
	case SMTP_CONN_FAIL:		return *smtp_code_text[1] ;
	case SMTP_CONN_REFUSED:		return *smtp_code_text[2] ;
	case SMTP_STATUS_NS:		return *smtp_code_text[3] ;
	case SMTP_STATUS:			return *smtp_code_text[4] ;
	case SMTP_HELP:				return *smtp_code_text[5] ;
	case SMTP_READY:			return *smtp_code_text[6] ;
	case SMTP_QUIT:				return *smtp_code_text[7] ;
	case SMTP_GO_AHEAD:			return *smtp_code_text[8] ;
	case SMTP_OK:				return *smtp_code_text[9] ;
	case SMTP_NOTLOCAL:			return *smtp_code_text[10] ;
	case SMTP_NOVRFY:			return *smtp_code_text[11] ;
	case SMTP_LOGINFO:			return *smtp_code_text[12] ;
	case SMTP_SENDDATA:			return *smtp_code_text[13] ;
	case SMTP_TIMEOUT:			return *smtp_code_text[14] ;
	case SMTP_UNAVAILABLE:		return *smtp_code_text[15] ;
	case SMTP_MBOX_FULL:		return *smtp_code_text[16] ;
	case SMTP_DISK_FULL:		return *smtp_code_text[17] ;
	case SMTP_MBOX_HALT:		return *smtp_code_text[18] ;
	case SMTP_TIMEOUT_PX:		return *smtp_code_text[19] ;
	case SMTP_CONN_XMIT:		return *smtp_code_text[20] ;
	case SMTP_CONN_HOPS:		return *smtp_code_text[21] ;
	case SMTP_TIMEOUT_NS:		return *smtp_code_text[22] ;
	case SMTP_ROUTE:			return *smtp_code_text[23] ;
	case SMTP_MBOXBUSY:			return *smtp_code_text[24] ;
	case SMTP_PROCERROR:		return *smtp_code_text[25] ;
	case SMTP_NOSPACE:			return *smtp_code_text[26] ;
	case SMTP_CLIENT_ERR:		return *smtp_code_text[27] ;
	case SMTP_BADCOMMAND:		return *smtp_code_text[28] ;
	case SMTP_BADSYNTAX:		return *smtp_code_text[29] ;
	case SMTP_NOCOMMAND:		return *smtp_code_text[30] ;
	case SMTP_BAD_CMD_SEQ:		return *smtp_code_text[31] ;
	case SMTP_BAD_PARAM:		return *smtp_code_text[32] ;
	case SMTP_BAD_SNDR_ADDR:	return *smtp_code_text[33] ;
	case SMTP_BAD_RCPT_ADDR:	return *smtp_code_text[34] ;
	case SMTP_BAD_HOST:			return *smtp_code_text[35] ;
	case SMTP_BAD_ADDR_TYPE:	return *smtp_code_text[36] ;
	case SMTP_MSG_SIZE:			return *smtp_code_text[37] ;
	case SMTP_BAD_SERVER:		return *smtp_code_text[38] ;
	case SMTP_BAD_SENDER:		return *smtp_code_text[39] ;
	case SMTP_NACK:				return *smtp_code_text[40] ;
	case SMTP_TRYOTHER:			return *smtp_code_text[41] ;
	case SMTP_MBOXFULL:			return *smtp_code_text[42] ;
	case SMTP_BADNAME:			return *smtp_code_text[43] ;
	case SMTP_FAILED:			return *smtp_code_text[44] ;
	}

	return *smtp_code_text[45] ;
}

static	hzString	s_EmailSystem ;		//	Email unique-Id seed

/*
**	MIME Types
*/

/*
**	Generic mail functions
*/

bool	GenerateMailId	(hzString& MailId)
{
	//	Category:	Mailer
	//
	//	Purpose:	Generate a unique mail id for sending out emails. The mail id will be of the form:-
	//					YYYYMMDDHHMMSS.pid.email_system_name@hostname.
	//				The default for email_system_name is 'hadronzoo-mailer'.
	//
	//	Arguments:	1)	MailId	Unique Mail Id supplied as a string
	//
	//	Returns:	True	If operation successful
	//				False	Otherwise

	hzChain	Z ;		//	Working chain buffer
	hzXDate	d ;		//	Current system time and date stamp

	if (!s_EmailSystem)
		s_EmailSystem = "hadronzoo-mailer" ;

	d.SysDateTime() ;

	Z.Printf("%04d%02d%02d%02d%02d%02d.%d.%s@%s",
		d.Year(), d.Month(), d.Day(), d.Hour(), d.Min(), d.Sec(), getpid(), *s_EmailSystem, *_hzGlobal_Hostname) ;

	MailId = Z ;
	return MailId ? true : false ;
}

void	CreateMessageID	(hzString& mailId, const hzDomain& domain)
{
	//	Category:	Mailer
	//
	//	As described in the Epistula document, the CreateMessageID function generates ids with the following components:-
	//
    //	- Date and time of the form YYYYMMDD.hhmmss
    //	- A 'last second' counter or LSC (see below)
    //	- The client IP address
    //	- A nmemonc of 'ep' (Epistula) or 'ds' (Dissemino)
    //
    //	So the overall form is YYYYMMDD.hhmmss.LSC_clientIP_ep@domainName
    //
    //	To cope with system clock retartations this function uses a static variable of 'latest known time' (LKT) and a dynamic variable of 'last-second' counter (LSC). The LKT has
    //	an intial value of midnight, Jan 1st, 0000. The process begins by reading the clock to an accuracy of one second. If the clock shows a LATER time than the LKT, the LKT is
    //	set equal to the clock and the LSC is set to 0 (this always happens in the first instance). If the clock shows a time EQUAL to or EARLIER than the LKT, the LKT is unchanged
    //	but the LSC is incremented by 1. In ALL cases the id is formed of the LKT and the LSC, however where the clock was found to have an EARLIER time than the LKT, the LKT and
    //	the LSC are separated by an underscore rather than a period.
	//
	//	Arguments:	1)	MailId	This will contain the generate message id.
	//				2)	domain	Domain name (final part of id)
	//
	//	Returns:	None

	static	hzXDate		LKT ;		//	Last known time

	hzChain		Z ;					//	Working chain buffer
	hzXDate		now ;				//	Current time
	uint32_t	LSC ;				//	Last second counter
	bool		bUscore = false ;	//	Use underscore

	//	Read system clock
	now.SysDateTime() ;

	if (now.AsEpoch() > LKT.AsEpoch())
	{
		LKT = now ;
		LSC = 0 ;
	}
	else
	{
		if (now.AsEpoch() < LKT.AsEpoch())
			bUscore = true ;
		LSC++ ;
	}

	Z.Printf("%04d%02d%02d%02d%02d%02d", LKT.Year(), LKT.Month(), LKT.Day(), LKT.Hour(), LKT.Min(), LKT.Sec()) ;
	if (bUscore)
		Z.AddByte(CHAR_USCORE) ;
	else
		Z.AddByte(CHAR_PERIOD) ;
	Z.Printf("%d", LSC) ;

	//Z.Printf("%04d%02d%02d%02d%02d%02d.%d.%s@%s",
	//	d.Year(), d.Month(), d.Day(), d.Hour(), d.Min(), d.Sec(), getpid(), *s_EmailSystem, *_hzGlobal_Hostname) ;

	mailId = Z ;
}

/*
**	hzEmail member functions
*/

void	hzEmail::Clear		(void)
{
	//	Clear hzEmail instance
	//
	//	Arguments:	None
	//	Returns:	None

	m_Recipients.Clear() ;		//	List of main recipient(s)
	m_CC.Clear() ;				//	List of cc recipient(s)
	m_BCC.Clear() ;				//	List of bcc recipient(s)
	m_Attachments.Clear() ;		//	List of file attachments (added by Attach() as part of forming an outgoing email)

	m_Dets.Clear() ;			//	List of file attachments (added by Detach() called on an incomming email)
	m_Parts.Clear() ;			//	List of message parts
	m_Hdrs.Clear() ;			//	Message headers: Note this is only filled by Import. Composure of an outgoing message does not populate this list.

	m_Final.Clear() ;			//	Fully composed email
	m_Text.Clear() ;			//	Body of email (Text part)
	m_Html.Clear() ;			//	Body of email (Html part)
	m_Err.Clear() ;				//	For error reporting (e.g. failed import)
	m_Date = 0 ;				//	Date of email (recv only)
	m_Id = 0 ;					//	Mail id (typically as set by first SMTP server to handle it)
	m_DomainSender = 0 ;		//	Sender's domain (all possible sender address varients must agree on this)
	m_RealReply = 0 ;			//	Real name given in ReplyTo header (if any)
	m_RealFrom = 0 ;			//	Real name of sender
	m_RealTo = 0 ;				//	Real name of recipient (rarely used)
	m_Subject = 0 ;				//	Subject
	m_AddrReturn = 0 ;			//	Return address (Return-path header)
	m_AddrReply = 0 ;			//	Address given in the ReplyTo header
	m_AddrSender = 0 ;			//	Address of sender as established in SMTP session. Epistula saves this as X-Epistula-Ingress
	m_AddrFrom = 0 ;			//	Address of sender given by From header
	m_AddrTo = 0 ;				//	Address of primary recipient
	m_SenderIP.Clear() ;		//	Sender IP address

	//	Reset types
	m_ContType = HZCONTENTTYPE_UNDEFINED ; 
	m_Encoding = HZCONTENTENCODE_UNDEFINED ;
}

//	FnSet:		Mail Composition
//	Category:	Mail Composition
//
//	Func:	hzEmail::SetSender(const char* cpSender, const char* cpRealname)
//	Func:	hzEmail::AddRecipient(hzEmaddr& ema)
//	Func:	hzEmail::AddRecipientCC(hzEmaddr& ema)
//	Func:	hzEmail::AddRecipientBCC(hzEmaddr& ema)
//	Func:	hzEmail::SetSubject(const char* cpSubject)
//	Func:	hzEmail::AddBody(const char* msg)
//	Func:	hzEmail::AddBody(hzString& S)
//	Func:	hzEmail::AddBody(hzChain& Z)
//	Func:	hzEmail::AddAttachment(const char* dir, const char* fname, hzMimetype mtype)
//	Func:	hzEmail::Compose(void)

hzEcode	hzEmail::SetSender	(const char* cpSender, const char* cpRealname)
{
	//	Category:	Mail Composition
	//
	//	Set the email's sender.
	//
	//	Arguments:	1)	cpSender	The sender email address
	//				2)	cpRealname	The apparent from field
	//
	//	Returns:	E_ARGUMENT	If the sender email address is not supplied
	//				E_FORMAT	If the sender email address is malformed
	//				E_OK		If this email sender is successfully set

	_hzfunc("hzEmail::SetSender") ;

	if (!cpSender || !cpSender[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No sender supplied") ;

	if (!IsEmaddr(cpSender))
		return hzerr(_fn, HZ_ERROR, E_FORMAT, "The supplied sender is not a valid email address") ;

	m_AddrSender = cpSender ;
	m_RealFrom = cpRealname ;
	return E_OK ;
}

hzEcode	hzEmail::AddRecipient		(hzEmaddr& ema)
{
	//	Category:	Mail Composition
	//
	//	Add a 'To' recipient to a hzEmail instance using a formal email address (hzEmaddr) as argument
	//
	//	Arguments:	1)	ema		Email address to be added to recipient list
	//
	//	Returns:	E_ARGUMENT	If the supplied recipient email address is not set
	//				E_OK		If the recipient is added

	if (!ema)
		return E_ARGUMENT ;

	return m_Recipients.Add(ema) ;
}

hzEcode	hzEmail::AddRecipientCC		(hzEmaddr& ema)
{
	//	Category:	Mail Composition
	//
	//	Add a 'Cc' recipient to a hzEmail instance using a formal email address (hzEmaddr) as argument
	//
	//	Arguments:	1)	ema		Email address to be added to CC recipient list
	//
	//	Returns:	E_ARGUMENT	If the supplied recipient email address is not set
	//				E_OK		If the recipient is added

	if (!ema)
		return E_NOINIT ;

	return m_CC.Add(ema) ;
}

hzEcode	hzEmail::AddRecipientBCC	(hzEmaddr& ema)
{
	//	Category:	Mail Composition
	//
	//	Add a 'Bcc' recipient to a hzEmail instance using a formal email address (hzEmaddr) as argument
	//
	//	Arguments:	1)	ema		Email address to be added to BCC recipient list
	//
	//	Returns:	E_ARGUMENT	If the supplied recipient email address is not set
	//				E_OK		If the recipient is added

	if (!ema)
		return E_NOINIT ;

	return m_BCC.Add(ema) ;
}

hzEcode	hzEmail::AddRecipient	(const char* cpRecipient)
{
	//	Category:	Mail Composition
	//
	//	Add a 'To' recipient to a hzEmail instance using an informal email address (const char*) as argument
	//
	//	Arguments:	1)	ema		Email address to be added to recipient list
	//
	//	Returns:	E_ARGUMENT	If the recipient email address is not supplied
	//				E_FORMAT	If the recipient email address is malformed
	//				E_OK		If the recipient is added

	hzEmaddr	e ;		//	Email address to add

	if (!cpRecipient)
		return E_ARGUMENT ;

	e = cpRecipient ;
	if (!e)
		return E_FORMAT ;

	return m_Recipients.Add(e) ;
}

hzEcode	hzEmail::AddRecipientCC	(const char* cpRecipient)
{
	//	Category:	Mail Composition
	//
	//	Add a 'Cc' recipient to a hzEmail instance using an informal email address (const char*) as argument
	//
	//	Arguments:	1)	ema		Email address to be added to CC recipient list
	//
	//	Returns:	E_ARGUMENT	If the recipient email address is not supplied
	//				E_FORMAT	If the recipient email address is malformed
	//				E_OK		If the recipient is added

	hzEmaddr	e ;		//	Email address to add

	e = cpRecipient ;
	if (!e)
		return E_MEMORY ;

	return m_CC.Add(e) ;
}

hzEcode	hzEmail::AddRecipientBCC	(const char* cpRecipient)
{
	//	Category:	Mail Composition
	//
	//	Add a 'Bcc' recipient to a hzEmail instance using an informal email address (const char*) as argument
	//
	//	Arguments:	1)	ema		Email address to be added to BCC recipient list
	//
	//	Returns:	E_ARGUMENT	If the recipient email address is not supplied
	//				E_FORMAT	If the recipient email address is malformed
	//				E_OK		If the recipient is added

	hzEmaddr	e ;		//	Email address to add

	e = cpRecipient ;
	if (!e)
		return E_MEMORY ;

	return m_BCC.Add(e) ;
}

hzEcode	hzEmail::SetSubject		(const char* cpSubject)
{
	//	Category:	Mail Composition
	//
	//	Set the subject of the email.
	//
	//	Arguments:	1)	cpSubject	Subject matter
	//
	//	Returns:	E_ARGUMENT	If the subject matter is not supplied
	//				E_OK		If the subject is set

	if (!cpSubject || !cpSubject[0])
		return E_ARGUMENT ;

	m_Subject = cpSubject ;
	return E_OK ;
}

hzEcode	hzEmail::AddBody	(hzChain& Z)
{
	//	Category:	Mail Composition
	//
	//	Addd chain content to the email body. Can be called multiple times.
	//
	//	Arguments:	1)	Z	Mail body as chain
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_OK		If the body is added

	if (!Z.Size())
		return E_NODATA ;

	m_Text += Z ;
	return E_OK ;
}

hzEcode	hzEmail::AddBody	(hzString& S)
{
	//	Category:	Mail Composition
	//
	//	Addd a string to the email body. Can be called multiple times.
	//
	//	Arguments:	1)	S	Mail body as string
	//
	//	Returns:	E_NODATA	If the supplied body is empty
	//				E_OK		If the body is added

	if (!S)
		return E_NODATA ;

	m_Text += S ;
	return E_OK ;
}

hzEcode	hzEmail::AddBody	(const char* msg)
{
	//	Category:	Mail Composition
	//
	//	Add a char string to the email body. Can be called multiple times.
	//
	//	Arguments:	1)	msg	Mail body as null terminated string	
	//
	//	Returns:	E_NODATA	If the supplied chain is empty
	//				E_OK		If the body is added

	if (!msg || !msg[0])
		return E_NODATA ;

	m_Text += msg ;
	return E_OK ;
}

hzEcode	hzEmail::AddAttachment	(const char* dir, const char* fname, hzMimetype mtype)
{
	//	Category:	Mail Composition
	//
	//	Add an attachment using both a directory path and a filename to name the file.
	//
	//	Arguments:	1)	dir		Directory
	//				2)	fname	Filename
	//				3)	mtype	MIME type
	//
	//	Returns:	E_ARGUMENT	If either the directory, filename or MIME type is not supplied
	//				E_NOTFOUND	If the file suggested as an attachment does not exist
	//				E_OK		If the attachment was added
	//				

	_hzfunc("hzEmail::AddAttachment(1)") ;

	_efile	A ;		//	Attchement to add

	if (!dir || !dir[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No directory supplied") ;

	if (!fname || !fname[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No filename supplied") ;

	if (mtype == HMTYPE_INVALID)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Invalid MIME type") ;

	A.m_Filepath = dir ;
	A.m_Filepath += "/" ;
	A.m_Filepath += fname ;
	A.m_eType = mtype ;

	if (TestFile(*A.m_Filepath) != E_OK)
		return E_NOTFOUND ;

	return m_Attachments.Add(A) ;
}

hzEcode	hzEmail::AddAttachment	(const char* fpath, hzMimetype mtype)
{
	//	Category:	Mail Composition
	//
	//	Add an attachment using a full file path to name the file.
	//
	//	Arguments:	1)	fpath	Full path to file
	//				2)	mtype	MIME type
	//
	//	Returns:	E_ARGUMENT	If either the filename or MIME type is not supplied
	//				E_NOTFOUND	If the file suggested as an attachment does not exist
	//				E_OK		If the attachment was added

	_hzfunc("hzEmail::AddAttachment(2)") ;

	_efile	A ;		//	Attchement to add

	if (!fpath || !fpath[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No filename supplied") ;

	if (mtype == HMTYPE_INVALID)
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "Invalid MIME type") ;

	A.m_Filepath = fpath ;
	A.m_eType = mtype ;

	if (TestFile(*A.m_Filepath) != E_OK)
		return E_NOTFOUND ;

	return m_Attachments.Add(A) ;
}

hzEcode	hzEmail::AddAttachment	(const char* fpath)
{
	//	Category:	Mail Composition
	//
	//	Add an attachment using a full file path to name the file, but do not specify the file's MIME type. This will be assigned later on the
	//	basis of file ending.
	//
	//	Arguments:	1)	fpath	Full path to file
	//
	//	Returns:	E_ARGUMENT	If the filename is not supplied
	//				E_NOTFOUND	If the file suggested as an attachment does not exist
	//				E_OK		If the attachment was added

	_hzfunc("hzEmail::AddAttachment(3)") ;

	_efile	A ;		//	Attchement to add

	if (!fpath || !fpath[0])
		return hzerr(_fn, HZ_ERROR, E_ARGUMENT, "No filename supplied") ;

	A.m_Filepath = fpath ;

	if (TestFile(*A.m_Filepath) != E_OK)
		return E_NOTFOUND ;

	return m_Attachments.Add(A) ;
}

hzEcode	hzEmail::Compose	(void)
{
	//	Category:	Mail Composition
	//
	//	Once everything has been added in, the sender, the complete list of reciepients, the subject, the body and any attachments,
	//	compile into a single chain, the header and the MIME.
	//
	//	Arguments:	None
	//
	//	Returns:	E_NOTFOUND	If any attachment filepath specified does not exist
	//				E_TYPE		If any attachment filepath names a directory entry that is not a file
	//				E_NODATA	If any attachment filepath is a file but empty
	//				E_OPENFAIL	If any attachment file specified could not be opened for reading
	//				E_OK		If this email message was successfully composed

	_hzfunc("hzEmail::Compose") ;

	hzList<hzEmaddr>::Iter	rx ;	//	Recipients iterator
	hzList<_efile>::Iter	iA ;	//	Attachments iterator

	ifstream		is ;			//	Attachment input stream
	hzEmaddr		ema ;			//	Email address
	_efile			A ;				//	Attachment
	hzXDate			now ;			//	Current time and date stamp
	hzChain			filedata ;		//	Working output chain
	hzLogger*		plog ;			//	Current thread logger
	const char*		i ;				//	Forward slash position
	uint32_t		pid ;			//	Process id (part of mail-id)
	char			idbuf[60] ;		//	Mail-Id buffer
	hzEcode			rc = E_OK ;		//	Return code

	plog = GetThreadLogger() ;
	if (!plog)
		Fatal("%s: No thrread logger\n", *_fn) ;

	m_Final.Clear() ;
	m_Final.Printf("From: %s <%s>\r\n", *m_RealFrom, *m_AddrSender) ;

	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("%s: Have a total of %d std recipients\n", *_fn, m_Recipients.Count()) ;

	//	Deal with main recipients
	rx = m_Recipients ;
	ema = rx.Element() ;
	m_Final.Printf("To: %s\r\n", *ema) ;

	for (rx++ ; rx.Valid() ; rx++)
	{
		ema = rx.Element() ;
		m_Final.Printf("    %s\r\n", *ema) ;
	}

	//	Deal with carbon copy recipients
	if (m_CC.Count())
	{
		rx = m_CC ;
		ema = rx.Element() ;
		m_Final.Printf("Cc: %s\r\n", *ema) ;

		for (rx++ ; rx.Valid() ; rx++)
		{
			ema = rx.Element() ;
			m_Final.Printf("    %s\r\n", *ema) ;
		}
	}

	//	Deal with blind carbon copy recipients
	if (m_BCC.Count())
	{
		rx = m_BCC ;
		ema = rx.Element() ;
		m_Final.Printf("Bcc: %s\r\n", *ema) ;

		for (rx++ ; rx.Valid() ; rx++)
		{
			ema = rx.Element() ;
			m_Final.Printf("    %s\r\n", *ema) ;
		}
	}

	m_Final.Printf("Subject: %s\r\n", *m_Subject) ;

	if (m_Attachments.Count())
	{
		now.SysDateTime() ;
		pid = getpid() ;
		sprintf(idbuf, "%d--%04d%02d%02d%02d%02d%02d.%06d--%x",
			pid, now.Year(), now.Month(), now.Day(), now.Hour(), now.Min(), now.Sec(), now.uSec(), pid) ;

		m_Final << "MIME-Version: 1.0\r\n" ;
		m_Final.Printf("Content-Type: multipart/mixed; boundary=\"%s\"\r\n\r\n", idbuf) ;
		m_Final << "This is a multi-part message in MIME format.\r\n\r\n" ;
		m_Final.Printf("--%s\r\n", idbuf) ;
		m_Final << "Content-Type: text/plain; charset=utf-8; format=flowed\r\n" ;
		m_Final << "Content-Transfer-Encoding: 7bit\r\n\r\n" ;

		m_Final += m_Text ;

		m_Final << "\r\n\r\n" ;

		for (iA = m_Attachments ; iA.Valid() ; iA++)
		{
			A = iA.Element() ;

			rc = OpenInputStrm(is, A.m_Filepath, *_fn) ;
			if (rc != E_OK)
				break ;

			filedata.Clear() ;
			filedata << is ;

			m_Final.Printf("--%s\r\n", idbuf) ;

			m_Final << "Content-Type: " ;
			m_Final << Mimetype2Txt(A.m_eType) ;

			i = strrchr(*A.m_Filepath, CHAR_FWSLASH) ;
			if (i)
				i++ ;
			else
				i = A.m_Filepath ;

			m_Final.Printf("; name=\"%s\"\r\n", i) ;
			m_Final << "Content-Transfer-Encoding: base64\r\n" ;
			m_Final.Printf("Content-Disposition: attachment; filename=\"%s\"\r\n\r\n", i) ;

			Base64Encode(m_Final, filedata) ;
			m_Final << "\r\n" ;
			is.close() ;
			is.clear() ;
		}

		m_Final.Printf("--%s--\r\n", idbuf) ;
	}
	else
	{
		//	No attachements. No multipart MIME thing
		m_Final << "MIME-Version: 1.0\r\n" ;
		m_Final << "Content-Type: text/plain; charset=utf-8; format=flowed\r\n" ;
		m_Final << "Content-Transfer-Encoding: 7bit\r\n\r\n" ;
		m_Final += m_Text ;
		m_Final << "\r\n\r\n" ;
	}

	return rc ;
}

hzEcode	hzEmail::Decompose	(void)
{
	//	Decomposing (an incoming) email which where applicable, will have a message body comprising multiple parts. These parts are embedded in the message body
	//	normally as base64 encoded form and separated by a boundary sequence. Commonly the whole body will comprise the message in plain text, the massage again
	//	in HTML and finally, the attachments.
	//
	//	The function will first clear then populate the m_Dets array of chains
	//
	//	Arguments:	None
	//
	//	Returns:	E_NODATA	If this email has no body
	//				E_OK		If this email does have a body

	_hzfunc("hzEmail::Decompose") ;

	hzSet<hzString> bounds ;		//	Set of boundary markers found

	hzChain		Z ;				//	Working chain buffer
	chIter		zi ;			//	Chain iterator
	hzString	filename ;		//	Attachment filename
	hzString	S ;				//	Boundary marker
	uint32_t	nB ;			//	Boundary iterator
	bool		bBoundary ;		//	Boundary indicator
	//hzEcode		rc = E_OK ;		//	Return code

	if (!m_Final.Size())
		return E_NODATA ;

	for (zi = m_Final ; !zi.eof() ; zi++)
	{
		if (zi == "boundary=")
		{
			//	All instances of this mark the id of a MIME block

			Z = "--" ;
			for (zi += 10 ; *zi != CHAR_DQUOTE ; zi++)
				Z.AddByte(*zi) ;
			S = Z ;
			bounds.Insert(S) ;
			continue ;
		}
	}

	if (!bounds.Count())
		return E_OK ;

	for (zi = m_Final ; !zi.eof() ; zi++)
	{
		//	First establish boundary markers

		//if (z == "Content-Disposition: attachment; filename=\"")
		if (zi == "Content-Disposition: attachment;")
		{
			Z.Clear() ;

			//	Go to one past first quote
			for (zi += 32 ; *zi != CHAR_DQUOTE ; zi++) ;

			//	Garner the quoted filename
			for (zi++ ; *zi != CHAR_DQUOTE ; zi++)
			{
				if (*zi == CHAR_CR)	continue ;
				if (*zi == CHAR_NL)	continue ;

				Z.AddByte(*zi) ;
			}

			filename = Z ;
			Z.Clear() ;

			//	On with job - Bypass whitespace
			for (zi++ ; *zi <= CHAR_SPACE ; zi++) ;

			//	Untill we see a boundary
			for (bBoundary = false ; !zi.eof() ; zi++)
			{
				if (*zi == CHAR_MINUS)
				{
					if (zi == "--")
					{
						for (nB = 0 ; nB < bounds.Count() ; nB++)
						{
							S = bounds.GetObj(nB) ;

							if (zi == *S)
							{
								bBoundary = true ;
								break ;
							}
						}
					}

					if (bBoundary)
						break ;
				}

				Z.AddByte(*zi) ;
			}

			m_Dets.Insert(filename, Z) ;
			Z.Clear() ;
		}
	}

	return E_OK ;
}

hzEcode	hzEmail::SendSmtp	(const char* server, const char* uname, const char* passwd)
{
	//	Directly relay the email to the target domain SMTP server for immediate transmission, as opposed to queuing the email to the local SMTP server.
	//
	//	This might be called by a HTTP application which will need to know the outcome as soon as possible, in order to formulate a HTTP response based on that
	//	outcome. A common scenario is email address verification during website member registration. An email is sent containing a code which the user must then
	//	enter into a form in order to verify they have access to the email account.
	//
	//	It is worth trying to get this right so the following is taken into account:-
	//
	//		1)	The DNS may be down or temporarily busy so it is not possible to determine if the domain part of the email address actually exists.#
	//		2)	The DNS provides the mail servers for the domain but none can be connected to at the moment.
	//		3)	The DNS is up but cannot find a mail server for the domain
	//		4)	The mail server was connected to but rejected the sender (the web application on your server)
	//		5)	The mail server was connected to but denied the existance of the recipient.
	//		6)	The mail server accepted the email. 
	//
	//	Of the above, scenarios 1 and 2 are inconclusive. Yet the HTTP response to the form submission (containing the user email address), must be sent without
	//	delay. The email should be queued and the user notified of this. The message should make clear the email could not be verified but if it is a live email
	//	address, the verification code will arrive in due course. Any sessions left unfullfilled after a certain period must be purged.
	//
	//	Senarios 3, 4 and 5 are terminal. The email will not be sent at any point. Only scenario 6 is where the response can say "an email has been sent".
	//
	//	Note that bypassing the local SMTP server foregoes any email management the local SMTP server may offer. If you need a permanent and easily searchable
	//	record of the email, the calling applications will have to make arrangements for this.
	//
	//	Arguments:	1)	server	The alien domain SMTP server
	//				2)	uname	The username for SMTP auth
	//				3)	passwd	The password for SMTP auth
	//
	//	Returns:	E_ARGUMENT	If no server, username, password or sender is supplied or if no recipients are specified
	//				E_NOACCOUNT	If the recipient does not exist
	//				E_HOSTRETRY	If the server is too busy

	_hzfunc("hzEmail::SendSmtp") ;

	hzList<hzEmaddr>::Iter	rx ;		//	Recipients iterator

	hzTcpClient	C ;						//	Client connection to destination server
	hzChain		inp ;					//	Input chain
	hzChain		oup ;					//	Output chain
	chIter		ci ;					//	For iterating the composed chain
	hzLogger*	plog ;					//	Current thread logger
	char*		i ;						//	Send buffer populator
	hzString	S ;						//	Temporary string
	hzEmaddr	e ;						//	Email address
	uint32_t	nRecv ;					//	Bytes received by Recv()
	uint32_t	nSend ;					//	Bytes sent by Send()
	uint32_t	nTotal = 0 ;			//	Total bytes sent (diagnostics only)
	char		sbuf[HZ_MAXPACKET+4] ;	//	Send buffer
	char		rbuf[HZ_MAXPACKET+4] ;	//	Recv buffer
	SMTPCode	smtpCode ;				//	Server SMTP return code
	hzEcode		rc = E_OK ;				//	Return code

	//	Check arguments
	plog = GetThreadLogger() ;
	if (!plog)
		Fatal("%s: No thread logger\n", *_fn) ;

	if (!server || !server[0])	{ rc = E_ARGUMENT ; plog->Out("No SMTP server supplied\n") ; }
	if (!uname || !uname[0])	{ rc = E_ARGUMENT ; plog->Out("No SMTP username supplied\n") ; }
	if (!passwd || !passwd[0])	{ rc = E_ARGUMENT ; plog->Out("No SMTP password supplied\n") ; }

	if (!m_Recipients.Count())	{ rc = E_ARGUMENT ; plog->Out("No recipients specified\n") ; }
	if (!m_AddrSender)				{ rc = E_ARGUMENT ; plog->Out("No sender address specified\n") ; }

	if (rc != E_OK)
		return rc ;

	if (!m_RealFrom)
		m_RealFrom = *m_AddrSender ;

	//	Connect to port 25 on destination machine.
	S = server ;
	rc = C.ConnectStd(S, 25) ;
	if (rc != E_OK)
		return hzerr(_fn, HZ_ERROR, rc, "Could not conect to SMTP server [%s]", server) ;

	//	Wait for SMTP greeting
	C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
	if (nRecv >= 0)
		rbuf[nRecv] = 0 ;
	else
		rbuf[0] = 0 ;

	smtpCode = _getSmtpCode(rbuf) ;
	if (smtpCode != SMTP_READY)
	{
		plog->Out("%d Server not ready so quiting\n", smtpCode) ;
		rc = E_HOSTRETRY ;
		goto Quit ;
	}

	//	Send the EHLO command
	sprintf(sbuf, "EHLO %s\r\n", *m_AddrSender) ;

	if ((rc = C.Send(sbuf, strlen(sbuf))) != E_OK)
	{
		plog->Out("Could not send HELO command\n") ;
		goto Quit ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Client -> %s", sbuf) ;

	//	Recv the server responds
	if ((rc = C.Recv(rbuf, nRecv, HZ_MAXPACKET)) != E_OK)
	{
		plog->Out("Could not get ACK to EHLO msg\n") ;
		goto Quit ;
	}
	if (nRecv >= 0)
		rbuf[nRecv] = 0 ;
	else
		rbuf[0] = 0 ;

	smtpCode = _getSmtpCode(rbuf) ;
	if (smtpCode != SMTP_OK)
		{ rc = E_PROTOCOL ; plog->Out("Expected ACK so quitting\n") ; goto Quit ; }
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Server -> %s\n", rbuf) ;

	//	If there is an AUTH, do this first
	if (uname && passwd)
	{
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Have username of %s and password of %s so will auth\n", uname, passwd) ;

		strcpy(sbuf, "AUTH LOGIN\r\n") ;
		if ((rc = C.Send(sbuf, strlen(sbuf))) != E_OK)
			{ plog->Out("Could not send MAIL FROM message\n") ; goto Quit ; }
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Client -> %s", sbuf) ;

		//	Get back the message 'send username' (written in base64)
		C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
		if (nRecv > 0)
			rbuf[nRecv] = 0 ;
		else
			rbuf[0] = 0 ;

		smtpCode = _getSmtpCode(rbuf) ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Server -> %s", rbuf) ;
		if (smtpCode != SMTP_LOGINFO)
			{ rc = E_BADSENDER ; plog->Out("Expected code 334. Got instead code %d\n", smtpCode) ; goto Quit ; }

		//	Send the username
		inp.Clear() ;
		oup.Clear() ;
		inp = uname ;
		Base64Encode(oup, inp) ;
		oup << "\r\n" ;
		S = oup ;

		if ((rc = C.Send(*S, S.Length())) != E_OK)
			{ plog->Out("Could not send username\n") ; goto Quit ; }
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Client -> %s", *S) ;

		//	Expect the message 'send password' (written in base64)
		C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
		if (nRecv > 0)
			rbuf[nRecv] = 0 ;
		else
			rbuf[0] = 0 ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Server -> %s", rbuf) ;

		smtpCode = _getSmtpCode(rbuf) ;
		if (smtpCode != SMTP_LOGINFO)
		{
			plog->Out("Expected code 334 (request for password). Got %d\n", smtpCode) ;
			rc = E_BADSENDER ;
			goto Quit ;
		}

		//	Send the password
		inp.Clear() ;
		oup.Clear() ;
		inp = passwd ;
		Base64Encode(oup, inp) ;
		oup << "\r\n" ;
		S = oup ;

		if ((rc = C.Send(*S, S.Length())) != E_OK)
			{ plog->Out("Could not send password\n") ; goto Quit ; }
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Client -> %s", *S) ;

		//	Expect the 235 Go Ahead
		C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
		if (nRecv > 0)
			rbuf[nRecv] = 0 ;
		else
			rbuf[0] = 0 ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Server -> %s", rbuf) ;

		smtpCode = _getSmtpCode(rbuf) ;
		if (smtpCode != SMTP_GO_AHEAD)
		{
			plog->Out("Expected code 235 (Login OK, Go Ahead). Got %d\n", smtpCode) ;
			rc = E_BADSENDER ;
			goto Quit ;
		}
	}

	/*
	**	Now send the sender details
	*/

	sprintf(sbuf, "MAIL FROM: <%s>\r\n", *m_AddrSender) ;
	if ((rc = C.Send(sbuf, strlen(sbuf))) != E_OK)
		{ plog->Out("Could not send MAIL FROM message\n") ; goto Quit ; }
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Client -> %s", sbuf) ;

	//	Expect 'sender ok'
	C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
	if (nRecv > 0)
		rbuf[nRecv] = 0 ;
	else
		rbuf[0] = 0 ;
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Server -> %s", rbuf) ;

	smtpCode = _getSmtpCode(rbuf) ;
	if (smtpCode != SMTP_OK)
	{
		plog->Out("Expected a 250 (OK) code. Got %d\n", smtpCode) ;
		rc = E_BADSENDER ;
		goto Quit ;
	}

	/*
	**	Now send the reciprient details
	*/

	for (rx = m_Recipients ; rx.Valid() ; rx++)
	{
		e = rx.Element() ;

		sprintf(sbuf, "RCPT TO: <%s>\r\n", *e) ;
		C.Send(sbuf, strlen(sbuf)) ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Client -> %s", sbuf) ;

		//	Expect 'recipient ok'

		rc = C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
		if (rc != E_OK)
		{
			plog->Out("Broken pipe while sending recipient details\n") ;
			goto Quit ;
		}

		if (nRecv > 0)
			rbuf[nRecv] = 0 ;
		else
			rbuf[0] = 0 ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Server -> %s", rbuf) ;

		smtpCode = _getSmtpCode(rbuf) ;
		if (smtpCode != SMTP_OK && smtpCode != SMTP_GO_AHEAD)
		{
			plog->Out("Expected a 250 (OK) or 235 (Go Ahead) code. Got %d\n", smtpCode) ;
			rc = E_NOACCOUNT ;
			goto Quit ;
		}
	}

	//	Carbon copy recipients
	for (rx = m_CC ; rx.Valid() ; rx++)
	{
		e = rx.Element() ;

		sprintf(sbuf, "RCPT TO: <%s>\r\n", *e) ;
		C.Send(sbuf, strlen(sbuf)) ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Client -> %s", sbuf) ;

		//	Expect 'recipient ok'

		rc = C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
		if (rc != E_OK)
		{
			plog->Out("Broken pipe while sending recipient details\n") ;
			goto Quit ;
		}

		if (nRecv > 0)
			rbuf[nRecv] = 0 ;
		else
			rbuf[0] = 0 ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Server -> %s", rbuf) ;

		smtpCode = _getSmtpCode(rbuf) ;
		if (smtpCode != SMTP_OK)
		{
			plog->Out("Target user not ok so quiting\n") ;
			rc = E_NOACCOUNT ;
			goto Quit ;
		}
	}

	//	Blind carbon copy recipients
	for (rx = m_BCC ; rx.Valid() ; rx++)
	{
		e = rx.Element() ;

		sprintf(sbuf, "RCPT TO: <%s>\r\n", *e) ;
		C.Send(sbuf, strlen(sbuf)) ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Client -> %s", sbuf) ;

		//	Expect 'recipient ok'

		rc = C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
		if (rc != E_OK)
		{
			plog->Out("Broken pipe while sending recipient details\n") ;
			goto Quit ;
		}

		if (nRecv > 0)
			rbuf[nRecv] = 0 ;
		else
			rbuf[0] = 0 ;
		if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
			plog->Out("Server -> %s", rbuf) ;

		smtpCode = _getSmtpCode(rbuf) ;
		if (smtpCode != SMTP_OK)
		{
			plog->Out("Target user not ok so quiting\n") ;
			rc = E_NOACCOUNT ;
			goto Quit ;
		}
	}

	//	Now send the data marker

	sprintf(sbuf, "DATA\r\n") ;
	C.Send(sbuf, strlen(sbuf)) ;
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Client -> %s", sbuf) ;

	//	The target should ask for the mail
	//	354 Enter mail, end with "." on a line by itself

	C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
	if (nRecv > 0)
		rbuf[nRecv] = 0 ;
	else
		rbuf[0] = 0 ;
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Server -> %s", rbuf) ;

	//	So we send it

	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Sending %d bytes of data\n", m_Final.Size()) ;

	//m_Final.Rewind() ;

	//	Repeat calls to Send() to transmit email message
	ci = m_Final ;
	for (rc = E_OK ; rc == E_OK ;)
	{
		for (i = sbuf, nSend = 0 ; !ci.eof() && nSend < HZ_MAXPACKET ; nSend++, ci++)
			*i++ = *ci ;

		if (nSend == 0)
			break ;
		nTotal += nSend ;
		rc = C.Send(sbuf, nSend) ;
	}

	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Sent %d of %d bytes\n", nTotal, m_Final.Size()) ;

	//	Send terminator sequence
	strcpy(sbuf, ".\r\n") ;
	C.Send(sbuf, strlen(sbuf)) ;
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Client -> %s", sbuf) ;

	//rc = C.Send(m_Final) ;
	if (rc != E_OK)
	{
		plog->Out("Could not transmit msg body\n") ;
		goto Quit ;
	}

	//	Target should send us a message like
	//	250 SAA00984 Message accepted for delivery

	C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
	if (nRecv > 0)
		rbuf[nRecv] = 0 ;
	else
		rbuf[0] = 0 ;
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Server -> %s", rbuf) ;

Quit:
	//	Now send QUIT command

	sprintf(sbuf, "QUIT\r\n") ;
	C.Send(sbuf, strlen(sbuf)) ;
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Client -> %s", sbuf) ;

	//	Expect the following msg from the target
	//	221 osuks.densitron.net closing connection

	C.Recv(rbuf, nRecv, HZ_MAXPACKET) ;
	if (nRecv > 0)
		rbuf[nRecv] = 0 ;
	else
		rbuf[0] = 0 ;
	if (_hzGlobal_Debug & HZ_DEBUG_MAILER)
		plog->Out("Server -> %s", rbuf) ;

	//	It's all over!

	C.Close() ;
	return rc ;
}

hzEcode	hzEmail::SendEpistula	(hzChain& report)
{
	//	Directly inject email into the epistula mail que.
	//
	//	Arguments:	1)	report		The error report of the send operation

	_hzfunc("hzEmail::SendEpistula") ;

	static uint32_t	nSeq = 1000 ;

	hzList<hzEmaddr>::Iter	R ;		//	Recipients iterator

	ofstream	os ;				//	Output stream
	hzChain		Z ;					//	Working output chain
	hzString	S ;					//	Working string
	hzEmaddr	ema ;				//	Email address
	char		cvId[12] ;			//	Mail-id

	//	Create mail-id
	sprintf(cvId, "%04x.%04x", getpid(), ++nSeq) ;

	//	Create body file
	Z.Printf("/usr/epistula/mailque/%s_body", cvId) ;
	S = Z ;
	os.open(*S) ;
	if (os.fail())
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Cannot open mail item file (%s) for writing", *S) ;

	os << m_Final ;
	os.close() ;
	os.clear() ;

	//	Create header file
	Z.Clear() ;
	Z.Printf("/usr/epistula/mailque/%s_head", cvId) ;
	S = Z ;
	os.open(*S) ;
	if (os.fail())
		return hzerr(_fn, HZ_ERROR, E_WRITEFAIL, "Cannot open mail item file (%s) for writing", *S) ;

	//	Record delivery info
	os << "from      : " << m_AddrSender << "\r\n" ;
	if (m_RealFrom.Length())
		os << "announce  : " << m_RealFrom << "\r\n" ;
	else
		os << "announce  : " << m_AddrSender << "\r\n" ;
	os << "ip_addr   : " << "127.0.0.1" << "\r\n" ;

	os << "resolved  : " << m_AddrSender.GetDomain() << "\r\n" ;
	os << "mail_id   : " << cvId << "\r\n" ;

	//	Group recipients by domain
	for (R = m_Recipients ; R.Valid() ; R++)
	{
		ema = R.Element() ;
		os << "relay_to  : " << ema.GetDomain() << "\r\n" ;
	}
	os.close() ;

	return E_OK ;
}

static	bool	_reademaddr	(hzString& aux, hzEmaddr& ema, hzChain::Iter& ci)
{
	//	Support fuction to read message headers expected to contain an email address. The following forms are expected:-
	//
	//		a)	Straight email address, no preceeding name e.g. john.doe@myorg.com
	//		b)	Address only but in angle brackets e.g. <john.doe@myorg.com>
	//		c)	Address preceeded by name in quotes e.g. "John Doe" <john.doe@myorg.com>
	//		d)	Address preceeded by name without quotes e.g. John Doe <john.doe@myorg.com>
	//
	//	Arguments:	1)	aux		Aux string (expected to be common name of the address holder)
	//				2)	ema		Reference to email address instance
	//				2)	ci		The current chain iterator
	//
	//	Returns:	True	If the email address was established
	//				False	Otherwise

	_hzfunc(__func__) ;

	chIter		xi ;		//	Input chain iterator
	hzChain		W ;			//	For building value
	hzString	S1 ;		//	Temp string
	hzString	S2 ;		//	Temp string

	if (ci.eof())
		return false ;

	xi = ci ;
	xi.Skipwhite() ;

	if (xi != CHAR_LESS)
	{
		//	Could be case (a), (c) or (d)

		if (xi == CHAR_DQUOTE)
		{
			//	Case (c)
			for (xi++ ; !xi.eof() && *xi != CHAR_DQUOTE ; xi++)
				W.AddByte(*xi) ;
			S1 = W ;
			W.Clear() ;

			if (*xi == CHAR_DQUOTE)
				xi++ ;
			xi.Skipwhite() ;
		}
		else
		{
			//	Case (a) if string encountered is an email address, (d) otherwise
			for (; !xi.eof() && *xi != CHAR_LESS ; xi++)
				W.AddByte(*xi) ;
			S1 = W ;
			W.Clear() ;
		}
	}

	if (xi == CHAR_LESS)
	{
		for (xi++ ; !xi.eof() && *xi != CHAR_MORE ; xi++)
			W.AddByte(*xi) ;
		S2 = W ;
		W.Clear() ;

		if (*xi == CHAR_MORE)
			xi++ ;
	}

	if (S1 && !S2)
		ema = S1 ;
	else if (S2 && !S1)
		ema = S2 ;
	else if (S1 && S2)
	{
		S1.TopAndTail() ;
		aux = S1 ;
		ema = S2 ;
	}
	else
		return false ;

	if (!ema)
		return false ;

	ci = xi ;
	return true ;
}

bool	_readangle	(hzString& str, hzChain::Iter& ci)
{
	//	Support function tot hzEmaddr::CheckEmail(). The chain iterator is at either a <> block or at the start of a quoted string - the content of which is to
	//	be tested by setting an email address instance.
	//
	//	Arguments:	1)	ema		Reference to email address instance
	//				2)	ci		The current chain iterator
	//
	//	Returns:	True	If the email address was established
	//				False	Otherwise

	_hzfunc(__func__) ;

	chIter	zi ;		//	Input chain iterator
	hzChain	W ;			//	For building value

	if (ci.eof())
		return false ;
	zi = ci ;
	if (*zi != CHAR_LESS)
		return false ;

	for (zi++ ; !zi.eof() && *zi != CHAR_MORE ; zi++)
		W.AddByte(*zi) ;

	if (*zi != CHAR_MORE)
		return false ;

	str = W ;
	W.Clear() ;
	ci = zi ;
	return true ;
}

hzEcode	_readStrval	(hzString& S, hzChain::Iter& ci)
{
	//	Get a string regardless of weather it is quoted or not
	//
	//	Arguments:	1)	S		The string to be populated
	//				2)	ci		The chain iterator
	//
	//	Returns:	E_FORMAT	If there is a single/double quote mismatch
	//				E_OK		If a string was garnered.

	_hzfunc(__func__) ;

	chIter		zi ;		//	Chain iterator
	hzChain		word ;		//	Word being garnered
	int32_t		quote ;		//	Quote char (either double or single quote)

	S.Clear() ;

	zi = ci ;
	quote = *zi == CHAR_DQUOTE ? CHAR_DQUOTE : *zi == CHAR_SQUOTE ? CHAR_SQUOTE : 0 ;

	if (quote)
	{
		for (zi++ ; !zi.eof() && *zi != quote && *zi >= CHAR_SPACE ; zi++)
			word.AddByte(*zi) ;

		if (*zi != quote)
			return E_FORMAT ;
		zi++ ;
	}
	else
	{
		for (; !zi.eof() && *zi >= CHAR_SPACE && *zi != CHAR_SCOLON ; zi++)
		{
			if (*zi == CHAR_SPACE && zi == " <")
				{ zi++ ; break ; }
			//if (zi == stop)
			//	break ;
			word.AddByte(*zi) ;
		}
	}

	if (*zi == CHAR_SCOLON)
		zi++ ;

	S = word ;
	ci = zi ;
	return E_OK ;
}

hzEcode	hzEmpart::Process	(hzChain& error)
{
	//	Process (decode) an encoded email part (eg attachment)
	//
	//	Arguments:	1)	error	The error reporting chain
	//
	//	Returns:	E_FORMAT	If the encoded content could not be decoded
	//				E_OK		If the content was decoded

	_hzfunc("hzEmpart::Process") ;

	hzChain		Z ;					//	For forming resulting part contents
	chIter		zi ;				//	For iteration
	chIter		xi ;				//	For iteration
	hzString	boundary ;			//	For boundary (if found within part)
	hzString	boundaryBeg ;		//	Level 0 boundary
	hzString	boundaryEnd ;		//	Level 0 boundary

	zi = m_Content ;
	zi.Skipwhite() ;

	for (; !zi.eof() ; zi++)
	{
		xi = zi ;

		if (zi == "Content-Disposition:")
		{
			xi += 21 ;
			if (xi == "attachment;")
			{
				xi += 12 ;
				if (xi == "filename=")
					_readStrval(m_Filename, xi) ;
			}

			for (zi++ ; !zi.eof() && *zi != CHAR_NL ; zi++) ;
			continue ;
		}

		if (zi == "Content-Type:")
		{
			//if (!_procContType(boundary, m_eContentType, xi, error))
			//{
				//error.Printf("case 1 proc_part FAILED\n") ;
				//return E_FORMAT ;
			//}
			zi += 13 ;
			zi.Skipwhite() ;
			if		(zi.Equiv("text/plain"))			{ zi += 10 ; m_eContentType = HZCONTENTTYPE_TEXT_PLAIN ; }
			else if (zi.Equiv("text/html"))				{ zi +=  9 ; m_eContentType = HZCONTENTTYPE_TEXT_HTML ; }
			else if (zi.Equiv("multipart/alternative"))	{ zi += 21 ; m_eContentType = HZCONTENTTYPE_MULTI_ALTERNATIVE ; }
			else if (zi.Equiv("multipart/mixed"))		{ zi += 15 ; m_eContentType = HZCONTENTTYPE_MULTI_MIXED ; }
			else if (zi.Equiv("multipart/related"))		{ zi += 17 ; m_eContentType = HZCONTENTTYPE_MULTI_RELATED ; }
			else
				{ error.Printf("Unknown Content-Type\n") ; return E_FORMAT ; }

			if (*zi == CHAR_SCOLON)
				zi++ ;
			zi.Skipwhite() ;

			if (m_eContentType == HZCONTENTTYPE_MULTI_ALTERNATIVE || m_eContentType == HZCONTENTTYPE_MULTI_MIXED || m_eContentType == HZCONTENTTYPE_MULTI_RELATED)
			{
				//	Get the boundary arg
				if (zi.Equiv("boundary="))	
					{ zi += 9 ; _readStrval(boundary, zi) ; zi.Skipwhite() ; }
				else
					{ error.Printf("Line %d: Expected a boundary to be specified\n", zi.Line()) ; return E_FORMAT ; }
			}

			//if (*zi == CHAR_SCOLON)
				//zi++ ;
			//zi.Skipwhite() ;

			if (boundary)
			{
				boundaryBeg = "--" + boundary ;
				boundaryEnd = boundaryBeg + "--" ;
			}

			for (zi++ ; !zi.eof() && *zi != CHAR_NL ; zi++) ;
			continue ;
		}

		if (zi == "Content-Transfer-Encoding:")
		{
			zi += 26 ;
			if		(zi.Equiv("7bit"))				{ zi +=  4 ; m_eContentEncode = HZCONTENTENCODE_7BIT ; }
			else if (zi.Equiv("8bit"))				{ zi +=  4 ; m_eContentEncode = HZCONTENTENCODE_8BIT ; }
			else if (zi.Equiv("binary"))			{ zi +=  6 ; m_eContentEncode = HZCONTENTENCODE_8BIT ; }
			else if (zi.Equiv("base64"))			{ zi +=  6 ; m_eContentEncode = HZCONTENTENCODE_BASE64 ; }
			else if (zi.Equiv("quoted-printable"))	{ zi += 16 ; m_eContentEncode = HZCONTENTENCODE_QUOTE_PRINT ; }
			else
				{ error.Printf("Unknown Content-Transfer-Encoding value\n") ; return E_FORMAT ; }

			for (zi++ ; !zi.eof() && *zi != CHAR_NL ; zi++) ;
			continue ;
		}

		if (*zi == CHAR_NL)
			break ;

		error.Printf("%s: Line %d: Unknown PART Header directive\n", *_fn, zi.Line()) ;
		return E_FORMAT ;
	}

	for (zi++ ; !zi.eof() ; zi++)
		Z.AddByte(*zi) ;
	
	m_Content.Clear() ;

	switch	(m_eContentEncode)
	{
    case HZCONTENTENCODE_UNDEFINED:		//  Content encoding not defined
    case HZCONTENTENCODE_7BIT:			//  Content is not encoded but chars are limited to lower ASCII
    case HZCONTENTENCODE_8BIT:			//  Content is not encoded and chars may be upper ASCII
    case HZCONTENTENCODE_BINARY:		//  Content is not encoded and chars may be anything

		m_Content = Z ;
		break ;

    case HZCONTENTENCODE_BASE64:

		if (Base64Decode(m_Content, Z) != E_OK)
		{
			error.Printf("%s: Could not decode content\n", *_fn) ;
			m_Content = Z ;
		}
		break ;

    case HZCONTENTENCODE_QUOTE_PRINT:

		if (QPDecode(m_Content, Z) != E_OK)
		{
			error.Printf("%s: Could not QP-decode content\n", *_fn) ;
			m_Content = Z ;
		}
		break ;
	}

	return E_OK ;
}

hzEcode	hzEmail::Import	(const hzChain& emRaw)
{
	//	Populate this hzEmail instance by importing a serialized email message
	//
	//	Note that various different headers can give essentially the same information. For example 'From:' and 'Reply-To:'. Both of these can supply the sender
	//	real name and email address, or just the email address. The formats can vary. The real name if present, can be plain text, plain text in quotes or as a
	//	base64 string. The email address will if preceeded by a real name, always be enclosed in a <> block. However if there is no real name, the email address
	//	could also be plain text, plain text in quotes or a base64 string.
	//
	//	Argument:	emRaw	The serialized email message supplied as a hzChain
	//	
	//	Returns:	E_FORMAT	If any aspect of the previously exported email are malformed
	//				E_OK		If the import was successful

	_hzfunc("hzEmail::Import") ;

	hzList<hzEmpart>::Iter	pi ;	//	Iterator for the email parts

	chIter		zi ;				//	For iteration
	chIter		xi ;				//	For iteration
	chIter		ti ;				//	For iteration
	hzChain		Hdr ;				//	For processing email header data
	hzChain		Body ;				//	For processing email body data
	hzChain		Wln ;				//	For building header line
	hzChain		Wtok ;				//	For building header token
	hzEmpart	epart ;				//	Email part
	hzPair		pair ;				//	Header/parameter
	hzEmaddr	emtmp ;				//	For holding email addresses in Cc and Bcc
	hzString	filepath ;			//	Full path to email file
	hzString	boundary ;			//	Boundary value
	hzString	boundaryBeg ;		//	Boundary value prepended with '--'
	//hzString	boundaryEnd ;		//	Boundary value pre and postpended with '--'
	hzString	strval ;			//	Temp string
	uint32_t	nLen ;				//	No of chars to advance iterator by
	bool		bErr = false ;		//	Set on error
	hzEcode		rc = E_OK ;			//	Return code

	Clear() ;
	m_Err.Clear() ;

	//	Ensure standard message headers are loaded
	if (!_hzGlobal_EmsgHdrs.Count())
		HadronZooInitMessageHdrs() ;

	//	Pre-process headers in message to hand
	for (zi = emRaw ; !zi.eof() ; zi++)
	{
		if (*zi == CHAR_CR && zi == "\r\n\r\n")
			break ;
		Hdr.AddByte(*zi) ;
	}

	//	Isolate tail
	for (zi += 4 ; !zi.eof() ; zi++)
		Body.AddByte(*zi) ;

	for (zi = Hdr ; !bErr && !zi.eof() ;)
	{
		if (*zi != CHAR_CR)
			{ Wln.AddByte(*zi) ; zi++ ; continue ; }

		if (zi != "\r\n")
			{ bErr = true ; m_Err.Printf("Line %d: Malformed header. Lone CR\n", zi.Line()) ; break ; }

		//	Now have the \r\n end of line
		zi += 2 ;
		if (*zi == CHAR_SPACE || *zi == CHAR_TAB)
		{
			//	Continuation line
			Wln.AddByte(CHAR_NL) ;
			for (zi++ ; *zi == CHAR_SPACE || *zi == CHAR_TAB ; zi++) ;
			continue ;
			
		}

		//	Have complete header so break it into name-value pairs. Get header name first
		for (xi = Wln ; !xi.eof() && *xi != CHAR_COLON ; xi++)
			Wtok.AddByte(*xi) ;
		if (*xi != CHAR_COLON)
			{ bErr = true ; m_Err.Printf("Line %d: Malformed header. No colon\n", xi.Line()) ; }
		xi++ ;
		xi.Skipwhite() ;
		strval = Wtok ;
		Wtok.Clear() ;
		pair.name = _hzGlobal_EmsgHdrs[strval] ;
		if (!pair.name)
			pair.name = strval ;

		//	Get header value
		for (ti = xi ; !ti.eof() ; ti++)
		{
			if (*ti == CHAR_CR)
				ti++ ;
			if (*ti == CHAR_NL)
				for (ti++ ; *ti == CHAR_SPACE ; ti++) ;
			Wtok.AddByte(*ti) ;
		}
		pair.value = Wtok ;
		m_Hdrs.Add(pair) ;

		//	Check for strategic header
		if (pair.name == "Return-Path")
		{
			if (!_reademaddr(strval, m_AddrReturn, xi))
				{ bErr = true ; m_Err.Printf("Return-Path must amount to an email address\n") ; }
		}
		else if (pair.name == "Reply-To")
		{
			if (!_reademaddr(m_RealReply, m_AddrReply, xi))
				{ bErr = true ; m_Err.Printf("Reply-To: arg must amount to an email address\n") ; }
		}

		else if (pair.name == "Message-ID")		_readangle(m_Id, xi) ;
		else if (pair.name == "Subject")		m_Subject = pair.value ;

		else if (pair.name == "X-Epistula-Ingress")
		{
			if (!_reademaddr(strval, m_AddrSender, xi))
				{ bErr = true ; m_Err.Printf("X-Epistula-Ingress: arg must amount to an email address\n") ; }
		}
		else if (pair.name == "From")
		{
			if (!_reademaddr(m_RealFrom, m_AddrFrom, xi))
				{ bErr = true ; m_Err.Printf("From: arg must amount to an email address\n") ; }
		}
		else if (pair.name == "To")
		{
			if (!_reademaddr(m_RealTo, emtmp, xi))
				{ bErr = true ; m_Err.Printf("To: arg must amount to an email address\n") ; }
			if (!m_AddrTo)
				m_AddrTo = emtmp ;
			m_Recipients.Add(emtmp) ;
		}
		else if (pair.name == "Cc")
		{
			if (!_reademaddr(strval, emtmp, xi))
				{ bErr = true ; m_Err.Printf("Cc: arg must amount to an email address\n") ; }
			m_CC.Add(emtmp) ;
		}
		else if (pair.name == "Bcc")
		{
			if (!_reademaddr(strval, emtmp, xi))
				{ bErr = true ; m_Err.Printf("Bcc: arg must amount to an email address\n") ; }
			m_CC.Add(emtmp) ;
		}
		else if (pair.name == "Date")
		{
			nLen = IsFormalDate(m_Date, xi) ;
			if (!nLen)
				m_Err.Printf("Date arg must amount to legal date\n") ;
		}
		else if (pair.name == "Content-Type")
		{
			if		(xi.Equiv("text/plain"))			{ xi += 10 ; m_ContType = HZCONTENTTYPE_TEXT_PLAIN ; }
			else if (xi.Equiv("text/html"))				{ xi +=  9 ; m_ContType = HZCONTENTTYPE_TEXT_HTML ; }
			else if (xi.Equiv("multipart/alternative"))	{ xi += 21 ; m_ContType = HZCONTENTTYPE_MULTI_ALTERNATIVE ; }
			else if (xi.Equiv("multipart/mixed"))		{ xi += 15 ; m_ContType = HZCONTENTTYPE_MULTI_MIXED ; }
			else if (xi.Equiv("multipart/related"))		{ xi += 17 ; m_ContType = HZCONTENTTYPE_MULTI_RELATED ; }
			else
				{ bErr = true ; m_Err.Printf("Unknown Content-Type\n") ; }

			if (*xi == CHAR_SCOLON)
				xi++ ;
			xi.Skipwhite() ;

			if (m_ContType == HZCONTENTTYPE_MULTI_ALTERNATIVE || m_ContType == HZCONTENTTYPE_MULTI_MIXED || m_ContType == HZCONTENTTYPE_MULTI_RELATED)
			{
				//	Get the boundary arg
				if (xi.Equiv("boundary="))	
					{ xi += 9 ; _readStrval(boundary, xi) ; xi.Skipwhite() ; }
				else
					{ bErr = true ; m_Err.Printf("Expected a boundary to be specified\n") ; }
			}
		}
		else if (pair.name == "Content-Transfer-Encoding")
		{
			if		(pair.value == "7bit")				m_Encoding = HZCONTENTENCODE_7BIT ;
			else if	(pair.value == "8bit")				m_Encoding = HZCONTENTENCODE_8BIT ;
			else if	(pair.value == "binary")			m_Encoding = HZCONTENTENCODE_8BIT ;
			else if	(pair.value == "base64")			m_Encoding = HZCONTENTENCODE_BASE64 ;
			else if	(pair.value == "quoted-printable")	m_Encoding = HZCONTENTENCODE_QUOTE_PRINT ;
			else
				{ bErr = true ; m_Err.Printf("Unknown Content-Transfer-Encoding value %s\n", *pair.value) ; }
		}
		Wtok.Clear() ;
		Wln.Clear() ;
	}

	if (bErr)
	{
		m_Err.Printf("IMPORT ABORTED\n") ;
		return E_FORMAT ;
	}

	/*
	**	Check values in the header for inconsistances
	*/

	if (!m_AddrSender)
		m_AddrSender = m_AddrFrom ;

	/*
	**	Process the body. This may or may not come in blocks marked out by the boundary.
	*/

	if (!Body.Size())
	{
		m_Err.Printf("IMPORT ABORTED - No body content\n") ;
		return E_NODATA ;
	}

	if (!boundary)
	{
		//	The email is comprised of a single part with no boundary. In this case the content of the part is simply the rest of the email body

		if (m_ContType == HZCONTENTTYPE_TEXT_PLAIN)
			for (zi = Body ; !zi.eof() ; zi++)
				m_Text.AddByte(*zi) ;
		if (m_ContType == HZCONTENTTYPE_TEXT_HTML)
			for (zi = Body ; !zi.eof() ; zi++)
				m_Html.AddByte(*zi) ;
	}
	else
	{
		//	The email is comprised of multiple parts. We skip whitespace and then should be at the start of a boundary

		boundaryBeg = "--" + boundary ;
		//boundaryEnd = boundaryBeg + "--" ;

		m_Err.Printf("Operating with BOUNDARY=%s\n", *boundary) ;

		zi = Body ;
		zi.Skipwhite() ;

		if (zi != boundaryBeg)
		{
			//	Assign data to the text or html part until a boudary is encountered
			if (m_ContType == HZCONTENTTYPE_TEXT_PLAIN)
				for (zi = Body ; !zi.eof() && zi != boundaryBeg ; zi++)
					m_Text.AddByte(*zi) ;
			if (m_ContType == HZCONTENTTYPE_TEXT_HTML)
				for (zi = Body ; !zi.eof() && zi != boundaryBeg ; zi++)
					m_Html.AddByte(*zi) ;
		}

		//	Now continue with the data within the boundaries
		for (; !zi.eof() ;)
		{
			epart.Clear() ;
			zi += boundaryBeg.Length() ;
			if (zi == "--")
				break ;
			zi.Skipwhite() ;

			//	Now copy everything until the next boundary marker (can be begining or end)
			for (; !zi.eof() ;)
			{
				if (*zi == '-')
				{
					if (zi == boundaryBeg)
						break ;
				}
				epart.m_Content.AddByte(*zi) ;
				zi++ ;
			}

			//	Insert the part
			m_Err.Printf("Adding PART of %d bytes\n", epart.m_Content.Size()) ;
			m_Parts.Add(epart) ;
		}

		/*
		**	Final stage is to process the parts
		*/

		m_Err.Printf("Processing %d parts\n", m_Parts.Count()) ;
		for (pi = m_Parts ; pi.Valid() ; pi++)
		{
			pi.Element().Process(m_Err) ;
		}

		/*
		**	Assemble final email by assigning processed parts as the email body and as attachments or embedded files
		*/

		m_Err.Printf("Assembling %d parts\n", m_Parts.Count()) ;
		for (pi = m_Parts ; pi.Valid() ; pi++)
		{
			epart = pi.Element() ;
			m_Err.Printf("Assembling part with content of %d bytes\n", epart.m_Content.Size()) ;

			if (epart.m_eContentType == HZCONTENTTYPE_TEXT_PLAIN)
				m_Text << epart.m_Content ;
			if (epart.m_eContentType == HZCONTENTTYPE_TEXT_HTML)
				m_Html << epart.m_Content ;
		}
	}

	return rc ;
}
