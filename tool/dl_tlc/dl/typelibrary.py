''' 
    should be lib to parse dl-typelibrary file
    
    dl_header_generate.py with functions:
        generate_cpp, generate_c and generate_cs should use this lib to read tld-file.
'''

import logging
import struct
import json

class PlatformValue( object ):
    def __init__(self, val=(0,0)):
        if isinstance( val, tuple ):
            self.ptr32, self.ptr64 = val
        else:
            self.ptr32, self.ptr64 = ( val, val )
        
    def __repr__(self):            return '(' + self.ptr32.__repr__() + ', ' + self.ptr64.__repr__() + ')'
    def __add__(self, other):      return PlatformValue( ( self.ptr32 + other.ptr32, self.ptr64 + other.ptr64 ) )
    def __sub__(self, other):      return PlatformValue( ( self.ptr32 - other.ptr32, self.ptr64 - other.ptr64 ) )
    def __mul__(self, multiplier): return PlatformValue( ( self.ptr32 * multiplier,  self.ptr64 * multiplier ) )
    
    @staticmethod
    def max(val1, val2): return PlatformValue( ( max(val1.ptr32, val2.ptr32), max(val1.ptr64, val2.ptr64) ) )
    
    @staticmethod
    def align(val, align_value):
        def align(val, alignment):
            return val if alignment == 0 else (val + alignment - 1) & ~(alignment - 1)
        return PlatformValue( ( align(val.ptr32, align_value.ptr32), align(val.ptr64, align_value.ptr64) ) )

class DLTypeLibraryError(Exception):
    def __init__(self, err): self.err = err
    def __str__(self):       return self.err

class BuiltinType( object ):
    def __init__(self, name, size, align):
        self.name      = name
        self.my_size   = PlatformValue(size)
        self.my_align  = PlatformValue(align)
        self.typeid    = 0
        
    def base_type(self): return self
    def size(self):      return self.my_size
    def align(self):     return self.my_align
    
class ArrayType( object ):
    def __init__( self, type ):
        self.type  = type
        
    def base_type(self): return self.type.base_type()
    def size(self):      return PlatformValue( ( 8, 16 ) )
    def align(self):     return PlatformValue( ( 4,  8 ) )

class InlineArrayType( object ):
    def __init__( self, type, count ):
        self.type  = type
        self.count = count
        
    def base_type(self): return self.type.base_type()
    def size(self):      return self.type.size() * self.count
    def align(self):     return self.type.align()

class PointerType( object ):
    def __init__( self, type ):
        self.type  = type
        
    def base_type(self): return self.type.base_type()
    def size(self):      return PlatformValue( ( 4, 8 ) )
    def align(self):     return PlatformValue( ( 4, 8 ) ) 
    
class BitfieldType( object ):
    def __init__( self, bits ):
        self.type  = None
        self.bits  = bits

    def base_type(self): return self.type.base_type()
    def size(self):      return self.type.size()
    def align(self):     return self.type.align()

BUILTIN_TYPES = { 'int8'   : BuiltinType('int8',   (1, 1), (1, 1)),
                  'int16'  : BuiltinType('int16',  (2, 2), (2, 2)),
                  'int32'  : BuiltinType('int32',  (4, 4), (4, 4)),
                  'int64'  : BuiltinType('int64',  (8, 8), (8, 8)),
                  'uint8'  : BuiltinType('uint8',  (1, 1), (1, 1)),
                  'uint16' : BuiltinType('uint16', (2, 2), (2, 2)),
                  'uint32' : BuiltinType('uint32', (4, 4), (4, 4)),
                  'uint64' : BuiltinType('uint64', (8, 8), (8, 8)),
                  'fp32'   : BuiltinType('fp32',   (4, 4), (4, 4)),
                  'fp64'   : BuiltinType('fp64',   (8, 8), (8, 8)),
                  'string' : BuiltinType('string', (4, 8), (4, 8)) }

class Enum( object ):
    class EnumValue( object ):
        def __init__( self, name, header_name, value ):
            self.name        = name
            self.header_name = header_name
            self.value       = value

    def __init__(self, name, values):
        self.name   = name 
        self.values = []
        
        current_value = 0
        
        for val in values:
            name, header_name, value = '', '', 0
            
            if isinstance( val, basestring ):
                self.values.append( self.EnumValue( val, val, current_value ) )

                current_value = value + 1
            else:
                items = val.items()
                assert len(items) == 1

                name, value = items[0]
                header_name = name

                if isinstance( value, int ):
                    name, value = items[0]
                    self.values.append( self.EnumValue( name, name, value ) )
                    current_value = value + 1
                elif isinstance( value, dict ):
                    current_value = value.get( 'value', current_value )
                    self.values.append( self.EnumValue( name, value.get( 'header_name', name ), current_value ) )
                    current_value = current_value + 1
                else:
                    assert False, 'bad type!'
            
        # calculate dl-type-id
        # better typeid-generation plox!
        def hash_buffer( str ):
            hash = 5381
            for char in str:
                hash = (hash * 33) + ord(char)
            return (hash - 5381) & 0xFFFFFFFF;

        id_str = name + json.dumps(values).replace(' ', '')
        self.typeid = hash_buffer( id_str ) # hash_buffer( self.name )
        
    def base_type(self): return self
    def size(self):      return PlatformValue( 4 )
    def align(self):     return PlatformValue( 4 )
        
    def get_value(self, name):
        for val in self.values:
            if val.name == name:
                return val.value
        assert False, 'could not find enum-value %s' % name
        return 0

def create_member( data, typelibrary ):
    assert not 'subtype' in data, 'old format in tld-data'
    
    import re
    '''
        search for symbols describing type.
        
        TODO: Current gives to many positives!
        
        - start with alphanum, starting with alpha-char.
        followed by:
        - [] for array of prev symbol
        - [NUM] for inline array of prev symbol with length NUM
        - * for pointer to prev symbol
    '''
    p = re.compile( r'(\w+)|(\[\d+\])|(\[\])|(\*)' )
    found = p.findall( data['type'].strip() )
    
    mem_type = None
    
    for index, f in enumerate(found):
        if f[0]: # type
            assert index == 0, 'types can only occur as element 0 in type-definition!'
            
            if f[0] == 'bitfield':
                mem_type = BitfieldType( data['bits'] )
            else:
                mem_type = typelibrary.find_type( f[0] )
        
        if f[1]:
            assert index > 0, 'element 0 in type need to be a concrete type!'
            count    = int( f[1][1:-1] )
            mem_type = InlineArrayType( mem_type, count )
            
        if f[2]:
            assert index > 0, 'element 0 in type need to be a concrete type!' 
            mem_type = ArrayType( mem_type )
            
        if f[3]: 
            assert index > 0, 'element 0 in type need to be a concrete type!' 
            mem_type = PointerType( mem_type )
        
    new_member         = Type.Member( mem_type );            
    new_member.name    = data['name']
    new_member.comment = data.get('comment', '')
    if 'default' in data:
        new_member.default = data[ 'default' ]
        if isinstance( new_member.type, PointerType ) and new_member.default != None:
            raise  'only null is supported as default for ptrs!'
        
    return new_member

class Type( object ):
    class Member( object ):
        def __init__(self, type):
            self.type   = type
            self.offset = PlatformValue( 0 )

    
    def __init__(self, name, data, typelibrary):
        def hash_buffer( str ):
            hash = 5381
            for char in str:
                hash = (hash * 33) + ord(char)
            return (hash - 5381) & 0xFFFFFFFF;
        
        if len(name) > 32:
            raise DLException('Type name longer than 32 on type %s' % name)
        
        self.name     = name
        self.typeid   = hash_buffer( name + json.dumps(data).replace(' ', '') )
        self.comment  = data.get('comment', '') 
        self.my_size  = PlatformValue( 0 )
        self.my_align = PlatformValue( data.get('align', 0) )
        
    def base_type(self): return self
    def size(self):      return self.my_size
    def align(self):     return self.my_align
        
    def read_members(self, data, typelibrary):
        # TODO: Handle aliases here!
        self.members = [ create_member( m, typelibrary ) for m in data['members'] ]
        self.finalize_bitfield_members()
        
    def finalize_bitfield_members(self):
        ''' does the final calculations for bitfield-members like calculating storage-type '''
        
        def find_bitfield_groups( list ):
            ret = [ ]
            current = None
            for m in list:
                if isinstance( m.type, BitfieldType ):
                    if not current:
                        current = [ ]
                    
                    current.append( m )
                else:
                    if current:
                        ret.append( current )
                    current = None

            if current:
                ret.append( current )
                
            return ret
        
        def select_storage_type( bits ):
            if bits <= 8:  return BUILTIN_TYPES['uint8']
            if bits <= 16: return BUILTIN_TYPES['uint16']
            if bits <= 32: return BUILTIN_TYPES['uint32']
            if bits <= 64: return BUILTIN_TYPES['uint64']
            assert False, 'TODO: Handle this plox!!!'
        
        size  = 0
        align = 0
        for group in find_bitfield_groups( self.members ):
            bits = 0
            for mem in group:
                bits += mem.type.bits
                mem.type.last_in_group = False
            
            storage_type = select_storage_type( bits )
            group[-1].type.last_in_group = True
            
            bit_offset = 0
            
            for mem in group:
                mem.type.type        = storage_type
                mem.type.bit_offset  = bit_offset
                bit_offset          += mem.type.bits
            
                
        
    def calc_size_and_align(self):
        user_align = self.align()
        
        self.my_size  = PlatformValue()
        self.my_align = PlatformValue()
        
        # calculate member offsets
        for member in self.members:
            member.offset = PlatformValue.align( self.size(), member.type.align() )
            if isinstance( member.type, BitfieldType ) and not member.type.last_in_group:
                continue
            self.my_size  = member.offset + member.type.size()
            self.my_align = PlatformValue.max( self.align(), member.type.align() )
        
        self.useralign = user_align.ptr32 > self.align().ptr32
        
        if self.useralign:
            self.useralign = True
            self.my_align = user_align
        
        self.my_size = PlatformValue.align( PlatformValue.max( self.size(), self.align() ), self.align() )

class TypeLibrary( object ):
    def __init__( self, lib = None ):        
        if lib != None:
            self.enums, self.types, self.type_order = {}, {}, []
            self.read( open(lib, 'r').read() )
        else:
            self.enums, self.types, self.type_order = None, None, None
    
    def read( self, lib ):
        ''' 
            read a typelibrary from either text-format or a binary packed lib.
            
            the function itself will try to determine format
        '''
        '''if lib.startswith(DL_TYPELIB_ID):
            self.from_binary( lib )
        else:'''
        self.from_text( lib )
    
    def from_binary( self, lib ):
        ''' read typelibrary from binary packed lib. '''
        
        ''' will this be doable? for example namespaces and alias:es are not stored in binary '''
        
        raise DLTypeLibraryError( "Not supported right now, will it ever be?" )
        
    def find_type(self, name):
        if name in self.types: return self.types[name]
        if name in self.enums: return self.enums[name]
        return None
    
    def __read_enums( self, enum_data ):
        for name, values in enum_data.items():
            if name in self.enums: raise DLTypeLibraryError( name + ' already in library!' )    
            self.enums[name] = Enum( name, values )
    
    def __read_types( self, type_data ): 
        for name, values in type_data.items():
            if name in self.types: raise DLTypeLibraryError( name + ' already in library!' )
            self.types[name] = Type( name, values, self )
        
        for type in self.types.values():
            if type.name in type_data:
                type.read_members( type_data[type.name], self )
                
    
    def __calc_type_order( self ):
        def ready_to_remove( temp, typename ):
            type = self.types[typename]

            for member in type.members:
                if isinstance( member.type, BitfieldType ):
                    continue
                
                
                # TODO: remove temp-code!
                base_type_name = member.type.base_type().name
                
                if base_type_name is typename:
                    continue
                if base_type_name in temp: 
                    for member_member in self.types[base_type_name].members:
                        other_base_name = member_member.type.base_type().name
                        if typename is other_base_name:
                            return True
                    return False
                
            return True
        
        temp = self.types.keys()
        
        while len(temp) > 0:
            for type in temp: 
                if ready_to_remove( temp, type ):
                    self.type_order.append( type )
                    temp.remove(type)
                    break
    
    def __calc_size_and_align( self ):
        for type_name in self.type_order:
            type = self.types[type_name]
            type.calc_size_and_align()
    
    def from_text( self, lib ):
        ''' read typelibrary from text-format lib. '''
        import re
        
        def replacer(match):
            s = match.group(0)
            return "" if s.startswith('/') else s 
        pattern = re.compile( r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"', re.DOTALL | re.MULTILINE )

        data = json.loads( re.sub(pattern, replacer, lib ) )
        
        self.name = data['module']
        
        for name, type in BUILTIN_TYPES.items():
            self.types[name] = type
        
        if 'enums' in data: self.__read_enums( data['enums'] )
        if 'types' in data: self.__read_types( data['types'] )
        
        # remove builtin types again
        for name, type in BUILTIN_TYPES.items():
            self.types.pop(name)
        
        self.__calc_type_order()
        self.__calc_size_and_align()
    
    def to_binary( self ):
        return 'string with binary typelib'
    
    def to_text( self ):
        assert False, "Not supported right now, will it ever be?"
        return 'string with typelib in json-format'

def read_binary( stream ):
    pass

def read_text( stream ):
    return TypeLibrary()

def read( stream ):
    return read_text( stream );

def compile( typelibrary, stream ):
    ''' create binary lib '''
    
    HEADER_FMT  = '<ccccIIIIII'
    HEADER_SIZE = struct.calcsize(HEADER_FMT)
    
    def build_header( typelibrary, type_lookup, type_data, enum_lookup, enum_data, default_data ):
        header = ''.join( [ struct.pack( '<4sI', 'LTLD', 3 ), # typelibrary identifier and version
                            struct.pack( '<II', len(typelibrary.type_order), len(type_data) ), # type info
                            struct.pack( '<II', len(typelibrary.enums),      len(enum_data) ), # enum info
                            struct.pack( '<I',                               len(default_data) ) ] ) # default data
        
        assert HEADER_SIZE == len(header)
        
        return header
    
    def build_enum_data( typelibrary ):
        lookup, data  = '', ''
        
        logging.debug('ENUMS')
        logging.debug('                                    name          id    offset')
        logging.debug('------------------------------------------------------------------------------------------------')
        
        for enum in typelibrary.enums.values():
            enum_offset = len(data)
            logging.debug('%40s  0x%08X%10u' % (enum.name, enum.typeid, enum_offset))
            lookup += struct.pack('<II', enum.typeid, enum_offset)
            data   += struct.pack( '<32sI', str(enum.name), len(enum.values))
            data   += ''.join( [ struct.pack( '<32sI', str(value.name), value.value ) for value in enum.values ] )
        
        return lookup, data
    
    def build_type_data( typelibrary, def_values ):
        import sys, os
        sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../../bind/python/')))

        import libdl
        
        lookup, data, def_values_out  = '', '', {}
        
        logging.debug('TYPES')
        logging.debug('                                    name          id    offset  size32  size64  align32  align64')
        logging.debug('------------------------------------------------------------------------------------------------')
        for type in typelibrary.types.values():
            type_offset = len(data)
            
            logging.debug( '%40s  0x%08X%10u%8u%8u%9u%9u' % ( type.name, type.typeid, type_offset, 
                                                              type.size().ptr32,  type.size().ptr64,
                                                              type.align().ptr32, type.align().ptr64 ) )
            
            lookup += struct.pack('<II', type.typeid, type_offset)
            logging.debug('%s: 0x%08X -> %s', type.name, type.typeid, type_offset)
            
            data += struct.pack( '<32sIIIII', str(type.name), 
                                              type.size().ptr32, type.size().ptr64,
                                              type.align().ptr32, type.align().ptr64,
                                              len(type.members) )
            
            def to_dl_type( the_type ):
                TYPE_ENUM_MAP = { 'int8'   : libdl.DL_TYPE_STORAGE_INT8,
                                  'int16'  : libdl.DL_TYPE_STORAGE_INT16,
                                  'int32'  : libdl.DL_TYPE_STORAGE_INT32,
                                  'int64'  : libdl.DL_TYPE_STORAGE_INT64,
                                  'uint8'  : libdl.DL_TYPE_STORAGE_UINT8,
                                  'uint16' : libdl.DL_TYPE_STORAGE_UINT16,
                                  'uint32' : libdl.DL_TYPE_STORAGE_UINT32,
                                  'uint64' : libdl.DL_TYPE_STORAGE_UINT64,
                                  'fp32'   : libdl.DL_TYPE_STORAGE_FP32,
                                  'fp64'   : libdl.DL_TYPE_STORAGE_FP64,
                                  'string' : libdl.DL_TYPE_STORAGE_STR }
                
                def atom_type( the_type ):
                    if isinstance( the_type, Type ):            return libdl.DL_TYPE_ATOM_POD
                    if isinstance( the_type, BuiltinType ):     return libdl.DL_TYPE_ATOM_POD
                    if isinstance( the_type, Enum ):            return libdl.DL_TYPE_ATOM_POD
                    if isinstance( the_type, PointerType ):     return libdl.DL_TYPE_ATOM_POD
                    if isinstance( the_type, ArrayType ):       return libdl.DL_TYPE_ATOM_ARRAY
                    if isinstance( the_type, InlineArrayType ): return libdl.DL_TYPE_ATOM_INLINE_ARRAY
                    if isinstance( the_type, BitfieldType ):    return libdl.DL_TYPE_ATOM_BITFIELD
                    assert False, 'missing type (%s)!' % type( the_type )
                    
                def storage_type( the_type ):
                    if isinstance( the_type.base_type(), Enum ):        return libdl.DL_TYPE_STORAGE_ENUM
                    if isinstance( the_type.base_type(), BuiltinType ): return TYPE_ENUM_MAP[member.type.base_type().name]
                    if isinstance( the_type,             PointerType ): return libdl.DL_TYPE_STORAGE_PTR
                    return libdl.DL_TYPE_STORAGE_STRUCT
                
                extra_bits = 0
                if isinstance( the_type, BitfieldType ):
                    extra_bits = libdl.M_INSERT_BITS(extra_bits, the_type.bits,       libdl.DL_TYPE_BITFIELD_SIZE_MIN_BIT,   libdl.DL_TYPE_BITFIELD_SIZE_MAX_BIT + 1)
                    extra_bits = libdl.M_INSERT_BITS(extra_bits, the_type.bit_offset, libdl.DL_TYPE_BITFIELD_OFFSET_MIN_BIT, libdl.DL_TYPE_BITFIELD_OFFSET_MAX_BIT + 1)
                
                return atom_type( the_type ) | storage_type( the_type ) | extra_bits
            
            MEMBER_FMT = '<32sIIIIIIIII'
            
            for member in type.members:
                mem_type = member.type
                data += struct.pack( MEMBER_FMT, str( member.name ), 
                                                 to_dl_type( mem_type ),   mem_type.base_type().typeid,
                                                 mem_type.size().ptr32,    mem_type.size().ptr64,
                                                 mem_type.align().ptr32,   mem_type.align().ptr64,
                                                 member.offset.ptr32,      member.offset.ptr64,
                                                 def_values.get( type.name + '.' + member.name, ( None, 0xFFFFFFFF) )[1] )
                
                if hasattr( member, 'default' ):
                    def_values_out[type.name + '.' + member.name] = ( member, 0xFFFFFFFF )
                
        return lookup, data, def_values_out
    
    def build_default_data( typelib_data, def_values ):
        import os, sys, json, libdl
        sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../bind/python/'))) # TODO: Blarg!!!

        DL_INSTANCE_HEADER_SIZE = 20
        
        # create typelibrary without defaults used to build default-structs
        # TODO: this should maybe use raw dl-dll calls to get rid of the dependency on the python-bindings.
        dl_ctx = libdl.DLContext( typelib_buffer = typelib_data )
        
        default_data = ''
        for key_name in def_values.keys():
            ''' pack them values '''            
            POD_PACK_FMT = { 'int8'   : '<b', 'int16'  : '<h', 'int32'  : '<i', 'int64'  : '<q',
                             'uint8'  : '<B', 'uint16' : '<H', 'uint32' : '<I', 'uint64' : '<Q',
                             'fp32'   : '<f', 'fp64'   : '<d' }
            
            member, offset = def_values[ key_name ] 
            def_values[ key_name ] = ( member, len(default_data) )
            if   isinstance( member.type, BuiltinType ): 
                if member.type.name == 'string':
                    default_data += struct.pack( '<I%us' % ( len( member.default ) + 1 ), len(default_data) + 4, str(member.default) )
                else:
                    default_data += struct.pack( POD_PACK_FMT[member.type.name], member.default )
            elif isinstance( member.type, Enum ):
                default_data += struct.pack( '<I', member.type.get_value( member.default ) )
            elif isinstance( member.type, Type ):
                instance_str =  '{ "type" : "%s", "data" : %s }' % ( member.type.name, str( json.dumps(member.default) ) )
                default_data += dl_ctx.PackText( instance_str )[DL_INSTANCE_HEADER_SIZE:]
            elif isinstance( member.type, ArrayType ) or isinstance( member.type, InlineArrayType ):
                assert not isinstance( member.type, Type ), 'default arrays/inline-arrays of structs not supported yet'
                
                if isinstance( member.type, ArrayType ):
                    default_data += struct.pack('<II', len(default_data) + 8, len(member.default))
                
                the_type = member.type.base_type()
                
                if the_type.name == 'string':
                    curr_offset = len( default_data ) + len( member.default ) * struct.calcsize( '<I' )
                    for string in member.default:
                        default_data += struct.pack( '<I', curr_offset )
                        curr_offset += len( string ) + 1
                    default_data += ''.join( struct.pack( '<%us' % ( len( string ) + 1 ), str( string ) ) for string in member.default )
                elif isinstance( the_type, BuiltinType ): default_data += ''.join( struct.pack(POD_PACK_FMT[the_type.name], val) for val in member.default )
                elif isinstance( the_type, Enum ):        default_data += ''.join( struct.pack('<I', the_type.get_value( val )) for val in member.default )
                else:
                    assert False, 'type not handled? %s' % type( the_type )
            elif isinstance( member.type, PointerType ): default_data += struct.pack('<I', 0xFFFFFFFF)
            else:
                assert False, "type: %s" % type( member )
        
        return default_data
    
    enum_lookup, enum_data             = build_enum_data( typelibrary )
    type_lookup, type_data, def_values = build_type_data( typelibrary, {} )
    header                             = build_header( typelibrary, type_lookup, type_data, enum_lookup, enum_data, '' )
    
    typelib_data = '%s%s%s%s%s' % ( header, type_lookup, type_data, enum_lookup, enum_data )
    
    ''' build default data '''
    default_data = build_default_data( typelib_data, def_values )
    
    type_lookup, type_data, def_values = build_type_data( typelibrary, def_values )
    header                             = build_header( typelibrary, type_lookup, type_data, enum_lookup, enum_data, default_data )
    
    stream.write( '%s%s%s%s%s%s' % ( header, type_lookup, type_data, enum_lookup, enum_data, default_data ) )

def generate( typelibrary, stream ):
    ''' generate json typelibrary definition ( some info might be lost ) '''
    
    lib = {
        'module' : 'mod',
        'enums'  : 'enums',
        'types'  : 'types'
    }
    
    stream.write( json.dumps( lib, indent = 4 ) )
