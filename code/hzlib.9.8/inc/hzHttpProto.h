//
//	File:	hzHttpProto.h
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
//	This file serves only to define HTTP methods and return codes
//

#ifndef hzHttpProto_h
#define hzHttpProto_h

/*
**	Definitions
*/

#define HTTP_MAXHDR		4096	//	Header can never be this big!

enum	HttpMethod
{
	//	Category:	Internet
	//
	//	HTTP Request types

	HTTP_INVALID,		//	Invalid HTTP request method
	HTTP_GET,			//	(1.0) Request a page
	HTTP_HEAD,			//	(1.0) Request page header only
	HTTP_POST,			//	(1.0) Post a form submission
	HTTP_OPTIONS,		//	(1.1) Post a form submission
	HTTP_PUT,			//	(1.1) Put or overwrite a URL resource on the server (permissions required)
	HTTP_DELETE,		//	(1.1) Delete a URL resource (permissions required)
	HTTP_TRACE,			//	(1.1) Echos back a request so that a client can see what changes have been made by intermeadiate servers.
	HTTP_CONNECT		//	(1.1) HTTP tunneling
} ;

enum	HttpRC
{
	//	Category:	Internet
	//
	//	HTTP return codes

	HTTPMSG_NULL					= 0,	//	Initial unset value for variables of this type. Not a valid HTTP response code.

	//	Request is granted
	HTTPMSG_OK						= 200,	//	OK - No error
	HTTPMSG_NOCONTENT				= 204,	//	OK - No error

	//	Request is OK but no page supplied because ...
	HTTPMSG_REDIRECT_PERM			= 301,	//	Server is redirecting to another page (permanent redirect)
	HTTPMSG_FOUND_GOTO				= 302,	//	Temporary redirect
	HTTPMSG_NOT_MODIFIED			= 304,	//	The requested page has not been mofified, please use cached version.
	HTTPMSG_REDIRECT_TEMP			= 307,	//	Server is redirecting to another page (temporary redirect)

	//	Errors in Request (not found or otherwise denied)
	HTTPMSG_BAD_REQUEST				= 400,	//	ErrorDocument 400 /error/HTTP_BAD_REQUEST.html.var
	HTTPMSG_UNAUTHORIZED			= 401,	//	ErrorDocument 401 /error/HTTP_UNAUTHORIZED.html.var
	HTTPMSG_FORBIDDEN				= 403,	//	ErrorDocument 403 /error/HTTP_FORBIDDEN.html.var
	HTTPMSG_NOTFOUND				= 404,	//	ErrorDocument 404 /error/HTTP_NOT_FOUND.html.var
	HTTPMSG_METHOD_NOT_ALLOWED		= 405,	//	ErrorDocument 405 /error/HTTP_METHOD_NOT_ALLOWED.html.var
	HTTPMSG_REQUEST_TIME_OUT		= 408,	//	ErrorDocument 408 /error/HTTP_REQUEST_TIME_OUT.html.var
	HTTPMSG_GONE					= 410,	//	ErrorDocument 410 /error/HTTP_GONE.html.var
	HTTPMSG_LENGTH_REQUIRED			= 411,	//	ErrorDocument 411 /error/HTTP_LENGTH_REQUIRED.html.var
	HTTPMSG_PRECONDITION_FAILED		= 412,	//	ErrorDocument 412 /error/HTTP_PRECONDITION_FAILED.html.var
	HTTPMSG_ENTITY_TOO_LARGE		= 413,	//	ErrorDocument 413 /error/HTTP_REQUEST_ENTITY_TOO_LARGE.html.var
	HTTPMSG_REQUEST_URI_TOO_LARGE	= 414,	//	ErrorDocument 414 /error/HTTP_REQUEST_URI_TOO_LARGE.html.var
	HTTPMSG_UNSUPPORTED_MEDIA_TYPE	= 415,	//	ErrorDocument 415 /error/HTTP_UNSUPPORTED_MEDIA_TYPE.html.var

	//	System Errors. Can't help you regardless of how reasonable the request!
	HTTPMSG_INTERNAL_SERVER_ERROR	= 500,	//	ErrorDocument 500 /error/HTTP_INTERNAL_SERVER_ERROR.html.var
	HTTPMSG_NOT_IMPLEMENTED			= 501,	//	ErrorDocument 501 /error/HTTP_NOT_IMPLEMENTED.html.var
	HTTPMSG_BAD_GATEWAY				= 502,	//	ErrorDocument 502 /error/HTTP_BAD_GATEWAY.html.var
	HTTPMSG_SERVICE_UNAVAILABLE		= 503,	//	ErrorDocument 503 /error/HTTP_SERVICE_UNAVAILABLE.html.var
	HTTPMSG_VARIANT_ALSO_VARIES		= 506	//	ErrorDocument 506 /error/HTTP_VARIANT_ALSO_VARIES.html.var
} ;

#endif	//	hzHttpProto_h
