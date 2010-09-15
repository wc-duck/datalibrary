''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

'''
	DL-binding for python
'''

from ctypes import *
import platform, os, sys

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

DL_TO_C_TYPE = { DL_TYPE_STORAGE_INT8   : c_byte,
				 DL_TYPE_STORAGE_INT16  : c_short,
				 DL_TYPE_STORAGE_INT32  : c_long,
				 DL_TYPE_STORAGE_INT64  : c_longlong,
				 DL_TYPE_STORAGE_UINT8  : c_ubyte,
				 DL_TYPE_STORAGE_UINT16 : c_ushort,
				 DL_TYPE_STORAGE_UINT32 : c_ulong,
				 DL_TYPE_STORAGE_UINT64 : c_ulonglong,
				 DL_TYPE_STORAGE_FP32   : c_float,
				 DL_TYPE_STORAGE_FP64   : c_double,
				 DL_TYPE_STORAGE_ENUM   : c_ulong,
				 
				 DL_TYPE_STORAGE_STR    : c_char_p }

DL_TO_PY_TYPE = { DL_TYPE_STORAGE_INT8   : 0,
				  DL_TYPE_STORAGE_INT16  : 0,
				  DL_TYPE_STORAGE_INT32  : 0,
				  DL_TYPE_STORAGE_INT64  : 0,
				  DL_TYPE_STORAGE_UINT8  : 0,
				  DL_TYPE_STORAGE_UINT16 : 0,
				  DL_TYPE_STORAGE_UINT32 : 0,
				  DL_TYPE_STORAGE_UINT64 : 0,
				  DL_TYPE_STORAGE_FP32   : 0,
				  DL_TYPE_STORAGE_FP64   : 0,
				  DL_TYPE_STORAGE_ENUM   : 0,
				 
				  DL_TYPE_STORAGE_STR    : '',
				  DL_TYPE_STORAGE_PTR    : None }
				 
# Hack fix: (how should we find the dl-dll, right now the path is hard-codede and it tries to find one dll that fits!)
DLDllPath = os.getcwd() + '/local/%s/win32/dldyn.dll' if platform.architecture()[0] == '32bit' else '/local/%s/winx64/dldyn.dll'

if 'DL_DLL_PATH' in os.environ: # was the path to the dll set as an environment variable? (might not be the best solution but it has to do for now)
	DLDllPath = os.environ['DL_DLL_PATH']
else:
	for conf in [ 'rtm', 'optimized', 'debug' ]:
		if os.path.exists(DLDllPath % conf):
			DLDllPath = DLDllPath % conf
			break
		
global g_DLDll
g_DLDll = CDLL(DLDllPath)
g_DLDll.DLErrorToString.restype = c_char_p

class DLError(Exception):
	def __init__(self, err, value):
		self.err = err
		self.value = g_DLDll.DLErrorToString(value)
	def __str__(self):
		return self.err + ' Error-code: ' + self.value

class DLContext:
	def __TypeIDOf(self, _TypeName):
		# temporary hack to calculate a type-id!
		hash = 5381
		for char in _TypeName:
			hash = (hash * 33) + ord(char)
		return (hash - 5381) & 0xFFFFFFFF;
	
	def __GetTypes(self, _MemberInfo):
		def GetSimpleCType(_StorageType, _SubTypeID):
			if StorageType == DL_TYPE_STORAGE_STRUCT:
				return self.__GetPythonType(_SubTypeID)._DL_C_TYPE
			elif StorageType == DL_TYPE_STORAGE_PTR:
				return POINTER(self.__GetPythonType(_SubTypeID)._DL_C_TYPE)
			
			return DL_TO_C_TYPE[_StorageType]
				
		def GetSimplePYType(_StorageType, _SubTypeID):
			if StorageType == DL_TYPE_STORAGE_STRUCT:
				return self.__GetPythonType(_SubTypeID)()
			return DL_TO_PY_TYPE[_StorageType]
		
		def ToPyArray(Array, Count, py_instances, py_patch):
			if hasattr(Array[0], 'AsPYType'):
				return [ Array[i].AsPYTypeInternal(py_instances, py_patch) for i in range(Count) ]
			else:
				return [ Array[i] for i in range(Count) ]
			
		def ArrayAsPyType(self, py_instances, py_patch):       return ToPyArray(self.m_Data, self.m_Count,  py_instances, py_patch)
		def InlineArrayAsPyType(self, py_instances, py_patch): return ToPyArray(self,        self._length_, py_instances, py_patch)
		
		TypeID = _MemberInfo.m_TypeID
		
		AtomType    = _MemberInfo.AtomType()
		StorageType = _MemberInfo.StorageType()
		
		if AtomType == DL_TYPE_ATOM_POD or AtomType == DL_TYPE_ATOM_BITFIELD:
			return GetSimpleCType(StorageType, TypeID), GetSimplePYType(StorageType, TypeID)
		
		elif AtomType == DL_TYPE_ATOM_INLINE_ARRAY:
			ArrayCount = _MemberInfo.m_ArrayCount
			ArrayType = GetSimpleCType(StorageType, TypeID) * ArrayCount
			setattr(ArrayType, 'AsPYTypeInternal', InlineArrayAsPyType)
			return ArrayType, [ GetSimplePYType(StorageType, TypeID) ] * ArrayCount
			
		elif AtomType == DL_TYPE_ATOM_ARRAY:
			ArrayType = type('c_dl_array', (Structure, ), { '_fields_' : [ ( 'm_Data', POINTER(GetSimpleCType(StorageType, TypeID)) ), ( 'm_Count', c_ulong ) ], 'AsPYTypeInternal' : ArrayAsPyType } )
			return ArrayType, []
		
		else:
			assert False
	
	def __GetPythonType(self, _TypeID):
		'''
			Returns a python-type for the DL-type with the specified ID
			
			_TypeID -- Id of type.
		'''
		if self.TypeCache.has_key(_TypeID):
			return self.TypeCache[_TypeID]
		
		class DLTypeInfo(Structure):   _fields_ = [ ('m_Name', c_char_p), ('m_nMembers', c_ulong) ]
		class DLMemberInfo(Structure): 
			_fields_ = [ ('m_Name', c_char_p), ('m_Type', c_ulong), ('m_TypeID', c_ulong), ('m_ArrayCount', c_ulong) ]
			
			def AtomType(self):       return self.m_Type & DL_TYPE_ATOM_MASK
			def StorageType(self):    return self.m_Type & DL_TYPE_STORAGE_MASK
			def BitFieldBits(self):   return M_EXTRACT_BITS(self.m_Type, DL_TYPE_BITFIELD_SIZE_MIN_BIT, DL_TYPE_BITFIELD_SIZE_BITS_USED)
			def BitFieldOffset(self): return M_EXTRACT_BITS(self.m_Type, DL_TYPE_BITFIELD_OFFSET_MIN_BIT, DL_TYPE_BITFIELD_OFFSET_BITS_USED)
			
		def DLNoMemberAdd(self, name, value):
			if not hasattr(self, name):
				raise AttributeError
			self.__dict__[name] = value
		
		def AsPYType(self):
			py_instances = {}
			py_patch = []
			
			PyStruct = self.AsPYTypeInternal(py_instances, py_patch)
			
			for struct, member, addr in py_patch:
				setattr(struct, member, py_instances[addr])
				
			return PyStruct
			
		def AsPYTypeInternal(self, py_instances, py_patch):
			PyStruct = self.__class__._DL_PY_TYPE()
			
			py_instances[addressof(self)] = PyStruct
			
			for m in self.__class__.__dict__.keys():
				if m.startswith('m_'):
					c_attr = getattr(self, m)

					if not c_attr: # ptr to null, skip it!
						continue
					elif hasattr(c_attr, 'contents'): # Is pointer!
						addr = addressof(c_attr.contents)
						if not py_instances.has_key(addr):
							py_instances[addr] = None
							py_instances[addr] = c_attr.contents.AsPYTypeInternal(py_instances, py_patch)
						
						py_patch.append((PyStruct, m, addr))
					elif hasattr(c_attr, 'AsPYTypeInternal'):
						py_attr = c_attr.AsPYTypeInternal(py_instances, py_patch)
						setattr(PyStruct, m, py_attr)
					else:
						py_attr = c_attr
						setattr(PyStruct, m, py_attr)
			
			return PyStruct
				
		def AsCType(self):
			ptr_instances = {}
			patch_members = []
			
			CStruct = self.AsCTypeInternal(ptr_instances, patch_members)
			
			for member, id in patch_members:
				member.contents = ptr_instances[id]
				
			return CStruct
			
		def AsCTypeInternal(self, ptr_instances, patch_members):
			CStruct = self.__class__._DL_C_TYPE()
			
			ptr_instances[id(self)] = CStruct
			
			for m in self.__class__.__dict__.keys():
				if m.startswith('m_'):
					py_attr = getattr(self, m)
					
					if py_attr == None: # default-value of ptr is Null, no need to act!
						pass
					elif hasattr(py_attr, 'AsCTypeInternal'):
						c_attr = getattr(CStruct, m)
						
						if hasattr(c_attr.__class__, 'contents'):
							instance_id = id(py_attr)
							if not ptr_instances.has_key(instance_id):
								ptr_instances[instance_id] = None
								ptr_instances[instance_id] = py_attr.AsCTypeInternal(ptr_instances, patch_members)

							patch_members.append((getattr(CStruct, m), instance_id))
						else:
							setattr(CStruct, m, py_attr.AsCTypeInternal(ptr_instances, patch_members))
							
					elif type(py_attr) == list:
						c_attr = getattr(CStruct, m)
						length = len(py_attr)
						
						if hasattr(c_attr, 'm_Count'): # dynamic sized array.
							c_attr.m_Count = length
							c_attr.m_Data  = (c_attr.m_Data._type_ * length)()
							
							c_attr = c_attr.m_Data
						
						if hasattr(py_attr[0], 'AsCTypeInternal'):
							for i in range(length): c_attr[i] = py_attr[i].AsCTypeInternal(ptr_instances, patch_members)
						elif type(py_attr[0]) == str:
							for i in range(length): c_attr[i] = c_char_p(py_attr[i])
						else:
							for i in range(length): c_attr[i] = py_attr[i]
					else:
						setattr(CStruct, m, py_attr)

			return CStruct

		MemberInfo128 = DLMemberInfo * 128
		
		type_info   = DLTypeInfo()
		member_info = MemberInfo128()
	
		err = g_DLDll.DLReflectGetTypeInfo(self.DLContext, _TypeID, byref(type_info), member_info, c_ulong(128))
		if err != 0:
			raise DLError('Could not create type!', err)
		
		py_members = {}
		c_members  = []
		
		self_typed_ptrs = []
		
		for i in range(0, type_info.m_nMembers):
			MemberName = 'm_' + member_info[i].m_Name
			if member_info[i].m_TypeID == _TypeID:
				self_typed_ptrs.append(i)
				PYType = None
			else:
				CType, PYType = self.__GetTypes(member_info[i])
			
				if member_info[i].AtomType() == DL_TYPE_ATOM_BITFIELD:
					c_members.append((MemberName, CType, member_info[i].BitFieldBits()))
				else:
					c_members.append((MemberName, CType))
				
			py_members[MemberName] = PYType
		
		c_type = type( 'DLCType_' + type_info.m_Name, (Structure, ), { '_DL_PY_TYPE' : None, 'AsPYType' : AsPYType, 'AsPYTypeInternal' : AsPYTypeInternal } )
		
		for index in self_typed_ptrs:
			c_members.insert(index, ('m_' + member_info[index].m_Name, POINTER(c_type)))
		
		c_type._fields_ = c_members
		
		py_members['AsCType']         = AsCType
		py_members['AsCTypeInternal'] = AsCTypeInternal
		py_members['_DL_TYPE_ID']     = _TypeID
		py_members['_DL_C_TYPE' ]     = c_type
		py_members['__setattr__']     = DLNoMemberAdd
		
		py_type = type( type_info.m_Name, (object, ), py_members )
		
		c_type._DL_PY_TYPE = py_type
		
		# add types
		self.TypeCache[_TypeID] = py_type
		
		return py_type
		
	def __init__(self, _TLBuffer = None, _TLFile = None):
		self.DLContext = c_void_p(0)
		err = g_DLDll.DLContextCreate(byref(self.DLContext), c_void_p(0), c_void_p(0))
		
		if err != 0:
			raise DLError('Could not create DLContext', err)
			
		self.TypeCache = {}
		
		assert not (_TLBuffer != None and _TLFile != None)
		
		if _TLBuffer != None: self.LoadTypeLibrary(_TLBuffer)
		if _TLFile   != None: self.LoadTypeLibraryFromFile(_TLFile)
		
	def __del__(self):
		err = g_DLDll.DLContextDestroy(self.DLContext)
		
		if err != 0:
			raise DLError('Could not destroy DLContext', err)
		
	def LoadTypeLibrary(self, _DataBuffer):
		'''
			Loads a binary typelibrary into the DLContext
			
			_DataBuffer -- string with the binary file loaded.
		'''
		err = g_DLDll.DLLoadTypeLibrary(self.DLContext, _DataBuffer, len(_DataBuffer))	
		if err != 0:
			raise DLError('Could not load type library into context DLContext', err)
	
	def LoadTypeLibraryFromFile(self, _File):
		'''
			Loads a binary typelibrary into the DLContext
			
			_File -- filename of file to load typelibrary from.
		'''
		self.LoadTypeLibrary(open(_File, 'rb').read())
	
	def CreateType(self, _TypeName):
		'''
			Get the a python type for a DL-type with the specified name.
			
			_TypeName -- name of type.
		'''
		return self.__GetPythonType(self.__TypeIDOf(_TypeName))
	
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
		
		err = g_DLDll.DLLoadInstanceInplace(self.DLContext, byref(UnpackedData), _DataBuffer, len(_DataBuffer));
		if err != 0:
			raise DLError('Could not store instance!', err)
		
		Type = self.__GetPythonType(self.__TypeIDOf(_TypeName))
		c_instance = cast(UnpackedData, POINTER(Type._DL_C_TYPE)).contents
		
		return c_instance.AsPYType()
	
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
		TypeID     = _Instance._DL_TYPE_ID
		c_instance = _Instance.AsCType()	
		
		DataSize = c_ulong(0)
		err = g_DLDll.DLInstaceSizeStored(self.DLContext, TypeID, byref(c_instance), byref(DataSize))
		if err != 0:
			raise DLError('Could not calculate size!', err)
			
		PackedData = (c_ubyte * DataSize.value)()
		
		err = g_DLDll.DLStoreInstace(self.DLContext, TypeID, byref(c_instance), PackedData, DataSize)
		if err != 0:
			raise DLError('Could not store instance!', err)
		
		ConvertedSize = c_ulong(0)
		
		err = g_DLDll.DLInstanceSizeConverted(self.DLContext, PackedData, len(PackedData), _PtrSize, byref(ConvertedSize));
		if err != 0:
			raise DLError('Could not calc convert instance size!', err)
		
		if DataSize == ConvertedSize:
			# can convert inplace
			err = g_DLDll.DLConvertInstanceInplace(self.DLContext, PackedData, len(PackedData), 0 if _Endian == 'big' else 1, _PtrSize);
			if err != 0:
				raise DLError('Could not convert instance!', err)
		else:
			# need new memory
			ConvertedData = (c_ubyte * ConvertedSize.value)()
			err = g_DLDll.DLConvertInstance(self.DLContext, PackedData, len(PackedData), ConvertedData, len(ConvertedData), 0 if _Endian == 'big' else 1, _PtrSize);
			
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
		
		if g_DLDll.DLRequiredUnpackSize(self.DLContext, Packed, len(Packed), byref(DataSize)) != 0:
			raise DLError('Could not calculate txt-unpack-size', err)
		
		PackedData = create_string_buffer(DataSize.value)
		
		if g_DLDll.DLUnpack(self.DLContext, Packed, len(Packed), PackedData, DataSize) != 0:
			raise DLError('Could not calculate txt-unpack-size', err)
			
		return PackedData.raw
		
	def StoreInstanceToTextFile(self, _Instance, _File):
		open(_File, 'w').write(self.StoreInstanceToString(_Instance))
	
	def PackText(self, _Text):
		InstanceData = create_string_buffer(_Text)
		DataSize = c_ulong(0)

		if g_DLDll.DLRequiredTextPackSize(self.DLContext, InstanceData, byref(DataSize)) != 0:
			raise DLError('Could not calculate txt-pack-size', err)

		PackedData = create_string_buffer(DataSize.value)
		if g_DLDll.DLPackText(self.DLContext, InstanceData, PackedData, DataSize) != 0:
			raise DLError('Could not pack txt', err)
	
		return PackedData.raw
