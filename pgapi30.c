/*-------
 * Module:			pgapi30.c
 *
 * Description:		This module contains routines related to ODBC 3.0
 *			most of their implementations are temporary
 *			and must be rewritten properly.
 *			2001/07/23	inoue
 *
 * Classes:			n/a
 *
 * API functions:	PGAPI_ColAttribute, PGAPI_GetDiagRec,
			PGAPI_GetConnectAttr, PGAPI_GetStmtAttr,
			PGAPI_SetConnectAttr, PGAPI_SetStmtAttr
 *-------
 */

#include "psqlodbc.h"

#if (ODBCVER >= 0x0300)
#include <stdio.h>
#include <string.h>

#include "environ.h"
#include "connection.h"
#include "statement.h"
#include "descriptor.h"
#include "qresult.h"
#include "pgapifunc.h"

static HSTMT statementHandleFromDescHandle(SQLHDESC, SQLINTEGER *descType); 
/*	SQLError -> SQLDiagRec */
RETCODE		SQL_API
PGAPI_GetDiagRec(SQLSMALLINT HandleType, SQLHANDLE Handle,
		SQLSMALLINT RecNumber, SQLCHAR *Sqlstate,
		SQLINTEGER *NativeError, SQLCHAR *MessageText,
		SQLSMALLINT BufferLength, SQLSMALLINT *TextLength)
{
	RETCODE		ret;
	static const char *func = "PGAPI_GetDiagRec";

	mylog("%s entering rec=%d", func, RecNumber);
	switch (HandleType)
	{
		case SQL_HANDLE_ENV:
			ret = PGAPI_EnvError(Handle, RecNumber, Sqlstate,
					NativeError, MessageText,
					BufferLength, TextLength, 0);
			break;
		case SQL_HANDLE_DBC:
			ret = PGAPI_ConnectError(Handle, RecNumber, Sqlstate,
					NativeError, MessageText, BufferLength,
					TextLength, 0);
			break;
		case SQL_HANDLE_STMT:
			ret = PGAPI_StmtError(Handle, RecNumber, Sqlstate,
					NativeError, MessageText, BufferLength,
					TextLength, 0);
			break;
		case SQL_HANDLE_DESC:
			ret = PGAPI_StmtError(statementHandleFromDescHandle(Handle, NULL),
					RecNumber, Sqlstate, NativeError,
					MessageText, BufferLength,
					TextLength, 0);
			break;
		default:
			ret = SQL_ERROR;
	}
	mylog("%s exiting %d\n", func, ret);
	return ret;
}

/*
 *	Minimal implementation. 
 *
 */
RETCODE		SQL_API
PGAPI_GetDiagField(SQLSMALLINT HandleType, SQLHANDLE Handle,
		SQLSMALLINT RecNumber, SQLSMALLINT DiagIdentifier,
		PTR DiagInfoPtr, SQLSMALLINT BufferLength,
		SQLSMALLINT *StringLengthPtr)
{
	RETCODE		ret = SQL_ERROR, rtn;
	ConnectionClass	*conn;
	SQLHANDLE	stmtHandle;
	StatementClass	*stmt;
	SDWORD		rc;
	SWORD		pcbErrm;
	static const char *func = "PGAPI_GetDiagField";

	mylog("%s entering rec=%d", func, RecNumber);
	switch (HandleType)
	{
		case SQL_HANDLE_ENV:
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
				case SQL_DIAG_SERVER_NAME:
					strcpy((char *) DiagInfoPtr, "");
					if (StringLengthPtr)
						*StringLengthPtr = 0;
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_MESSAGE_TEXT:
					ret = PGAPI_EnvError(Handle, RecNumber,
                        			NULL, NULL, DiagInfoPtr,
						BufferLength, StringLengthPtr, 0);  
					break;
				case SQL_DIAG_NATIVE:
					ret = PGAPI_EnvError(Handle, RecNumber,
                        			NULL, DiagInfoPtr, NULL,
						0, NULL, 0);  
					if (StringLengthPtr)  
						*StringLengthPtr = sizeof(SQLINTEGER);  
					if (SQL_SUCCESS_WITH_INFO == ret)  
						ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_NUMBER:
					ret = PGAPI_EnvError(Handle, RecNumber,
                        			NULL, NULL, NULL,
						0, NULL, 0);
					if (SQL_SUCCESS == ret ||
					    SQL_SUCCESS_WITH_INFO == ret)
					{
						*((SQLINTEGER *) DiagInfoPtr) = 1;
						if (StringLengthPtr)
							*StringLengthPtr = sizeof(SQLINTEGER);
						ret = SQL_SUCCESS;
					}
					break;
				case SQL_DIAG_SQLSTATE:
					ret = PGAPI_EnvError(Handle, RecNumber,
                        			DiagInfoPtr, NULL, NULL,
						0, NULL, 0);
					if (StringLengthPtr)  
						*StringLengthPtr = 5;  
					if (SQL_SUCCESS_WITH_INFO == ret)  
						ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
				case SQL_DIAG_ROW_COUNT:
				case SQL_DIAG_DYNAMIC_FUNCTION:
				case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
					/* options for statement type only */
					break;
			}
			break;
		case SQL_HANDLE_DBC:
			conn = (ConnectionClass *) Handle;
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
					strcpy((char *) DiagInfoPtr, "");
					if (StringLengthPtr)
						*StringLengthPtr = 0;
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_SERVER_NAME:
					strcpy((SQLCHAR *) DiagInfoPtr, CC_get_DSN(conn));
					if (StringLengthPtr)
						*StringLengthPtr = strlen(CC_get_DSN(conn));
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_MESSAGE_TEXT:
					ret = PGAPI_ConnectError(Handle, RecNumber,
                        			NULL, NULL, DiagInfoPtr,
						BufferLength, StringLengthPtr, 0);  
					break;
				case SQL_DIAG_NATIVE:
					ret = PGAPI_ConnectError(Handle, RecNumber,
                        			NULL, DiagInfoPtr, NULL,
						0, NULL, 0);  
					if (StringLengthPtr)  
						*StringLengthPtr = sizeof(SQLINTEGER);  
					if (SQL_SUCCESS_WITH_INFO == ret)  
						ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_NUMBER:
					ret = PGAPI_ConnectError(Handle, RecNumber,
                        			NULL, NULL, NULL,
						0, NULL, 0);
					if (SQL_SUCCESS == ret ||
					    SQL_SUCCESS_WITH_INFO == ret)
					{
						*((SQLINTEGER *) DiagInfoPtr) = 1;
						if (StringLengthPtr)
							*StringLengthPtr = sizeof(SQLINTEGER);
						ret = SQL_SUCCESS;
					}
					break;  
				case SQL_DIAG_SQLSTATE:
					ret = PGAPI_ConnectError(Handle, RecNumber,
                        			DiagInfoPtr, NULL, NULL,
						0, NULL, 0);
					if (StringLengthPtr)  
						*StringLengthPtr = 5;  
					if (SQL_SUCCESS_WITH_INFO == ret)  
						ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
				case SQL_DIAG_ROW_COUNT:
				case SQL_DIAG_DYNAMIC_FUNCTION:
				case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
					/* options for statement type only */
					break;
			}
			break;
		case SQL_HANDLE_STMT:
			conn = (ConnectionClass *) SC_get_conn(((StatementClass *) Handle));
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
					strcpy((char *) DiagInfoPtr, "");
					if (StringLengthPtr)
						*StringLengthPtr = 0;
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_SERVER_NAME:
					strcpy((SQLCHAR *) DiagInfoPtr, CC_get_DSN(conn));
					if (StringLengthPtr)
						*StringLengthPtr = strlen(CC_get_DSN(conn));
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_MESSAGE_TEXT:
					ret = PGAPI_StmtError(Handle, RecNumber,
                        			NULL, NULL, DiagInfoPtr,
						BufferLength, StringLengthPtr, 0);  
					break;
				case SQL_DIAG_NATIVE:
					ret = PGAPI_StmtError(Handle, RecNumber,
                        			NULL, DiagInfoPtr, NULL,
						0, NULL, 0);  
					if (StringLengthPtr)  
						*StringLengthPtr = sizeof(SQLINTEGER);  
					if (SQL_SUCCESS_WITH_INFO == ret)  
						ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_NUMBER:
					*((SQLINTEGER *) DiagInfoPtr) = 0;
					ret = SQL_NO_DATA_FOUND;
					stmt = (StatementClass *) Handle;
					rtn = PGAPI_StmtError(Handle, -1, NULL,
						 NULL, NULL, 0, &pcbErrm, 0);
					switch (rtn)
					{
						case SQL_SUCCESS:
						case SQL_SUCCESS_WITH_INFO:
							ret = SQL_SUCCESS;
							if (pcbErrm > 0)
								*((SQLINTEGER *) DiagInfoPtr) = (pcbErrm  - 1)/ stmt->error_recsize + 1;
							break;
						default:
							break;
					}
					if (StringLengthPtr)
						*StringLengthPtr = sizeof(SQLINTEGER);
					break;
				case SQL_DIAG_SQLSTATE:
					ret = PGAPI_StmtError(Handle, RecNumber,
                        			DiagInfoPtr, NULL, NULL,
						0, NULL, 0);
					if (StringLengthPtr)  
						*StringLengthPtr = 5;
					if (SQL_SUCCESS_WITH_INFO == ret)  
						ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
					stmt = (StatementClass *) Handle;
					rc = -1;
					if (stmt->status == STMT_FINISHED)
					{
						QResultClass *res = SC_get_Curres(stmt);

						if (res && QR_NumResultCols(res) > 0 && !SC_is_fetchcursor(stmt))
							rc = QR_get_num_total_tuples(res) - res->dl_count;
					} 
					*((SQLINTEGER *) DiagInfoPtr) = rc;
					if (StringLengthPtr)
						*StringLengthPtr = sizeof(SQLINTEGER);
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_ROW_COUNT:
					stmt = (StatementClass *) Handle;
					*((SQLINTEGER *) DiagInfoPtr) = stmt->diag_row_count;
					if (StringLengthPtr)
						*StringLengthPtr = sizeof(SQLINTEGER);
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_ROW_NUMBER:
					*((SQLINTEGER *) DiagInfoPtr) = SQL_ROW_NUMBER_UNKNOWN;
					if (StringLengthPtr)
						*StringLengthPtr = sizeof(SQLINTEGER);
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_COLUMN_NUMBER:
					*((SQLINTEGER *) DiagInfoPtr) = SQL_COLUMN_NUMBER_UNKNOWN;
					if (StringLengthPtr)
						*StringLengthPtr = sizeof(SQLINTEGER);
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
			}
			break;
		case SQL_HANDLE_DESC:
			stmtHandle = statementHandleFromDescHandle(Handle, NULL); 
			conn = (ConnectionClass *) SC_get_conn(((StatementClass *) stmtHandle)); 
			switch (DiagIdentifier)
			{
				case SQL_DIAG_CLASS_ORIGIN:
				case SQL_DIAG_SUBCLASS_ORIGIN:
				case SQL_DIAG_CONNECTION_NAME:
					strcpy((char *) DiagInfoPtr, "");
					if (StringLengthPtr)
						*StringLengthPtr = 0;
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_SERVER_NAME:
					strcpy((SQLCHAR *) DiagInfoPtr, CC_get_DSN(conn));
					if (StringLengthPtr)
						*StringLengthPtr = strlen(CC_get_DSN(conn));
					ret = SQL_SUCCESS;
					break;
				case SQL_DIAG_MESSAGE_TEXT:
				case SQL_DIAG_NATIVE:
				case SQL_DIAG_NUMBER:
				case SQL_DIAG_SQLSTATE:
					ret = PGAPI_GetDiagField(SQL_HANDLE_STMT,
						stmtHandle, RecNumber,
						DiagIdentifier, DiagInfoPtr,
						BufferLength, StringLengthPtr);
					break;
				case SQL_DIAG_RETURNCODE: /* driver manager returns */
					break;
				case SQL_DIAG_CURSOR_ROW_COUNT:
				case SQL_DIAG_ROW_COUNT:
				case SQL_DIAG_DYNAMIC_FUNCTION:
				case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
					/* options for statement type only */
					break;
			}
			break;
		default:
			ret = SQL_ERROR;
	}
	mylog("%s exiting %d\n", func, ret);
	return ret;
}

/*	SQLGetConnectOption -> SQLGetconnectAttr */
RETCODE		SQL_API
PGAPI_GetConnectAttr(HDBC ConnectionHandle,
			SQLINTEGER Attribute, PTR Value,
			SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
	static const char *func = "PGAPI_GetConnectAttr";
	ConnectionClass *conn = (ConnectionClass *) ConnectionHandle;
	RETCODE	ret = SQL_SUCCESS;
	SQLINTEGER	len = 4;

	mylog("PGAPI_GetConnectAttr %d\n", Attribute);
	switch (Attribute)
	{
		case SQL_ATTR_ASYNC_ENABLE:
			*((SQLUINTEGER *) Value) = SQL_ASYNC_ENABLE_OFF;
			break;
		case SQL_ATTR_AUTO_IPD:
			*((SQLUINTEGER *) Value) = SQL_FALSE;
			break;
		case SQL_ATTR_CONNECTION_DEAD:
			*((SQLUINTEGER *) Value) = (conn->status == CONN_NOT_CONNECTED || conn->status == CONN_DOWN);
			break;
		case SQL_ATTR_CONNECTION_TIMEOUT:
			*((SQLUINTEGER *) Value) = 0;
			break;
		case SQL_ATTR_METADATA_ID:
			CC_set_error(conn, STMT_INVALID_OPTION_IDENTIFIER, "Unsupported connect attribute (Get)");
			CC_log_error(func, "", conn);
			return SQL_ERROR;
		default:
			ret = PGAPI_GetConnectOption(ConnectionHandle, (UWORD) Attribute, Value);
	}
	if (StringLength)
		*StringLength = len;
	return ret;
}

static SQLHDESC
descHandleFromStatementHandle(HSTMT StatementHandle, SQLINTEGER descType) 
{
	switch (descType)
	{
		case SQL_ATTR_APP_ROW_DESC:		/* 10010 */
			return StatementHandle;	/* this is bogus */
		case SQL_ATTR_APP_PARAM_DESC:	/* 10011 */
			return (HSTMT) ((SQLUINTEGER) StatementHandle + 1) ; /* this is bogus */
		case SQL_ATTR_IMP_ROW_DESC:		/* 10012 */
			return (HSTMT) ((SQLUINTEGER) StatementHandle + 2); /* this is bogus */
		case SQL_ATTR_IMP_PARAM_DESC:	/* 10013 */
			return (HSTMT) ((SQLUINTEGER) StatementHandle + 3); /* this is bogus */
	}
	return (HSTMT) 0;
}
static HSTMT
statementHandleFromDescHandle(SQLHDESC DescHandle, SQLINTEGER *descType) 
{
	SQLUINTEGER res = (SQLUINTEGER) DescHandle % 4;
	if (descType)
	{
		switch (res)
		{
			case 0: *descType = SQL_ATTR_APP_ROW_DESC; /* 10010 */
				break;
			case 1: *descType = SQL_ATTR_APP_PARAM_DESC; /* 10011 */
				break;
			case 2: *descType = SQL_ATTR_IMP_ROW_DESC; /* 10012 */
				break;
			case 3: *descType = SQL_ATTR_IMP_PARAM_DESC; /* 10013 */
				break;
		}
	}
	return (HSTMT) ((SQLUINTEGER) DescHandle - res);
}

void	Desc_set_error(SQLHDESC hdesc, int errornumber, const char *errormsg)
{
	SQLINTEGER	descType;
	HSTMT	hstmt = statementHandleFromDescHandle(hdesc, &descType);
	StatementClass	*stmt;

	if (!hstmt)
		return;
	stmt = (StatementClass *) hstmt;
	SC_set_error(stmt, errornumber, errormsg);
}

static  void column_bindings_set(ARDFields *opts, int cols, BOOL maxset)
{
	int	i;

	if (cols == opts->allocated)
		return;
	if (cols > opts->allocated)
	{
		extend_column_bindings(opts, cols);
		return;
	}
	if (maxset)	return;

	for (i = opts->allocated; i > cols; i--)
		reset_a_column_binding(opts, i);
	opts->allocated = cols;
	if (0 == cols)
	{
		free(opts->bindings);
		opts->bindings = NULL;
	}
}

static RETCODE SQL_API
ARDSetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	PTR		tptr;
	ARDFields	*opts = SC_get_ARD(stmt);
	SQLSMALLINT	row_idx;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			opts->rowset_size = (SQLUINTEGER) Value;
			return ret; 
		case SQL_DESC_ARRAY_STATUS_PTR:
			opts->row_operation_ptr = Value;
			return ret;
		case SQL_DESC_BIND_OFFSET_PTR:
			opts->row_offset_ptr = Value;
			return ret;
		case SQL_DESC_BIND_TYPE:
			opts->bind_size = (SQLUINTEGER) Value;
			return ret;
		case SQL_DESC_COUNT:
			column_bindings_set(opts, (SQLUINTEGER) Value, FALSE);
			return ret;

		case SQL_DESC_TYPE:
		case SQL_DESC_DATETIME_INTERVAL_CODE:
		case SQL_DESC_CONCISE_TYPE:
			column_bindings_set(opts, RecNumber, TRUE);
			break;
	}
	if (RecNumber < 0 || RecNumber > opts->allocated)
	{
		SC_set_errornumber(stmt, STMT_INVALID_COLUMN_NUMBER_ERROR);
		return SQL_ERROR;
	}
	if (0 == RecNumber) /* bookmark column */
	{
		switch (FieldIdentifier)
		{
			case SQL_DESC_DATA_PTR:
				opts->bookmark->buffer = Value;
				break;
			case SQL_DESC_INDICATOR_PTR:
				tptr = opts->bookmark->used;
				if (Value != tptr)
				{
					SC_set_error(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER, "INDICATOR != OCTET_LENGTH_PTR"); 
					ret = SQL_ERROR;
				}
				break;
			case SQL_DESC_OCTET_LENGTH_PTR:
				opts->bookmark->used = Value;
				break;
			default:
				SC_set_errornumber(stmt, STMT_INVALID_COLUMN_NUMBER_ERROR);
				ret = SQL_ERROR;
		}
		return ret;
	}
	row_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_TYPE:
			reset_a_column_binding(opts, RecNumber);
			opts->bindings[row_idx].returntype = (Int4) Value;
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (opts->bindings[row_idx].returntype)
			{
				case SQL_DATETIME:
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
				switch ((Int4) Value)
				{
					case SQL_CODE_DATE:
						opts->bindings[row_idx].returntype = SQL_C_TYPE_DATE;
						break;
					case SQL_CODE_TIME:
						opts->bindings[row_idx].returntype = SQL_C_TYPE_TIME;
						break;
					case SQL_CODE_TIMESTAMP:
						opts->bindings[row_idx].returntype = SQL_C_TYPE_TIMESTAMP;
						break;
				}
				break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			opts->bindings[row_idx].returntype = (Int4) Value;
			break;
		case SQL_DESC_DATA_PTR:
			opts->bindings[row_idx].buffer = Value;
			break;
		case SQL_DESC_INDICATOR_PTR:
			tptr = opts->bindings[row_idx].used;
			if (Value != tptr)
			{
				ret = SQL_ERROR;
				SC_set_error(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER, "INDICATOR != OCTET_LENGTH_PTR"); 
			}
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			opts->bindings[row_idx].used = Value;
			break;
		case SQL_DESC_OCTET_LENGTH:
			opts->bindings[row_idx].buflen = (Int4) Value;
			break;
		case SQL_DESC_PRECISION:
			opts->bindings[row_idx].precision = (Int2)((Int4) Value);
			break;
		case SQL_DESC_SCALE:
			opts->bindings[row_idx].scale = (Int4) Value;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		case SQL_DESC_NUM_PREC_RADIX:
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	return ret;
}

static  void parameter_bindings_set(APDFields *opts, int params, BOOL maxset)
{
	int	i;

	if (params == opts->allocated)
		return;
	if (params > opts->allocated)
	{
		extend_parameter_bindings(opts, params);
		return;
	}
	if (maxset)	return;

	for (i = opts->allocated; i > params; i--)
		reset_a_parameter_binding(opts, i);
	opts->allocated = params;
	if (0 == params)
	{
		free(opts->parameters);
		opts->parameters = NULL;
	}
}

static  void parameter_ibindings_set(IPDFields *opts, int params, BOOL maxset)
{
	int	i;

	if (params == opts->allocated)
		return;
	if (params > opts->allocated)
	{
		extend_iparameter_bindings(opts, params);
		return;
	}
	if (maxset)	return;

	for (i = opts->allocated; i > params; i--)
		reset_a_iparameter_binding(opts, i);
	opts->allocated = params;
	if (0 == params)
	{
		free(opts->parameters);
		opts->parameters = NULL;
	}
}

static RETCODE SQL_API
APDSetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	APDFields	*opts = SC_get_APD(stmt);
	SQLSMALLINT	para_idx;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			opts->paramset_size = (SQLUINTEGER) Value;
			return ret; 
		case SQL_DESC_ARRAY_STATUS_PTR:
			opts->param_operation_ptr = Value;
			return ret;
		case SQL_DESC_BIND_OFFSET_PTR:
			opts->param_offset_ptr = Value;
			return ret;
		case SQL_DESC_BIND_TYPE:
			opts->param_bind_type = (SQLUINTEGER) Value;
			return ret;
		case SQL_DESC_COUNT:
			parameter_bindings_set(opts, (SQLUINTEGER) Value, FALSE);
			return ret; 

		case SQL_DESC_TYPE:
		case SQL_DESC_DATETIME_INTERVAL_CODE:
		case SQL_DESC_CONCISE_TYPE:
			parameter_bindings_set(opts, RecNumber, TRUE);
			break;
	}
	if (RecNumber <=0 || RecNumber > opts->allocated)
	{
		SC_set_errornumber(stmt, STMT_BAD_PARAMETER_NUMBER_ERROR);
		return SQL_ERROR;
	}
	para_idx = RecNumber - 1; 
	switch (FieldIdentifier)
	{
		case SQL_DESC_TYPE:
			reset_a_parameter_binding(opts, RecNumber);
			opts->parameters[para_idx].CType = (Int4) Value;
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (opts->parameters[para_idx].CType)
			{
				case SQL_DATETIME:
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
				switch ((Int4) Value)
				{
					case SQL_CODE_DATE:
						opts->parameters[para_idx].CType = SQL_C_TYPE_DATE;
						break;
					case SQL_CODE_TIME:
						opts->parameters[para_idx].CType = SQL_C_TYPE_TIME;
						break;
					case SQL_CODE_TIMESTAMP:
						opts->parameters[para_idx].CType = SQL_C_TYPE_TIMESTAMP;
						break;
				}
				break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			opts->parameters[para_idx].CType = (Int4) Value;
			break;
		case SQL_DESC_DATA_PTR:
			opts->parameters[para_idx].buffer = Value;
			break;
		case SQL_DESC_INDICATOR_PTR:
			if (Value != opts->parameters[para_idx].used)
			{
				ret = SQL_ERROR;
				SC_set_error(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER, "INDICATOR != OCTET_LENGTH_PTR"); 
			}
			break;
		case SQL_DESC_OCTET_LENGTH:
			opts->parameters[para_idx].buflen = (Int4) Value;
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			opts->parameters[para_idx].used = Value;
			break;
		case SQL_DESC_PRECISION:
			opts->parameters[para_idx].precision = (Int2) ((Int4) Value);
			break;
		case SQL_DESC_SCALE:
			opts->parameters[para_idx].scale = (Int2) ((Int4) Value);
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		case SQL_DESC_NUM_PREC_RADIX:
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	return ret;
}

static RETCODE SQL_API
IRDSetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	IRDFields	*opts = SC_get_IRD(stmt);

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			opts->rowStatusArray = (SQLUSMALLINT *) Value;
			break;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			opts->rowsFetched = (SQLUINTEGER *) Value;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
		case SQL_DESC_COUNT: /* read-only */
		case SQL_DESC_AUTO_UNIQUE_VALUE: /* read-only */
		case SQL_DESC_BASE_COLUMN_NAME: /* read-only */
		case SQL_DESC_BASE_TABLE_NAME: /* read-only */
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_CATALOG_NAME: /* read-only */
		case SQL_DESC_CONCISE_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_CODE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION: /* read-only */
		case SQL_DESC_DISPLAY_SIZE: /* read-only */
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LABEL: /* read-only */
		case SQL_DESC_LENGTH: /* read-only */
		case SQL_DESC_LITERAL_PREFIX: /* read-only */
		case SQL_DESC_LITERAL_SUFFIX: /* read-only */
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NAME: /* read-only */
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX: /* read-only */
		case SQL_DESC_OCTET_LENGTH: /* read-only */
		case SQL_DESC_PRECISION: /* read-only */
#if (ODBCVER >= 0x0350)
		case SQL_DESC_ROWVER: /* read-only */
#endif /* ODBCVER */
		case SQL_DESC_SCALE: /* read-only */
		case SQL_DESC_SCHEMA_NAME: /* read-only */
		case SQL_DESC_SEARCHABLE: /* read-only */
		case SQL_DESC_TABLE_NAME: /* read-only */
		case SQL_DESC_TYPE: /* read-only */
		case SQL_DESC_TYPE_NAME: /* read-only */
		case SQL_DESC_UNNAMED: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		case SQL_DESC_UPDATABLE: /* read-only */
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	return ret;
}

static RETCODE SQL_API
IPDSetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	IPDFields	*ipdopts = SC_get_IPD(stmt);
	SQLSMALLINT	para_idx;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			ipdopts->param_status_ptr = (SQLUSMALLINT *) Value;
			return ret;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			ipdopts->param_processed_ptr = (SQLUINTEGER *) Value;
			return ret;
		case SQL_DESC_UNNAMED: /* only SQL_UNNAMED is allowed */ 
			if (SQL_UNNAMED !=  (SQLUINTEGER) Value)
			{
				ret = SQL_ERROR;
				SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER);
			}
			return ret;
		case SQL_DESC_COUNT:
			parameter_ibindings_set(ipdopts, (SQLUINTEGER) Value, FALSE);
			return ret;
		case SQL_DESC_TYPE:
		case SQL_DESC_DATETIME_INTERVAL_CODE:
		case SQL_DESC_CONCISE_TYPE:
			parameter_ibindings_set(ipdopts, RecNumber, TRUE);
			break;
	}
	if (RecNumber <= 0 || RecNumber > ipdopts->allocated)
	{
		SC_set_errornumber(stmt, STMT_BAD_PARAMETER_NUMBER_ERROR);
		return SQL_ERROR;
	}
	para_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_TYPE:
			reset_a_iparameter_binding(ipdopts, RecNumber);
			ipdopts->parameters[para_idx].SQLType = (Int4) Value;
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_DATETIME:
				case SQL_TYPE_DATE:
				case SQL_TYPE_TIME:
				case SQL_TYPE_TIMESTAMP:
				switch ((Int4) Value)
				{
					case SQL_CODE_DATE:
						ipdopts->parameters[para_idx].SQLType = SQL_TYPE_DATE;
						break;
					case SQL_CODE_TIME:
						ipdopts->parameters[para_idx].SQLType = SQL_TYPE_TIME;
						break;
					case SQL_CODE_TIMESTAMP:
						ipdopts->parameters[para_idx].SQLType = SQL_TYPE_TIMESTAMP;
						break;
				}
				break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			ipdopts->parameters[para_idx].SQLType = (Int4) Value;
			break;
		case SQL_DESC_PARAMETER_TYPE:
			ipdopts->parameters[para_idx].paramType = (Int2) ((Int4) Value);
			break;
		case SQL_DESC_SCALE:
			ipdopts->parameters[para_idx].decimal_digits = (Int2) ((Int4) Value);
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */ 
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LENGTH:
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NAME:
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX:
		case SQL_DESC_OCTET_LENGTH:
		case SQL_DESC_PRECISION:
#if (ODBCVER >= 0x0350)
		case SQL_DESC_ROWVER: /* read-only */
#endif /* ODBCVER */
		case SQL_DESC_TYPE_NAME: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	return ret;
}


static RETCODE SQL_API
ARDGetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
		SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLINTEGER	len, ival, rettype = 0;
	PTR		ptr = NULL;
	const ARDFields	*opts = SC_get_ARD(stmt);
	SQLSMALLINT	row_idx;

	len = 4;
	if (0 == RecNumber) /* bookmark */
	{
		switch (FieldIdentifier)
		{
			case SQL_DESC_DATA_PTR:
				rettype = SQL_IS_POINTER;
				ptr = opts->bookmark->buffer;
				break;
			case SQL_DESC_INDICATOR_PTR:
				rettype = SQL_IS_POINTER;
				ptr = opts->bookmark->used;
				break;
			case SQL_DESC_OCTET_LENGTH_PTR:
				rettype = SQL_IS_POINTER;
				ptr = opts->bookmark->used;
				break;
		}
		if (ptr)
		{
			*((void **) Value) = ptr;
			if (StringLength)
				*StringLength = len;
			return ret;
		}
	}
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
		case SQL_DESC_ARRAY_STATUS_PTR:
		case SQL_DESC_BIND_OFFSET_PTR:
		case SQL_DESC_BIND_TYPE:
		case SQL_DESC_COUNT:
			break;
		default:
			if (RecNumber <= 0 || RecNumber > opts->allocated)
			{
				SC_set_errornumber(stmt, STMT_INVALID_COLUMN_NUMBER_ERROR);
				return SQL_ERROR;
			}
	}
	row_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			ival = opts->rowset_size;
			break; 
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->row_operation_ptr;
			break;
		case SQL_DESC_BIND_OFFSET_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->row_offset_ptr;
			break;
		case SQL_DESC_BIND_TYPE:
			ival = opts->bind_size;
			break;
		case SQL_DESC_TYPE:
			switch (opts->bindings[row_idx].returntype)
			{
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_DATETIME;
					break;
				default:
					ival = opts->bindings[row_idx].returntype;
			}
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (opts->bindings[row_idx].returntype)
			{
				case SQL_C_TYPE_DATE:
					ival = SQL_CODE_DATE;
					break;
				case SQL_C_TYPE_TIME:
					ival = SQL_CODE_TIME;
					break;
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_CODE_TIMESTAMP;
					break;
				default:
					ival = 0;
					break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			ival = opts->bindings[row_idx].returntype;
			break;
		case SQL_DESC_DATA_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->bindings[row_idx].buffer;
			break;
		case SQL_DESC_INDICATOR_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->bindings[row_idx].used;
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->bindings[row_idx].used;
			break;
		case SQL_DESC_COUNT:
			ival = opts->allocated;
			break;
		case SQL_DESC_OCTET_LENGTH:
			ival = opts->bindings[row_idx].buflen;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			ival = SQL_DESC_ALLOC_AUTO;
			break;
		case SQL_DESC_PRECISION:
			ival = opts->bindings[row_idx].precision;
			break;
		case SQL_DESC_SCALE:
			ival = opts->bindings[row_idx].scale;
			break;
		case SQL_DESC_NUM_PREC_RADIX:
			ival = 10;
			break;
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	switch (rettype)
	{
		case 0:
		case SQL_IS_INTEGER:
			len = 4;
			*((SQLINTEGER *) Value) = ival;
			break;
		case SQL_IS_POINTER:
			len = 4;
			*((void **) Value) = ptr;
			break;
	}
			
	if (StringLength)
		*StringLength = len;
	return ret;
}

static RETCODE SQL_API
APDGetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
		SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLINTEGER	ival = 0, len, rettype = 0;
	PTR		ptr = NULL;
	const APDFields	*opts = SC_get_APD(stmt);
	SQLSMALLINT	para_idx;

	len = 4;
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
		case SQL_DESC_ARRAY_STATUS_PTR:
		case SQL_DESC_BIND_OFFSET_PTR:
		case SQL_DESC_BIND_TYPE:
		case SQL_DESC_COUNT:
			break; 
		default:if (RecNumber <= 0 || RecNumber > opts->allocated)
			{
				SC_set_errornumber(stmt, STMT_BAD_PARAMETER_NUMBER_ERROR);
				return SQL_ERROR;
			} 
	}
	para_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_SIZE:
			rettype = SQL_IS_POINTER;
			ival = opts->paramset_size;
			break; 
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->param_operation_ptr;
			break;
		case SQL_DESC_BIND_OFFSET_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->param_offset_ptr;
			break;
		case SQL_DESC_BIND_TYPE:
			ival = opts->param_bind_type;
			break;

		case SQL_DESC_TYPE:
			switch (opts->parameters[para_idx].CType)
			{
				case SQL_C_TYPE_DATE:
				case SQL_C_TYPE_TIME:
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_DATETIME;
					break;
				default:
					ival = opts->parameters[para_idx].CType;
			}
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (opts->parameters[para_idx].CType)
			{
				case SQL_C_TYPE_DATE:
					ival = SQL_CODE_DATE;
					break;
				case SQL_C_TYPE_TIME:
					ival = SQL_CODE_TIME;
					break;
				case SQL_C_TYPE_TIMESTAMP:
					ival = SQL_CODE_TIMESTAMP;
					break;
				default:
					ival = 0;
					break;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			ival = opts->parameters[para_idx].CType;
			break;
		case SQL_DESC_DATA_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->parameters[para_idx].buffer;
			break;
		case SQL_DESC_INDICATOR_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->parameters[para_idx].used;
			break;
		case SQL_DESC_OCTET_LENGTH:
			ival = opts->parameters[para_idx].buflen;
			break;
		case SQL_DESC_OCTET_LENGTH_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->parameters[para_idx].used;
			break;
		case SQL_DESC_COUNT:
			ival = opts->allocated;
			break; 
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			ival = SQL_DESC_ALLOC_AUTO;
			break;
		case SQL_DESC_NUM_PREC_RADIX:
			ival = 10;
			break;
		case SQL_DESC_PRECISION:
			ival = opts->parameters[para_idx].precision;
			break;
		case SQL_DESC_SCALE:
			ival = opts->parameters[para_idx].scale;
			break;
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_LENGTH:
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	switch (rettype)
	{
		case 0:
		case SQL_IS_INTEGER:
			len = 4;
			*((Int4 *) Value) = ival;
			break;
		case SQL_IS_POINTER:
			len = 4;
			*((void **) Value) = ptr;
			break;
	}
			
	if (StringLength)
		*StringLength = len;
	return ret;
}

static RETCODE SQL_API
IRDGetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
		SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLINTEGER	ival = 0, len, rettype = 0;
	PTR		ptr = NULL;
	BOOL		bCallColAtt = FALSE;
	const IRDFields	*opts = SC_get_IRD(stmt);

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->rowStatusArray;
			break;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			rettype = SQL_IS_POINTER;
			ptr = opts->rowsFetched;
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			ival = SQL_DESC_ALLOC_AUTO;
			break;
		case SQL_DESC_COUNT: /* read-only */
		case SQL_DESC_AUTO_UNIQUE_VALUE: /* read-only */
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_CONCISE_TYPE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_CODE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION: /* read-only */
		case SQL_DESC_DISPLAY_SIZE: /* read-only */
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LENGTH: /* read-only */
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX: /* read-only */
		case SQL_DESC_OCTET_LENGTH: /* read-only */
		case SQL_DESC_PRECISION: /* read-only */
#if (ODBCVER >= 0x0350)
		case SQL_DESC_ROWVER: /* read-only */
#endif /* ODBCVER */
		case SQL_DESC_SCALE: /* read-only */
		case SQL_DESC_SEARCHABLE: /* read-only */
		case SQL_DESC_TYPE: /* read-only */
		case SQL_DESC_UNNAMED: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		case SQL_DESC_UPDATABLE: /* read-only */
			bCallColAtt = TRUE;
			break;
		case SQL_DESC_BASE_COLUMN_NAME: /* read-only */
		case SQL_DESC_BASE_TABLE_NAME: /* read-only */
		case SQL_DESC_CATALOG_NAME: /* read-only */
		case SQL_DESC_LABEL: /* read-only */
		case SQL_DESC_LITERAL_PREFIX: /* read-only */
		case SQL_DESC_LITERAL_SUFFIX: /* read-only */
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NAME: /* read-only */
		case SQL_DESC_SCHEMA_NAME: /* read-only */
		case SQL_DESC_TABLE_NAME: /* read-only */
		case SQL_DESC_TYPE_NAME: /* read-only */
			rettype = SQL_NTS;
			bCallColAtt = TRUE;
			break; 
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	if (bCallColAtt)
	{
		SQLSMALLINT	pcbL;

		ret = PGAPI_ColAttributes(stmt, RecNumber,
			FieldIdentifier, Value, (SQLSMALLINT) BufferLength,
				&pcbL, &ival);
		len = pcbL;
	} 
	switch (rettype)
	{
		case 0:
		case SQL_IS_INTEGER:
			len = 4;
			*((Int4 *) Value) = ival;
			break;
		case SQL_IS_UINTEGER:
			len = 4;
			*((UInt4 *) Value) = ival;
			break;
		case SQL_IS_POINTER:
			len = 4;
			*((void **) Value) = ptr;
			break;
	}
			
	if (StringLength)
		*StringLength = len;
	return ret;
}

static RETCODE SQL_API
IPDGetField(StatementClass *stmt, SQLSMALLINT RecNumber,
		SQLSMALLINT FieldIdentifier, PTR Value, SQLINTEGER BufferLength,
		SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	SQLINTEGER	ival = 0, len, rettype = 0;
	PTR		ptr = NULL;
	const IPDFields	*ipdopts = SC_get_IPD(stmt);
	SQLSMALLINT	para_idx;

	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
		case SQL_DESC_ROWS_PROCESSED_PTR:
		case SQL_DESC_COUNT:
			break; 
		default:if (RecNumber <= 0 || RecNumber > ipdopts->allocated)
			{
				SC_set_errornumber(stmt, STMT_BAD_PARAMETER_NUMBER_ERROR);
				return SQL_ERROR;
			}
	}
	para_idx = RecNumber - 1;
	switch (FieldIdentifier)
	{
		case SQL_DESC_ARRAY_STATUS_PTR:
			rettype = SQL_IS_POINTER;
			ptr = ipdopts->param_status_ptr;
			break;
		case SQL_DESC_ROWS_PROCESSED_PTR:
			rettype = SQL_IS_POINTER;
			ptr = ipdopts->param_processed_ptr;
			break;
		case SQL_DESC_UNNAMED: /* only SQL_UNNAMED is allowed */
			ival = SQL_UNNAMED;
			break;
		case SQL_DESC_TYPE:
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_TYPE_DATE:
				case SQL_TYPE_TIME:
				case SQL_TYPE_TIMESTAMP:
					ival = SQL_DATETIME;
					break;
				default:
					ival = ipdopts->parameters[para_idx].SQLType;
			}
			break;
		case SQL_DESC_DATETIME_INTERVAL_CODE:
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_TYPE_DATE:
					ival = SQL_CODE_DATE;
				case SQL_TYPE_TIME:
					ival = SQL_CODE_TIME;
					break;
				case SQL_TYPE_TIMESTAMP:
					ival = SQL_CODE_TIMESTAMP;
					break;
				default:
					ival = 0;
			}
			break;
		case SQL_DESC_CONCISE_TYPE:
			ival = ipdopts->parameters[para_idx].SQLType;
			break;
		case SQL_DESC_COUNT:
			ival = ipdopts->allocated;
			break; 
		case SQL_DESC_PARAMETER_TYPE:
			ival = ipdopts->parameters[para_idx].paramType;
			break;
		case SQL_DESC_PRECISION:
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_TYPE_DATE:
				case SQL_TYPE_TIME:
				case SQL_TYPE_TIMESTAMP:
				case SQL_DATETIME:
					ival = ipdopts->parameters[para_idx].decimal_digits;
					break;
			}
			break;
		case SQL_DESC_SCALE:
			switch (ipdopts->parameters[para_idx].SQLType)
			{
				case SQL_NUMERIC:
					ival = ipdopts->parameters[para_idx].decimal_digits;
					break;
			}
			break;
		case SQL_DESC_ALLOC_TYPE: /* read-only */
			ival = SQL_DESC_ALLOC_AUTO;
			break; 
		case SQL_DESC_CASE_SENSITIVE: /* read-only */
		case SQL_DESC_DATETIME_INTERVAL_PRECISION:
		case SQL_DESC_FIXED_PREC_SCALE: /* read-only */
		case SQL_DESC_LENGTH:
		case SQL_DESC_LOCAL_TYPE_NAME: /* read-only */
		case SQL_DESC_NAME:
		case SQL_DESC_NULLABLE: /* read-only */
		case SQL_DESC_NUM_PREC_RADIX:
		case SQL_DESC_OCTET_LENGTH:
#if (ODBCVER >= 0x0350)
		case SQL_DESC_ROWVER: /* read-only */
#endif /* ODBCVER */
		case SQL_DESC_TYPE_NAME: /* read-only */
		case SQL_DESC_UNSIGNED: /* read-only */
		default:ret = SQL_ERROR;
			SC_set_errornumber(stmt, STMT_INVALID_DESCRIPTOR_IDENTIFIER); 
	}
	switch (rettype)
	{
		case 0:
		case SQL_IS_INTEGER:
			len = 4;
			*((Int4 *) Value) = ival;
			break;
		case SQL_IS_POINTER:
			len = 4;
			*((void **)Value) = ptr;
			break;
	}
			
	if (StringLength)
		*StringLength = len;
	return ret;
}

/*	SQLGetStmtOption -> SQLGetStmtAttr */
RETCODE		SQL_API
PGAPI_GetStmtAttr(HSTMT StatementHandle,
		SQLINTEGER Attribute, PTR Value,
		SQLINTEGER BufferLength, SQLINTEGER *StringLength)
{
	static char *func = "PGAPI_GetStmtAttr";
	StatementClass *stmt = (StatementClass *) StatementHandle;
	RETCODE		ret = SQL_SUCCESS;
	int			len = 0;

	mylog("%s Handle=%u %d\n", func, StatementHandle, Attribute);
	switch (Attribute)
	{
		case SQL_ATTR_FETCH_BOOKMARK_PTR:		/* 16 */
			*((void **) Value) = stmt->options.bookmark_ptr;
			len = 4;
			break;
		case SQL_ATTR_PARAM_BIND_OFFSET_PTR:	/* 17 */
			*((SQLUINTEGER **) Value) = SC_get_APD(stmt)->param_offset_ptr;
			len = 4;
			break;
		case SQL_ATTR_PARAM_BIND_TYPE:	/* 18 */
			*((SQLUINTEGER *) Value) = SC_get_APD(stmt)->param_bind_type;
			len = 4;
			break;
		case SQL_ATTR_PARAM_OPERATION_PTR:		/* 19 */
			*((SQLUSMALLINT **) Value) = SC_get_APD(stmt)->param_operation_ptr;
			len = 4;
			break;
		case SQL_ATTR_PARAM_STATUS_PTR: /* 20 */
			*((SQLUSMALLINT **) Value) = SC_get_IPD(stmt)->param_status_ptr;
			len = 4;
			break;
		case SQL_ATTR_PARAMS_PROCESSED_PTR:		/* 21 */
			*((SQLUINTEGER **) Value) = SC_get_IPD(stmt)->param_processed_ptr;
			len = 4;
			break;
		case SQL_ATTR_PARAMSET_SIZE:	/* 22 */
			*((SQLUINTEGER *) Value) = SC_get_APD(stmt)->paramset_size;
			len = 4;
			break;
		case SQL_ATTR_ROW_BIND_OFFSET_PTR:		/* 23 */
			*((SQLUINTEGER **) Value) = SC_get_ARD(stmt)->row_offset_ptr;
			len = 4;
			break;
		case SQL_ATTR_ROW_OPERATION_PTR:		/* 24 */
			*((SQLUSMALLINT **) Value) = SC_get_ARD(stmt)->row_operation_ptr;
			len = 4;
			break;
		case SQL_ATTR_ROW_STATUS_PTR:	/* 25 */
			*((SQLUSMALLINT **) Value) = SC_get_IRD(stmt)->rowStatusArray;
			len = 4;
			break;
		case SQL_ATTR_ROWS_FETCHED_PTR: /* 26 */
			*((SQLUINTEGER **) Value) = SC_get_IRD(stmt)->rowsFetched;
			len = 4;
			break;
		case SQL_ATTR_ROW_ARRAY_SIZE:	/* 27 */
			*((SQLUINTEGER *) Value) = SC_get_ARD(stmt)->rowset_size;
			len = 4;
			break;
		case SQL_ATTR_APP_ROW_DESC:		/* 10010 */
		case SQL_ATTR_APP_PARAM_DESC:	/* 10011 */
		case SQL_ATTR_IMP_ROW_DESC:		/* 10012 */
		case SQL_ATTR_IMP_PARAM_DESC:	/* 10013 */
			len = 4;
			*((HSTMT *) Value) = descHandleFromStatementHandle(StatementHandle, Attribute); 
			break;

		case SQL_ATTR_CURSOR_SCROLLABLE:		/* -1 */
			len = 4;
			if (SQL_CURSOR_FORWARD_ONLY == stmt->options.cursor_type)
				*((SQLUINTEGER *) Value) = SQL_NONSCROLLABLE;
			else
				*((SQLUINTEGER *) Value) = SQL_SCROLLABLE;
			break;
		case SQL_ATTR_CURSOR_SENSITIVITY:		/* -2 */
			len = 4;
			if (SQL_CONCUR_READ_ONLY == stmt->options.scroll_concurrency)
				*((SQLUINTEGER *) Value) = SQL_INSENSITIVE;
			else
				*((SQLUINTEGER *) Value) = SQL_UNSPECIFIED;
			break;
		case SQL_ATTR_AUTO_IPD:	/* 10001 */
			/* case SQL_ATTR_ROW_BIND_TYPE: ** == SQL_BIND_TYPE(ODBC2.0) */
		case SQL_ATTR_ENABLE_AUTO_IPD:	/* 15 */
		case SQL_ATTR_METADATA_ID:		/* 10014 */

			/*
			 * case SQL_ATTR_PREDICATE_PTR: case
			 * SQL_ATTR_PREDICATE_OCTET_LENGTH_PTR:
			 */
			SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "Unsupported statement option (Get)");
			SC_log_error(func, "", stmt);
			return SQL_ERROR;
		default:
			len = 4;
			ret = PGAPI_GetStmtOption(StatementHandle, (UWORD) Attribute, Value);
	}
	if (ret == SQL_SUCCESS && StringLength)
		*StringLength = len;
	return ret;
}

/*	SQLSetConnectOption -> SQLSetConnectAttr */
RETCODE		SQL_API
PGAPI_SetConnectAttr(HDBC ConnectionHandle,
			SQLINTEGER Attribute, PTR Value,
			SQLINTEGER StringLength)
{
	ConnectionClass *conn = (ConnectionClass *) ConnectionHandle;
	RETCODE	ret = SQL_SUCCESS;

	mylog("PGAPI_SetConnectAttr %d\n", Attribute);
	switch (Attribute)
	{
		case SQL_ATTR_ASYNC_ENABLE:
		case SQL_ATTR_AUTO_IPD:
		case SQL_ATTR_CONNECTION_DEAD:
		case SQL_ATTR_CONNECTION_TIMEOUT:
		case SQL_ATTR_METADATA_ID:
		case SQL_ATTR_ENLIST_IN_DTC:
			CC_set_error(conn, STMT_OPTION_NOT_FOR_THE_DRIVER, "Unsupported connect attribute (Set)");
			return SQL_ERROR;
		default:
			ret = PGAPI_SetConnectOption(ConnectionHandle, (UWORD) Attribute, (UDWORD) Value);
	}
	return ret;
}

/*	new function */
RETCODE		SQL_API
PGAPI_GetDescField(SQLHDESC DescriptorHandle,
			SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
			PTR Value, SQLINTEGER BufferLength,
			SQLINTEGER *StringLength)
{
	RETCODE		ret = SQL_SUCCESS;
	HSTMT		hstmt;
	SQLUINTEGER	descType;
	StatementClass *stmt;
	static const char *func = "PGAPI_GetDescField";

	mylog("%s h=%u rec=%d field=%d blen=%d\n", func, DescriptorHandle, RecNumber, FieldIdentifier, BufferLength);
	hstmt = statementHandleFromDescHandle(DescriptorHandle, &descType);
	mylog("stmt=%x type=%d\n", hstmt, descType);
	stmt = (StatementClass *) hstmt;
	switch (descType)
	{
		case SQL_ATTR_APP_ROW_DESC:
			ret = ARDGetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
			break;
		case SQL_ATTR_APP_PARAM_DESC:
			ret = APDGetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
			break;
		case SQL_ATTR_IMP_ROW_DESC:
			ret = IRDGetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
			break;
		case SQL_ATTR_IMP_PARAM_DESC:
			ret = IPDGetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength, StringLength);
			break;
		default:ret = SQL_ERROR;
			SC_set_error(stmt, STMT_INTERNAL_ERROR, "Error not implemented");
	}
	if (ret == SQL_ERROR)
	{
		if (!SC_get_errormsg(stmt))
		{
			switch (SC_get_errornumber(stmt))
			{
				case STMT_INVALID_DESCRIPTOR_IDENTIFIER:
					SC_set_errormsg(stmt, "can't SQLGetDescField for this descriptor identifier");
					break;
				case STMT_INVALID_COLUMN_NUMBER_ERROR:
					SC_set_errormsg(stmt, "can't SQLGetDescField for this column number");
					break;
				case STMT_BAD_PARAMETER_NUMBER_ERROR:
					SC_set_errormsg(stmt, "can't SQLGetDescField for this parameter number");
					break;
			}
		} 
		SC_log_error(func, "", stmt);
	}
	return ret;
}

/*	new function */
RETCODE		SQL_API
PGAPI_SetDescField(SQLHDESC DescriptorHandle,
			SQLSMALLINT RecNumber, SQLSMALLINT FieldIdentifier,
			PTR Value, SQLINTEGER BufferLength)
{
	RETCODE		ret = SQL_SUCCESS;
	HSTMT		hstmt;
	SQLUINTEGER	descType;
	StatementClass *stmt;
	static const char *func = "PGAPI_SetDescField";

	mylog("%s h=%u rec=%d field=%d val=%x,%d\n", func, DescriptorHandle, RecNumber, FieldIdentifier, Value, BufferLength);
	hstmt = statementHandleFromDescHandle(DescriptorHandle, &descType);
	mylog("stmt=%x type=%d\n", hstmt, descType);
	stmt = (StatementClass *) hstmt;
	switch (descType)
	{
		case SQL_ATTR_APP_ROW_DESC:
			ret = ARDSetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength);
			break;
		case SQL_ATTR_APP_PARAM_DESC:
			ret = APDSetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength);
			break;
		case SQL_ATTR_IMP_ROW_DESC:
			ret = IRDSetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength);
			break;
		case SQL_ATTR_IMP_PARAM_DESC:
			ret = IPDSetField(stmt, RecNumber, FieldIdentifier, Value, BufferLength);
			break;
		default:ret = SQL_ERROR;
			SC_set_error(stmt, STMT_INTERNAL_ERROR, "Error not implemented");
	}
	if (ret == SQL_ERROR)
	{
		if (!SC_get_errormsg(stmt))
		{
			switch (SC_get_errornumber(stmt))
			{
				case STMT_INVALID_DESCRIPTOR_IDENTIFIER:
					SC_set_errormsg(stmt, "can't SQLSetDescField for this descriptor identifier");
				case STMT_INVALID_COLUMN_NUMBER_ERROR:
					SC_set_errormsg(stmt, "can't SQLSetDescField for this column number");
					break;
				case STMT_BAD_PARAMETER_NUMBER_ERROR:
					SC_set_errormsg(stmt, "can't SQLSetDescField for this parameter number");
					break;
				break;
			}
		} 
		SC_log_error(func, "", stmt);
	}
	return ret;
}

/*	SQLSet(Param/Scroll/Stmt)Option -> SQLSetStmtAttr */
RETCODE		SQL_API
PGAPI_SetStmtAttr(HSTMT StatementHandle,
		SQLINTEGER Attribute, PTR Value,
		SQLINTEGER StringLength)
{
	static char *func = "PGAPI_SetStmtAttr";
	StatementClass *stmt = (StatementClass *) StatementHandle;

	mylog("%s Handle=%u %d,%u\n", func, StatementHandle, Attribute, Value);
	switch (Attribute)
	{
		case SQL_ATTR_CURSOR_SCROLLABLE:		/* -1 */
		case SQL_ATTR_CURSOR_SENSITIVITY:		/* -2 */

		case SQL_ATTR_ENABLE_AUTO_IPD:	/* 15 */

		case SQL_ATTR_APP_ROW_DESC:		/* 10010 */
		case SQL_ATTR_APP_PARAM_DESC:	/* 10011 */
		case SQL_ATTR_AUTO_IPD:	/* 10001 */
		/* case SQL_ATTR_ROW_BIND_TYPE: ** == SQL_BIND_TYPE(ODBC2.0) */
		case SQL_ATTR_IMP_ROW_DESC:	/* 10012 (read-only) */
		case SQL_ATTR_IMP_PARAM_DESC:	/* 10013 (read-only) */
		case SQL_ATTR_METADATA_ID:		/* 10014 */

			/*
			 * case SQL_ATTR_PREDICATE_PTR: case
			 * SQL_ATTR_PREDICATE_OCTET_LENGTH_PTR:
			 */
			SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "Unsupported statement option (Set)");
			SC_log_error(func, "", stmt);
			return SQL_ERROR;

		case SQL_ATTR_FETCH_BOOKMARK_PTR:		/* 16 */
			stmt->options.bookmark_ptr = Value;
			break;
		case SQL_ATTR_PARAM_BIND_OFFSET_PTR:	/* 17 */
			SC_get_APD(stmt)->param_offset_ptr = (SQLUINTEGER *) Value;
			break;
		case SQL_ATTR_PARAM_BIND_TYPE:	/* 18 */
			SC_get_APD(stmt)->param_bind_type = (SQLUINTEGER) Value;
			break;
		case SQL_ATTR_PARAM_OPERATION_PTR:		/* 19 */
			SC_get_APD(stmt)->param_operation_ptr = Value;
			break;
		case SQL_ATTR_PARAM_STATUS_PTR:			/* 20 */
			SC_get_IPD(stmt)->param_status_ptr = (SQLUSMALLINT *) Value;
			break;
		case SQL_ATTR_PARAMS_PROCESSED_PTR:		/* 21 */
			SC_get_IPD(stmt)->param_processed_ptr = (SQLUINTEGER *) Value;
			break;
		case SQL_ATTR_PARAMSET_SIZE:	/* 22 */
			SC_get_APD(stmt)->paramset_size = (SQLUINTEGER) Value;
			break;
		case SQL_ATTR_ROW_BIND_OFFSET_PTR:		/* 23 */
			SC_get_ARD(stmt)->row_offset_ptr = (SQLUINTEGER *) Value;
			break;
		case SQL_ATTR_ROW_OPERATION_PTR:		/* 24 */
			SC_get_ARD(stmt)->row_operation_ptr = Value;
			break;
		case SQL_ATTR_ROW_STATUS_PTR:	/* 25 */
			SC_get_IRD(stmt)->rowStatusArray = (SQLUSMALLINT *) Value;
			break;
		case SQL_ATTR_ROWS_FETCHED_PTR: /* 26 */
			SC_get_IRD(stmt)->rowsFetched = (SQLUINTEGER *) Value;
			break;
		case SQL_ATTR_ROW_ARRAY_SIZE:	/* 27 */
			SC_get_ARD(stmt)->rowset_size = (SQLUINTEGER) Value;
			break;
		default:
			return PGAPI_SetStmtOption(StatementHandle, (UWORD) Attribute, (UDWORD) Value);
	}
	return SQL_SUCCESS;
}

#ifdef DRIVER_CURSOR_IMPLEMENT
#define	CALC_BOOKMARK_ADDR(book, offset, bind_size, index) \
	(book->buffer + offset + \
	(bind_size > 0 ? bind_size : (SQL_C_VARBOOKMARK == book->returntype ? book->buflen : sizeof(UInt4))) * index)    	

RETCODE	SQL_API
PGAPI_BulkOperations(HSTMT hstmt, SQLSMALLINT operation)
{
	static char	*func = "PGAPI_BulkOperations";
	StatementClass	*stmt = (StatementClass *) hstmt;
	ARDFields	*opts = SC_get_ARD(stmt);
	RETCODE		ret;
	UInt4		offset, bind_size = opts->bind_size,
			global_idx;
	int		i, processed;
	ConnectionClass	*conn;
	BOOL		auto_commit_needed = FALSE;
	QResultClass	*res;
	BindInfoClass	*bookmark;

	mylog("%s operation = %d\n", func, operation);
	SC_clear_error(stmt);
	offset = opts->row_offset_ptr ? *opts->row_offset_ptr : 0;
	
	if (SQL_FETCH_BY_BOOKMARK != operation)
	{
		conn = SC_get_conn(stmt);
		if (auto_commit_needed = CC_is_in_autocommit(conn), auto_commit_needed)
			PGAPI_SetConnectOption(conn, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
	}
	if (SQL_ADD != operation)
	{
		if (!(bookmark = opts->bookmark) || !(bookmark->buffer))
		{
			SC_set_error(stmt, STMT_INVALID_OPTION_IDENTIFIER, "bookmark isn't specified");
			return SQL_ERROR;
		}
	}
	for (i = 0, processed = 0; i < opts->rowset_size; i++)
	{
		if (SQL_ADD != operation)
		{
			memcpy(&global_idx, CALC_BOOKMARK_ADDR(bookmark, offset, bind_size, i), sizeof(UInt4));
			global_idx--;
		}
		/* Note opts->row_operation_ptr is ignored */
		switch (operation)
		{
			case SQL_ADD:
				ret = SC_pos_add(stmt, (UWORD) i);
				break;
			case SQL_UPDATE_BY_BOOKMARK:
				ret = SC_pos_update(stmt, (UWORD) i, global_idx);
				break;
			case SQL_DELETE_BY_BOOKMARK:
				ret = SC_pos_delete(stmt, (UWORD) i, global_idx);
				break;
			case SQL_FETCH_BY_BOOKMARK:
				ret = SC_pos_refresh(stmt, (UWORD) i, global_idx);
				break;
		}
		processed++;
		if (SQL_ERROR == ret)
			break;
	}
	if (auto_commit_needed)
		PGAPI_SetConnectOption(conn, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);
	if (SC_get_IRD(stmt)->rowsFetched)
		*(SC_get_IRD(stmt)->rowsFetched) = processed;

	if (res = SC_get_Curres(stmt), res)
		res->recent_processed_row_count = stmt->diag_row_count = processed;
	return ret;
}	
#endif /* DRIVER_CURSOR_IMPLEMENT */
#endif /* ODBCVER >= 0x0300 */
