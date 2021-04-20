//
//	File:	hzMailer.h
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
//	Prototypes for the HadronZoo email facilities
//

#ifndef hzMailer_h
#define hzMailer_h

#include "hzMimetype.h"
#include "hzIpaddr.h"
#include "hzEmaddr.h"
#include "hzTmplList.h"
#include "hzTmplVect.h"
#include "hzTmplSet.h"
#include "hzTmplMapS.h"

enum	SMTPCode
{
	//	Category:	Internet
	//
	//	All possible SMTP return codes

	SMTP_NOOP			= 0,		//	Returned by GetSMTPCode() in error

	//	Initial Connection Errors
	SMTP_CONN_FAIL		= 101,		//	The server is unable to connect
	SMTP_CONN_REFUSED	= 111,		//	Connection refused or inability to open an SMTP stream

	//	Success
	SMTP_STATUS_NS		= 200,		//	Non standard system status message or help reply
	SMTP_STATUS			= 211,		//	System status, or system help reply
	SMTP_HELP			= 214,		//	Help message (this reply is useful only to the human user)
	SMTP_READY			= 220,		//	Service ready
	SMTP_QUIT			= 221,		//	Service closing transmission channel
	SMTP_GO_AHEAD		= 235,		//	Tells client to go ahead
	SMTP_OK				= 250,		//	Requested mail action okay, completed
	SMTP_NOTLOCAL		= 251,		//	User not local; will forward to
	SMTP_NOVRFY			= 252,		//	Cannot VRFY user, but will accept message and attempt delivery

	//	Directives to send
	SMTP_LOGINFO		= 334,		//	Login info, either username or password
	SMTP_SENDDATA		= 354,		//	Start mail input; end with <CRLF>.<CRLF>

	//	Temporary Errors
	SMTP_TIMEOUT		= 420,		//	General connection timeout issue
	SMTP_UNAVAILABLE	= 421,		//	Service not available,
	SMTP_MBOX_FULL		= 422,		//	The recipient’s mailbox has exceeded its storage limit
	SMTP_DISK_FULL		= 431,		//	Not enough space on the disk - Expected to be temporary.
	SMTP_MBOX_HALT		= 432,		//	Recipient’s incoming mail queue has been stopped
	SMTP_TIMEOUT_PX		= 441,		//	The recipient’s server is not responding
	SMTP_CONN_XMIT		= 442,		//	The connection was dropped during the transmission.
	SMTP_CONN_HOPS		= 446,		//	The maximum hop count was exceeded for the message
	SMTP_TIMEOUT_NS		= 447,		//	Non standard timeout: Some SMTP servers will send this if they have decided to time out the client (for being too slow).
	SMTP_ROUTE			= 449,		//	Routing error
	SMTP_MBOXBUSY		= 450,		//	Requested mail action not taken: mailbox unavailable (E.g., mailbox busy)
	SMTP_PROCERROR		= 451,		//	Requested action aborted: local error in processing
	SMTP_NOSPACE		= 452,		//	Action not taken: no space left
	SMTP_CLIENT_ERR		= 471,		//	Some SMTP servers return this in respose to errors on the part of the connecting client (as they see it).

	//	Permanent Errors
	SMTP_BADCOMMAND		= 500,		//	Syntax error, command unrecognized (This may include errors such as cmd line too long)
	SMTP_BADSYNTAX		= 501,		//	Syntax error in parameters or arguments
	SMTP_NOCOMMAND		= 502,		//	Command not implemented
	SMTP_BAD_CMD_SEQ	= 503,		//	Bad sequence of commands
	SMTP_BAD_PARAM		= 504,		//	Command parameter not implemented

	SMTP_BAD_SNDR_ADDR	= 510,		//	Bad email address
	SMTP_BAD_RCPT_ADDR	= 511,		//	Bad email address
	SMTP_BAD_HOST		= 512,		//	Host server for the recipient’s domain name cannot be found in DNS
	SMTP_BAD_ADDR_TYPE	= 513,		//	Address type is incorrect
	SMTP_MSG_SIZE		= 523,		//	Size of your mail exceeds the server limits
	SMTP_BAD_SERVER		= 530,		//	Authentication problem
	SMTP_BAD_SENDER		= 541,		//	The recipient address rejected your message

	SMTP_NACK			= 550,		//	Non-existent email address
	SMTP_TRYOTHER		= 551,		//	User not local or address invalid.
	SMTP_MBOXFULL		= 552,		//	Requested mail action aborted: exceeded storage allocation
	SMTP_BADNAME		= 553,		//	Mailbox name not allowed (E.g., mailbox syntax incorrect)
	SMTP_FAILED			= 554,		//	Transaction failed (Or in the case of a connection-opening response, No SMTP service)

	SMTP_INVALID		= 999		//	Not a 3 digit entity
} ;

/*
**	Email item
*/

enum	hzContentType
{
	//	Category:	Internet
	//
	//	The hzContentType enumeration refers to the MIME type of files and includes MIME types used in constructing emails

	HZCONTENTTYPE_UNDEFINED,				//	Content type not defined

	//	For body email parts
	HZCONTENTTYPE_TEXT_PLAIN,				//	Content is in plain text and there are no further levels
	HZCONTENTTYPE_TEXT_HTML,				//	Content is in HTML and there are no further levels
	HZCONTENTTYPE_IMAGE_GIF,				//	Content is an image (gif)
	HZCONTENTTYPE_IMAGE_JPEG,				//	Content is an image (jpeg)
	HZCONTENTTYPE_IMAGE_PNG,				//	Content is an image (png)
	HZCONTENTTYPE_IMAGE_ICO,				//	Content is an image (ico)
	HZCONTENTTYPE_SCRIPT_JS,				//	Content is a java script
	HZCONTENTTYPE_APP_MSWORD,				//	Content is an application file (ms-word document)
	HZCONTENTTYPE_APP_MSEXCEL,				//	Content is an application file (ms-excel document)
	HZCONTENTTYPE_APP_PDF,					//	Content is an application file (pdf)
	HZCONTENTTYPE_APP_TAR,					//	Content is an application file (tar)
	HZCONTENTTYPE_APP_GZIP,					//	Content is an application file (gzipped)
	HZCONTENTTYPE_APP_ZIP,					//	Content is an application file (zipped)

	//	For headers only
	HZCONTENTTYPE_MULTI_ALTERNATIVE,		//	Content is presented more than once in different parts with each part representing a
											//	different format (eg text and HTML).
	HZCONTENTTYPE_MULTI_MIXED,				//	The content is comprised of parts that may be dissimilar in format
	HZCONTENTTYPE_MULTI_RELATED,			//	The content is comprised of related parts (only applies to email root part)
} ;

enum	hzContentEncoding
{
	//	Category:	Internet
	//
	//	The hzContentEncoding enumeration describes what form of text encoding is being used (in emails inter alia)

	HZCONTENTENCODE_UNDEFINED,				//	Content encoding not defined
	HZCONTENTENCODE_7BIT,					//	Content is not encoded but chars are limited to lower ASCII
	HZCONTENTENCODE_8BIT,					//	Content is not encoded and chars may be upper ASCII
	HZCONTENTENCODE_BINARY,					//	Content is not encoded and chars may be anything
	HZCONTENTENCODE_BASE64,					//	Content is encoded as base64
	HZCONTENTENCODE_QUOTE_PRINT,			//	Content is encoded as 'quoted-prinatble' meaning that chars ? and = are escaped
} ;

class	hzEmpart
{
	//	Category:	Internet
	//
	//	Component of an email

public:
	hzContentType		m_eContentType ;	//	Type of content (eg text/plain)
	hzContentEncoding	m_eContentEncode ;	//	Method of encoding (eg base64)

	hzChain		m_Content ;					//	The content of the email part
	hzString	m_Location ;				//	Applies only to attached files
	hzString	m_Filename ;				//	Applies only to attached files

	hzEmpart	(void)
	{
		m_eContentType = HZCONTENTTYPE_UNDEFINED ;
		m_eContentEncode = HZCONTENTENCODE_UNDEFINED ;
	}

	~hzEmpart	(void)
	{
		Clear() ;
	}

	void	Clear	(void)
	{
		m_eContentType = HZCONTENTTYPE_UNDEFINED ;
		m_eContentEncode = HZCONTENTENCODE_UNDEFINED ;
		m_Content.Clear() ;
	}

	hzEmpart&	operator=	(const hzEmpart& op)
	{
		//m_Parts = op.m_Parts ;
		m_eContentType = op.m_eContentType ;
		m_eContentEncode = op.m_eContentEncode ;
		m_Content = op.m_Content ;
		m_Location = op.m_Location ;
		m_Filename = op.m_Filename ;

		return *this ;
	}

	hzEcode	Process	(hzChain& error) ;
} ;

class	hzEmail
{
	//	Category:	Internet
	//
	//	The hzEmail class holds a single email message in an 'open format' so that it may be directly operated on. This can work both ways. An email message can
	//	be 'built' from scratch in a step by step process before being sent. The sender is set, the recipients added one by one, the subject is set, a text body
	//	is added and attachments are added. Then hzEmail::Compose() is called to assemble the whole into a single POP3 datum. The email message is then ready to
	//	send.
	//
	//	Conversely the sender, the list of recipients, the subject, body and any attachments, can be derived from a POP3 datum.

	struct	_efile
	{
		//	Attachment description

		hzString	m_Filepath ;			//	Full pathname of file as given by AddAttachment() or as supplied in an incoming message 
		hzMimetype	m_eType ;				//	File type
	} ;

	hzList	<hzEmaddr>	m_Recipients ;		//	List of main recipient(s)
	hzList	<hzEmaddr>	m_CC ;				//	List of cc recipient(s)
	hzList	<hzEmaddr>	m_BCC ;				//	List of bcc recipient(s)
	hzList	<_efile>	m_Attachments ;		//	List of file attachments (added by Attach() as part of forming an outgoing email)

	hzChain		m_Final ;					//	Fully composed email

public:
	hzMapS	<hzString,hzChain>	m_Dets ;	//	List of file attachments (added by Detach() called on an incomming email)
	hzList	<hzEmpart>			m_Parts ;	//	List of message parts
	hzVect	<hzPair>			m_Hdrs ;	//	Message headers: Note this is only filled by Import. Composure of an outgoing message does not populate this list.

	hzChain		m_Text ;					//	Body of email (Text part)
	hzChain		m_Html ;					//	Body of email (Html part)
	hzChain		m_Err ;						//	For error reporting (e.g. failed import)
	hzXDate		m_Date ;					//	Date of email (recv only)
	hzString	m_Id ;						//	Mail id (typically as set by first SMTP server to handle it)
	hzString	m_DomainSender ;			//	Sender's domain (all possible sender address varients must agree on this)
	hzString	m_RealReply ;				//	Real name given in Reply-To header (if any)
	hzString	m_RealFrom ;				//	Real name of sender
	hzString	m_RealTo ;					//	Real name of recipient (rarely used)
	hzString	m_Subject ;					//	Subject
	hzEmaddr	m_AddrReturn ;				//	Return address (Return-path header)
	hzEmaddr	m_AddrReply ;				//	Address given in the ReplyTo header
	hzEmaddr	m_AddrSender ;				//	Address of sender as established in SMTP session. Epistula saves this as X-Epistula-Ingress
	hzEmaddr	m_AddrFrom ;				//	Address of sender given by From header
	hzEmaddr	m_AddrTo ;					//	Address of primary recipient
	hzIpaddr	m_SenderIP ;				//	Sender IP address

	hzContentType		m_ContType ;		//	Content type
	hzContentEncoding	m_Encoding ;		//	Content encoding

	hzEmail	(void)
	{
		m_ContType = HZCONTENTTYPE_UNDEFINED ;
		m_Encoding = HZCONTENTENCODE_UNDEFINED ;
	}

	~hzEmail	(void)
	{
	}

	void	Clear	(void) ;

	uint32_t	CountRecipientsStd	(void) const	{ return m_Recipients.Count() ; }
	uint32_t	CountRecipientsCC	(void) const	{ return m_CC.Count() ; }
	uint32_t	CountRecipientsBC	(void) const	{ return m_BCC.Count() ; }
	uint32_t	CountRecipientsAll	(void) const	{ return m_Recipients.Count() + m_CC.Count() + m_BCC.Count() ; }

	//	Functions to compile and send an email
	hzEcode		SetSender		(hzEmaddr& sender)	{ m_AddrSender = sender ; return m_AddrSender? E_OK : E_NODATA ; }

	hzEcode		SetSender		(const char* cpSender, const char* cpRealname) ;
	hzEcode		AddRecipient	(hzEmaddr& recipient) ;
	hzEcode		AddRecipientCC	(hzEmaddr& recipient) ;
	hzEcode		AddRecipientBCC	(hzEmaddr& recipient) ;

	hzEcode		AddRecipient	(const char* cpRecipient) ;
	hzEcode		AddRecipientCC	(const char* cpRecipient) ;
	hzEcode		AddRecipientBCC	(const char* cpRecipient) ;

	hzEcode		SetSubject		(const char* cpSublect) ;
	hzEcode		AddBody			(hzChain& Z) ;
	hzEcode		AddBody			(hzString& S) ;
	hzEcode		AddBody			(const char* cpText) ;
	hzEcode		AddAttachment	(const char* cpDir, const char* cpFilename, hzMimetype eType) ;
	hzEcode		AddAttachment	(const char* cpFilepath, hzMimetype eType) ;
	hzEcode		AddAttachment	(const char* cpFilepath) ;
	hzEcode		Compose			(void) ;
	hzEcode		Decompose		(void) ;
	hzEcode		SendSmtp		(const char* cpMailserver, const char* username, const char* password) ;
	hzEcode		SendEpistula	(hzChain& report) ;

	//	Functions to load a recived email and extract headers and parts from it
	hzEcode		Import			(const hzChain& Z) ;
	//hzEcode		Export			(hzChain& Z) ;

	//	Get functions
	hzEmaddr&	GetSender		(void)	{ return m_AddrSender ; }
	hzString&	GetSenderReal	(void)	{ return m_RealFrom ; }
	hzString&	GetSubject		(void)	{ return m_Subject ; }

	hzList<hzEmaddr>&	GetRecipientsStd	(void)	{ return m_Recipients ; }
	hzList<hzEmaddr>&	GetRecipientsCC		(void)	{ return m_CC ; }
	hzList<hzEmaddr>&	GetRecipientsBCC	(void)	{ return m_BCC ; }
} ;

class	hzPop3Acc
{
	//	Category:	Internet
	//
	//	POP3 Account.
	//
	//	The hzPop3Acc (POP3 account) class enables applications to operate as a POP3 email client and recieve email messages from a POP3 server. Each hzPop3Acc
	//	instance comprises a single username and password pair and so can only access a single 'mailbox' or 'email account'. What constitutes a mailbox or email
	//	account is strictly a matter for the POP3 server. Some POP3 servers such as Epistula, have user centric mailboxes that can span multiple recipient email
	//	addresses. Other POP3 servers have address centric mailboxes which will only contain email messages for a single recipient email address.
	//
	//	hzPop3Acc specifies a single file into which all emails for the account are aggregated and maintains a set of email ids for lookups and to avoid repeat
	//	downloads. The email messages themselves are available to the application as populated hzEmail instances.

	hzSet<hzString>	m_Already ;		//	IDs of already downloaded messages

	hzString	m_Server ;			//	Hostname for the account
	hzString	m_Username ;		//	Username for the account
	hzString	m_Password ;		//	Password for the account
	hzString	m_Repos ;			//	Pathname of repository

public:
	hzChain		m_Error ;			//	For error messages arising in a POP3 session

	hzPop3Acc	(void)	{}
	~hzPop3Acc	(void)	{}

	hzEcode	Init	(hzString Server, hzString Username, hzString Password, hzString Repos) ;


	uint32_t	Count	(void) const	{ return m_Already.Count() ; }

	//	Collect (download) emails
	hzEcode	Collect   (hzVect<hzString>& messages) ;

	//	Fetch a downloaded email or header
	hzEcode	GetEmail	(hzEmail& em, hzString& mailId) ;
} ;

/*
**	Prototypes
*/

void	CreateMessageID	(hzString& mailId, const hzDomain& domain) ;
#endif	//	hzMailer_h
