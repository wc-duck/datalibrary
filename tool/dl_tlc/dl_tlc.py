''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

import json, sys

def read_type_library_definition(filename):
	try:
		def comment_remover(text):
			import re
			def replacer(match):
				s = match.group(0)
				if s.startswith('/'):
					return ""
				else:
					return s
			pattern = re.compile( r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"', re.DOTALL | re.MULTILINE )
			return re.sub(pattern, replacer, text)

		return json.loads( comment_remover(open(filename).read()) )
	except ValueError as error:
		print 'Error parsing type library:\n\t' + str(error)
		sys.exit(1)
	except IOError as error:
		print error
		sys.exit(1)

def align(val, alignment):
	return (val + alignment - 1) & ~(alignment - 1)

def is_power_of_2(n):
	from math import log
	return log(n, 2) % 1.0 == 0.0

def next_power_of_2(_Val):
	from math import log, ceil
	return int(2 ** ceil(log((_Val / 8) + 1, 2)))

def prepare_bitfields(_Members):
	i = 0
	while i < len(_Members):
		member = _Members[i]
		if member['type'] == 'bitfield':
			bf_pos  = i
			bf_bits = 0

			while i < len(_Members) and _Members[i]['type'] == 'bitfield':
				member = _Members[i]
				member.update( last_in_bf = False, bfoffset = bf_bits )
				bf_bits += member['bits']
				i += 1
			
			member['last_in_bf'] = True
			
			BITFIELD_STORAGE_TYPES = { 1 : 'uint8', 2 : 'uint16', 4 : 'uint32', 8 : 'uint64' }
			if is_power_of_2(bf_bits):
				size = bf_bits / 8
			else:
				size = next_power_of_2(bf_bits)
			
			for bfmember in _Members[bf_pos:i]:
				bfmember.update( size32 = size, size64 = size, align32 = size, align64 = size, subtype = BITFIELD_STORAGE_TYPES[size] )
				
			i -= 1
		i += 1

def calc_size_and_align_struct_r(struct, types, type_info):
	struct_name   = struct[0]
	struct_attrib = struct[1]
	if struct_name in type_info:
		return
	
	members = struct_attrib['members']
	
	prepare_bitfields(members)
		
	offset32 = 0
	offset64 = 0
	type_align32  = 0
	type_align64  = 0
	
	if 'align' in struct_attrib:
		type_align32 = struct_attrib['align']
		type_align64 = struct_attrib['align']
	
	struct_attrib['original_align'] = type_align32
	for member in members:
		type = member['type']

		if type == 'inline-array':
			subtype = member['subtype']
			if subtype not in type_info:
				calc_size_and_align_struct_r((subtype, types[subtype]), types, type_info)
			
			subtype = type_info[subtype]
			count   = member['count']
			member.update( size32 = count * subtype['size32'], size64 = count * subtype['size64'], align32 = subtype['align32'], align64 = subtype['align64'] )

		elif type == 'bitfield': pass
		else:
			if type not in type_info:
				calc_size_and_align_struct_r((type, types[type]), types, type_info)
			subtype = type_info[type]

			if 'align' in member:
				forced_align = member['align']
				member.update( size32 = subtype['size32'], size64 = subtype['size64'], align32 = forced_align, align64 = forced_align )
			else:
				member.update( size32 = subtype['size32'], size64 = subtype['size64'], align32 = subtype['align32'], align64 = subtype['align64'] )
		
		# calc offset
		offset32 = align(offset32, member['align32'])
		offset64 = align(offset64, member['align64'])
		
		member.update( offset32 = offset32, offset64 = offset64 )
		
		if type != 'bitfield' or member['last_in_bf']:
			offset32 += member['size32']
			offset64 += member['size64']
		
		type_align32 = max(type_align32, member['align32'])
		type_align64 = max(type_align64, member['align64'])
	
	type_size32 = align(offset32, type_align32)
	type_size64 = align(offset64, type_align64)
	
	struct_attrib.update( size32 = type_size32, size64 = type_size64, align32 = type_align32, align64 = type_align64 )
	
	type_info[struct_name] = { 'size32' : type_size32, 'align32' : type_align32, 'size64' : type_size64, 'align64' : type_align64 }
	
	if 'cpp-alias' in struct_attrib: type_info[struct_name].update( alias = struct_attrib['cpp-alias'] )
	if 'cs-alias' in struct_attrib:  type_info[struct_name].update( alias = struct_attrib['cs-alias'] )
		
def calc_size_and_align(data):
	# fill type_info with default types!
	type_info = {	'uint8'   : { 'size32' : 1, 'align32' : 1, 'size64' : 1,  'align64' : 1 },
					'uint16'  : { 'size32' : 2, 'align32' : 2, 'size64' : 2,  'align64' : 2 },
					'uint32'  : { 'size32' : 4, 'align32' : 4, 'size64' : 4,  'align64' : 4 }, 
					'uint64'  : { 'size32' : 8, 'align32' : 8, 'size64' : 8,  'align64' : 8 }, 
					'int8'    : { 'size32' : 1, 'align32' : 1, 'size64' : 1,  'align64' : 1 }, 
					'int16'   : { 'size32' : 2, 'align32' : 2, 'size64' : 2,  'align64' : 2 }, 
					'int32'   : { 'size32' : 4, 'align32' : 4, 'size64' : 4,  'align64' : 4 }, 
					'int64'   : { 'size32' : 8, 'align32' : 8, 'size64' : 8,  'align64' : 8 }, 
					'fp32'    : { 'size32' : 4, 'align32' : 4, 'size64' : 4,  'align64' : 4 }, 
					'fp64'    : { 'size32' : 8, 'align32' : 8, 'size64' : 8,  'align64' : 8 },
					'string'  : { 'size32' : 4, 'align32' : 4, 'size64' : 8,  'align64' : 8 },
					'pointer' : { 'size32' : 4, 'align32' : 4, 'size64' : 8,  'align64' : 8 },
					'array'   : { 'size32' : 8, 'align32' : 4, 'size64' : 12, 'align64' : 8 } }

	# fill enum types!
	if 'module_enums' in data:
		for enum_type, values in data['module_enums'].items():
			type_info[enum_type] = { 'size32' : 4, 'align32' : 4, 'size64' : 4, 'align64' : 4 }
	
	types = data['module_types']
	for struct in types.items():
		calc_size_and_align_struct_r(struct, types, type_info)
		
def calc_enum_vals(data):
	if 'module_enums' in data:
		enum_dict = data['module_enums']
		for enum in enum_dict.items():
			values = enum[1]
			
			last_value = -1
			new_values = []
			
			val_name = ''
			for val in values:
				if type(val) == unicode:
					last_value += 1
					val_name = str(val)
				else:
					assert len(val) == 1
					key = val.keys()[0]
					last_value = val[key]
					val_name = str(key)
				
				new_values.append((val_name, last_value))
			
			enum_dict[enum[0]] = new_values

def check_for_bad_data(_Data):
	# add error-check plox!
	pass

from header_writer_cs import HeaderWriterCS
from type_lib_writer import TypeLibraryWriter
import logging

# 'main'-function!

config = {
	'cpp' : {
		'header' : 
'''#if defined(_MSC_VER)

        typedef signed   __int8  int8_t;
        typedef signed   __int16 int16_t;
        typedef signed   __int32 int32_t;
        typedef signed   __int64 int64_t;
        typedef unsigned __int8  uint8_t;
        typedef unsigned __int16 uint16_t;
        typedef unsigned __int32 uint32_t;
        typedef unsigned __int64 uint64_t;

#elif defined(__GNUC__)
        #include <stdint.h>
#endif''',

		'array_names' : {
			'count' : 'count',
			'data'  : 'data'
		},
		
		'pod_names' : {
			'int8'   : 'int8_t',
			'int16'  : 'int16_t',
			'int32'  : 'int32_t',
			'int64'  : 'int64_t',
			'uint8'  : 'uint8_t',
			'uint16' : 'uint16_t',
			'uint32' : 'uint32_t',
			'uint64' : 'uint64_t',
			'fp32'   : 'float',
			'fp64'   : 'double'
		}
	}
}

def parse_options():
	from optparse import OptionParser

	parser = OptionParser(description = "dl_tlc is the type library compiler for DL and converts a text-typelibrary to binary type-libraray.")
	parser.add_option("", "--dldll")
	parser.add_option("-o", "--output",     dest="output",    help="write type library to file")
	parser.add_option("-x", "--hexarray",   dest="hexarray",  help="write type library as hex-array that can be included in c/cpp-code")
	parser.add_option("",   "--c-header",   dest="cheader",   help="write C-header to file")
	parser.add_option("-c", "--cpp-header", dest="cppheader", help="write C++-header to file")
	parser.add_option("-s", "--cs-header",  dest="csheader",  help="write C#-header to file")	
	parser.add_option("",   "--config",     dest="config",    help="configuration file to configure generated headers.")	
	parser.add_option("-v", "--verbose",    dest="verbose",   help="enable verbose output", action="store_true", default=False)

	(options, args) = parser.parse_args()

	if len(args) < 1:
		parser.print_help()
		sys.exit(0)
	
	options.input = args[0]
	
	if options.verbose:
		logging.basicConfig(level=logging.DEBUG)
	else:
		logging.basicConfig(level=logging.ERROR)
		
	if options.config:
		def merge_dict(update_me, update_with):
			for key in update_me.keys():
				if update_with.has_key(key):
					val_me   = update_me[key]
					val_with = update_with[key]
					
					if isinstance( val_me, dict ) and isinstance( val_with, dict ):
						merge_dict( val_me, val_with )
					else:
						update_me[key] = val_with
		
		user_cfg = eval(open(options.config).read())
		merge_dict( config, user_cfg )
	
	return options

if __name__ == "__main__":
	import dl.typelibrary
	import dl.generate
	
	options = parse_options()

	data = read_type_library_definition(options.input)
	
	calc_enum_vals(data)
	calc_size_and_align(data)
	
	check_for_bad_data(data)
	
	tl = dl.typelibrary.TypeLibrary( options.input )

	dl.typelibrary.compile( tl, None )
	
	# write headers
	if options.cppheader:
		file = open( options.cppheader, 'w' )
		dl.generate.cpp.generate( tl, None, file )
		file.close()
		
	if options.cheader:
		options.cppheader = open(options.cppheader, 'w')
		hw = HeaderWriterCPP(options.cppheader, config['cpp'], pure_c = True)
		hw.write_header(data)
		hw.write_enums(data)
		hw.write_structs(data)
		hw.finalize(data)
		options.cppheader.close()
		
	if options.csheader:
		file = open( options.csheader, 'w' )
		dl.generate.cs.generate( tl, None, file )
		file.close()

	# write binary type library
	if options.output or options.hexarray:
		from StringIO import StringIO
		import array
		output = StringIO()
		tlw = TypeLibraryWriter(output)
		tlw.write(data)
		
		if options.output:
			open(options.output, 'wb').write(output.getvalue())
			
		if options.hexarray:
			hex_file = open(options.hexarray, 'wb')
			data = array.array( 'B' )
			data.fromstring( output.getvalue() )
			
			count = 1
			
			hex_file.write( '0x%02X' % data[0] )
			
			for c in data[1:]:
				if count > 15:
					hex_file.write( ',\n0x%02X' % c )
					count = 0
				else:
					hex_file.write( ', 0x%02X ' % c )
			
				count += 1
				
			hex_file.close()
