''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

import logging, struct, os, sys, platform, ctypes, json

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../../bind/python/')))

import libdl

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

DL_INSTANCE_HEADER_SIZE = 20
				  
def temp_hash_func(str):
	hash = 5381
	for char in str:
		hash = (hash * 33) + ord(char)
	return (hash - 5381) & 0xFFFFFFFF;
					
class TypeLibraryWriter:
	def prepare_members(self, types, enums):
		'''
			Walks through all members of all types and fills out info like the type that it will have in the dl-library,
			type-id if that is applicable etc.
		'''
		for unicode_name, attribs in types.items():
			for member in attribs['members']:
				member_type = member['type']
				
				subtype = member['subtype'] if 'subtype' in member else ''
				
				extra_bits = 0
				member_types = (0,0,0)
				if   member_type in TYPE_ENUM_MAP: member_types = (libdl.DL_TYPE_ATOM_POD, TYPE_ENUM_MAP[member_type], 0)
				elif member_type == 'array':
					if   subtype in TYPE_ENUM_MAP: member_types = (libdl.DL_TYPE_ATOM_ARRAY, TYPE_ENUM_MAP[subtype], 0)
					elif subtype in enums:         member_types = (libdl.DL_TYPE_ATOM_ARRAY, libdl.DL_TYPE_STORAGE_ENUM,   temp_hash_func(subtype))
					else:                          member_types = (libdl.DL_TYPE_ATOM_ARRAY, libdl.DL_TYPE_STORAGE_STRUCT, temp_hash_func(subtype))
				elif member_type == 'inline-array':
					if   subtype in TYPE_ENUM_MAP: member_types = (libdl.DL_TYPE_ATOM_INLINE_ARRAY, TYPE_ENUM_MAP[subtype], 0)
					elif subtype in enums:         member_types = (libdl.DL_TYPE_ATOM_INLINE_ARRAY, libdl.DL_TYPE_STORAGE_ENUM,   temp_hash_func(subtype))
					else:                          member_types = (libdl.DL_TYPE_ATOM_INLINE_ARRAY, libdl.DL_TYPE_STORAGE_STRUCT, temp_hash_func(subtype))
				elif member_type == 'bitfield':
					extra_bits = libdl.M_INSERT_BITS(extra_bits, member['bits'],     libdl.DL_TYPE_BITFIELD_SIZE_MIN_BIT,   libdl.DL_TYPE_BITFIELD_SIZE_MAX_BIT + 1)
					extra_bits = libdl.M_INSERT_BITS(extra_bits, member['bfoffset'], libdl.DL_TYPE_BITFIELD_OFFSET_MIN_BIT, libdl.DL_TYPE_BITFIELD_OFFSET_MAX_BIT + 1)
					member_types = (libdl.DL_TYPE_ATOM_BITFIELD, TYPE_ENUM_MAP[subtype], 0)
				elif member_type == 'pointer':  member_types = (libdl.DL_TYPE_ATOM_POD, libdl.DL_TYPE_STORAGE_PTR,    temp_hash_func(subtype))
				elif member_type in enums:      member_types = (libdl.DL_TYPE_ATOM_POD, libdl.DL_TYPE_STORAGE_ENUM,   temp_hash_func(member_type))
				else:                           member_types = (libdl.DL_TYPE_ATOM_POD, libdl.DL_TYPE_STORAGE_STRUCT, temp_hash_func(member_type))
				
				member.update( dl_type = member_types[0] | member_types[1] | extra_bits, dl_type_id = member_types[2], dl_default_value_offset = 0xFFFFFFFF )
	
	def build_defaults(self, types, enums, _Ctx):
		'''
			If a value has a default-value it will be packed into the return-string of this function. The member 
			will also be updated to point its default_value_offset to that value!
		'''
		default_data_str = ''
		for unicode_name, attribs in types.items():
			for member in attribs['members']:
				member_type = member['type']
				
				PACK_FORMAT = { 'int8'   : 'b', 'int16'  : 'h', 'int32'  : 'i', 'int64'  : 'q',
								'uint8'  : 'B', 'uint16' : 'H', 'uint32' : 'I', 'uint64' : 'Q',
								'fp32'   : 'f', 'fp64'   : 'd' }
				
				def PackDefaultPod(_Type, _Value): return struct.pack('<' + PACK_FORMAT[_Type], _Value)
				def PackDefaultStr(_Str):          return struct.pack('<%us' % (len(_Str) + 1), str(_Str))
				
				def PackDefaultEnum(_Value, _Enum):
					for Elem in _Enum:
						if Elem[0] == _Value:
							return struct.pack('<I', Elem[1])
					assert False # did not find enum value!
					
				def align(val, alignment):
					return (val + alignment - 1) & ~(alignment - 1)
				
				if 'default' in member:
					current_offset = len(default_data_str)
					needed_align = align(current_offset, member['align32']) - current_offset

					default_data_str += struct.pack('<%ux' % needed_align)
				
					member['dl_default_value_offset'] = len(default_data_str)
					defval = member['default']
					
					if   member_type in PACK_FORMAT: default_data_str += PackDefaultPod(member_type, defval)
					elif member_type in enums:       default_data_str += PackDefaultEnum(str(defval), enums[member_type])
					elif member_type == 'string':
						default_data_str += struct.pack('<I', len(default_data_str) + 4)
						default_data_str += PackDefaultStr(defval)
					elif member_type == 'pointer':
						assert member['default'] == None # only null is supported for ptrs!
						default_data_str += struct.pack('<I', 0xFFFFFFFF)
					elif member_type == 'inline-array' or member_type == 'array':
						assert type(defval) == list
						# need to add check that all members are the correct type! but it should be done in dl_tlc! (as should the other error-checks!)
						
						if member_type == 'array':
							default_data_str += struct.pack('<II', len(default_data_str) + 8, len(defval))
						else:
							assert len(defval) == member['count']
						
						subtype = member['subtype']
						if   subtype in PACK_FORMAT: default_data_str += ''.join([PackDefaultPod(subtype, item) for item in defval])
						elif subtype in enums:       default_data_str += ''.join([PackDefaultEnum(str(item), enums[subtype]) for item in defval])
						elif subtype == 'string':
							strings_string = ''
							first_str = len(default_data_str) + len(defval) * 4
							for s in defval:
								default_data_str += struct.pack('<I', first_str + len(strings_string))
								strings_string   += PackDefaultStr(s)
							default_data_str += strings_string
						else:
							assert False
					else:
						instance_str =  '{ "root" : { "type" : "%s", "data" : %s } }' % ( member_type, str(json.dumps(defval)) )
						default_data_str += _Ctx.PackText(instance_str)[DL_INSTANCE_HEADER_SIZE:]
						
						
						
					
		return default_data_str
	
	def write_types(self, types, dump):
		type_lookup_str  = ''
		type_data_str    = ''
		
		if dump:
			logging.debug('TYPES')
			logging.debug('                                    name          id    offset  size32  size64  align32  align64')
			logging.debug('------------------------------------------------------------------------------------------------')
		
		for unicode_name, attribs in types.items():
			name = str(unicode_name)
			members = attribs['members']
			
			type_id     = temp_hash_func(name)
			type_offset = len(type_data_str)
			if dump:
				logging.debug('%40s  0x%08X%10u%8u%8u%9u%9u' % ( name, type_id, type_offset, 
																	 attribs['size32'],  attribs['size64'],
																	 attribs['align32'], attribs['align64'] ))
			type_lookup_str += struct.pack('<II', type_id, type_offset)
			
			# write type-data.	
			if len(name) > 32:
				raise DLException('Type name longer than 32 on type %s' % name)
			
			type_data_str += struct.pack( '<32sIIIII', name, 
										  attribs['size32'],  attribs['size64'],
										  attribs['align32'], attribs['align64'],
										  len(members) )
			
			# write all members					
			for member in members:
				type_data_str += struct.pack( '<32sIIIIIIIII', 
											  str(member['name']), 
											  member['dl_type'],  member['dl_type_id'],
											  member['size32'],   member['size64'],
											  member['align32'],  member['align64'],
											  member['offset32'], member['offset64'],
											  member['dl_default_value_offset'] )

		return type_lookup_str, type_data_str
	
	def write_enums(self, enums):
		enum_lookup_str = ''
		enum_data_str   = ''
		
		logging.debug('ENUMS')
		logging.debug('                                    name          id    offset')
		logging.debug('------------------------------------------------------------------------------------------------')
		
		for name, values in enums.items():
			enum_id     = temp_hash_func(name)
			enum_offset = len(enum_data_str)
			logging.debug('%40s  0x%08X%10u' % (name, enum_id, enum_offset))
			enum_lookup_str += struct.pack('<II', enum_id, enum_offset)
			enum_data_str   += struct.pack( '<32sII', str(name), enum_id, len(values))
			enum_data_str   += ''.join( [ struct.pack( '<32sI', name, value ) for name, value in values ] )
		
		return enum_lookup_str, enum_data_str
	
	def write_lib(self, types, type_lookup_str, type_data_str, enums, enum_lookup_str, enum_data_str, default_data_str, dump ):
		DL_VERSION  = struct.pack('<I', 1)
		DLTL_BINARY = struct.pack('<cccc', 'L', 'T', 'L', 'D') # THIS IS BROKEN AND BACKWARDS!
		
		DLTL_HEADER_FMT   = '<4s4sIIIIIIII'
		DL_TL_HEADER_SIZE = struct.calcsize(DLTL_HEADER_FMT)
		
		num_types        = len(types)
		type_data_offset = DL_TL_HEADER_SIZE + len(type_lookup_str)
		type_data_size   = len(type_data_str)
		
		num_enums        = len(enums)
		enum_data_offset = type_data_offset + type_data_size + len(enum_lookup_str)
		enum_data_size   = len(enum_data_str)
		
		default_data_offset = enum_data_offset + enum_data_size
		default_data_size   = len(default_data_str)
		
		if dump:
			logging.debug('TOTAL')
			logging.debug('                                       num    offset      size')
			logging.debug('------------------------------------------------------------------------------------------------')
			logging.debug('types:  %34u%10u%10u' % (num_types, type_data_offset, type_data_size))
			logging.debug('enums:  %34u%10u%10u' % (num_enums, enum_data_offset, enum_data_size))
			logging.debug('defdata:%44u%10u'     % (default_data_offset, default_data_size))
		
		tld_data = struct.pack(DLTL_HEADER_FMT, DLTL_BINARY, DL_VERSION,
												num_types, type_data_offset, type_data_size,
												num_enums, enum_data_offset, enum_data_size,
												default_data_offset, default_data_size ) 
		tld_data += type_lookup_str
		tld_data += type_data_str
		tld_data += enum_lookup_str
		tld_data += enum_data_str
		tld_data += default_data_str
		
		return tld_data
	
	def write(self, data):
		logging.debug('writing type library')
		
		module_enums = data.get('module_enums', {})
		module_types = data.get('module_types', {})
		
		# prepare types, calc id, dl-types etc!
		self.prepare_members(module_types, module_enums)
		
		type_lookup_str, type_data_str = self.write_types(module_types, True)
		enum_lookup_str, enum_data_str = self.write_enums(module_enums)
		
		tld_data = self.write_lib(module_types, type_lookup_str, type_data_str, module_enums, enum_lookup_str, enum_data_str, '', False)

		# load the tl without defaults into dl to be able to create default-instances with it!
		# do code here!

		Ctx = libdl.DLContext(_TLBuffer = tld_data)
		
		# build defaults values.
		default_data_str = self.build_defaults(module_types, module_enums, Ctx)
		type_lookup_str, type_data_str = self.write_types(module_types, False)
		
		# build the real data-library here with default values!
		tld_data = self.write_lib(module_types, type_lookup_str, type_data_str, module_enums, enum_lookup_str, enum_data_str, default_data_str, True)
		
		self.stream.write(tld_data)
		
	def __init__(self, out):
		self.stream = out
		self.verbose = True
