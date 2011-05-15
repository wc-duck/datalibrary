/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef DL_DL_BINARY_WRITER_H_INCLUDED
#define DL_DL_BINARY_WRITER_H_INCLUDED

#include <stdio.h>

#if 0 // BinaryWriterVerbose
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...) printf("DL: " _Fmt "\n", ##__VA_ARGS__)
#else
	#define DL_LOG_BIN_WRITER_VERBOSE(_Fmt, ...)
#endif

struct dl_binary_writer
{
	bool          dummy;
	dl_endian_t   source_endian;
	dl_endian_t   target_endian;
	dl_ptr_size_t ptr_size;
	pint          pos;
	pint          needed_size;
	uint8*        data;
	pint          data_size;
	pint          back_alloc_pos;
};

static inline void dl_binary_writer_init( dl_binary_writer* writer,
										  uint8* out_data, pint out_data_size, bool dummy,
										  dl_endian_t source_endian, dl_endian_t target_endian,
										  dl_ptr_size_t target_ptr_size )
{
	writer->dummy          = dummy;
	writer->source_endian  = source_endian;
	writer->target_endian  = target_endian;
	writer->ptr_size       = target_ptr_size;
	writer->pos            = 0;
	writer->needed_size    = 0;
	writer->data           = out_data;
	writer->data_size      = out_data_size;
	writer->back_alloc_pos = out_data_size;
}

static inline void dl_binary_writer_finalize( dl_binary_writer* writer )
{
	DL_ASSERT( writer->back_alloc_pos == writer->data_size );
}

static inline void dl_binary_writer_seek_set( dl_binary_writer* writer, pint pos ) { writer->pos  = pos;                 DL_LOG_BIN_WRITER_VERBOSE("Seek Set: " DL_PINT_FMT_STR, writer->pos); }
static inline void dl_binary_writer_seek_end( dl_binary_writer* writer )           { writer->pos  = writer->needed_size; DL_LOG_BIN_WRITER_VERBOSE("Seek End: " DL_PINT_FMT_STR, writer->pos); }
static inline pint dl_binary_writer_tell( dl_binary_writer* writer )               { return writer->pos; }
static inline pint dl_binary_writer_needed_size( dl_binary_writer* writer )        { return writer->needed_size; }

static inline void dl_binary_writer_update_needed_size( dl_binary_writer* writer )
{
	if( writer->back_alloc_pos == writer->data_size || writer->pos <= writer->back_alloc_pos )
		writer->needed_size = writer->needed_size >= writer->pos ? writer->needed_size : writer->pos;
}

static inline void dl_binary_writer_write( dl_binary_writer* writer, const void* data, pint size )
{
	if( !writer->dummy && ( writer->pos + size <= writer->data_size ) )
	{
		switch( size )
		{
			case 1: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (%u)",                    writer->pos, size, *(char*)data   ); break;
			case 2: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (%u)",                    writer->pos, size, *(uint16*)data ); break;
			case 4: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (%u)",                    writer->pos, size, *(uint32*)data ); break;
			case 8: DL_LOG_BIN_WRITER_VERBOSE ("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (" DL_UINT64_FMT_STR ")", writer->pos, size, *(uint64*)data ); break;
			default: DL_LOG_BIN_WRITER_VERBOSE("Write: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR, writer->pos, size ); break;
		}
		DL_ASSERT( writer->pos + size <= writer->data_size && "To small buffer!" );
		memmove( writer->data + writer->pos, data, size);
	}

	writer->pos += size;
	dl_binary_writer_update_needed_size( writer );
}

static inline void dl_binary_writer_write_int8  ( dl_binary_writer* writer, int8   u ) { dl_binary_writer_write( writer, &u, sizeof(int8) );   }
static inline void dl_binary_writer_write_int16 ( dl_binary_writer* writer, int16  u ) { dl_binary_writer_write( writer, &u, sizeof(int16) );  }
static inline void dl_binary_writer_write_int32 ( dl_binary_writer* writer, int32  u ) { dl_binary_writer_write( writer, &u, sizeof(int32) );  }
static inline void dl_binary_writer_write_int64 ( dl_binary_writer* writer, int64  u ) { dl_binary_writer_write( writer, &u, sizeof(int64) );  }
static inline void dl_binary_writer_write_uint8 ( dl_binary_writer* writer, uint8  u ) { dl_binary_writer_write( writer, &u, sizeof(uint8) );  }
static inline void dl_binary_writer_write_uint16( dl_binary_writer* writer, uint16 u ) { dl_binary_writer_write( writer, &u, sizeof(uint16) ); }
static inline void dl_binary_writer_write_uint32( dl_binary_writer* writer, uint32 u ) { dl_binary_writer_write( writer, &u, sizeof(uint32) ); }
static inline void dl_binary_writer_write_uint64( dl_binary_writer* writer, uint64 u ) { dl_binary_writer_write( writer, &u, sizeof(uint64) ); }
static inline void dl_binary_writer_write_fp32  ( dl_binary_writer* writer, float  u ) { dl_binary_writer_write( writer, &u, sizeof(float)  ); }
static inline void dl_binary_writer_write_fp64  ( dl_binary_writer* writer, double u ) { dl_binary_writer_write( writer, &u, sizeof(double) ); }
static inline void dl_binary_writer_write_pint  ( dl_binary_writer* writer, pint   u ) { dl_binary_writer_write( writer, &u, sizeof(pint) );   }

static inline void dl_binary_writer_write_1byte( dl_binary_writer* writer, const void* data )
{
	dl_binary_writer_write( writer, data, 1 );
}

static inline void dl_binary_writer_write_2byte( dl_binary_writer* writer, const void* data )
{
	union { uint16* u16; const void* data; } conv;
	conv.data = data;
	uint16 val = *conv.u16;

	if( writer->source_endian != writer->target_endian )
		val = dl_swap_endian_uint16( val );

	dl_binary_writer_write( writer, &val, 2 );
}

static inline void dl_binary_writer_write_4byte( dl_binary_writer* writer, const void* data )
{
	union { uint32* u32; const void* data; } conv;
	conv.data = data;
	uint32 val = *conv.u32;

	if( writer->source_endian != writer->target_endian )
		val = dl_swap_endian_uint32( val );

	dl_binary_writer_write( writer, &val, 4 );
}

static inline void dl_binary_writer_write_8byte( dl_binary_writer* writer, const void* data )
{
	union { uint64* u64; const void* data; } conv;
	conv.data = data;
	uint64 val = *conv.u64;

	if( writer->source_endian != writer->target_endian )
		val = dl_swap_endian_uint64( val );

	dl_binary_writer_write( writer, &val, 8 );
}

static inline void dl_binary_writer_write_swap( dl_binary_writer* writer, const void* data, pint size )
{
	switch( size )
	{
		case 1: dl_binary_writer_write_1byte( writer, data ); break;
		case 2: dl_binary_writer_write_2byte( writer, data ); break;
		case 4: dl_binary_writer_write_4byte( writer, data ); break;
		case 8: dl_binary_writer_write_8byte( writer, data ); break;
		default:
			DL_ASSERT( false && "unhandled case!" );
	}
}

static inline void dl_binary_writer_write_array( dl_binary_writer* writer, const void* array, pint count, pint elem_size )
{
	if( !writer->dummy )
	{
		DL_LOG_BIN_WRITER_VERBOSE( "Write Array: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (" DL_PINT_FMT_STR ")", writer->pos, elem_size, count );
		for (pint i = 0; i < count; ++i)
		{
			switch( elem_size )
			{
				case 1: { DL_LOG_BIN_WRITER_VERBOSE( "\t%c",                 ((const uint8*)  array)[i] ); } break;
				case 2: { DL_LOG_BIN_WRITER_VERBOSE( "\t%u",                 ((const uint16*) array)[i] ); } break;
				case 4: { DL_LOG_BIN_WRITER_VERBOSE( "\t%u",                 ((const uint32*) array)[i] ); } break;
				case 8: { DL_LOG_BIN_WRITER_VERBOSE( "\t" DL_UINT64_FMT_STR, ((const uint64*) array)[i] ); } break;
			}
		}
	}

	if( writer->source_endian != writer->target_endian )
	{
		for(pint i = 0; i < count; ++i)
			dl_binary_writer_write_swap( writer, (const uint8*)array + i * elem_size, elem_size );
	}
	else
		dl_binary_writer_write( writer, array, elem_size * count );
}

// val is expected to be in host-endian!!!
static inline void dl_binary_writer_write_ptr( dl_binary_writer* writer, pint val )
{
	if( writer->target_endian != DL_ENDIAN_HOST )
	{
		switch( writer->ptr_size )
		{
			case DL_PTR_SIZE_32BIT: { uint32 u = (uint32)val; dl_binary_writer_write_4byte( writer, &u ); break; }
			case DL_PTR_SIZE_64BIT: { uint64 u = (uint64)val; dl_binary_writer_write_8byte( writer, &u ); break; }
			default:
				DL_ASSERT(false && "Bad ptr-size!?!");
		}
	}
	else
	{
		switch( writer->ptr_size )
		{
			case DL_PTR_SIZE_32BIT: { uint32 u = (uint32)val; dl_binary_writer_write( writer, &u, 4 ); } break;
			case DL_PTR_SIZE_64BIT: { uint64 u = (uint64)val; dl_binary_writer_write( writer, &u, 8 ); } break;
			default:
				DL_ASSERT(false && "Bad ptr-size!?!");
		}
	}
}

static inline void dl_binary_writer_write_string( dl_binary_writer* writer, const void* str, pint len )
{
	char str_end = '\0';
	dl_binary_writer_write( writer, str, len );
	dl_binary_writer_write( writer, &str_end, sizeof(char) );
}

static inline void dl_binary_writer_write_zero( dl_binary_writer* writer, pint bytes )
{
	if( !writer->dummy )
	{
		DL_LOG_BIN_WRITER_VERBOSE("Write zero: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR, writer->pos, bytes);
		DL_ASSERT( writer->pos + bytes <= writer->data_size && "To small buffer!" );
		memset( writer->data + writer->pos, 0x0, bytes );
	}

	writer->pos += bytes;
	dl_binary_writer_update_needed_size( writer );
}

static inline void dl_binary_writer_reserve( dl_binary_writer* writer, pint bytes )
{
	DL_LOG_BIN_WRITER_VERBOSE( "Reserve: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR, writer->pos, bytes );
	writer->needed_size = writer->needed_size >= writer->pos + bytes ? writer->needed_size : writer->pos + bytes;
}

static inline uint8  dl_binary_writer_read_uint8 ( dl_binary_writer* writer ) { return writer->dummy ? 0 : *(uint8*) ( writer->data + writer->pos ); }
static inline uint16 dl_binary_writer_read_uint16( dl_binary_writer* writer ) { return writer->dummy ? 0 : *(uint16*)( writer->data + writer->pos ); }
static inline uint32 dl_binary_writer_read_uint32( dl_binary_writer* writer ) { return writer->dummy ? 0 : *(uint32*)( writer->data + writer->pos ); }
static inline uint64 dl_binary_writer_read_uint64( dl_binary_writer* writer ) { return writer->dummy ? 0 : *(uint64*)( writer->data + writer->pos ); }

static inline void dl_binary_writer_align( dl_binary_writer* writer, pint align )
{
	pint alignment = dl_internal_align_up( writer->pos, align );
	if( !writer->dummy && alignment != writer->pos )
	{
		DL_LOG_BIN_WRITER_VERBOSE( "Align: " DL_PINT_FMT_STR " + " DL_PINT_FMT_STR " (" DL_PINT_FMT_STR ")", writer->pos, alignment - writer->pos, align );
		memset( writer->data + writer->pos, 0x0, alignment - writer->pos);
	}
	writer->pos = alignment;
	dl_binary_writer_update_needed_size( writer );
};

static inline pint dl_binary_writer_push_back_alloc( dl_binary_writer* writer, pint bytes )
{
	// allocates a chunk of "bytes" bytes from the back of the buffer and return the
	// offset to it. function ignores alignment and tries to pack data as tightly as possible
	// since data that will be stored here is only temporary.

	writer->back_alloc_pos -= bytes;
	DL_LOG_BIN_WRITER_VERBOSE("push back-alloc: " DL_PINT_FMT_STR " (" DL_PINT_FMT_STR ")\n", writer->back_alloc_pos, bytes);
	return writer->back_alloc_pos;
}

// return start of new array!
static inline pint dl_binary_writer_pop_back_alloc( dl_binary_writer* writer, uint32 num_elem, uint32 elem_size, uint32 elem_align )
{
	DL_ASSERT( writer->back_alloc_pos != writer->data_size );
	DL_ASSERT( writer->back_alloc_pos >= writer->needed_size && "back-alloc and front-alloc overlap!" );

	dl_binary_writer_seek_end( writer );
	dl_binary_writer_align( writer, elem_align );
	dl_binary_writer_reserve( writer, elem_size * num_elem ); // Reserve( elem_size * num_elem );
	pint arr_pos = dl_binary_writer_tell( writer );

	pint first_elem = writer->back_alloc_pos;
	pint last_elem  = first_elem + elem_size * ( num_elem - 1 );

	DL_LOG_BIN_WRITER_VERBOSE("First: " DL_PINT_FMT_STR " last " DL_PINT_FMT_STR, first_elem, last_elem);

	unsigned char* curr_first = writer->data + first_elem;
	unsigned char* curr_last  = writer->data + last_elem;

	unsigned char swap_buffer[256]; // TODO: Handle bigger elements!

	if( !writer->dummy )
		while( curr_first < curr_last )
		{
			DL_ASSERT( elem_size <= DL_ARRAY_LENGTH(swap_buffer));

			DL_LOG_BIN_WRITER_VERBOSE("Swap: " DL_PINT_FMT_STR " to " DL_PINT_FMT_STR, curr_first - writer->data, curr_last - writer->data );

			memcpy( swap_buffer, curr_first,  elem_size );
			memcpy( curr_first,  curr_last,   elem_size );
			memcpy( curr_last,   swap_buffer, elem_size );

			curr_first += elem_size;
			curr_last  -= elem_size;
		}

	if( elem_size == elem_align )
		dl_binary_writer_write( writer, writer->data + first_elem, elem_size * num_elem );
	else
	{
		for( uint32 elem = 0; elem < num_elem; ++elem )
		{
			dl_binary_writer_align( writer, elem_align );
			dl_binary_writer_write( writer, writer->data + first_elem + elem * elem_size, elem_size );
		}
	}

	writer->back_alloc_pos += elem_size * ( num_elem );

	return arr_pos;
}

static inline bool dl_binary_writer_in_back_alloc_area( dl_binary_writer* writer ) { return writer->pos > writer->needed_size; }

#endif // DL_DL_BINARY_WRITER_H_INCLUDED
