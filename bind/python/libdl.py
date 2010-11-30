''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

'''
    DL-binding for python
'''

from ctypes import *
import platform, os, sys, logging

def M_BIT(_Bit): 
    return 1 << _Bit

def M_BITMASK(_Bits):
    return M_BIT(_Bits) - 1

def M_BITRANGE(_MinBit, _MaxBit): 
    return (M_BIT(_MaxBit) | ( M_BIT(_MaxBit) - 1 )) ^ ( M_BIT(_MinBit) - 1 )

def M_ZERO_BITS(_Target, _Start, _Bits):
    return _Target & ~M_BITRANGE(_Start, _Start + _Bits - 1)

def M_EXTRACT_BITS(_Val, _Start, _Bits):
    return (_Val >> _Start) & M_BITMASK(_Bits)

def M_INSERT_BITS(_Target, _Val, _Start, _Bits):
    return M_ZERO_BITS(_Target, _Start, _Bits) | ( ( M_BITMASK(_Bits) & _Val ) << _Start)

# need to match enum in dl.h
DL_TYPE_ATOM_MIN_BIT            = 0
DL_TYPE_ATOM_MAX_BIT            = 7
DL_TYPE_STORAGE_MIN_BIT         = 8
DL_TYPE_STORAGE_MAX_BIT         = 15
DL_TYPE_BITFIELD_SIZE_MIN_BIT   = 16
DL_TYPE_BITFIELD_SIZE_MAX_BIT   = 23
DL_TYPE_BITFIELD_OFFSET_MIN_BIT = 24
DL_TYPE_BITFIELD_OFFSET_MAX_BIT = 31

DL_TYPE_BITFIELD_SIZE_BITS_USED   = DL_TYPE_BITFIELD_SIZE_MAX_BIT + 1   - DL_TYPE_BITFIELD_SIZE_MIN_BIT
DL_TYPE_BITFIELD_OFFSET_BITS_USED = DL_TYPE_BITFIELD_OFFSET_MAX_BIT + 1 - DL_TYPE_BITFIELD_OFFSET_MIN_BIT

# Masks
DL_TYPE_ATOM_MASK            = M_BITRANGE(DL_TYPE_ATOM_MIN_BIT,            DL_TYPE_ATOM_MAX_BIT)
DL_TYPE_STORAGE_MASK         = M_BITRANGE(DL_TYPE_STORAGE_MIN_BIT,         DL_TYPE_STORAGE_MAX_BIT)
DL_TYPE_BITFIELD_SIZE_MASK   = M_BITRANGE(DL_TYPE_BITFIELD_SIZE_MIN_BIT,   DL_TYPE_BITFIELD_SIZE_MAX_BIT)
DL_TYPE_BITFIELD_OFFSET_MASK = M_BITRANGE(DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_MAX_BIT)

# Atomic types
DL_TYPE_ATOM_POD          = M_INSERT_BITS(0x00000000, 1, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1)
DL_TYPE_ATOM_ARRAY        = M_INSERT_BITS(0x00000000, 2, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1)
DL_TYPE_ATOM_INLINE_ARRAY = M_INSERT_BITS(0x00000000, 3, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1)
DL_TYPE_ATOM_BITFIELD     = M_INSERT_BITS(0x00000000, 4, DL_TYPE_ATOM_MIN_BIT, DL_TYPE_ATOM_MAX_BIT + 1)

# Storage type
DL_TYPE_STORAGE_INT8   = M_INSERT_BITS(0x00000000,  1, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_INT16  = M_INSERT_BITS(0x00000000,  2, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_INT32  = M_INSERT_BITS(0x00000000,  3, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_INT64  = M_INSERT_BITS(0x00000000,  4, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_UINT8  = M_INSERT_BITS(0x00000000,  5, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_UINT16 = M_INSERT_BITS(0x00000000,  6, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_UINT32 = M_INSERT_BITS(0x00000000,  7, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_UINT64 = M_INSERT_BITS(0x00000000,  8, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_FP32   = M_INSERT_BITS(0x00000000,  9, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_FP64   = M_INSERT_BITS(0x00000000, 10, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_STR    = M_INSERT_BITS(0x00000000, 11, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_PTR    = M_INSERT_BITS(0x00000000, 12, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_STRUCT = M_INSERT_BITS(0x00000000, 13, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)
DL_TYPE_STORAGE_ENUM   = M_INSERT_BITS(0x00000000, 14, DL_TYPE_STORAGE_MIN_BIT, DL_TYPE_STORAGE_MAX_BIT + 1)

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

'''DL_TO_C_TYPE = { DL_TYPE_STORAGE_INT8   : c_int8,
                 DL_TYPE_STORAGE_INT16  : c_int16,
                 DL_TYPE_STORAGE_INT32  : c_int32,
                 DL_TYPE_STORAGE_INT64  : c_int64,
                 DL_TYPE_STORAGE_UINT8  : c_uint8,
                 DL_TYPE_STORAGE_UINT16 : c_uint16,
                 DL_TYPE_STORAGE_UINT32 : c_uint32,
                 DL_TYPE_STORAGE_UINT64 : c_uint64,
                 DL_TYPE_STORAGE_FP32   : c_float,
                 DL_TYPE_STORAGE_FP64   : c_double,
                 DL_TYPE_STORAGE_ENUM   : c_uint32,
                 
                 DL_TYPE_STORAGE_STR    : c_char_p }'''

'''DL_TO_PY_TYPE = { DL_TYPE_STORAGE_INT8   : int,
                  DL_TYPE_STORAGE_INT16  : int,
                  DL_TYPE_STORAGE_INT32  : int,
                  DL_TYPE_STORAGE_INT64  : int,
                  DL_TYPE_STORAGE_UINT8  : int,
                  DL_TYPE_STORAGE_UINT16 : int,
                  DL_TYPE_STORAGE_UINT32 : int,
                  DL_TYPE_STORAGE_UINT64 : int,
                  DL_TYPE_STORAGE_FP32   : float,
                  DL_TYPE_STORAGE_FP64   : float,
                  DL_TYPE_STORAGE_ENUM   : int,
                 
                  DL_TYPE_STORAGE_STR    : str,
                  DL_TYPE_STORAGE_PTR    : object }'''
    
global g_DLDll
g_DLDll = None

class DLError(Exception):
    def __init__(self, err, value):
        self.err = err
        if g_DLDll != None:
            self.value = g_DLDll.dl_error_to_string(value)
        else:
            self.value = 'Unknown error'
    def __str__(self):
        return self.err + ' Error-code: ' + self.value
            
def libdl_init( dll_path ):
    global g_DLDll
    g_DLDll = CDLL( dll_path )
    g_DLDll.dl_error_to_string.restype = c_char_p
         
def try_default_dl_dll():
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
            libdl_init( path )

try_default_dl_dll()

class DLContext:
    class dl_cache_entry:
        def __init__( self, type_id, members, c_type, py_type ):
            self.type_id = type_id
            self.members = members
            self.c_type  = c_type
            self.py_type = py_type
            
    class dl_type_info(Structure):
        _fields_ = [ ('name',         c_char_p), 
                     ('size',         c_uint), 
                     ('alignment',    c_uint), 
                     ('member_count', c_uint) ]
             
    class dl_member_info(Structure):
        _fields_ = [ ('name',        c_char_p),
                     ('type',        c_uint32), 
                     ('type_id',     c_uint32), 
                     ('array_count', c_uint32) ]
        
        def AtomType(self):       return self.type & DL_TYPE_ATOM_MASK
        def StorageType(self):    return self.type & DL_TYPE_STORAGE_MASK
        def BitFieldBits(self):   return M_EXTRACT_BITS(self.type, DL_TYPE_BITFIELD_SIZE_MIN_BIT, DL_TYPE_BITFIELD_SIZE_BITS_USED)
        def BitFieldOffset(self): return M_EXTRACT_BITS(self.type, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED)
    
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
        storage_type = member_info.StorageType()
        
        if storage_type == DL_TYPE_STORAGE_PTR:
            return None   

        type_name = self.__get_type_name( storage_type, member_info.type_id )
        
        if not type_name in self.type_cache:
            self.__cache_type(member_info.type_id)
            
        c_type = self.type_cache[ type_name ].c_type
        
        atom_type = member_info.AtomType()
              
        if   atom_type == DL_TYPE_ATOM_POD:          return c_type 
        elif atom_type == DL_TYPE_ATOM_INLINE_ARRAY: return c_type * member_info.array_count 
        elif atom_type == DL_TYPE_ATOM_ARRAY:
            class dl_array( Structure ):
                _fields_ = [ ( 'data',  POINTER( c_type ) ), 
                             ( 'count', c_uint32 ) ]
            return dl_array
        elif atom_type == DL_TYPE_ATOM_BITFIELD:     return c_type
        else:
            return None
        
    def __get_python_type(self, member_info):
        storage_type = member_info.StorageType()
        if storage_type == DL_TYPE_STORAGE_PTR:
            return None

        type_name = self.__get_type_name( storage_type, member_info.type_id )
            
        if not type_name in self.type_cache:
            self.__cache_type(typeid)
            
        py_type = self.type_cache[ type_name ].py_type
        
        atom_type = member_info.AtomType()
        if   atom_type == DL_TYPE_ATOM_POD:          return py_type
        elif atom_type == DL_TYPE_ATOM_INLINE_ARRAY: return ( list, py_type, member_info.array_count )
        elif atom_type == DL_TYPE_ATOM_ARRAY:        return list
        elif atom_type == DL_TYPE_ATOM_BITFIELD:     return int # TODO: need to be long if bitfield-size is to big
        else:
            return None
                
    def __py_type_to_ctype( self, instance ):
        typename = self.type_info_cache[ instance._dl_type ].name
        typeinfo = self.type_cache[typename]
        
        c_instance = typeinfo.c_type() 

        for member_name in typeinfo.members:
            member = getattr( instance, member_name )
            
            if isinstance( member, self.dl_type ):
                setattr( c_instance, member_name, self.__py_type_to_ctype( member ) )
            elif isinstance(member, list):
                c_member = getattr( c_instance, member_name )
                
                conv_member = None
                if isinstance( member[0], self.dl_type ):
                    conv_member = [ self.__py_type_to_ctype(inst) for inst in member ]
                else:
                    conv_member = member
                        
                if hasattr( c_member, 'data' ):
                    c_member.data  = ( c_member.data._type_ * len( member ) )( *conv_member )
                    c_member.count = len( member )
                else: # inline array
                    setattr( c_instance, member_name, type(c_member)( *conv_member ) )
            else:
                setattr( c_instance, member_name, member )
        
        return c_instance
    
    def __ctype_to_py_type( self, instance ):
        typename = self.type_info_cache[ instance._dl_type ].name
        typeinfo = self.type_cache[typename]
        
        py_instance = typeinfo.py_type() 
        
        for member_name in typeinfo.members:
            member = getattr( instance, member_name )
            
            if type(member).__name__ == 'dl_array': # TODO: cache type to look for!
                if hasattr( member.data[0], '_dl_type' ): # TODO: Ugly check for convertable!
                    setattr( py_instance, member_name, [ self.__ctype_to_py_type(member.data[i]) for i in range(0, member.count) ] ) # todo can this be dene better?
                else:
                    setattr( py_instance, member_name, [ member.data[i] for i in range(0, member.count) ] ) # todo can this be dene better?
            elif hasattr(member, '_length_'): # TODO: bad check here!
                if hasattr( member[0], '_dl_type' ): # TODO: Ugly check for convertable!
                    setattr( py_instance, member_name, [ self.__ctype_to_py_type( member[i] ) for i in range(0, len(member)) ] ) # todo can this be dene better?
                else:
                    setattr( py_instance, member_name, [ member[i] for i in range(0, len(member)) ] ) # todo can this be dene better?
            else:
                if hasattr( member, '_dl_type' ): # TODO: Ugly check for convertable!
                    setattr( py_instance, member_name, self.__ctype_to_py_type( member ) )
                else:
                    setattr( py_instance, member_name, member )
                
        
        return py_instance
        
    def __init__(self, _TLBuffer = None, _TLFile = None):
        self.DLContext = c_void_p(0)
        
        class dl_create_params(Structure):
            _fields_ = [ ('alloc_func', c_void_p), 
                         ('free_func',  c_void_p), 
                         ('alloc_ctx',  c_void_p) ]
            
        params = dl_create_params()
        params.alloc_func = 0
        params.free_func  = 0
        params.alloc_ctx  = 0
        err = g_DLDll.dl_context_create( byref(self.DLContext), byref(params) )
        
        if err != 0:
            raise DLError('Could not create DLContext', err)
            
        self.type_cache = {}
        self.type_info_cache = {}
        
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
        
        
        assert not (_TLBuffer != None and _TLFile != None)
        
        if _TLBuffer != None: self.LoadTypeLibrary(_TLBuffer)
        if _TLFile   != None: self.LoadTypeLibraryFromFile(_TLFile)
        
    def __del__(self):
        err = g_DLDll.dl_context_destroy(self.DLContext)
        
        if err != 0:
            raise DLError('Could not destroy DLContext', err)
    
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
        err = g_DLDll.dl_reflect_get_type_members( self.DLContext, typeid, member_info, type_info.member_count )
        if err != 0:
            raise DLError( 'Could not fetch members!', err )
        
        members    = []
        c_members  = []
        py_members = []
        
        for member in member_info:
            members.append( member.name )
            
            if member.AtomType() == DL_TYPE_ATOM_BITFIELD: # TODO: Do not like this!
                c_members.append ( ( member.name, self.__get_ctypes_type( member ), member.BitFieldBits() ) )
            else:
                c_members.append ( ( member.name, self.__get_ctypes_type( member ) ) )
                
            py_members.append( ( member.name, self.__get_python_type( member ) ) )
    
        complete = True
        for m in c_members:
            if m[1] == None:
                complete = False
                break;

        c_type  = None
        py_type = None
    
        # build py-type
        if complete:
            c_type  = type( 'c.'  + type_info.name, (Structure, ),    { '_dl_type' : typeid, 
                                                                        '_fields_' : c_members } )
            py_type = type( 'py.' + type_info.name, (self.dl_type, ), { '_dl_type'    : typeid, 
                                                                        '_dl_members' : py_members } )
        
        # cache it!
        self.type_cache[type_info.name] = self.dl_cache_entry( typeid, members, c_type, py_type )
        
    def LoadTypeLibrary(self, _DataBuffer):
        '''
            Loads a binary typelibrary into the DLContext
            
            _DataBuffer -- string with the binary file loaded.
        '''
        err = g_DLDll.dl_context_load_type_library(self.DLContext, _DataBuffer, len(_DataBuffer))    
        if err != 0:
            raise DLError('Could not load type library into context DLContext', err)
        
        # load all types in type-library
        
        num_types = c_uint()
        err = g_DLDll.dl_reflect_num_types_loaded( self.DLContext, byref(num_types) )
        if err != 0:
            raise DLError('Could not get loaded types!', err)
        
        loaded_types = (c_uint * num_types.value)()
        err = g_DLDll.dl_reflect_loaded_types( self.DLContext, byref(loaded_types), num_types );
        if err != 0:
            raise DLError('Could not get loaded types!', err)
        
        for typeid in loaded_types:
            type_info = self.dl_type_info()
    
            err = g_DLDll.dl_reflect_get_type_info(self.DLContext, typeid, byref(type_info))
            if err != 0:
                raise DLError( 'Could not create type!', err )
            
            self.type_info_cache[ typeid ] = type_info
        
        for type in self.type_info_cache:
            # print 'type', type
            
            '''
            1 ) if type not exist in type_cache, create it. ( recurse to create types used by types )
            '''
            self.__cache_type( type )
        # Add ability to read all arrays of numbers to numpy-arrays, even read normal numbers to numpy?
    
    def LoadTypeLibraryFromFile(self, _File):
        '''
            Loads a binary typelibrary into the DLContext
            
            _File -- filename of file to load typelibrary from.
        '''
        self.LoadTypeLibrary(open(_File, 'rb').read())
    
    def CreateType(self, typename):
        '''
            Get the a python type for a DL-type with the specified name.
            
            typename -- name of type.
        '''
        
        return self.type_cache[ typename ].py_type
    
        '''if self.type_cache.has_key(_TypeName):
            return self.type_cache[_TypeName].py_type
        
        return self.__GetPythonType(self.__TypeIDOf(_TypeName))'''
    
    def CreateInstance(self, _TypeName):
        '''
            Get an instance of a DL-type with the specified name.
            
            _TypeName -- name of type.
        '''
        return self.CreateType(_TypeName)()
    
    def LoadInstance(self, _TypeName, _DataBuffer):
        '''
            Load an instance of a DL-type from a buffer.
            
            _TypeName   -- name of type.
            _DataBuffer -- string containing buffer to load from.
        '''
        UnpackedData = (c_ubyte * len(_DataBuffer))() # guessing that sizeof buffer will suffice to load data. (it should)
        
        err = g_DLDll.dl_instance_load(self.DLContext, byref(UnpackedData), _DataBuffer, len(_DataBuffer));
        if err != 0:
            raise DLError('Could not store instance!', err)
        
        # TODO: Guess that there is a bug here, what happens if type is not loaded!
        type_info = self.type_cache[_TypeName]
        
        c_instance = cast(UnpackedData, POINTER(type_info.c_type)).contents
        
        return self.__ctype_to_py_type( c_instance )
    
    def LoadInstanceFromFile(self, _TypeName, _File):
        '''
            Load an instance of a DL-type from a file.
            
            _TypeName -- name of type.
            _File     -- path to file to read.
        '''
        return self.LoadInstance(_TypeName, open(_File, 'rb').read())
        
    def LoadInstanceFromTextFile(self, _TypeName, _File):
        '''
            Load an instance of a DL-type from a text file.
            
            _TypeName -- name of type.
            _File     -- path to file to read.
        '''
        return self.LoadInstance(_TypeName, self.PackText(open(_File, 'rb').read()))
    
    def StoreInstance(self, _Instance, _Endian = sys.byteorder, _PtrSize = 4 if platform.architecture()[0] == '32bit' else 8):
        '''
            Store a DL-instance to buffer that is returned as a string.
            
            _Instance -- instance to store.
            _Endian   -- endian to store it in
            _PtrSize  -- pointer size to store it with
        '''   
             
        TypeID     = _Instance._dl_type
        c_instance = self.__py_type_to_ctype( _Instance ) # _Instance.AsCType()    
        
        DataSize = c_ulong(0)
        err = g_DLDll.dl_instance_calc_size(self.DLContext, TypeID, byref(c_instance), byref(DataSize))
        if err != 0:
            raise DLError('Could not calculate size!', err)
            
        PackedData = (c_ubyte * DataSize.value)()
        
        err = g_DLDll.dl_instance_store(self.DLContext, TypeID, byref(c_instance), PackedData, DataSize)
        if err != 0:
            raise DLError('Could not store instance!', err)
        
        ConvertedSize = c_ulong(0)
        
        err = g_DLDll.dl_convert_calc_size(self.DLContext, PackedData, len(PackedData), _PtrSize, byref(ConvertedSize));
        if err != 0:
            raise DLError('Could not calc convert instance size!', err)
        
        if DataSize == ConvertedSize:
            # can convert inplace
            err = g_DLDll.dl_convert_inplace(self.DLContext, PackedData, len(PackedData), 0 if _Endian == 'big' else 1, _PtrSize);
            if err != 0:
                raise DLError('Could not convert instance!', err)
        else:
            # need new memory
            ConvertedData = (c_ubyte * ConvertedSize.value)()
            err = g_DLDll.dl_convert(self.DLContext, PackedData, len(PackedData), ConvertedData, len(ConvertedData), 0 if _Endian == 'big' else 1, _PtrSize);
            
            PackedData = ConvertedData
            

        return string_at(PackedData, sizeof(PackedData))
    
    def StoreInstanceToFile(self, _Instance, _File, _Endian = sys.byteorder, _PtrSize = 4 if platform.architecture()[0] == '32bit' else 8):
        '''
            Store a DL-instance to file.
            
            _Instance -- instance to store.
            _File     -- path to file to write to.
            _Endian   -- endian to store it in
            _PtrSize  -- pointer size to store it with
        '''
        open(_File, 'wb').write(self.StoreInstance(_Instance, _Endian, _PtrSize))

    def StoreInstanceToString(self, _Instance):
        Packed = self.StoreInstance(_Instance)
        
        DataSize = c_ulong(0)
        
        if g_DLDll.dl_txt_unpack_calc_size(self.DLContext, Packed, len(Packed), byref(DataSize)) != 0:
            raise DLError('Could not calculate txt-unpack-size', err)
        
        PackedData = create_string_buffer(DataSize.value)
        
        if g_DLDll.dl_txt_unpack(self.DLContext, Packed, len(Packed), PackedData, DataSize) != 0:
            raise DLError('Could not calculate txt-unpack-size', err)
            
        return PackedData.raw
        
    def StoreInstanceToTextFile(self, _Instance, _File):
        open(_File, 'w').write(self.StoreInstanceToString(_Instance))
    
    def PackText(self, _Text):
        InstanceData = create_string_buffer(_Text)
        DataSize = c_ulong(0)

        if g_DLDll.dl_txt_pack_calc_size(self.DLContext, InstanceData, byref(DataSize)) != 0:
            raise DLError('Could not calculate txt-pack-size', err)

        PackedData = create_string_buffer(DataSize.value)
        if g_DLDll.dl_txt_pack(self.DLContext, InstanceData, PackedData, DataSize) != 0:
            raise DLError('Could not pack txt', err)
    
        return PackedData.raw
