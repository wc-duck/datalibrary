#ifndef DL_TXT_READ_H_INCLUDED
#define DL_TXT_READ_H_INCLUDED

#include <ctype.h>
#include <setjmp.h>
#include "dl_types.h"

struct dl_txt_read_ctx
{
	const char* start;
	const char* iter;
	dl_error_t err;
	jmp_buf jumpbuf;
};

struct dl_txt_read_substr
{
	const char* str;
	int len;
};


#if defined( __GNUC__ )
static void dl_txt_read_failed( dl_ctx_t ctx, dl_txt_read_ctx* readctx, dl_error_t err, const char* fmt, ... ) __attribute__((format( printf, 4, 5 )));
#endif

static /*no inline?*/ void dl_txt_read_failed( dl_ctx_t ctx, dl_txt_read_ctx* readctx, dl_error_t err, const char* fmt, ... )
{
	if( ctx->error_msg_func )
	{
		char buffer[256];
		va_list args;
		va_start( args, fmt );
		vsnprintf( buffer, DL_ARRAY_LENGTH(buffer), fmt, args );
		va_end(args);

		buffer[DL_ARRAY_LENGTH(buffer) - 1] = '\0';

		ctx->error_msg_func( buffer, ctx->error_msg_ctx );
	}
	readctx->err = err;
	longjmp( readctx->jumpbuf, 1 );
}

inline const char* dl_txt_skip_white( const char* str )
{
	while( true )
	{
		while( isspace(*str) ) ++str;
		if( *str == '/' )
		{
			// ... skip comment ...
			switch( str[1] )
			{
				case '/':
					while( *str != '\n' ) ++str;
					break;
				case '*':
					str = &str[2];
					while( true )
					{
						while( *str != '*' ) ++str;
						if( str[1] == '/' )
						{
							str = &str[2];
							break;
						}
						++str;
					}
			}
		}
		else
			return str;
	}
	return 0x0;
}

inline void dl_txt_eat_white( dl_txt_read_ctx* readctx )
{
	readctx->iter = dl_txt_skip_white( readctx->iter );
}

static dl_txt_read_substr dl_txt_eat_string( dl_txt_read_ctx* readctx )
{
	dl_txt_read_substr res = {0x0, 0};
	if( *readctx->iter != '"' )
		return res;

	const char* key_start = readctx->iter + 1;
	const char* key_end = key_start;
	while( *key_end )
	{
		switch( *key_end )
		{
		case '"':
			res.str = key_start;
			res.len = (int)(key_end - key_start);
			readctx->iter = res.str + res.len + 1;
			return res;
		case '\\':
			++key_end;
			// fallthrough
		default:
			++key_end;
		}
	}
	return res;
}

static void dl_txt_eat_char( dl_ctx_t dl_ctx, dl_txt_read_ctx* readctx, char expect )
{
	dl_txt_eat_white( readctx );
	if( *readctx->iter != expect )
		dl_txt_read_failed( dl_ctx, readctx, DL_ERROR_TXT_PARSE_ERROR, "expected '%c' but got '%c' at %u", expect, *readctx->iter, __LINE__ );
	++readctx->iter;
}

static long dl_txt_eat_bool( dl_txt_read_ctx* packctx )
{
	if( strncmp( packctx->iter, "true", 4 ) == 0 )
	{
		packctx->iter += 4;
		return 1;
	}
	if( strncmp( packctx->iter, "false", 5 ) == 0 )
	{
		packctx->iter += 5;
		return 0;
	}
	return 2;
}

static void dl_report_error_location( dl_ctx_t ctx, const char* txt, const char* error_pos )
{
	int line = 0;
	int col = 0;
	const char* last_line = txt;
	const char* iter = txt;
	while( iter != error_pos )
	{
		if( *iter == '\n' )
		{
			last_line = iter + 1;
			++line;
			col = 0;
		}
		else
		{
			++col;
		}
		++iter;
	}

	const char* line_end = strchr( last_line, '\n' );
	dl_log_error( ctx, "at line %d, col %d:\n%.*s\n%*c^", line, col, (int)(line_end-last_line), last_line, col, ' ');
}

#endif // DL_TXT_READ_H_INCLUDED
