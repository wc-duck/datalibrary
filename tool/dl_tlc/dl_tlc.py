''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

import logging, sys

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
	parser.add_option("",   "--tld-text",   dest="tld_text",  help="write read type-library in text-format to file")	
	parser.add_option("",   "--config",     dest="config",    help="configuration file to configure generated headers.")	
	parser.add_option("-v", "--verbose",    dest="verbose",   help="enable verbose output", action="store_true", default=False)

	(options, args) = parser.parse_args()

	if len(args) < 1:
		parser.print_help()
		sys.exit(0)
	
	options.input = args[0]
	
	logging.getLogger().setLevel( level=logging.DEBUG if options.verbose else logging.ERROR )

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
    import StringIO
    import dl.typelibrary
    import dl.generate
    
    options = parse_options()

    try:
        tl =  dl.typelibrary.TypeLibrary( options.input )
    except dl.typelibrary.DLTypeLibraryError, e:
        print 'ERROR: %s' % e
        sys.exit( 1 )
    except ValueError, e:
    	print 'ERROR: Failed during json-parse, %s' % e
    	sys.exit( 1 )
    
    # write headers
    if options.cheader:   dl.generate.c.generate( tl, None, open( options.cheader, 'w' ) )
    if options.csheader:  dl.generate.csharp.generate( tl, None, open( options.csheader, 'w' ) )
    if options.cppheader: dl.generate.cplusplus.generate( tl, None, open( options.cppheader, 'w' ) )

    compiled_tl = None
    
    if options.tld_text:
    	import sys
    	dl.typelibrary.generate( tl, sys.stdout )
    
    if options.output or options.hexarray:
        tl_str = StringIO.StringIO()
        dl.typelibrary.compile( tl, tl_str )
        compiled_tl = tl_str.getvalue()
        
    if options.output:
    	open( options.output, 'wb' ).write(compiled_tl)
    	
    if options.hexarray:
    	import array
		
    	hex_file = open(options.hexarray, 'wb')
    	tl_data = array.array( 'B' )
    	tl_data.fromstring(compiled_tl )
    	
    	count = 1
    	
    	hex_file.write( '0x%02X' % tl_data[0] )
    	
    	for c in tl_data[1:]:
    		if count > 15:
    			hex_file.write( ',\n0x%02X' % c )
    			count = 0
    		else:
    			hex_file.write( ', 0x%02X ' % c )
    	
    		count += 1
    		
    	hex_file.close()
