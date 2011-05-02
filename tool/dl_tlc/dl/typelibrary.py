''' 
    should be lib to parse dl-typelibrary file
    
    dl_header_generate.py with functions:
        generate_cpp, generate_c and generate_cs should use this lib to read tld-file.
'''

import logging
import struct

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
        self.name   = name
        self.size   = PlatformValue(size)
        self.align  = PlatformValue(align)
        self.typeid = 0
        
    def read_members(self, data, typelibrary):
        pass

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
        self.size   = PlatformValue( 4 )
        self.align  = PlatformValue( 4 ) 
        self.values = []
        
        current_value = 0
        counted_enum_type = 1
        counted_enum_max_count = 0
        
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
        
        self.type_mask  = 0
        self.count_mask = 0

        if counted_enum_max_count > 0:
            import math
            hi_bit = math.floor( math.log( counted_enum_max_count, 2 ) )
            self.count_mask = ( 1 << ( int(hi_bit) + 1 ) ) - 1
            self.type_mask  = 0xFFFFFFFF - self.count_mask
            
        # calculate dl-type-id
        # better typeid-generation plox!
        def hash_buffer( str ):
            hash = 5381
            for char in str:
                hash = (hash * 33) + ord(char)
            return (hash - 5381) & 0xFFFFFFFF;
        
        self.typeid = hash_buffer( self.name )
        
    def get_value(self, name):
        for val in self.values:
            if val.name == name:
                return val.value
        assert False, 'could not find enum-value %s' % name
        return 0

class Member( object ):
    def __init__(self, name, data):
        self.name    = name
        self.comment = data.get('comment', '')
        if 'default' in data:
            self.default = data['default']
            
    def calc_size_and_align(self, typelibrary):
        pass

class PodMember( Member ):
    def __init__(self, name, data, typelibrary):
        Member.__init__( self, name, data )
        self.type = typelibrary.find_type( data['type'] )
            
    def calc_size_and_align(self, typelibrary):
        self.size  = self.type.size
        self.align = self.type.align
    
class ArrayMember( Member ):
    def __init__(self, name, data, typelibrary):
        Member.__init__( self, name, data )
        self.type = typelibrary.find_type( data['subtype'] )    
        self.size  = PlatformValue( (8, 12) )
        self.align = PlatformValue( (4, 8) )
    
class InlineArrayMember( Member ):
    def __init__(self, name, data, typelibrary):
        Member.__init__( self, name, data )
        self.type = typelibrary.find_type( data['subtype'] )
        self.count = data['count']
    
    def calc_size_and_align(self, typelibrary):
        self.size  = self.type.size * self.count
        self.align = self.type.align

class BitfieldMember( Member ):
    def __init__(self, name, data, typelibrary):
        from sys import maxint
        
        Member.__init__( self, name, data )
        
        self.index = data.get('index', -maxint) # if no index, sort bitfields first
        self.bits  = data['bits']
        self.type  = 'bitfield' # TODO: this will be recalculated later on
        self.size  = PlatformValue( 0 )
        self.align = PlatformValue( 0 )

class PointerMember( Member ):
    def __init__(self, name, data, typelibrary):
        Member.__init__( self, name, data )
        
        if hasattr( self, 'default' ) and self.default != None:
            raise  'only null is supported as default for ptrs!'
        
        self.type = typelibrary.find_type( data['subtype'] )
        self.size  = PlatformValue( (4, 8) )
        self.align = PlatformValue( (4, 8) )
    
def create_member( data, typelibrary ):
    name = data['name']
    type = data['type']

    if type == 'array':        return ArrayMember( name, data, typelibrary )
    if type == 'inline-array': return InlineArrayMember( name, data, typelibrary )
    if type == 'bitfield':     return BitfieldMember( name, data, typelibrary )
    if type == 'pointer':      return PointerMember( name, data, typelibrary )
    return PodMember( name, data, typelibrary )

class Type( object ):
    def __init__(self, name, data, typelibrary):    
        self.name    = name
        self.typeid  = 0
        self.comment = data.get('comment', '') 
        self.size    = PlatformValue( 0 )
        self.align   = PlatformValue( data.get('align', 0) )
        
    def read_members(self, data, typelibrary):
        # TODO: Handle aliases here!
        self.members = [ create_member( m, typelibrary ) for m in data['members'] ]
        
        if len(self.name) > 32:
            raise DLException('Type name longer than 32 on type %s' % self.name)
        
        # calculate dl-type-id
        # better typeid-generation plox!
        def hash_buffer( str ):
            hash = 5381
            for char in str:
                hash = (hash * 33) + ord(char)
            return (hash - 5381) & 0xFFFFFFFF;
        
        self.typeid = hash_buffer( self.name )
        
    def finalize_bitfield_members(self, typelib):
        ''' does the final calculations for bitfield-members like calculating storage-type '''
        
        def find_bitfield_groups( list ):
            ret = [ ]
            current = None
            for m in list:
                if isinstance( m, BitfieldMember ):
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
        
        def select_storage_type( bits, typelib ):
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
                bits += mem.bits
                mem.last_in_group = False
            
            storage_type = select_storage_type( bits, typelib )
            group[-1].last_in_group = True
            
            bit_offset = 0
            
            for mem in group:
                mem.type        = storage_type
                mem.size        = storage_type.size
                mem.align       = storage_type.align
                mem.bit_offset  = bit_offset
                bit_offset     += mem.bits
            
                
        
    def calc_size_and_align(self, typelibrary):
        for member in self.members:
            member.calc_size_and_align(typelibrary)

        self.finalize_bitfield_members(typelibrary)
        
        user_align = self.align
        
        self.size  = PlatformValue()
        self.align = PlatformValue()
        # calculate member offsets
        for member in self.members:
            member.offset = PlatformValue.align( self.size, member.align )
            if isinstance( member, BitfieldMember ) and not member.last_in_group:
                continue
            self.size  = member.offset + member.size
            self.align = PlatformValue.max( self.align, member.align )
        
        self.useralign = user_align.ptr32 > self.align.ptr32
        
        if self.useralign:
            self.useralign = True
            self.align = user_align
        
        self.size = PlatformValue.align( PlatformValue.max( self.size, self.align ), self.align )

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
                if isinstance( member, BitfieldMember ):
                    continue
                if member.type.name is typename:
                    continue
                if member.type.name in temp:
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
            
            type.calc_size_and_align(self)
            
            for member in type.members:
                member.calc_size_and_align(self)
    
    def from_text( self, lib ):
        ''' read typelibrary from text-format lib. '''
        import json, re
        
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
        header = ''.join( [ struct.pack( '<4sI', 'LTLD', 2 ), # typelibrary identifier and version
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
            data   += struct.pack( '<32sII', str(enum.name), enum.typeid, len(enum.values))
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
                                                              type.size.ptr32,  type.size.ptr64,
                                                              type.align.ptr32, type.align.ptr64 ) )
            
            lookup += struct.pack('<II', type.typeid, type_offset)
            logging.debug('%s: 0x%08X -> %s', type.name, type.typeid, type_offset)
            
            data += struct.pack( '<32sIIIII', str(type.name), 
                                              type.size.ptr32, type.size.ptr64,
                                              type.align.ptr32, type.align.ptr64,
                                              len(type.members) )
            
            def to_dl_type( member ):
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
                
                def atom_type( member ):
                    if isinstance( member, PodMember ):         return libdl.DL_TYPE_ATOM_POD
                    if isinstance( member, PointerMember ):     return libdl.DL_TYPE_ATOM_POD
                    if isinstance( member, ArrayMember ):       return libdl.DL_TYPE_ATOM_ARRAY
                    if isinstance( member, InlineArrayMember ): return libdl.DL_TYPE_ATOM_INLINE_ARRAY
                    if isinstance( member, BitfieldMember ):    return libdl.DL_TYPE_ATOM_BITFIELD
                    assert False, 'missing type (%s)!' % type( member )
                    
                def storage_type( member ):
                    if isinstance( member.type, Enum ):           return libdl.DL_TYPE_STORAGE_ENUM
                    if isinstance( member.type, BuiltinType ):    return TYPE_ENUM_MAP[member.type.name]
                    if isinstance( member,      PointerMember ):  return libdl.DL_TYPE_STORAGE_PTR
                    return libdl.DL_TYPE_STORAGE_STRUCT
                
                extra_bits = 0
                if isinstance( member, BitfieldMember ):
                    extra_bits = libdl.M_INSERT_BITS(extra_bits, member.bits,       libdl.DL_TYPE_BITFIELD_SIZE_MIN_BIT,   libdl.DL_TYPE_BITFIELD_SIZE_MAX_BIT + 1)
                    extra_bits = libdl.M_INSERT_BITS(extra_bits, member.bit_offset, libdl.DL_TYPE_BITFIELD_OFFSET_MIN_BIT, libdl.DL_TYPE_BITFIELD_OFFSET_MAX_BIT + 1)
                
                return atom_type( member ) | storage_type( member ) | extra_bits
            
            MEMBER_FMT = '<32sIIIIIIIII'
            
            for member in type.members:
                data += struct.pack( MEMBER_FMT, str( member.name ), 
                                                 to_dl_type( member ), member.type.typeid,
                                                 member.size.ptr32,    member.size.ptr64,
                                                 member.align.ptr32,   member.align.ptr64,
                                                 member.offset.ptr32,  member.offset.ptr64,
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
            if isinstance( member, PodMember ):
                if   member.type.name == 'string':           default_data += struct.pack( '<I%us' % ( len( member.default ) + 1 ), len(default_data) + 4, str(member.default) )
                elif isinstance( member.type, BuiltinType ): default_data += struct.pack( POD_PACK_FMT[member.type.name], member.default )
                elif isinstance( member.type, Enum ):        default_data += struct.pack( '<I', member.type.get_value( member.default ) )
                elif isinstance( member.type, Type ):
                    pass 
                    instance_str =  '{ "type" : "%s", "data" : %s }' % ( member.type.name, str( json.dumps(member.default) ) )
                    default_data += dl_ctx.PackText( instance_str )[DL_INSTANCE_HEADER_SIZE:]
                else:
                    assert False, 'type not handled? %s' % type( member.type )
            elif isinstance( member, ArrayMember ) or isinstance( member, InlineArrayMember ):
                assert not isinstance( member.type, Type ), 'default arrays/inline-arrays of structs not supported yet'
                
                if isinstance( member, ArrayMember ):
                    default_data += struct.pack('<II', len(default_data) + 8, len(member.default))
                
                if   member.type.name == 'string':
                    curr_offset = len( default_data ) + len( member.default ) * struct.calcsize( '<I' )
                    for string in member.default:
                        default_data += struct.pack( '<I', curr_offset )
                        curr_offset += len( string ) + 1
                    default_data += ''.join( struct.pack( '<%us' % ( len( string ) + 1 ), str( string ) ) for string in member.default )
                elif isinstance( member.type, BuiltinType ): default_data += ''.join( struct.pack(POD_PACK_FMT[member.type.name], val) for val in member.default )
                elif isinstance( member.type, Enum ):        default_data += ''.join( struct.pack('<I', member.type.get_value( val )) for val in member.default )
                else:
                    assert False, 'type not handled? %s' % type( member.type )
            elif isinstance( member, PointerMember ): default_data += struct.pack('<I', 0xFFFFFFFF)
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
    
    assert False, 'implement me!!!'
