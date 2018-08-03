''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

'''
    DL-binding for python
'''

from ctypes import *
import platform, os, sys, logging

# Atomic types
DL_TYPE_ATOM_POD          = 1
DL_TYPE_ATOM_ARRAY        = 2
DL_TYPE_ATOM_INLINE_ARRAY = 3
DL_TYPE_ATOM_BITFIELD     = 4

# Storage type
DL_TYPE_STORAGE_INT8   =  1
DL_TYPE_STORAGE_INT16  =  2
DL_TYPE_STORAGE_INT32  =  3
DL_TYPE_STORAGE_INT64  =  4
DL_TYPE_STORAGE_UINT8  =  5
DL_TYPE_STORAGE_UINT16 =  6
DL_TYPE_STORAGE_UINT32 =  7
DL_TYPE_STORAGE_UINT64 =  8
DL_TYPE_STORAGE_FP32   =  9
DL_TYPE_STORAGE_FP64   = 10
DL_TYPE_STORAGE_STR    = 11
DL_TYPE_STORAGE_PTR    = 12
DL_TYPE_STORAGE_STRUCT = 13
DL_TYPE_STORAGE_ENUM   = 14

DL_STORAGE_TO_NAME = { DL_TYPE_STORAGE_INT8   : 'int8',
                       DL_TYPE_STORAGE_INT16  : 'int16',
                       DL_TYPE_STORAGE_INT32  : 'int32',
                       DL_TYPE_STORAGE_INT64  : 'int64',
                       DL_TYPE_STORAGE_UINT8  : 'uint8',
                       DL_TYPE_STORAGE_UINT16 : 'uint16',
                       DL_TYPE_STORAGE_UINT32 : 'uint32',
                       DL_TYPE_STORAGE_UINT64 : 'uint64',
                       DL_TYPE_STORAGE_FP32   : 'fp32',
                       DL_TYPE_STORAGE_FP64   : 'fp64',
                       DL_TYPE_STORAGE_ENUM   : 'uint32',
                 
                       DL_TYPE_STORAGE_STR    : 'string' }

class ptr_wrapper():
    pass

# TODO: should there be a subclass each for correct dl-errors?
class DLError(Exception):
    def __init__(self, err, value):
        self.err   = err
        self.value = value
    def __str__(self):
        return '%s(0x%08X)' % ( self.err, self.value )

def host_ptr_size():
    return c_size_t(4) if platform.architecture()[0] == '32bit' else c_size_t(8)

class DLContext:
    class dl_cache_entry:
        def __init__( self, type_id, members, c_type, py_type ):
            self.type_id = type_id
            self.members = members
            self.c_type  = c_type
            self.py_type = py_type
            
    class dl_type_context_info(Structure): _fields_ = [ ('num_types', c_uint),   ('num_enums',   c_uint) ]
    class dl_instance_info(Structure):     _fields_ = [ ('load_size', c_uint),   ('ptrsize',     c_uint), ('endian',     c_uint), ('root_type',    c_uint32) ]
    class dl_type_info(Structure):         _fields_ = [ ('tid',       c_uint),   ('name',      c_char_p), ('size',       c_uint),  ('alignment', c_uint), ('member_count', c_uint) ]
    class dl_enum_info(Structure):         _fields_ = [ ('tid',       c_uint),   ('name',      c_char_p), ('value_count', c_uint) ]
    class dl_enum_value_info(Structure):   _fields_ = [ ('name',      c_char_p), ('value',       c_uint) ]
    class dl_member_info(Structure):
        _fields_ = [ ('name',        c_char_p),
                     ('atom',        c_uint32), 
                     ('storage',     c_uint32), 
                     ('type_id',     c_uint32),
                     ('size',        c_uint32),
                     ('alignment',   c_uint32),
                     ('offset',      c_uint32), 
                     ('array_count', c_uint32),
                     ('bits',        c_uint32) ]
    
    class dl_type(object):
        def __init__(self):
            for ( name, type ) in self._dl_members:
                if isinstance(type, tuple): # TODO: buhubuhu!!!!
                    setattr(self, name, [ type[1]() for i in range(type[2]) ])
                else:
                    setattr(self, name, type())
        
        def __eq__(self, other):
            return isinstance(other, type(self)) and self.__dict__ == other.__dict__
        
    def __get_ctypes_type( self, member_info ):
        storage_type = member_info.storage
        
        if storage_type == DL_TYPE_STORAGE_PTR:
            return None

        type_name = self.__get_type_name( storage_type, member_info.type_id )
        
        if not type_name in self.type_cache:
            self.__cache_type(member_info.type_id)
            
        c_type = self.type_cache[ type_name ].c_type
        
        atom_type = member_info.atom

        if atom_type == DL_TYPE_ATOM_POD:
            if storage_type == DL_TYPE_STORAGE_PTR:
                return POINTER( c_type )
            return c_type 
        elif atom_type == DL_TYPE_ATOM_INLINE_ARRAY: return c_type * member_info.array_count 
        elif atom_type == DL_TYPE_ATOM_ARRAY:
            class dl_array( Structure ):
                _fields_ = [ ( 'data',  POINTER( c_type ) ), 
                             ( 'count', c_uint32 ) ]
            return dl_array
        elif atom_type == DL_TYPE_ATOM_BITFIELD: return c_type 
        else:
            return None
        
    def __get_python_type(self, member_info):
        storage_type = member_info.storage
        if storage_type == DL_TYPE_STORAGE_PTR:
            return None

        type_name = self.__get_type_name( storage_type, member_info.type_id )
            
        if not type_name in self.type_cache:
            self.__cache_type(typeid)
            
        py_type = self.type_cache[ type_name ].py_type
        
        if member_info.atom == DL_TYPE_ATOM_POD:
            if storage_type == DL_TYPE_STORAGE_PTR:
                return ptr_wrapper
            return py_type
        elif member_info.atom == DL_TYPE_ATOM_INLINE_ARRAY: return ( list, py_type, member_info.array_count )
        elif member_info.atom == DL_TYPE_ATOM_ARRAY:        return list
        elif member_info.atom == DL_TYPE_ATOM_BITFIELD:     return int # TODO: need to be long if bitfield-size is to big
        else:
            return None
                
    def __py_type_to_ctype( self, instance ):
        typename = self.type_info_cache[ instance.TYPE_ID ].name
        typeinfo = self.type_cache[typename]
        
        c_instance = typeinfo.c_type() 

        for member_name in typeinfo.members:
            member = getattr( instance, member_name )
            
            if isinstance( member, self.dl_type ):
                setattr( c_instance, member_name, self.__py_type_to_ctype( member ) )
            elif isinstance(member, list):
                c_member = getattr( c_instance, member_name )
                
                conv_member = None
                if len(member) == 0:
                    pass #handle empty arrays
                elif isinstance( member[0], self.dl_type ):
                    conv_member = [ self.__py_type_to_ctype(inst) for inst in member ]
                else:
                    conv_member = member
                        
                if hasattr( c_member, 'data' ):
                    if len( member ) == 0:
                        c_member.data  = None
                        c_member.count = 0
                    else:
                        c_member.data  = ( c_member.data._type_ * len( member ) )( *conv_member )
                        c_member.count = len( member )
                else: # inline array
                    setattr( c_instance, member_name, type(c_member)( *conv_member ) )
            elif isinstance(member, ptr_wrapper):
                setattr( c_instance, member_name, None )
            else:
                setattr( c_instance, member_name, member )
        
        return c_instance
    
    def __ctype_to_py_type( self, instance ):
        typename = self.type_info_cache[ instance.TYPE_ID ].name
        typeinfo = self.type_cache[typename]
        
        py_instance = typeinfo.py_type() 
        
        for member_name in typeinfo.members:
            member = getattr( instance, member_name )
            member_val = None
            
            if type(member).__name__ == 'dl_array': # TODO: cache type to look for!
                if hasattr( member.data[0], 'TYPE_ID' ): # TODO: Ugly check for convertable!
                    member_val = [ self.__ctype_to_py_type(member.data[i]) for i in range(0, member.count) ] # todo can this be dene better?
                else:
                    member_val = [ member.data[i] for i in range(0, member.count) ] # todo can this be dene better?
            elif hasattr(member, '_length_'): # TODO: bad check here!
                if hasattr( member[0], 'TYPE_ID' ): # TODO: Ugly check for convertable!
                    member_val = [ self.__ctype_to_py_type( member[i] ) for i in range(0, len(member)) ] # todo can this be dene better?
                else:
                    member_val = [ member[i] for i in range(0, len(member)) ] # todo can this be dene better?
            else:
                if hasattr( member, 'TYPE_ID' ): # TODO: Ugly check for convertable!
                    member_val = self.__ctype_to_py_type( member )
                else:
                    member_val = member
                
            setattr( py_instance, member_name, member_val )
        
        return py_instance
    
    def try_default_dl_dll( self ):
        '''
            load .so/.dll
            
            search order:
            1) check for "--dl-dll"-flag in argv for path
            2) check cwd
            3) check same path as libdl.py
            4) system paths ( on linux? )
        '''
        
        def find_dldll_in_argv():
            for arg in sys.argv: 
                if arg.startswith( "--dldll" ): 
                    return arg.split('=')[1]
            return ""
        
        DLL_NAME = 'dl'
        if sys.platform in [ 'win32', 'win64' ]:
            DLL_NAME += '.dll'
        else:
            DLL_NAME += '.so'
    
        dl_path_generators = [ 
            find_dldll_in_argv, 
            lambda: os.path.join( os.path.dirname(__file__), DLL_NAME ),
            lambda: os.path.join( os.getcwd(), DLL_NAME ) ]
        
        for dl_path in dl_path_generators:
            path = dl_path()
            
            logging.debug( 'trying to load dl-shared library from: %s' % path )
            
            if( os.path.exists( path ) ):
                logging.debug( 'loading dl-shared library from: %s' % path )
                return path
                
        return None
        
    def __init__( self, typelib_buffer = None, typelib_file = None, dll_path = None ):
        if not dll_path:
            dll_path = self.try_default_dl_dll() 
        if not dll_path:
            raise DLError( 'could not find dl dynamic library', 0 )

        self.dl = CDLL( dll_path )
        self.dl.dl_error_to_string.restype = c_char_p
        self.dl_ctx = c_void_p(0)
        
        def dl_msg_handler( msg, userdata ):
            print msg
            
        self.CDL_MSG_HANDLER = CFUNCTYPE( None, c_char_p, c_void_p )
        self.msg_handler = self.CDL_MSG_HANDLER(dl_msg_handler)
        
        class dl_create_params(Structure):
            _fields_ = [ ('alloc_func',     c_void_p), 
                         ('free_func',      c_void_p), 
                         ('alloc_ctx',      c_void_p),
                         ('error_msg_func', self.CDL_MSG_HANDLER),
                         ('error_msg_ctx',  c_void_p) ]
            
        params = dl_create_params()
        params.alloc_func = 0
        params.free_func  = 0
        params.alloc_ctx  = 0
        params.error_msg_func = self.msg_handler
        params.error_msg_ctx  = 0
        err = self.dl.dl_context_create( byref(self.dl_ctx), byref(params) )
        
        if err != 0:
            raise DLError('Could not create dl_ctx', err)
        
        class dummy( object ): pass
        
        self.type_cache = {}
        self.type_info_cache = {}
        self.types = dummy()
        self.enums = dummy()
        
        ''' bootstrap type cache '''
        self.type_cache['int8']   = self.dl_cache_entry( 0, [], c_int8,   type(c_int8().value)   )
        self.type_cache['int16']  = self.dl_cache_entry( 0, [], c_int16,  type(c_int16().value)  )
        self.type_cache['int32']  = self.dl_cache_entry( 0, [], c_int32,  type(c_int32().value)  )
        self.type_cache['int64']  = self.dl_cache_entry( 0, [], c_int64,  type(c_int64().value)  )
        self.type_cache['uint8']  = self.dl_cache_entry( 0, [], c_uint8,  type(c_int8().value)   )
        self.type_cache['uint16'] = self.dl_cache_entry( 0, [], c_uint16, type(c_int16().value)  )
        self.type_cache['uint32'] = self.dl_cache_entry( 0, [], c_uint32, type(c_int32().value)  )
        self.type_cache['uint64'] = self.dl_cache_entry( 0, [], c_uint64, type(c_int64().value)  )
        self.type_cache['fp32']   = self.dl_cache_entry( 0, [], c_float,  type(c_float().value)  )
        self.type_cache['fp64']   = self.dl_cache_entry( 0, [], c_double, type(c_double().value) )
        self.type_cache['string'] = self.dl_cache_entry( 0, [], c_char_p, str )
        
        if typelib_buffer != None: self.LoadTypeLibrary(typelib_buffer)
        if typelib_file   != None: self.LoadTypeLibraryFromFile(typelib_file)
        
    def dl_dll_call( self, func_name, *args ):
        err = getattr( self.dl, func_name )( self.dl_ctx, *args )
        if err != 0:
            raise DLError( 'Call to %s failed with error %s' % ( func_name, self.dl.dl_error_to_string( err ) ), err )
        
    def context_destroy( self ):                  self.dl_dll_call( 'dl_context_destroy' )
    def context_load_type_library(self, *args):   self.dl_dll_call( 'dl_context_load_type_library', *args )
    def load_txt_type_library(self, *args):       self.dl_dll_call( 'dl_context_load_txt_type_library', *args )
    def write_type_library(self, *args):          self.dl_dll_call( 'dl_context_write_type_library', *args )
    def write_type_library_c_header(self, *args): self.dl_dll_call( 'dl_context_write_type_library_c_header', *args )
    def instance_load( self, *args ):             self.dl_dll_call( 'dl_instance_load',             *args )
    def instance_store( self, *args ):            self.dl_dll_call( 'dl_instance_store',            *args )
    def convert( self, *args ):                   self.dl_dll_call( 'dl_convert',                   *args )
    def txt_pack( self, *args ):                  self.dl_dll_call( 'dl_txt_pack',                  *args )
    def txt_unpack( self, *args ):                self.dl_dll_call( 'dl_txt_unpack',                *args )
    def reflect_context_info( self, *args):       self.dl_dll_call( 'dl_reflect_context_info',      *args )
    def reflect_loaded_types( self, *args ):      self.dl_dll_call( 'dl_reflect_loaded_types',      *args )
    def reflect_loaded_enums( self, *args ):      self.dl_dll_call( 'dl_reflect_loaded_enums',      *args )
    def reflect_get_type_members( self, *args ):  self.dl_dll_call( 'dl_reflect_get_type_members',  *args )
    def reflect_get_enum_values( self, *args ):   self.dl_dll_call( 'dl_reflect_get_enum_values',   *args )
        
    def __del__(self):
        self.context_destroy()
        self.dl = None
    
    def __get_type_name( self, storage_type, typeid ):
        if storage_type in DL_STORAGE_TO_NAME:
            return DL_STORAGE_TO_NAME[ storage_type ]
        else:
            return self.type_info_cache[ typeid ].name
    
    def __cache_type( self, typeid ):
        type_info = self.type_info_cache[typeid]
        
        if type_info.name in self.type_cache:
            return

        member_info = ( self.dl_member_info * type_info.member_count )()
        self.reflect_get_type_members( typeid, member_info, type_info.member_count )
        
        members, c_members, py_members = [], [], []
        
        for member in member_info:
            members.append( member.name )            
            if member.atom == DL_TYPE_ATOM_BITFIELD: # TODO: Do not like this!
                c_members.append ( ( member.name, self.__get_ctypes_type( member ), member.bits ) )
            else:
                c_members.append ( ( member.name, self.__get_ctypes_type( member ) ) )
                
            py_members.append( ( member.name, self.__get_python_type( member ) ) )
    
        complete = True
        for m in c_members:
            if m[1] == None:
                complete = False
                break;

        c_type, py_type = None, None
        # build py-type
        if complete:
            c_type  = type( 'c.'  + type_info.name, (Structure, ),    { 'TYPE_ID' : typeid, '_fields_'    : c_members } )
            py_type = type( 'py.' + type_info.name, (self.dl_type, ), { 'TYPE_ID' : typeid, '_dl_members' : py_members } )
        
        # cache it!
        self.type_cache[type_info.name] = self.dl_cache_entry( typeid, members, c_type, py_type )
        
        setattr( self.types, type_info.name, py_type )

    def _load_types(self):
        ctx_info = self.dl_type_context_info()
        self.reflect_context_info( byref(ctx_info) )
        
        loaded_types = (self.dl_type_info * ctx_info.num_types)()
        loaded_enums = (self.dl_enum_info * ctx_info.num_enums)()
        self.reflect_loaded_types( byref(loaded_types), ctx_info.num_types )                
        self.reflect_loaded_enums( byref(loaded_enums), ctx_info.num_enums )
        
        for t in loaded_types:
            self.type_info_cache[ t.tid ] = t
            self.__cache_type( t.tid )
        
        for e in loaded_enums:
            enum_values = ( e.value_count * self.dl_enum_value_info )()
            self.reflect_get_enum_values( e.tid, enum_values, e.value_count );
            
            values = {}
            for value in enum_values:
                values[value.name] = value.value

            setattr( self.enums, e.name, type( e.name, (), values )() )

    def LoadTypeLibrary(self, _DataBuffer):
        '''
            Loads a binary typelibrary into the dl_ctx

            _DataBuffer -- string with the binary file loaded.
        '''
        
        self.context_load_type_library( _DataBuffer, len(_DataBuffer) )
        self._load_types()
    
    def LoadTypeLibraryFromFile(self, lib_file):
        '''
            Loads a binary typelibrary into the dl_ctx

            lib_file -- filename of file to load typelibrary from.
        '''
        self.LoadTypeLibrary(open(lib_file, 'rb').read())

    def LoadTypeLibraryTxt(self, data_buffer):
        '''
            Loads a text typelibrary into the dl_ctx

            data_buffer -- string with the binary file loaded.
        '''
        self.load_txt_type_library( data_buffer, len(data_buffer) )
        self._load_types()

    def LoadTypeLibraryTxtFromFile(self, lib_file):
        '''
            Loads a text typelibrary into the dl_ctx

            lib_file -- filename of file to load typelibrary from.
        '''
        self.LoadTypeLibraryTxt(open(lib_file, 'rb').read())

    def Save(self):
        store_size = c_size_t(0)
        self.write_type_library( 0, 0, byref(store_size) )
        packed_data = ( c_ubyte * store_size.value )()
        self.write_type_library( packed_data, store_size, c_void_p(0) )
        return string_at( packed_data, len(packed_data) )

    def CHeader(self, module_name):
        store_size = c_size_t(0)
        self.write_type_library_c_header( module_name, 0, 0, byref(store_size) )
        packed_data = ( c_ubyte * store_size.value )()
        self.write_type_library_c_header( module_name, packed_data, store_size, c_void_p(0) )
        return string_at( packed_data, len(packed_data) )
    
    def LoadInstance(self, type_name, data_buffer):
        '''
            Load an instance of a DL-type from a buffer.
            
            type_name   -- name of type.
            data_buffer -- string containing buffer to load from.
        '''
        type_info = self.type_cache[type_name]
        
        unpacked_data = ( c_ubyte * len(data_buffer) )() # guessing that sizeof buffer will suffice to load data. (it should)        
        self.instance_load( type_info.type_id, byref(unpacked_data), sizeof(unpacked_data), data_buffer, len(data_buffer), c_void_p(0) )
        return self.__ctype_to_py_type( cast( unpacked_data, POINTER(type_info.c_type) ).contents )
    
    def LoadInstanceFromFile(self, type_name, in_file):
        '''
            Load an instance of a DL-type from a file.
            
            type_name -- name of type.
            in_file   -- path to file to read.
        '''
        return self.LoadInstance(type_name, open(in_file, 'rb').read())
    
    def LoadInstanceFromString(self, type_name, in_str):
        '''
            Load an instance of a DL-type from a string.
            
            type_name -- name of type.
            in_str    -- string with dl-text-data.
        '''
        return self.LoadInstance( type_name, self.PackText( in_str ) )
     
    def LoadInstanceFromTextFile(self, type_name, in_file):
        '''
            Load an instance of a DL-type from a text file.
            
            type_name -- name of type.
            in_file   -- path to file to read.
        '''
        return self.LoadInstanceFromString( type_name, open(in_file, 'rb').read() )
    
    def ConvertInstance( self, type_id, instance, endian = sys.byteorder, ptr_size = host_ptr_size() ):
        endian = 0 if endian == 'big' else 1
        
        converted_size = c_size_t(0)
        self.convert( type_id, instance, len(instance), 0, 0, endian, ptr_size, byref( converted_size ) )
        
        converted_data = ( c_ubyte * converted_size.value )() 
        self.convert( type_id, instance, len(instance), converted_data, len(converted_data), endian, ptr_size, c_void_p(0) )

        return string_at( converted_data, sizeof(converted_data) )
    
    def StoreInstance( self, instance, endian = sys.byteorder, ptr_size = host_ptr_size() ):
        '''
            Store a DL-instance to buffer that is returned as a string.
            
            instance  -- instance to store.
            endian    -- endian to store it in
            ptr_size  -- pointer size to store it with
        '''   
             
        type_id    = instance.TYPE_ID
        c_instance = self.__py_type_to_ctype( instance )    
        
        store_size = c_size_t(0)
        self.instance_store( type_id, byref(c_instance), 0, 0, byref(store_size) )            
        packed_data = ( c_ubyte * store_size.value )()
        self.instance_store( type_id, byref(c_instance), packed_data, store_size, c_void_p(0) )
        
        return self.ConvertInstance( type_id, packed_data, endian, ptr_size )
    
    def StoreInstanceToFile(self, instance, file_name, endian = sys.byteorder, ptr_size = host_ptr_size() ):
        '''
            Store a DL-instance to file.
            
            instance  -- instance to store.
            file_name -- path to file to write to.
            endian    -- endian to store it in
            ptr_size  -- pointer size to store it with
        '''
        open(file_name, 'wb').write(self.StoreInstance(instance, endian, ptr_size))

    def StoreInstanceToString( self, instance ):
        packed_instance = self.StoreInstance(instance)
        
        text_size = c_size_t(0)
        self.txt_unpack( instance.TYPE_ID, packed_instance, len(packed_instance), c_void_p(0), 0, byref( text_size ) )
        
        text_instance = create_string_buffer( text_size.value )
        self.txt_unpack( instance.TYPE_ID, packed_instance, len(packed_instance), text_instance, text_size, c_void_p(0) )
            
        return text_instance.raw
        
    def StoreInstanceToTextFile( self, instance, file_name ):
        open(file_name, 'w').write(self.StoreInstanceToString(instance))
    
    def PackText( self, text, endian = sys.byteorder, ptr_size = host_ptr_size() ):
        instance_data = create_string_buffer(text)
        
        packed_size = c_size_t(0)
        self.txt_pack( instance_data, c_void_p(0), 0, byref(packed_size) )
        
        packed_data = create_string_buffer(packed_size.value)
        self.txt_pack( instance_data, packed_data, packed_size, c_void_p(0) )
            
        info = self.dl_instance_info()
        err = self.dl.dl_instance_get_info( packed_data, packed_size, byref(info) )
        if err != 0:
            raise DLError( 'Call to %s failed with error %s' % ( func_name, self.dl.dl_error_to_string( err ) ), err )        
        return self.ConvertInstance( info.root_type, packed_data, endian, ptr_size )
