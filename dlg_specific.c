/*-------
 * Module:			dlg_specific.c
 *
 * Description:		This module contains any specific code for handling
 *					dialog boxes such as driver/datasource options.  Both the
 *					ConfigDSN() and the SQLDriverConnect() functions use
 *					functions in this module.  If you were to add a new option
 *					to any dialog box, you would most likely only have to change
 *					things in here rather than in 2 separate places as before.
 *
 * Classes:			none
 *
 * API functions:	none
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */
/* Multibyte support	Eiji Tokuya 2001-03-15 */

#include <ctype.h>
#include "dlg_specific.h"
#include "misc.h"

#include "pgapifunc.h"

#define	NULL_IF_NULL(a) ((a) ? ((const char *)(a)) : "(null)")

static void encode(const pgNAME, char *out, int outlen);
static pgNAME decode(const char *in);
static pgNAME decode_or_remove_braces(const char *in);

#define	OVR_EXTRA_BITS (BIT_FORCEABBREVCONNSTR | BIT_FAKE_MSS | BIT_BDE_ENVIRONMENT | BIT_CVT_NULL_DATE | BIT_ACCESSIBLE_ONLY | BIT_IGNORE_ROUND_TRIP_TIME | BIT_DISABLE_KEEPALIVE)
UInt4	getExtraOptions(const ConnInfo *ci)
{
	UInt4	flag = ci->extra_opts & (~OVR_EXTRA_BITS);

	if (ci->force_abbrev_connstr > 0)
		flag |= BIT_FORCEABBREVCONNSTR;
	else if (ci->force_abbrev_connstr == 0)
		flag &= (~BIT_FORCEABBREVCONNSTR);
	if (ci->fake_mss > 0)
		flag |= BIT_FAKE_MSS;
	else if (ci->fake_mss == 0)
		flag &= (~BIT_FAKE_MSS);
	if (ci->bde_environment > 0)
		flag |= BIT_BDE_ENVIRONMENT;
	else if (ci->bde_environment == 0)
		flag &= (~BIT_BDE_ENVIRONMENT);
	if (ci->cvt_null_date_string > 0)
		flag |= BIT_CVT_NULL_DATE;
	else if (ci->cvt_null_date_string == 0)
		flag &= (~BIT_CVT_NULL_DATE);
	if (ci->accessible_only > 0)
		flag |= BIT_ACCESSIBLE_ONLY;
	else if (ci->accessible_only == 0)
		flag &= (~BIT_ACCESSIBLE_ONLY);
	if (ci->ignore_round_trip_time > 0)
		flag |= BIT_IGNORE_ROUND_TRIP_TIME;
	else if (ci->ignore_round_trip_time == 0)
		flag &= (~BIT_IGNORE_ROUND_TRIP_TIME);
	if (ci->disable_keepalive > 0)
		flag |= BIT_DISABLE_KEEPALIVE;
	else if (ci->disable_keepalive == 0)
		flag &= (~BIT_DISABLE_KEEPALIVE);

	return flag;
}

CSTR	hex_format = "%x";
CSTR	dec_format = "%u";
CSTR	octal_format = "%o";
static UInt4	replaceExtraOptions(ConnInfo *ci, UInt4 flag, BOOL overwrite)
{
	if (overwrite)
		ci->extra_opts = flag;
	else
		ci->extra_opts |= (flag & ~(OVR_EXTRA_BITS));
	if (overwrite || ci->force_abbrev_connstr < 0)
		ci->force_abbrev_connstr = (0 != (flag & BIT_FORCEABBREVCONNSTR));
	if (overwrite || ci->fake_mss < 0)
		ci->fake_mss = (0 != (flag & BIT_FAKE_MSS));
	if (overwrite || ci->bde_environment < 0)
		ci->bde_environment = (0 != (flag & BIT_BDE_ENVIRONMENT));
	if (overwrite || ci->cvt_null_date_string < 0)
		ci->cvt_null_date_string = (0 != (flag & BIT_CVT_NULL_DATE));
	if (overwrite || ci->accessible_only < 0)
		ci->accessible_only = (0 != (flag & BIT_ACCESSIBLE_ONLY));
	if (overwrite || ci->ignore_round_trip_time < 0)
		ci->ignore_round_trip_time = (0 != (flag & BIT_IGNORE_ROUND_TRIP_TIME));
	if (overwrite || ci->disable_keepalive < 0)
		ci->disable_keepalive = (0 != (flag & BIT_DISABLE_KEEPALIVE));

	return (ci->extra_opts = getExtraOptions(ci));
}
BOOL	setExtraOptions(ConnInfo *ci, const char *optstr, const char *format)
{
	UInt4	flag = 0;

	if (!format)
	{
		if ('0' == *optstr)
		{
			switch (optstr[1])
			{
				case '\0':
					format = dec_format;
					break;
				case 'x':
				case 'X':
					optstr += 2;
					format = hex_format;
					break;
				default:
					format = octal_format;
					break;
			}
		}
		else
			format = dec_format;
	}

	if (sscanf(optstr, format, &flag) < 1)
		return FALSE;
	replaceExtraOptions(ci, flag, TRUE);
	return TRUE;
}
UInt4	add_removeExtraOptions(ConnInfo *ci, UInt4 aflag, UInt4 dflag)
{
	ci->extra_opts |= aflag;
	ci->extra_opts &= (~dflag);
	if (0 != (aflag & BIT_FORCEABBREVCONNSTR))
		ci->force_abbrev_connstr = TRUE;
	if (0 != (aflag & BIT_FAKE_MSS))
		ci->fake_mss = TRUE;
	if (0 != (aflag & BIT_BDE_ENVIRONMENT))
		ci->bde_environment = TRUE;
	if (0 != (aflag & BIT_CVT_NULL_DATE))
		ci->cvt_null_date_string = TRUE;
	if (0 != (aflag & BIT_ACCESSIBLE_ONLY))
		ci->accessible_only = TRUE;
	if (0 != (aflag & BIT_IGNORE_ROUND_TRIP_TIME))
		ci->ignore_round_trip_time = TRUE;
	if (0 != (aflag & BIT_DISABLE_KEEPALIVE))
		ci->disable_keepalive = TRUE;
	if (0 != (dflag & BIT_FORCEABBREVCONNSTR))
		ci->force_abbrev_connstr = FALSE;
	if (0 != (dflag & BIT_FAKE_MSS))
		ci->fake_mss =FALSE;
	if (0 != (dflag & BIT_CVT_NULL_DATE))
		ci->cvt_null_date_string = FALSE;
	if (0 != (dflag & BIT_ACCESSIBLE_ONLY))
		ci->accessible_only = FALSE;
	if (0 != (dflag & BIT_IGNORE_ROUND_TRIP_TIME))
		ci->ignore_round_trip_time = FALSE;
	if (0 != (dflag & BIT_DISABLE_KEEPALIVE))
		ci->disable_keepalive = FALSE;

	return (ci->extra_opts = getExtraOptions(ci));
}

static const char *
abbrev_sslmode(const char *sslmode, char *abbrevmode)
{
	switch (sslmode[0])
	{
		case SSLLBYTE_DISABLE:
		case SSLLBYTE_ALLOW:
		case SSLLBYTE_PREFER:
		case SSLLBYTE_REQUIRE:
			abbrevmode[0] = sslmode[0];
			abbrevmode[1] = '\0';
			break;
		case SSLLBYTE_VERIFY:
			abbrevmode[0] = sslmode[0];
			abbrevmode[2] = '\0';
			switch (sslmode[1])
			{
				case 'f':
				case 'c':
					abbrevmode[1] = sslmode[1];
					break;
				default:
					if (strnicmp(sslmode, "verify_", 7) == 0)
						abbrevmode[1] = sslmode[7];
					else
						strcpy(abbrevmode, sslmode);
			}
			break;
	}
	return abbrevmode;
}

static char *
makeKeepaliveConnectString(char *target, const ConnInfo *ci, BOOL abbrev)
{
	char	*buf = target;
	*buf = '\0';

	if (ci->disable_keepalive)
		return target;

	if (ci->keepalive_idle >= 0)
	{
		if (abbrev)
			sprintf(buf, ABBR_KEEPALIVETIME "=%u;", ci->keepalive_idle);
		else
			sprintf(buf, INI_KEEPALIVETIME "=%u;", ci->keepalive_idle);
		buf = strchr(buf, (int) '\0');
	}
	if (ci->keepalive_interval >= 0)
	{
		if (abbrev)
			sprintf(buf, ABBR_KEEPALIVEINTERVAL "=%u;", ci->keepalive_interval);
		else
			sprintf(buf, INI_KEEPALIVEINTERVAL "=%u;", ci->keepalive_interval);
	}
	return target;
}

#define OPENING_BRACKET '{'
#define CLOSING_BRACKET '}'
static const char *
makeBracketConnectString(char **target, pgNAME item, const char *optname)
{
	const char	*istr, *iptr;
	char	*buf, *optr;
	int	len;

	istr = SAFE_NAME(item);
	if (!istr[0])
		return NULL_STRING;

	for (iptr = istr, len = 0; *iptr; iptr++)
	{
		if (CLOSING_BRACKET == *iptr)
			len++;
		len++;
	}
	len += 30;
	if (buf = (char *) malloc(len), buf == NULL)
		return NULL_STRING;
	snprintf(buf, len, "%s=%c", optname, OPENING_BRACKET);
	optr = strchr(buf, '\0');
	for (iptr = istr; *iptr; iptr++)
	{
		if (CLOSING_BRACKET == *iptr)
			*(optr++) = *iptr;
		*(optr++) = *iptr;
	}
	*(optr++) = CLOSING_BRACKET;
	*(optr++) = ';';
	*optr = '\0';
	*target = buf;

	return buf;
}

#ifdef	_HANDLE_ENLIST_IN_DTC_
char *
makeXaOptConnectString(char *target, const ConnInfo *ci, BOOL abbrev)
{
	char	*buf = target;
	*buf = '\0';

	if (ci->xa_opt < 0)
		return target;

	if (abbrev)
	{
		if (DEFAULT_XAOPT != ci->xa_opt)
			sprintf(buf, ABBR_XAOPT "=%u;", ci->xa_opt);
	}
	else
		sprintf(buf, INI_XAOPT "=%u;", ci->xa_opt);
	return target;
}
#endif /* _HANDLE_ENLIST_IN_DTC_ */

void
makeConnectString(char *connect_string, const ConnInfo *ci, UWORD len)
{
	char		got_dsn = (ci->dsn[0] != '\0');
	char		encoded_item[LARGE_REGISTRY_LEN];
	char		*connsetStr = NULL;
	char		*pqoptStr = NULL;
	char		keepaliveStr[64];
#ifdef	_HANDLE_ENLIST_IN_DTC_
	char		xaOptStr[16];
#endif
	ssize_t		hlen, nlen, olen;
	/*BOOL		abbrev = (len <= 400);*/
	BOOL		abbrev = (len < 1024) || 0 < ci->force_abbrev_connstr;
	UInt4		flag;

inolog("force_abbrev=%d abbrev=%d\n", ci->force_abbrev_connstr, abbrev);
	encode(ci->password, encoded_item, sizeof(encoded_item));
	/* fundamental info */
	nlen = MAX_CONNECT_STRING;
	olen = snprintf(connect_string, nlen, "%s=%s;DATABASE=%s;SERVER=%s;PORT=%s;UID=%s;PWD=%s",
			got_dsn ? "DSN" : "DRIVER",
			got_dsn ? ci->dsn : ci->drivername,
			ci->database,
			ci->server,
			ci->port,
			ci->username,
			encoded_item);
	if (olen < 0 || olen >= nlen)
	{
		connect_string[0] = '\0';
		return;
	}

	/* extra info */
	hlen = strlen(connect_string);
	nlen = MAX_CONNECT_STRING - hlen;
inolog("hlen=%d", hlen);
	if (!abbrev)
	{
		char	protocol_and[16];

		if (ci->rollback_on_error >= 0)
			snprintf(protocol_and, sizeof(protocol_and), "7.4-%d", ci->rollback_on_error);
		else
			strcpy(protocol_and, "7.4");
		olen = snprintf(&connect_string[hlen], nlen, ";"
			INI_SSLMODE "=%s;"
			INI_READONLY "=%s;"
			INI_PROTOCOL "=%s;"
			INI_FAKEOIDINDEX "=%s;"
			INI_SHOWOIDCOLUMN "=%s;"
			INI_ROWVERSIONING "=%s;"
			INI_SHOWSYSTEMTABLES "=%s;"
			"=%s"		/* INI_CONNSETTINGS */
			INI_FETCH "=%d;"
			INI_UNKNOWNSIZES "=%d;"
			INI_MAXVARCHARSIZE "=%d;"
			INI_MAXLONGVARCHARSIZE "=%d;"
			INI_DEBUG "=%d;"
			INI_COMMLOG "=%d;"
			INI_USEDECLAREFETCH "=%d;"
			INI_TEXTASLONGVARCHAR "=%d;"
			INI_UNKNOWNSASLONGVARCHAR "=%d;"
			INI_BOOLSASCHAR "=%d;"
			INI_PARSE "=%d;"
			INI_EXTRASYSTABLEPREFIXES "=%s;"
			INI_LFCONVERSION "=%d;"
			INI_UPDATABLECURSORS "=%d;"
			INI_TRUEISMINUS1 "=%d;"
			INI_INT8AS "=%d;"
			INI_BYTEAASLONGVARBINARY "=%d;"
			INI_USESERVERSIDEPREPARE "=%d;"
			INI_LOWERCASEIDENTIFIER "=%d;"
			"%s"		/* INI_PQOPT */
			"%s"		/* INIKEEPALIVE TIME/INTERVAL */
#ifdef	WIN32
			INI_GSSAUTHUSEGSSAPI "=%d;"
#endif /* WIN32 */
#ifdef	_HANDLE_ENLIST_IN_DTC_
			INI_XAOPT "=%d"	/* XAOPT */
#endif /* _HANDLE_ENLIST_IN_DTC_ */
			,ci->sslmode
			,ci->onlyread
			,protocol_and
			,ci->fake_oid_index
			,ci->show_oid_column
			,ci->row_versioning
			,ci->show_system_tables
			,makeBracketConnectString(&connsetStr, ci->conn_settings, INI_CONNSETTINGS)
			,ci->drivers.fetch_max
			,ci->drivers.unknown_sizes
			,ci->drivers.max_varchar_size
			,ci->drivers.max_longvarchar_size
			,ci->drivers.debug
			,ci->drivers.commlog
			,ci->drivers.use_declarefetch
			,ci->drivers.text_as_longvarchar
			,ci->drivers.unknowns_as_longvarchar
			,ci->drivers.bools_as_char
			,ci->drivers.parse
			,ci->drivers.extra_systable_prefixes
			,ci->lf_conversion
			,ci->allow_keyset
			,ci->true_is_minus1
			,ci->int8_as
			,ci->bytea_as_longvarbinary
			,ci->use_server_side_prepare
			,ci->lower_case_identifier
			,makeBracketConnectString(&pqoptStr, ci->pqopt, INI_PQOPT)
			,makeKeepaliveConnectString(keepaliveStr, ci, FALSE)
#ifdef	WIN32
			,ci->gssauth_use_gssapi
#endif /* WIN32 */
#ifdef	_HANDLE_ENLIST_IN_DTC_
			,ci->xa_opt
#endif /* _HANDLE_ENLIST_IN_DTC_ */
				);
	}
	/* Abbreviation is needed ? */
	if (abbrev || olen >= nlen || olen < 0)
	{
		flag = 0;
		if (ci->allow_keyset)
			flag |= BIT_UPDATABLECURSORS;
		if (ci->lf_conversion)
			flag |= BIT_LFCONVERSION;
		if (ci->drivers.unique_index)
			flag |= BIT_UNIQUEINDEX;
		switch (ci->drivers.unknown_sizes)
		{
			case UNKNOWNS_AS_DONTKNOW:
				flag |= BIT_UNKNOWN_DONTKNOW;
				break;
			case UNKNOWNS_AS_MAX:
				flag |= BIT_UNKNOWN_ASMAX;
				break;
		}
		if (ci->drivers.commlog)
			flag |= BIT_COMMLOG;
		if (ci->drivers.debug)
			flag |= BIT_DEBUG;
		if (ci->drivers.parse)
			flag |= BIT_PARSE;
		if (ci->drivers.use_declarefetch)
			flag |= BIT_USEDECLAREFETCH;
		if (ci->onlyread[0] == '1')
			flag |= BIT_READONLY;
		if (ci->drivers.text_as_longvarchar)
			flag |= BIT_TEXTASLONGVARCHAR;
		if (ci->drivers.unknowns_as_longvarchar)
			flag |= BIT_UNKNOWNSASLONGVARCHAR;
		if (ci->drivers.bools_as_char)
			flag |= BIT_BOOLSASCHAR;
		if (ci->row_versioning[0] == '1')
			flag |= BIT_ROWVERSIONING;
		if (ci->show_system_tables[0] == '1')
			flag |= BIT_SHOWSYSTEMTABLES;
		if (ci->show_oid_column[0] == '1')
			flag |= BIT_SHOWOIDCOLUMN;
		if (ci->fake_oid_index[0] == '1')
			flag |= BIT_FAKEOIDINDEX;
		if (ci->true_is_minus1)
			flag |= BIT_TRUEISMINUS1;
		if (ci->bytea_as_longvarbinary)
			flag |= BIT_BYTEAASLONGVARBINARY;
		if (ci->use_server_side_prepare)
			flag |= BIT_USESERVERSIDEPREPARE;
		if (ci->lower_case_identifier)
			flag |= BIT_LOWERCASEIDENTIFIER;
		if (ci->gssauth_use_gssapi)
			flag |= BIT_GSSAUTHUSEGSSAPI;

		if (ci->sslmode[0])
		{
			char	abbrevmode[sizeof(ci->sslmode)];

			(void) snprintf(&connect_string[hlen], nlen, ";"
				ABBR_SSLMODE "=%s", abbrev_sslmode(ci->sslmode, abbrevmode));
		}
		hlen = strlen(connect_string);
		nlen = MAX_CONNECT_STRING - hlen;
		olen = snprintf(&connect_string[hlen], nlen, ";"
				"%s"		/* ABBR_CONNSETTINGS */
				ABBR_FETCH "=%d;"
				ABBR_MAXVARCHARSIZE "=%d;"
				ABBR_MAXLONGVARCHARSIZE "=%d;"
				INI_INT8AS "=%d;"
				ABBR_EXTRASYSTABLEPREFIXES "=%s;"
				"%s"		/* ABBR_PQOPT */
				"%s"		/* ABBRKEEPALIVE TIME/INTERVAL */
#ifdef	_HANDLE_ENLIST_IN_DTC_
				"%s"
#endif /* _HANDLE_ENLIST_IN_DTC_ */
				INI_ABBREVIATE "=%02x%x",
				makeBracketConnectString(&connsetStr, ci->conn_settings, ABBR_CONNSETTINGS),
				ci->drivers.fetch_max,
				ci->drivers.max_varchar_size,
				ci->drivers.max_longvarchar_size,
				ci->int8_as,
				ci->drivers.extra_systable_prefixes,
				makeBracketConnectString(&pqoptStr, ci->pqopt, ABBR_PQOPT),
				makeKeepaliveConnectString(keepaliveStr, ci, TRUE),
#ifdef	_HANDLE_ENLIST_IN_DTC_
				makeXaOptConnectString(xaOptStr, ci, TRUE),
#endif /* _HANDLE_ENLIST_IN_DTC_ */
				EFFECTIVE_BIT_COUNT, flag);
		if (olen < nlen || ci->rollback_on_error >= 0)
		{
			hlen = strlen(connect_string);
			nlen = MAX_CONNECT_STRING - hlen;
			/*
			 * The PROTOCOL setting must be placed after CX flag
			 * so that this option can override the CX setting.
			 */
			if (ci->rollback_on_error >= 0)
				olen = snprintf(&connect_string[hlen], nlen, ";"
				ABBR_PROTOCOL "=7.4-%d",
								ci->rollback_on_error);
			else
				olen = snprintf(&connect_string[hlen], nlen, ";"
								ABBR_PROTOCOL "=7.4");
		}
	}
	if (olen < nlen)
	{
		flag = getExtraOptions(ci);
		if (0 != flag)
		{
			hlen = strlen(connect_string);
			nlen = MAX_CONNECT_STRING - hlen;
			olen = snprintf(&connect_string[hlen], nlen, ";"
				INI_EXTRAOPTIONS "=%x;",
				flag);
		}
	}
	if (olen < 0 || olen >= nlen) /* failed */
		connect_string[0] = '\0';

	if (NULL != connsetStr)
		free(connsetStr);
	if (NULL != pqoptStr)
		free(pqoptStr);
}

static void
unfoldCXAttribute(ConnInfo *ci, const char *value)
{
	int	count;
	UInt4	flag;

	if (strlen(value) < 2)
	{
		count = 3;
		sscanf(value, "%x", &flag);
	}
	else
	{
		char	cnt[8];
		memcpy(cnt, value, 2);
		cnt[2] = '\0';
		sscanf(cnt, "%x", &count);
		sscanf(value + 2, "%x", &flag);
	}
	ci->allow_keyset = (char)((flag & BIT_UPDATABLECURSORS) != 0);
	ci->lf_conversion = (char)((flag & BIT_LFCONVERSION) != 0);
	if (count < 4)
		return;
	ci->drivers.unique_index = (char)((flag & BIT_UNIQUEINDEX) != 0);
	if ((flag & BIT_UNKNOWN_DONTKNOW) != 0)
		ci->drivers.unknown_sizes = UNKNOWNS_AS_DONTKNOW;
	else if ((flag & BIT_UNKNOWN_ASMAX) != 0)
		ci->drivers.unknown_sizes = UNKNOWNS_AS_MAX;
	else
		ci->drivers.unknown_sizes = UNKNOWNS_AS_LONGEST;
	ci->drivers.commlog = (char)((flag & BIT_COMMLOG) != 0);
	ci->drivers.debug = (char)((flag & BIT_DEBUG) != 0);
	ci->drivers.parse = (char)((flag & BIT_PARSE) != 0);
	ci->drivers.use_declarefetch = (char)((flag & BIT_USEDECLAREFETCH) != 0);
	sprintf(ci->onlyread, "%d", (char)((flag & BIT_READONLY) != 0));
	ci->drivers.text_as_longvarchar = (char)((flag & BIT_TEXTASLONGVARCHAR) !=0);
	ci->drivers.unknowns_as_longvarchar = (char)((flag & BIT_UNKNOWNSASLONGVARCHAR) !=0);
	ci->drivers.bools_as_char = (char)((flag & BIT_BOOLSASCHAR) != 0);
	sprintf(ci->row_versioning, "%d", (char)((flag & BIT_ROWVERSIONING) != 0));
	sprintf(ci->show_system_tables, "%d", (char)((flag & BIT_SHOWSYSTEMTABLES) != 0));
	sprintf(ci->show_oid_column, "%d", (char)((flag & BIT_SHOWOIDCOLUMN) != 0));
	sprintf(ci->fake_oid_index, "%d", (char)((flag & BIT_FAKEOIDINDEX) != 0));
	ci->true_is_minus1 = (char)((flag & BIT_TRUEISMINUS1) != 0);
	ci->bytea_as_longvarbinary = (char)((flag & BIT_BYTEAASLONGVARBINARY) != 0);
	ci->use_server_side_prepare = (char)((flag & BIT_USESERVERSIDEPREPARE) != 0);
	ci->lower_case_identifier = (char)((flag & BIT_LOWERCASEIDENTIFIER) != 0);
	ci->gssauth_use_gssapi = (char)((flag & BIT_GSSAUTHUSEGSSAPI) != 0);
}
BOOL
copyAttributes(ConnInfo *ci, const char *attribute, const char *value)
{
	CSTR	func = "copyAttributes";
	BOOL	found = TRUE;

	if (stricmp(attribute, "DSN") == 0)
		strcpy(ci->dsn, value);

	else if (stricmp(attribute, "driver") == 0)
		strcpy(ci->drivername, value);

	else if (stricmp(attribute, INI_KDESC) == 0)
		strcpy(ci->desc, value);

	else if (stricmp(attribute, INI_DATABASE) == 0)
		strcpy(ci->database, value);

	else if (stricmp(attribute, INI_SERVER) == 0 || stricmp(attribute, SPEC_SERVER) == 0)
		strcpy(ci->server, value);

	else if (stricmp(attribute, INI_USERNAME) == 0 || stricmp(attribute, INI_UID) == 0)
		strcpy(ci->username, value);

	else if (stricmp(attribute, INI_PASSWORD) == 0 || stricmp(attribute, "pwd") == 0)
		ci->password = decode_or_remove_braces(value);

	else if (stricmp(attribute, INI_PORT) == 0)
		strcpy(ci->port, value);

	else if (stricmp(attribute, INI_READONLY) == 0 || stricmp(attribute, ABBR_READONLY) == 0)
		strcpy(ci->onlyread, value);

	else if (stricmp(attribute, INI_PROTOCOL) == 0 || stricmp(attribute, ABBR_PROTOCOL) == 0)
	{
		char	*ptr;
		/*
		 * The first part of the Protocol used to be "6.2", "6.3" or
		 * "7.4" to denote which protocol version to use. Nowadays we
		 * only support the 7.4 protocol, also known as the protocol
		 * version 3. So just ignore the first part of the string,
		 * parsing only the rollback_on_error value.
		 */
		ptr = strchr(value, '-');
		if (ptr)
		{
			if ('-' != *value)
			{
				*ptr = '\0';
				/* ignore first part */
			}
			ci->rollback_on_error = atoi(ptr + 1);
			mylog("rollback_on_error=%d\n", ci->rollback_on_error);
		}
	}

	else if (stricmp(attribute, INI_SHOWOIDCOLUMN) == 0 || stricmp(attribute, ABBR_SHOWOIDCOLUMN) == 0)
		strcpy(ci->show_oid_column, value);

	else if (stricmp(attribute, INI_FAKEOIDINDEX) == 0 || stricmp(attribute, ABBR_FAKEOIDINDEX) == 0)
		strcpy(ci->fake_oid_index, value);

	else if (stricmp(attribute, INI_ROWVERSIONING) == 0 || stricmp(attribute, ABBR_ROWVERSIONING) == 0)
		strcpy(ci->row_versioning, value);

	else if (stricmp(attribute, INI_SHOWSYSTEMTABLES) == 0 || stricmp(attribute, ABBR_SHOWSYSTEMTABLES) == 0)
		strcpy(ci->show_system_tables, value);

	else if (stricmp(attribute, INI_CONNSETTINGS) == 0 || stricmp(attribute, ABBR_CONNSETTINGS) == 0)
	{
		/* We can use the conn_settings directly when they are enclosed with braces */
		ci->conn_settings = decode_or_remove_braces(value);
	}
	else if (stricmp(attribute, INI_PQOPT) == 0 || stricmp(attribute, ABBR_PQOPT) == 0)
	{
		ci->pqopt = decode_or_remove_braces(value);
	}
	else if (stricmp(attribute, INI_UPDATABLECURSORS) == 0 || stricmp(attribute, ABBR_UPDATABLECURSORS) == 0)
		ci->allow_keyset = atoi(value);
	else if (stricmp(attribute, INI_LFCONVERSION) == 0 || stricmp(attribute, ABBR_LFCONVERSION) == 0)
		ci->lf_conversion = atoi(value);
	else if (stricmp(attribute, INI_TRUEISMINUS1) == 0 || stricmp(attribute, ABBR_TRUEISMINUS1) == 0)
		ci->true_is_minus1 = atoi(value);
	else if (stricmp(attribute, INI_INT8AS) == 0)
		ci->int8_as = atoi(value);
	else if (stricmp(attribute, INI_BYTEAASLONGVARBINARY) == 0 || stricmp(attribute, ABBR_BYTEAASLONGVARBINARY) == 0)
		ci->bytea_as_longvarbinary = atoi(value);
	else if (stricmp(attribute, INI_USESERVERSIDEPREPARE) == 0 || stricmp(attribute, ABBR_USESERVERSIDEPREPARE) == 0)
		ci->use_server_side_prepare = atoi(value);
	else if (stricmp(attribute, INI_LOWERCASEIDENTIFIER) == 0 || stricmp(attribute, ABBR_LOWERCASEIDENTIFIER) == 0)
		ci->lower_case_identifier = atoi(value);
	else if (stricmp(attribute, INI_GSSAUTHUSEGSSAPI) == 0 || stricmp(attribute, ABBR_GSSAUTHUSEGSSAPI) == 0)
		ci->gssauth_use_gssapi = atoi(value);
	else if (stricmp(attribute, INI_KEEPALIVETIME) == 0 || stricmp(attribute, ABBR_KEEPALIVETIME) == 0)
		ci->keepalive_idle = atoi(value);
	else if (stricmp(attribute, INI_KEEPALIVEINTERVAL) == 0 || stricmp(attribute, ABBR_KEEPALIVEINTERVAL) == 0)
		ci->keepalive_interval = atoi(value);
	else if (stricmp(attribute, INI_SSLMODE) == 0 || stricmp(attribute, ABBR_SSLMODE) == 0)
	{
		switch (value[0])
		{
			case SSLLBYTE_ALLOW:
				strcpy(ci->sslmode, SSLMODE_ALLOW);
				break;
			case SSLLBYTE_PREFER:
				strcpy(ci->sslmode, SSLMODE_PREFER);
				break;
			case SSLLBYTE_REQUIRE:
				strcpy(ci->sslmode, SSLMODE_REQUIRE);
				break;
			case SSLLBYTE_VERIFY:
				switch (value[1])
				{
					case 'f':
						strcpy(ci->sslmode, SSLMODE_VERIFY_FULL);
						break;
					case 'c':
						strcpy(ci->sslmode, SSLMODE_VERIFY_CA);
						break;
					default:
						strcpy(ci->sslmode, value);
				}
				break;
			case SSLLBYTE_DISABLE:
			default:
				strcpy(ci->sslmode, SSLMODE_DISABLE);
				break;
		}
	}
	else if (stricmp(attribute, INI_ABBREVIATE) == 0)
		unfoldCXAttribute(ci, value);
#ifdef	_HANDLE_ENLIST_IN_DTC_
	else if (stricmp(attribute, INI_XAOPT) == 0)
		ci->xa_opt = atoi(value);
#endif /* _HANDLE_ENLIST_IN_DTC_ */
	else if (stricmp(attribute, INI_EXTRAOPTIONS) == 0)
	{
		UInt4	val1 = 0, val2 = 0;

		if ('+' == value[0])
		{
			sscanf(value + 1, "%x-%x", &val1, &val2);
			add_removeExtraOptions(ci, val1, val2);
		}
		else if ('-' == value[0])
		{
			sscanf(value + 1, "%x", &val2);
			add_removeExtraOptions(ci, 0, val2);
		}
		else
		{
			setExtraOptions(ci, value, hex_format);
		}
		mylog("force_abbrev=%d bde=%d cvt_null_date=%x\n", ci->force_abbrev_connstr, ci->bde_environment, ci->cvt_null_date_string);
	}
	else
		found = FALSE;

	mylog("%s: DSN='%s',server='%s',dbase='%s',user='%s',passwd='%s',port='%s',onlyread='%s',conn_settings='%s')\n", func, ci->dsn, ci->server, ci->database, ci->username, NAME_IS_VALID(ci->password) ? "xxxxx" : "", ci->port, ci->onlyread, ci->conn_settings);

	return found;
}

BOOL
copyCommonAttributes(ConnInfo *ci, const char *attribute, const char *value)
{
	CSTR	func = "copyCommonAttributes";
	BOOL	found = TRUE;

	if (stricmp(attribute, INI_FETCH) == 0 || stricmp(attribute, ABBR_FETCH) == 0)
		ci->drivers.fetch_max = atoi(value);
	else if (stricmp(attribute, INI_DEBUG) == 0 || stricmp(attribute, ABBR_DEBUG) == 0)
		ci->drivers.debug = atoi(value);
	else if (stricmp(attribute, INI_COMMLOG) == 0 || stricmp(attribute, ABBR_COMMLOG) == 0)
		ci->drivers.commlog = atoi(value);
	/*
	 * else if (stricmp(attribute, INI_UNIQUEINDEX) == 0 ||
	 * stricmp(attribute, "UIX") == 0) ci->drivers.unique_index =
	 * atoi(value);
	 */
	else if (stricmp(attribute, INI_UNKNOWNSIZES) == 0 || stricmp(attribute, ABBR_UNKNOWNSIZES) == 0)
		ci->drivers.unknown_sizes = atoi(value);
	else if (stricmp(attribute, INI_LIE) == 0)
		ci->drivers.lie = atoi(value);
	else if (stricmp(attribute, INI_PARSE) == 0 || stricmp(attribute, ABBR_PARSE) == 0)
		ci->drivers.parse = atoi(value);
	else if (stricmp(attribute, INI_USEDECLAREFETCH) == 0 || stricmp(attribute, ABBR_USEDECLAREFETCH) == 0)
		ci->drivers.use_declarefetch = atoi(value);
	else if (stricmp(attribute, INI_MAXVARCHARSIZE) == 0 || stricmp(attribute, ABBR_MAXVARCHARSIZE) == 0)
		ci->drivers.max_varchar_size = atoi(value);
	else if (stricmp(attribute, INI_MAXLONGVARCHARSIZE) == 0 || stricmp(attribute, ABBR_MAXLONGVARCHARSIZE) == 0)
		ci->drivers.max_longvarchar_size = atoi(value);
	else if (stricmp(attribute, INI_TEXTASLONGVARCHAR) == 0 || stricmp(attribute, ABBR_TEXTASLONGVARCHAR) == 0)
		ci->drivers.text_as_longvarchar = atoi(value);
	else if (stricmp(attribute, INI_UNKNOWNSASLONGVARCHAR) == 0 || stricmp(attribute, ABBR_UNKNOWNSASLONGVARCHAR) == 0)
		ci->drivers.unknowns_as_longvarchar = atoi(value);
	else if (stricmp(attribute, INI_BOOLSASCHAR) == 0 || stricmp(attribute, ABBR_BOOLSASCHAR) == 0)
		ci->drivers.bools_as_char = atoi(value);
	else if (stricmp(attribute, INI_EXTRASYSTABLEPREFIXES) == 0 || stricmp(attribute, ABBR_EXTRASYSTABLEPREFIXES) == 0)
		strcpy(ci->drivers.extra_systable_prefixes, value);
	else
		found = FALSE;

	mylog("%s: A7=%d;A9=%d;B0=%d;B1=%d;B2=%d;B3=%d;B6=%d;B7=%d;B8=%d;B9=%d;C0=%d;C2=%s", func,
		  ci->drivers.fetch_max,
		  ci->drivers.unknown_sizes,
		  ci->drivers.max_varchar_size,
		  ci->drivers.max_longvarchar_size,
		  ci->drivers.debug,
		  ci->drivers.commlog,
		  ci->drivers.use_declarefetch,
		  ci->drivers.text_as_longvarchar,
		  ci->drivers.unknowns_as_longvarchar,
		  ci->drivers.bools_as_char,
		  ci->drivers.parse,
		  ci->drivers.extra_systable_prefixes);

	return found;
}


void
getDSNdefaults(ConnInfo *ci)
{
	mylog("calling getDSNdefaults\n");

	if (ci->drivers.debug < 0)
		ci->drivers.debug = DEFAULT_DEBUG;
	if (ci->drivers.commlog < 0)
		ci->drivers.commlog = DEFAULT_COMMLOG;

	if (ci->onlyread[0] == '\0')
		sprintf(ci->onlyread, "%d", DEFAULT_READONLY);

	if (ci->fake_oid_index[0] == '\0')
		sprintf(ci->fake_oid_index, "%d", DEFAULT_FAKEOIDINDEX);

	if (ci->show_oid_column[0] == '\0')
		sprintf(ci->show_oid_column, "%d", DEFAULT_SHOWOIDCOLUMN);

	if (ci->show_system_tables[0] == '\0')
		sprintf(ci->show_system_tables, "%d", DEFAULT_SHOWSYSTEMTABLES);

	if (ci->row_versioning[0] == '\0')
		sprintf(ci->row_versioning, "%d", DEFAULT_ROWVERSIONING);

	if (ci->allow_keyset < 0)
		ci->allow_keyset = DEFAULT_UPDATABLECURSORS;
	if (ci->lf_conversion < 0)
		ci->lf_conversion = DEFAULT_LFCONVERSION;
	if (ci->true_is_minus1 < 0)
		ci->true_is_minus1 = DEFAULT_TRUEISMINUS1;
	if (ci->int8_as < -100)
		ci->int8_as = DEFAULT_INT8AS;
	if (ci->bytea_as_longvarbinary < 0)
		ci->bytea_as_longvarbinary = DEFAULT_BYTEAASLONGVARBINARY;
	if (ci->use_server_side_prepare < 0)
		ci->use_server_side_prepare = DEFAULT_USESERVERSIDEPREPARE;
	if (ci->lower_case_identifier < 0)
		ci->lower_case_identifier = DEFAULT_LOWERCASEIDENTIFIER;
	if (ci->gssauth_use_gssapi < 0)
		ci->gssauth_use_gssapi = DEFAULT_GSSAUTHUSEGSSAPI;
	if (ci->sslmode[0] == '\0')
		strcpy(ci->sslmode, DEFAULT_SSLMODE);
	if (ci->force_abbrev_connstr < 0)
		ci->force_abbrev_connstr = 0;
	if (ci->fake_mss < 0)
		ci->fake_mss = 0;
	if (ci->bde_environment < 0)
		ci->bde_environment = 0;
	if (ci->cvt_null_date_string < 0)
		ci->cvt_null_date_string = 0;
	if (ci->accessible_only < 0)
		ci->accessible_only = 0;
	if (ci->ignore_round_trip_time < 0)
		ci->ignore_round_trip_time = 0;
	if (ci->disable_keepalive < 0)
		ci->disable_keepalive = 0;
	if (ci->wcs_debug < 0)
	{
		const char *p;

		ci->wcs_debug = 0;
		if (NULL != (p = getenv("PSQLODBC_WCS_DEBUG")))
			if (strcmp(p, "1") == 0)
				ci->wcs_debug = 1;
	}
#ifdef	_HANDLE_ENLIST_IN_DTC_
	if (ci->xa_opt < 0)
		ci->xa_opt = DEFAULT_XAOPT;
#endif /* _HANDLE_ENLIST_IN_DTC_ */
}

int
getDriverNameFromDSN(const char *dsn, char *driver_name, int namelen)
{
#ifdef	WIN32
	return SQLGetPrivateProfileString(ODBC_DATASOURCES, dsn, NULL_STRING, driver_name, namelen, ODBC_INI);
#else /* WIN32 */
	int	cnt;

	cnt = SQLGetPrivateProfileString(dsn, "Driver", NULL_STRING, driver_name, namelen, ODBC_INI);
	if (!driver_name[0])
		return cnt;
	if (strchr(driver_name, '/') || /* path to the driver */
	    strchr(driver_name, '.'))
	{
		driver_name[0] = '\0';
		return 0;
	}
	return cnt;
#endif /* WIN32 */
}


static void
get_Ci_Drivers(const char *section, const char *filename, GLOBAL_VALUES *comval);

void getDriversDefaults(const char *drivername, GLOBAL_VALUES *comval)
{
	mylog("%s:%p of the driver %s\n", __FUNCTION__, comval, NULL_IF_NULL(drivername));
	get_Ci_Drivers(drivername, ODBCINST_INI, comval);
	if (NULL != drivername)
		STR_TO_NAME(comval->drivername, drivername);
}

void
getDSNinfo(ConnInfo *ci, char overwrite, const char *configDrvrname)
{
	CSTR	func = "getDSNinfo";
	char	   *DSN = ci->dsn;
	char		encoded_item[LARGE_REGISTRY_LEN],
				temp[SMALL_REGISTRY_LEN];
	const char *drivername;

/*
 *	If a driver keyword was present, then dont use a DSN and return.
 *	If DSN is null and no driver, then use the default datasource.
 */
	mylog("%s: DSN=%s driver=%s&%s overwrite=%d\n", func, DSN,
		ci->drivername, NULL_IF_NULL(configDrvrname),
			overwrite);
	drivername = ci->drivername;
	if (DSN[0] == '\0')
	{
		if (drivername[0] != '\0') /* dns-less connections */
		{
			getDriversDefaults(drivername, &(ci->drivers));
			return;
		}
		else	/* adding new DSN via configDSN */
		{
			drivername = configDrvrname;
			strncpy_null(DSN, INI_DSN, sizeof(ci->dsn));
		}
	}

	/* brute-force chop off trailing blanks... */
	while (*(DSN + strlen(DSN) - 1) == ' ')
		*(DSN + strlen(DSN) - 1) = '\0';

	if (drivername[0] == '\0' || overwrite)
	{
		if (DSN[0])
			getDriverNameFromDSN(DSN, (char *) drivername, sizeof(ci->drivername));
	}
mylog("drivername=%s\n", drivername);
	if (!drivername[0])
		drivername = INVALID_DRIVER;
	getDriversDefaults(drivername, &(ci->drivers));

	/* Proceed with getting info for the given DSN. */

	if (ci->desc[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_KDESC, "", ci->desc, sizeof(ci->desc), ODBC_INI);

	if (ci->server[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_SERVER, "", ci->server, sizeof(ci->server), ODBC_INI);

	if (ci->database[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_DATABASE, "", ci->database, sizeof(ci->database), ODBC_INI);

	if (ci->username[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_USERNAME, "", ci->username, sizeof(ci->username), ODBC_INI);

	if (NAME_IS_NULL(ci->password) || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_PASSWORD, "", encoded_item, sizeof(encoded_item), ODBC_INI);
		ci->password = decode(encoded_item);
	}

	if (ci->port[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_PORT, "", ci->port, sizeof(ci->port), ODBC_INI);

	/* It's appropriate to handle debug and commlog here */
	if (ci->drivers.debug < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_DEBUG, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->drivers.debug = atoi(temp);
	}
	if (ci->drivers.commlog < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_COMMLOG, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->drivers.commlog = atoi(temp);
	}

	if (ci->onlyread[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_READONLY, "", ci->onlyread, sizeof(ci->onlyread), ODBC_INI);

	if (ci->show_oid_column[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_SHOWOIDCOLUMN, "", ci->show_oid_column, sizeof(ci->show_oid_column), ODBC_INI);

	if (ci->fake_oid_index[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_FAKEOIDINDEX, "", ci->fake_oid_index, sizeof(ci->fake_oid_index), ODBC_INI);

	if (ci->row_versioning[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_ROWVERSIONING, "", ci->row_versioning, sizeof(ci->row_versioning), ODBC_INI);

	if (ci->show_system_tables[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_SHOWSYSTEMTABLES, "", ci->show_system_tables, sizeof(ci->show_system_tables), ODBC_INI);

	if (ci->rollback_on_error == -1 || overwrite)
	{
		char protocol[SMALL_REGISTRY_LEN];
		char	*ptr;
		SQLGetPrivateProfileString(DSN, INI_PROTOCOL, "", protocol, sizeof(protocol), ODBC_INI);
		if (ptr = strchr(protocol, '-'), NULL != ptr)
		{
			*ptr = '\0';
			if (overwrite || ci->rollback_on_error < 0)
			{
				ci->rollback_on_error = atoi(ptr + 1);
				mylog("rollback_on_error=%d\n", ci->rollback_on_error);
			}
		}
	}

	if (NAME_IS_NULL(ci->conn_settings) || overwrite)
	{
		const UCHAR *ptr;
		BOOL	percent_encoded = TRUE, pspace;
		int	nspcnt;

		SQLGetPrivateProfileString(DSN, INI_CONNSETTINGS, "", encoded_item, sizeof(encoded_item), ODBC_INI);
		/*
		 *	percent-encoding was used before.
		 *	Note that there's no space in percent-encoding.
		 */
		for (ptr = (UCHAR *) encoded_item, pspace = TRUE, nspcnt = 0; *ptr; ptr++)
		{
			if (isspace(*ptr))
				pspace = TRUE;
			else
			{
				if (pspace)
				{
					if (nspcnt++ > 1)
					{
						percent_encoded = FALSE;
						break;
					}
				}
				pspace = FALSE;
			}
		}
		if (percent_encoded)
			ci->conn_settings = decode(encoded_item);
		else
			STRX_TO_NAME(ci->conn_settings, encoded_item);
	}
	if (NAME_IS_NULL(ci->pqopt))
	{
		SQLGetPrivateProfileString(DSN, INI_PQOPT, "", encoded_item, sizeof(encoded_item), ODBC_INI);
		STRX_TO_NAME(ci->pqopt, encoded_item);
	}

	if (ci->translation_dll[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_TRANSLATIONDLL, "", ci->translation_dll, sizeof(ci->translation_dll), ODBC_INI);

	if (ci->translation_option[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_TRANSLATIONOPTION, "", ci->translation_option, sizeof(ci->translation_option), ODBC_INI);

	if (ci->allow_keyset < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_UPDATABLECURSORS, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->allow_keyset = atoi(temp);
	}

	if (ci->lf_conversion < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_LFCONVERSION, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->lf_conversion = atoi(temp);
	}

	if (ci->true_is_minus1 < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_TRUEISMINUS1, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->true_is_minus1 = atoi(temp);
	}

	if (ci->int8_as < -100 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_INT8AS, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->int8_as = atoi(temp);
	}

	if (ci->bytea_as_longvarbinary < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_BYTEAASLONGVARBINARY, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->bytea_as_longvarbinary = atoi(temp);
	}

	if (ci->use_server_side_prepare < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_USESERVERSIDEPREPARE, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->use_server_side_prepare = atoi(temp);
	}

	if (ci->lower_case_identifier < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_LOWERCASEIDENTIFIER, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->lower_case_identifier = atoi(temp);
	}

	if (ci->gssauth_use_gssapi < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_GSSAUTHUSEGSSAPI, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->gssauth_use_gssapi = atoi(temp);
	}

	if (ci->keepalive_idle < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_KEEPALIVETIME, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			if (0 == (ci->keepalive_idle = atoi(temp)))
				ci->keepalive_idle = -1;
	}
	if (ci->keepalive_interval < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_KEEPALIVEINTERVAL, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			if (0 == (ci->keepalive_interval = atoi(temp)))
				ci->keepalive_interval = -1;
	}

	if (ci->sslmode[0] == '\0' || overwrite)
		SQLGetPrivateProfileString(DSN, INI_SSLMODE, "", ci->sslmode, sizeof(ci->sslmode), ODBC_INI);

#ifdef	_HANDLE_ENLIST_IN_DTC_
	if (ci->xa_opt < 0 || overwrite)
	{
		SQLGetPrivateProfileString(DSN, INI_XAOPT, "", temp, sizeof(temp), ODBC_INI);
		if (temp[0])
			ci->xa_opt = atoi(temp);
	}
#endif /* _HANDLE_ENLIST_IN_DTC_ */

	/* Force abbrev connstr or bde */
	SQLGetPrivateProfileString(DSN, INI_EXTRAOPTIONS, "",
					temp, sizeof(temp), ODBC_INI);
	if (temp[0])
	{
		UInt4	val = 0;

		sscanf(temp, "%x", &val);
		replaceExtraOptions(ci, val, overwrite);
		mylog("force_abbrev=%d bde=%d cvt_null_date=%d\n", ci->force_abbrev_connstr, ci->bde_environment, ci->cvt_null_date_string);
	}

	/* Allow override of odbcinst.ini parameters here */
	get_Ci_Drivers(DSN, ODBC_INI, &(ci->drivers));
	STR_TO_NAME(ci->drivers.drivername, drivername);

	qlog("DSN info: DSN='%s',server='%s',port='%s',dbase='%s',user='%s',passwd='%s'\n",
		 DSN,
		 ci->server,
		 ci->port,
		 ci->database,
		 ci->username,
		 NAME_IS_VALID(ci->password) ? "xxxxx" : "");
	qlog("          onlyread='%s',showoid='%s',fakeoidindex='%s',showsystable='%s'\n",
		 ci->onlyread,
		 ci->show_oid_column,
		 ci->fake_oid_index,
		 ci->show_system_tables);

	if (get_qlog())
	{
#ifdef	NOT_USED
		char	*enc = (char *) check_client_encoding(ci->conn_settings);

		qlog("          conn_settings='%s', conn_encoding='%s'\n", ci->conn_settings,
			NULL != enc ? enc : "(null)");
		if (NULL != enc)
			free(enc);
#endif /* NOT_USED */
		qlog("          translation_dll='%s',translation_option='%s'\n",
			ci->translation_dll,
			ci->translation_option);
	}
}
/*
 *	This function writes any global parameters (that can be manipulated)
 *	to the ODBCINST.INI portion of the registry
 */
int
write_Ci_Drivers(const char *fileName, const char *sectionName,
			 const GLOBAL_VALUES *comval)
{
	char		tmp[128];
	int		errc = 0;

	if (stricmp(ODBCINST_INI, fileName) == 0)
	{
		if (NULL == sectionName)
			sectionName = DBMS_NAME;
	}

	if (stricmp(ODBCINST_INI, fileName) == 0)
		return errc;

	sprintf(tmp, "%d", comval->commlog);
	if (!SQLWritePrivateProfileString(sectionName, INI_COMMLOG, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->debug);
	if (!SQLWritePrivateProfileString(sectionName, INI_DEBUG, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->fetch_max);
	if (!SQLWritePrivateProfileString(sectionName, INI_FETCH, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->unique_index);
	if (!SQLWritePrivateProfileString(sectionName, INI_UNIQUEINDEX, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->use_declarefetch);
	if (!SQLWritePrivateProfileString(sectionName, INI_USEDECLAREFETCH, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->unknown_sizes);
	if (!SQLWritePrivateProfileString(sectionName, INI_UNKNOWNSIZES, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->text_as_longvarchar);
	if (!SQLWritePrivateProfileString(sectionName, INI_TEXTASLONGVARCHAR, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->unknowns_as_longvarchar);
	if (!SQLWritePrivateProfileString(sectionName, INI_UNKNOWNSASLONGVARCHAR, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->bools_as_char);
	if (!SQLWritePrivateProfileString(sectionName, INI_BOOLSASCHAR, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->parse);
	if (!SQLWritePrivateProfileString(sectionName, INI_PARSE, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->max_varchar_size);
	if (!SQLWritePrivateProfileString(sectionName, INI_MAXVARCHARSIZE, tmp, fileName))
		errc--;

	sprintf(tmp, "%d", comval->max_longvarchar_size);
	if (!SQLWritePrivateProfileString(sectionName, INI_MAXLONGVARCHARSIZE, tmp, fileName))
		errc--;

	if (!SQLWritePrivateProfileString(sectionName, INI_EXTRASYSTABLEPREFIXES, comval->extra_systable_prefixes, fileName))
		errc--;

	/*
	 * Never update the conn_setting from this module
	 * SQLWritePrivateProfileString(sectionName, INI_CONNSETTINGS,
	 * comval->conn_settings, fileName);
	 */

	return errc;
}

int
writeDriversDefaults(const char *drivername, const GLOBAL_VALUES *comval)
{
	return write_Ci_Drivers(ODBCINST_INI, drivername, comval);
}

/*	This is for datasource based options only */
void
writeDSNinfo(const ConnInfo *ci)
{
	const char *DSN = ci->dsn;
	char		encoded_item[MEDIUM_REGISTRY_LEN],
				temp[SMALL_REGISTRY_LEN];


	SQLWritePrivateProfileString(DSN,
								 INI_KDESC,
								 ci->desc,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_DATABASE,
								 ci->database,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_SERVER,
								 ci->server,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_PORT,
								 ci->port,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_USERNAME,
								 ci->username,
								 ODBC_INI);
	SQLWritePrivateProfileString(DSN, INI_UID, ci->username, ODBC_INI);

	encode(ci->password, encoded_item, sizeof(encoded_item));
	SQLWritePrivateProfileString(DSN,
								 INI_PASSWORD,
								 encoded_item,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_READONLY,
								 ci->onlyread,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_SHOWOIDCOLUMN,
								 ci->show_oid_column,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_FAKEOIDINDEX,
								 ci->fake_oid_index,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_ROWVERSIONING,
								 ci->row_versioning,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_SHOWSYSTEMTABLES,
								 ci->show_system_tables,
								 ODBC_INI);

	if (ci->rollback_on_error >= 0)
		sprintf(temp, "7.4-%d", ci->rollback_on_error);
	else
		strncpy_null(temp, "", sizeof(temp));
	SQLWritePrivateProfileString(DSN,
								 INI_PROTOCOL,
								 temp,
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_CONNSETTINGS,
								 SAFE_NAME(ci->conn_settings),
								 ODBC_INI);

	SQLWritePrivateProfileString(DSN,
								 INI_PQOPT,
								 SAFE_NAME(ci->pqopt),
								 ODBC_INI);

	sprintf(temp, "%d", ci->allow_keyset);
	SQLWritePrivateProfileString(DSN,
								 INI_UPDATABLECURSORS,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%d", ci->lf_conversion);
	SQLWritePrivateProfileString(DSN,
								 INI_LFCONVERSION,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%d", ci->true_is_minus1);
	SQLWritePrivateProfileString(DSN,
								 INI_TRUEISMINUS1,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%d", ci->int8_as);
	SQLWritePrivateProfileString(DSN,
								 INI_INT8AS,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%x", getExtraOptions(ci));
	SQLWritePrivateProfileString(DSN,
							INI_EXTRAOPTIONS,
							 temp,
							 ODBC_INI);
	sprintf(temp, "%d", ci->bytea_as_longvarbinary);
	SQLWritePrivateProfileString(DSN,
								 INI_BYTEAASLONGVARBINARY,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%d", ci->use_server_side_prepare);
	SQLWritePrivateProfileString(DSN,
								 INI_USESERVERSIDEPREPARE,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%d", ci->lower_case_identifier);
	SQLWritePrivateProfileString(DSN,
								 INI_LOWERCASEIDENTIFIER,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%d", ci->gssauth_use_gssapi);
	SQLWritePrivateProfileString(DSN,
								 INI_GSSAUTHUSEGSSAPI,
								 temp,
								 ODBC_INI);
	SQLWritePrivateProfileString(DSN,
								 INI_SSLMODE,
								 ci->sslmode,
								 ODBC_INI);
	sprintf(temp, "%d", ci->keepalive_idle);
	SQLWritePrivateProfileString(DSN,
								 INI_KEEPALIVETIME,
								 temp,
								 ODBC_INI);
	sprintf(temp, "%d", ci->keepalive_interval);
	SQLWritePrivateProfileString(DSN,
								 INI_KEEPALIVEINTERVAL,
								 temp,
								 ODBC_INI);
#ifdef	_HANDLE_ENLIST_IN_DTC_
	sprintf(temp, "%d", ci->xa_opt);
	SQLWritePrivateProfileString(DSN, INI_XAOPT, temp, ODBC_INI);
#endif /* _HANDLE_ENLIST_IN_DTC_ */
}


/*
 *	This function reads the ODBCINST.INI portion of
 *	the registry and gets any driver defaults.
 */
static void
get_Ci_Drivers(const char *section, const char *filename, GLOBAL_VALUES *comval)
{
	CSTR	func = "get_Ci_Drivers";
	char		temp[256];
	BOOL	inst_position = (stricmp(filename, ODBCINST_INI) == 0);

	if (0 != strcmp(ODBCINST_INI, filename))
		mylog("%s:setting %s position of %s(%p)\n", func, filename, section, comval);

	/*
	 * It's not appropriate to handle debug or commlog here.
	 * Now they are handled in getDSNinfo().
	 */

	if (inst_position)
	{
		comval->fetch_max = FETCH_MAX;
		comval->unique_index = DEFAULT_UNIQUEINDEX;
		comval->unknown_sizes = DEFAULT_UNKNOWNSIZES;
		comval->lie = DEFAULT_LIE;
		comval->parse = DEFAULT_PARSE;
		comval->use_declarefetch = DEFAULT_USEDECLAREFETCH;
		comval->max_varchar_size = MAX_VARCHAR_SIZE;
		comval->max_longvarchar_size = TEXT_FIELD_SIZE;
		comval->text_as_longvarchar = DEFAULT_TEXTASLONGVARCHAR;
		comval->unknowns_as_longvarchar = DEFAULT_UNKNOWNSASLONGVARCHAR;
		comval->bools_as_char = DEFAULT_BOOLSASCHAR;
		strcpy(comval->extra_systable_prefixes, DEFAULT_EXTRASYSTABLEPREFIXES);
		strcpy(comval->protocol, DEFAULT_PROTOCOL);
	}
	if (NULL == section || strcmp(section, INVALID_DRIVER) == 0)
		return;
	/*
	 * If inst_position of xxxxxx is present(usually not present),
	 * it is the default of ci->drivers.xxxxxx .
	 */
	/* Fetch Count is stored in driver section */
	SQLGetPrivateProfileString(section, INI_FETCH, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
	{
		if (atoi(temp) > 0)
			comval->fetch_max = atoi(temp);
	}

	/* Recognize Unique Index is stored in the driver section only */
	SQLGetPrivateProfileString(section, INI_UNIQUEINDEX, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->unique_index = atoi(temp);

	/* Unknown Sizes is stored in the driver section only */
	SQLGetPrivateProfileString(section, INI_UNKNOWNSIZES, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->unknown_sizes = atoi(temp);

	/* Lie about supported functions? */
	SQLGetPrivateProfileString(section, INI_LIE, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->lie = atoi(temp);

	/* Parse statements */
	SQLGetPrivateProfileString(section, INI_PARSE, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->parse = atoi(temp);

	/* UseDeclareFetch is stored in the driver section only */
	SQLGetPrivateProfileString(section, INI_USEDECLAREFETCH, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->use_declarefetch = atoi(temp);

	/* Max Varchar Size */
	SQLGetPrivateProfileString(section, INI_MAXVARCHARSIZE, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->max_varchar_size = atoi(temp);

	/* Max TextField Size */
	SQLGetPrivateProfileString(section, INI_MAXLONGVARCHARSIZE, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->max_longvarchar_size = atoi(temp);

	/* Text As LongVarchar	*/
	SQLGetPrivateProfileString(section, INI_TEXTASLONGVARCHAR, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->text_as_longvarchar = atoi(temp);

	/* Unknowns As LongVarchar	*/
	SQLGetPrivateProfileString(section, INI_UNKNOWNSASLONGVARCHAR, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->unknowns_as_longvarchar = atoi(temp);

	/* Bools As Char */
	SQLGetPrivateProfileString(section, INI_BOOLSASCHAR, "",
							   temp, sizeof(temp), filename);
	if (temp[0])
		comval->bools_as_char = atoi(temp);

	/* Extra Systable prefixes */

	/*
	 * Use @@@ to distinguish between blank extra prefixes and no key
	 * entry
	 */
	SQLGetPrivateProfileString(section, INI_EXTRASYSTABLEPREFIXES, "@@@",
							   temp, sizeof(temp), filename);
	if (strcmp(temp, "@@@"))
		strcpy(comval->extra_systable_prefixes, temp);

	mylog("comval=%p comval->extra_systable_prefixes = '%s'\n", comval, comval->extra_systable_prefixes);


	/* Dont allow override of an override! */
	if (inst_position)
	{
		/*
		 * Default state for future DSN's protocol attribute This isn't a
		 * real driver option YET.	This is more intended for
		 * customization from the install.
		 */
		SQLGetPrivateProfileString(section, INI_PROTOCOL, "@@@",
								   temp, sizeof(temp), filename);
		if (strcmp(temp, "@@@"))
			strncpy_null(comval->protocol, temp, sizeof(comval->protocol));
	}
}

static void
encode(const pgNAME in, char *out, int outlen)
{
	size_t i, ilen, o = 0;
	char	inc, *ins;

	if (NAME_IS_NULL(in))
	{
		out[0] = '\0';
		return;
	}
	ins = GET_NAME(in);
	ilen = strlen(ins);
	for (i = 0; i < ilen && o < outlen - 1; i++)
	{
		inc = ins[i];
		if (inc == '+')
		{
			if (o + 2 >= outlen)
				break;
			sprintf(&out[o], "%%2B");
			o += 3;
		}
		else if (isspace((unsigned char) inc))
			out[o++] = '+';
		else if (!isalnum((unsigned char) inc))
		{
			if (o + 2 >= outlen)
				break;
			sprintf(&out[o], "%%%02x", inc);
			o += 3;
		}
		else
			out[o++] = inc;
	}
	out[o++] = '\0';
}

static unsigned int
conv_from_hex(const char *s)
{
	int			i,
				y = 0,
				val;

	for (i = 1; i <= 2; i++)
	{
		if (s[i] >= 'a' && s[i] <= 'f')
			val = s[i] - 'a' + 10;
		else if (s[i] >= 'A' && s[i] <= 'F')
			val = s[i] - 'A' + 10;
		else
			val = s[i] - '0';

		y += val << (4 * (2 - i));
	}

	return y;
}

static pgNAME
decode(const char *in)
{
	size_t i, ilen = strlen(in), o = 0;
	char	inc, *outs;
	pgNAME	out;

	INIT_NAME(out);
	if (0 == ilen)
	{
		return out;
	}
	outs = (char *) malloc(ilen + 1);
	if (!outs)
		return out;
	for (i = 0; i < ilen; i++)
	{
		inc = in[i];
		if (inc == '+')
			outs[o++] = ' ';
		else if (inc == '%')
		{
			sprintf(&outs[o++], "%c", conv_from_hex(&in[i]));
			i += 2;
		}
		else
			outs[o++] = inc;
	}
	outs[o++] = '\0';
	STR_TO_NAME(out, outs);
	free(outs);
	return out;
}

/*
 *	Remove braces if the input value is enclosed by braces({}).
 *	Othewise decode the input value.
 */
static pgNAME
decode_or_remove_braces(const char *in)
{
	if (OPENING_BRACKET == in[0])
	{
		size_t inlen = strlen(in);
		if (CLOSING_BRACKET == in[inlen - 1]) /* enclosed with braces */
		{
			int	i;
			const char	*istr, *eptr;
			char	*ostr;
			pgNAME	out;

			INIT_NAME(out);
			if (NULL == (ostr = (char *) malloc(inlen)))
				return out;
			eptr = in + inlen - 1;
			for (istr = in + 1, i = 0; *istr && istr < eptr; i++)
			{
				if (CLOSING_BRACKET == istr[0] &&
				    CLOSING_BRACKET == istr[1])
					istr++;
				ostr[i] = *(istr++);
			}
			ostr[i] = '\0';
			SET_NAME_DIRECTLY(out, ostr);
			return out;
		}
	}
	return decode(in);
}

/*
 *	extract the specified attribute from the comment part.
 *		attribute=[']value[']
 */
char *extract_extra_attribute_setting(const pgNAME setting, const char *attr)
{
	const char *str = SAFE_NAME(setting);
	const char *cptr, *sptr = NULL;
	char	   *rptr;
	BOOL	allowed_cmd = FALSE, in_quote = FALSE, in_comment = FALSE;
	int	step = 0, step_last = 2;
	size_t	len = 0, attrlen = strlen(attr);

	for (cptr = str; *cptr; cptr++)
	{
		if (in_quote)
		{
			if (LITERAL_QUOTE == *cptr)
			{
				if (step_last == step)
				{
					len = cptr - sptr;
					step = 0;
				}
				in_quote = FALSE;
			}
			continue;
		}
		else if (in_comment)
		{
			if ('*' == *cptr &&
			    '/' == cptr[1])
			{
				if (step_last == step)
				{
					len = cptr - sptr;
					step = 0;
				}
				in_comment = FALSE;
				allowed_cmd = FALSE;
				cptr++;
				continue;
			}
		}
		else if ('/' == *cptr &&
			 '*' == cptr[1])
		{
			in_comment = TRUE;
			allowed_cmd = TRUE;
			cptr++;
			continue;
		}
		else
		{
			if (LITERAL_QUOTE == *cptr)
				in_quote = TRUE;
			continue;
		}
		/* now in comment */
		if (';' == *cptr ||
		    isspace((unsigned char) *cptr))
		{
			if (step_last == step)
				len = cptr - sptr;
			allowed_cmd = TRUE;
			step = 0;
			continue;
		}
		if (!allowed_cmd)
			continue;
		switch (step)
		{
			case 0:
				if (0 != strnicmp(cptr, attr, attrlen))
				{
					allowed_cmd = FALSE;
					continue;
				}
				if (cptr[attrlen] != '=')
				{
					allowed_cmd = FALSE;
					continue;
				}
				step++;
				cptr += attrlen;
				break;
			case 1:
				if (LITERAL_QUOTE == *cptr)
				{
					in_quote = TRUE;
					cptr++;
					sptr = cptr;
				}
				else
					sptr = cptr;
				step++;
				break;
		}
	}
	if (!sptr)
		return NULL;
	rptr = malloc(len + 1);
	if (!rptr)
		return NULL;
	memcpy(rptr, sptr, len);
	rptr[len] = '\0';
	mylog("extracted a %s '%s' from %s\n", attr, rptr, str);
	return rptr;
}

void
CC_conninfo_release(ConnInfo *conninfo)
{
	NULL_THE_NAME(conninfo->password);
	NULL_THE_NAME(conninfo->conn_settings);
	NULL_THE_NAME(conninfo->pqopt);
	finalize_globals(&conninfo->drivers);
}

void
CC_conninfo_init(ConnInfo *conninfo, UInt4 option)
{
	CSTR	func = "CC_conninfo_init";
	mylog("%s opt=%d\n", func, option);

	if (0 != (CLEANUP_FOR_REUSE & option))
		CC_conninfo_release(conninfo);
	memset(conninfo, 0, sizeof(ConnInfo));

	conninfo->allow_keyset = -1;
	conninfo->lf_conversion = -1;
	conninfo->true_is_minus1 = -1;
	conninfo->int8_as = -101;
	conninfo->bytea_as_longvarbinary = -1;
	conninfo->use_server_side_prepare = -1;
	conninfo->lower_case_identifier = -1;
	conninfo->rollback_on_error = -1;
	conninfo->force_abbrev_connstr = -1;
	conninfo->bde_environment = -1;
	conninfo->fake_mss = -1;
	conninfo->cvt_null_date_string = -1;
	conninfo->accessible_only = -1;
	conninfo->ignore_round_trip_time = -1;
	conninfo->disable_keepalive = -1;
	conninfo->gssauth_use_gssapi = -1;
	conninfo->keepalive_idle = -1;
	conninfo->keepalive_interval = -1;
	conninfo->wcs_debug = -1;
#ifdef	_HANDLE_ENLIST_IN_DTC_
	conninfo->xa_opt = -1;
#endif /* _HANDLE_ENLIST_IN_DTC_ */
	if (0 != (INIT_GLOBALS & option))
		init_globals(&(conninfo->drivers));
}

void	init_globals(GLOBAL_VALUES *glbv)
{
	memset(glbv, 0, sizeof(*glbv));
	glbv->debug = -1;
	glbv->commlog = -1;
}

#define	CORR_STRCPY(item)	strncpy_null(to->item, from->item, sizeof(to->item))
#define	CORR_VALCPY(item)	(to->item = from->item)

void	copy_globals(GLOBAL_VALUES *to, const GLOBAL_VALUES *from)
{
	memset(to, 0, sizeof(*to));
	/***
	memcpy(to, from, sizeof(GLOBAL_VALUES));
	SET_NAME_DIRECTLY(to->drivername, NULL);
	***/
	NAME_TO_NAME(to->drivername, from->drivername);
	CORR_VALCPY(fetch_max);
	CORR_VALCPY(unknown_sizes);
	CORR_VALCPY(max_varchar_size);
	CORR_VALCPY(max_longvarchar_size);
	CORR_VALCPY(debug);
	CORR_VALCPY(commlog);
	CORR_VALCPY(unique_index);
	CORR_VALCPY(use_declarefetch);
	CORR_VALCPY(text_as_longvarchar);
	CORR_VALCPY(unknowns_as_longvarchar);
	CORR_VALCPY(bools_as_char);
	CORR_VALCPY(lie);
	CORR_VALCPY(parse);
	CORR_STRCPY(extra_systable_prefixes);
	CORR_STRCPY(protocol);

	mylog("copy_globals driver=%s\n", SAFE_NAME(to->drivername));
}

void	finalize_globals(GLOBAL_VALUES *glbv)
{
	NULL_THE_NAME(glbv->drivername);
}

#undef	CORR_STRCPY
#undef	CORR_VALCPY
#define	CORR_STRCPY(item)	strncpy_null(ci->item, sci->item, sizeof(ci->item))
#define	CORR_VALCPY(item)	(ci->item = sci->item)

void
CC_copy_conninfo(ConnInfo *ci, const ConnInfo *sci)
{
	memset(ci, 0,sizeof(ConnInfo));

	CORR_STRCPY(dsn);
	CORR_STRCPY(desc);
	CORR_STRCPY(drivername);
	CORR_STRCPY(server);
	CORR_STRCPY(database);
	CORR_STRCPY(username);
	NAME_TO_NAME(ci->password, sci->password);
	CORR_STRCPY(port);
	CORR_STRCPY(sslmode);
	CORR_STRCPY(onlyread);
	CORR_STRCPY(fake_oid_index);
	CORR_STRCPY(show_oid_column);
	CORR_STRCPY(row_versioning);
	CORR_STRCPY(show_system_tables);
	CORR_STRCPY(translation_dll);
	CORR_STRCPY(translation_option);
	CORR_VALCPY(password_required);
	NAME_TO_NAME(ci->conn_settings, sci->conn_settings);
	CORR_VALCPY(allow_keyset);
	CORR_VALCPY(updatable_cursors);
	CORR_VALCPY(lf_conversion);
	CORR_VALCPY(true_is_minus1);
	CORR_VALCPY(int8_as);
	CORR_VALCPY(bytea_as_longvarbinary);
	CORR_VALCPY(use_server_side_prepare);
	CORR_VALCPY(lower_case_identifier);
	CORR_VALCPY(rollback_on_error);
	CORR_VALCPY(force_abbrev_connstr);
	CORR_VALCPY(bde_environment);
	CORR_VALCPY(fake_mss);
	CORR_VALCPY(cvt_null_date_string);
	CORR_VALCPY(accessible_only);
	CORR_VALCPY(ignore_round_trip_time);
	CORR_VALCPY(disable_keepalive);
	CORR_VALCPY(gssauth_use_gssapi);
	CORR_VALCPY(extra_opts);
	CORR_VALCPY(keepalive_idle);
	CORR_VALCPY(keepalive_interval);
#ifdef	_HANDLE_ENLIST_IN_DTC_
	CORR_VALCPY(xa_opt);
#endif
	copy_globals(&(ci->drivers), &(sci->drivers));	/* moved from driver's option */
}
#undef	CORR_STRCPY
#undef	CORR_VALCPY
