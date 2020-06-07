/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#include "dl_txt_read.h"

#include <errno.h>
#include <stdlib.h>

struct dl_txt_binlit
{
	uint64_t value;
	bool     was_binlit;
	bool     was_negative;
	bool     was_overflow;
};

static dl_txt_binlit dl_txt_pack_eat_binlit(dl_txt_read_ctx* read_ctx, int max_bits, char** next)
{
	const char* iter = read_ctx->iter;

	dl_txt_binlit v = {0,0,0, 0};
	v.was_negative = iter[0] == '-';
	v.was_binlit   = iter[0 + v.was_negative] == '0' && iter[1 + v.was_negative] == 'b';

	if(!v.was_binlit)
		return v;

	iter += 2 + v.was_negative;

	// ... skip initial zeros ...
	while(iter[0] == '0')
		++iter;

	int bits = 0;

	while(true)
	{
		switch(iter[0])
		{
			case '0': v.value <<= 1; v.value |= 0; ++bits; break;
			case '1': v.value <<= 1; v.value |= 1; ++bits; break;
			default:
				*next = (char*)iter;
				return v;
		}

		if(bits > max_bits)
		{
			v.was_overflow = true;
			return v;
		}
		++iter;
	}
}

long long dl_txt_pack_eat_strtoll( dl_ctx_t dl_ctx, dl_txt_read_ctx* read_ctx, long long range_min, long long range_max, const char* type )
{
	dl_txt_eat_white( read_ctx );

	long long v = dl_txt_eat_bool( read_ctx );
	if( v < 2 )
		return v;

	char* next = 0x0;
	dl_txt_binlit bin = dl_txt_pack_eat_binlit(read_ctx, 63, &next);
	if(bin.was_overflow)
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of type '%s', more than 63 bits in binary literal", type );

	if(bin.was_binlit)
		v = (bin.was_negative ? -1 : 1) * (long long)bin.value;
	else
	{
		errno = 0;
	#if defined(_MSC_VER)
		v = _strtoi64( read_ctx->iter, &next, 0 );
	#else
		v = strtoll( read_ctx->iter, &next, 0 );
	#endif

		if( read_ctx->iter == next )
		{
			if( tolower(read_ctx->iter[0]) == 'm')
			{
				if( tolower(read_ctx->iter[1]) == 'a' &&
					tolower(read_ctx->iter[2]) == 'x' &&
				!isalnum(read_ctx->iter[3]) )
				{
					read_ctx->iter += 3;
					return range_max;
				}
				if( tolower(read_ctx->iter[1]) == 'i' &&
					tolower(read_ctx->iter[2]) == 'n' &&
				!isalnum(read_ctx->iter[3]))
				{
					read_ctx->iter += 3;
					return range_min;
				}
			}
			dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_MALFORMED_DATA, "expected a value of type '%s'", type );
		}
	}

	if( !(v >= range_min && v <= range_max) || errno == ERANGE )
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of type '%s', %lld is out of range.", type, v );
	read_ctx->iter = next;
	return v;
}

unsigned long long dl_txt_pack_eat_strtoull( dl_ctx_t dl_ctx, dl_txt_read_ctx* read_ctx, unsigned long long range_max, const char* type )
{
	dl_txt_eat_white( read_ctx );

	if(read_ctx->iter[0] == '-')
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of unsigned type '%s', but value is negative!", type );

	unsigned long long v = (unsigned long long)dl_txt_eat_bool( read_ctx );
	if( v < 2 )
		return v;

	char* next = 0x0;
	dl_txt_binlit bin = dl_txt_pack_eat_binlit(read_ctx, 64, &next);
	if(bin.was_overflow)
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of type '%s', more than 64 bits in binary literal", type );

	if(bin.was_binlit)
	{
		if(bin.was_negative)
			dl_txt_read_failed(dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of unsigned type '%s', but value is negative!", type );
		v = bin.value;
	}
	else
	{
		errno = 0;
	#if defined(_MSC_VER)
		v = _strtoui64( read_ctx->iter, &next, 0 );
	#else
		v = strtoull( read_ctx->iter, &next, 0 );
	#endif

		if( read_ctx->iter == next )
		{
			if( tolower(read_ctx->iter[0]) == 'm' )
			{
				if( tolower(read_ctx->iter[1]) == 'a' &&
					tolower(read_ctx->iter[2]) == 'x' &&
				!isalnum(read_ctx->iter[3]))
				{
					read_ctx->iter += 3;
					return range_max;
				}
				if( tolower(read_ctx->iter[1]) == 'i' &&
					tolower(read_ctx->iter[2]) == 'n' &&
				!isalnum(read_ctx->iter[3]))
				{
					read_ctx->iter += 3;
					return 0;
				}
			}
			dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_MALFORMED_DATA, "expected a value of type '%s'", type );
		}
	}

	if( v > range_max || errno == ERANGE )
		dl_txt_read_failed( dl_ctx, read_ctx, DL_ERROR_TXT_RANGE_ERROR, "expected a value of type '%s', %llu is out of range.", type, v );
	read_ctx->iter = next;
	return v;
}

void dl_report_error_location( dl_ctx_t ctx, const char* txt, const char* end, const char* error_pos )
{
	int line = 1;
	int col = 1;
	const char* last_line = txt;
	const char* iter = txt;
	while( iter != end && iter != error_pos )
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

	if( iter == end )
		dl_log_error( ctx, "at end of buffer");
	else
	{
		const char* line_end = strchr( last_line, '\n' );
		dl_log_error( ctx, "at line %d, col %d:\n%.*s\n%*c^", line, col, (int)(line_end-last_line), last_line, col, ' ');
	}
}
