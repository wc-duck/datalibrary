''' 
    should be lib to parse dl-typelibrary file
    
    dl_header_generate.py with functions:
        generate_cpp, generate_c and generate_cs should use this lib to read tld-file.
'''

from sys import maxint

class PlatformValue( object ):
    def __init__(self, val=(0,0)):
        self.ptr32 = val[0]
        self.ptr64 = val[1]
        
    def __repr__(self):
        return '(' + self.ptr32.__repr__() + ', ' + self.ptr64.__repr__() + ')'
        
    def __add__(self, other):
        ret = PlatformValue()
        ret.ptr32 = self.ptr32 + other.ptr32
        ret.ptr64 = self.ptr64 + other.ptr64
        return ret
    
    def __sub__(self, other):
        ret = PlatformValue()
        ret.ptr32 = self.ptr32 - other.ptr32
        ret.ptr64 = self.ptr64 - other.ptr64
        return ret
    
    def __mul__(self, multiplier):
        ret = PlatformValue()
        ret.ptr32 = self.ptr32 * multiplier
        ret.ptr64 = self.ptr64 * multiplier
        return ret
    
    @staticmethod
    def align(val, align_value):
        def align(val, alignment):
            return (val + alignment - 1) & ~(alignment - 1)
        ret = PlatformValue()
        ret.ptr32 = align(val.ptr32, align_value.ptr32)
        ret.ptr64 = align(val.ptr64, align_value.ptr64)
        return ret
    
    @staticmethod
    def max(val1, val2):
        ret = PlatformValue()
        ret.ptr32 = max(val1.ptr32, val2.ptr32)
        ret.ptr64 = max(val1.ptr64, val2.ptr64)
        return ret

class BuiltinType( object ):
    def __init__(self, name, size, align):
        self.name  = name
        self.size  = PlatformValue(size)
        self.align = PlatformValue(align)

class Enum( object ):
    class EnumValue( object ):
        def __init__(self, name, value):
            self.name  = name
            self.value = value
        
    def __init__(self, name, values):
        self.name   = name
        self.size   = PlatformValue( (4, 4) )
        self.align  = PlatformValue( (4, 4) ) 
        self.values = []
        
        current_value = 0
        for val in values:
            name, value = '', 0
            
            if isinstance( val, basestring ):
                name  = val
                value = current_value
            else:
                items = val.items()
                assert len(items) == 1
                name, value = items[0]
            
            self.values.append( self.EnumValue( name, value ) )
            current_value = value + 1

class Member( object ):
    def __init__(self, name, data):
        self.name    = name
        self.comment = ''
        self.index   = maxint
        if not isinstance( data, basestring ):
            self.comment = data.get('comment', self.comment)
            self.index   = data.get('index',   self.index)
            
    def calc_size_and_align(self, typelibrary):
        pass

class PodMember( Member ):
    def __init__(self, name, data):
        Member.__init__( self, name, data )
        
        if isinstance( data, basestring ):
            self.type = data
        else:
            self.type = data['type']
            
    def calc_size_and_align(self, typelibrary):
        type = typelibrary.find_type(self.type)
        self.size  = type.size
        self.align = type.align
    
class ArrayMember( Member ):
    def __init__(self, name, data):
        Member.__init__( self, name, data )
        
        self.type  = data['subtype']
        self.size  = PlatformValue( (8, 12) )
        self.align = PlatformValue( (4, 8) )
    
class InlineArrayMember( Member ):
    def __init__(self, name, data):
        Member.__init__( self, name, data )
        
        self.type  = data['subtype']
        self.count = data['count']
    
    def calc_size_and_align(self, typelibrary):
        subtype = typelibrary.find_type(self.type)
        self.size  = subtype.size * self.count
        self.align = subtype.align

class BitfieldMember( Member ):
    def __init__(self, name, data):
        Member.__init__( self, name, data )
        
        self.index = data.get('index', -maxint) # if no index, sort bitfields first
        self.bits  = data['bits']
        self.type  = 'bitfield' # TODO: this will be recalculated later on
        self.size  = PlatformValue( (0, 0) )
        self.align = PlatformValue( (0, 0) )

class PointerMember( Member ):
    def __init__(self, name, data):
        Member.__init__( self, name, data )
        
        self.type  = data['subtype']
        self.size  = PlatformValue( (4, 8) )
        self.align = PlatformValue( (4, 8) )
    
def create_member( data ):
    name = data['name']
    type = data['type']

    if type == 'array':        return ArrayMember( name, data )
    if type == 'inline-array': return InlineArrayMember( name, data )
    if type == 'bitfield':     return BitfieldMember( name, data )
    if type == 'pointer':      return PointerMember( name, data )
    return PodMember( name, data )

class Type( object ):
    def __init__(self, name, data):    
        self.name    = name
        self.typeid  = 0
        self.comment = data.get('comment', '')
        align        = data.get('align', 0) 
        self.size    = PlatformValue( (0, 0) )
        self.align   = PlatformValue( (align, align) )
        # TODO: Handle aliases here!
        self.members = [ create_member( m ) for m in data['members'] ]
        
        # calculate dl-type-id
        # better typeid-generation plox!
        def hash_buffer( str ):
            hash = 5381
            for char in str:
                hash = (hash * 33) + ord(char)
            return (hash - 5381) & 0xFFFFFFFF;
        
        self.typeid = hash_buffer( self.name )
        
    def finalize_bitfield_members(self):
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
        
        def select_storage_type( bits ):
            if bits <= 8:  return 'uint8'
            if bits <= 16: return 'uint16'
            if bits <= 32: return 'uint32'
            if bits <= 64: return 'uint64'
            assert False, 'TODO: Handle this plox!!!'
        
        for group in find_bitfield_groups( self.members ):
            bits = 0
            for mem in group:
                bits += mem.bits
            
            storage_type = select_storage_type( bits )
            
            for mem in group:
                mem.type = storage_type
            
                
        
    def calc_size_and_align(self, typelibrary):
        for member in self.members:
            member.calc_size_and_align(typelibrary)

        self.finalize_bitfield_members()
        
        user_align = self.align
        
        self.size  = PlatformValue()
        self.align = PlatformValue()
        # calculate member offsets
        for member in self.members:
            member.offset  = PlatformValue.align( self.size, member.align )
            self.size  = member.offset + member.size
            self.align = PlatformValue.max( self.align, member.align )
        
        self.useralign = user_align.ptr32 > self.align.ptr32
        
        self.size = PlatformValue.max( self.size, self.align )
        if self.useralign:
            self.useralign = True
            self.align = user_align

class TypeLibrary( object ):
    def __init__( self, lib = None ):        
        if lib != None:
            self.enums = {} # same map as types?
            self.types = {}
            self.type_order = []
            
            self.read( open(lib, 'r').read() )
        else:
            self.enums = None
            self.types = None
            self.type_order = None
    
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
        
        assert False, "Not supported right now, will it ever be?"
        
    def find_type(self, name):
        if name in self.types:
            return self.types[name]
        if name in self.enums:
            return self.enums[name]
        
        return None
    
    def __read_enums( self, enum_data ):
        for name, values in enum_data.items():
            assert name not in self.enums, name + ' already in library!' # handle this with exception!    
            self.enums[name] = Enum( name, values )
    
    def __read_types( self, type_data ): 
        for name, values in type_data.items():
            assert name not in self.types, name + ' already in library!' # handle this with exception!
            self.types[name] = Type( name, values )
    
    def __calc_type_order( self ):
        def ready_to_remove( temp, type ):
            for member in self.types[type].members:
                if ( not member.type is type ) and ( not member.type in temp ):
                    return True
                
            return False
        
        temp = self.types.keys()
        
        while len(temp) > 0:
            for type in temp:    
                if ready_to_remove( temp, type ):
                    self.type_order.append( type )
                    temp.remove(type)
                    break
    
    def __add_builtin_types( self ):
        self.types['int8']   = BuiltinType('int8',   (1, 1), (1, 1))
        self.types['int16']  = BuiltinType('int16',  (2, 2), (2, 2))
        self.types['int32']  = BuiltinType('int32',  (4, 4), (4, 4))
        self.types['int64']  = BuiltinType('int64',  (8, 8), (8, 8))
        self.types['uint8']  = BuiltinType('uint8',  (1, 1), (1, 1))
        self.types['uint16'] = BuiltinType('uint16', (2, 2), (2, 2))
        self.types['uint32'] = BuiltinType('uint32', (4, 4), (4, 4))
        self.types['uint64'] = BuiltinType('uint64', (8, 8), (8, 8))
        self.types['fp32']   = BuiltinType('fp32',   (4, 4), (4, 4))
        self.types['fp64']   = BuiltinType('fp64',   (8, 8), (8, 8))
        self.types['string'] = BuiltinType('string', (4, 8), (4, 8))
    
    def __calc_size_and_align( self ):
        PTR_SIZE32 = 0
        PTR_SIZE64 = 1
        
        for type_name in self.type_order:
            type = self.types[type_name]
            
            type.calc_size_and_align(self)
            
            #print 'calc for:', type.name
            #print '\t size:', type.size, 'align:', type.align
            for member in type.members:
                member.calc_size_and_align(self)
                #print '\t\t', member.name, 'size:', member.size, 'align:', member.align, 'index:', member.index
    
    def from_text( self, lib ):
        ''' read typelibrary from text-format lib. '''
        import json, re
        
        def replacer(match):
            s = match.group(0)
            return "" if s.startswith('/') else s 
        pattern = re.compile( r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"', re.DOTALL | re.MULTILINE )

        data = json.loads( re.sub(pattern, replacer, lib ) )
        
        self.name = data['module_name'] # TODO: rename to 'name' when all is converted to this codepath
        
        # TODO: rename to 'enums' when all is converted to this codepath
        if 'module_enums' in data:
            self.__read_enums( data['module_enums'] )
        
        # TODO: rename to 'types' when all is converted to this codepath
        if 'module_types' in data:
            self.__read_types( data['module_types'] )
        
        self.__calc_type_order()
        
        self.__add_builtin_types()
        
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
    pass

def generate( typelibrary, stream ):
    ''' generate json typelibrary definition ( some info might be lost ) '''
    pass
