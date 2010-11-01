''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

import unittest

import sys, os
sys.path.append(os.path.dirname(__file__) + '/../bind/python/')
from libdl import DLContext

NUMBER_OF_EXTRA_MEMBERS = 9

class TestLibDL(unittest.TestCase):
	# unittest specific
	def setUp(self):
		self.DLContext = DLContext()
		import os
		self.DLContext.LoadTypeLibraryFromFile(os.path.dirname(__file__) + '/../../../local/generated/dl/unittest.bin')
		
	def tearDown(self):
		self.DLContext = None
	
	
	# helper asserts
	def AssertNumMembers(self, Members, Instance):
		self.assertEqual(Members, len(Instance.__class__.__dict__) - NUMBER_OF_EXTRA_MEMBERS)
	
	def AssertInstanceEqual(self, I1, I2):
		self.assertEqual(I1._DL_TYPE_ID, I2._DL_TYPE_ID)
		
		for m in dir(I1):
			if m.startswith('m_'):
				attr1 = getattr(I1, m)
				attr2 = getattr(I2, m)
				
				# make this work!
				#self.assertEqual(type(attr1), type(attr2))
				
				if type(attr1) == list:
					self.assertEqual(type(attr1), type(attr2))
					self.assertEqual(attr1[0], attr2[0])
					for i in range(0, len(attr1)):
						self.assertEqual(attr1[i], attr2[i])
				else:
					self.assertEqual(attr1, attr2)
				
	def AssertArrayHasSameType(self, Array):
		FirstType = type(Array[0])
		for item in Array:
			self.assertEqual(FirstType, type(item))
	
	def AssertArrayEqual(self, A1, A2):
		self.assertEqual(len(A1), len(A2))
		
		for i in range(0, len(A1)):
			self.assertEqual(A1[i], A2[i])
	
	def AssertDefaultPods(self, Pods):
		self.AssertNumMembers(10, Pods)
		
		self.assertEqual(0, Pods.m_int8)
		self.assertEqual(0, Pods.m_int16)
		self.assertEqual(0, Pods.m_int32)
		self.assertEqual(0, Pods.m_int64)
		self.assertEqual(0, Pods.m_uint8)
		self.assertEqual(0, Pods.m_uint16)
		self.assertEqual(0, Pods.m_uint32)
		self.assertEqual(0, Pods.m_uint64)
		self.assertEqual(0, Pods.m_fp32)
		self.assertEqual(0, Pods.m_fp64)
	
	
	# functionality tests
	def testHasSameTypeID(self):
		PodsType = self.DLContext.CreateType('Pods')
		Pods1    = PodsType()
		Pods2    = PodsType()
		
		self.assert_(hasattr(Pods1, '_DL_TYPE_ID'))
		self.assert_(hasattr(Pods2, '_DL_TYPE_ID'))
		
		self.assertEqual(Pods1._DL_TYPE_ID, Pods2._DL_TYPE_ID)
	
	def testCantAddExtraMember(self):
		PodsType = self.DLContext.CreateType('Pods')
		Pods     = PodsType()
		
		def ShouldRaiseException():
			Pods.m_Whooo = 1
		
		self.assertRaises(AttributeError, ShouldRaiseException)
		
		# test that a real set will not raise an exception!
		Pods.m_int32 = 1337
		self.assertEqual(1337, Pods.m_int32)
	
	
	# structure tests
	def testHasCorrectMembersPod1(self):
		PodsType = self.DLContext.CreateType('Pods')
		Pods     = PodsType()
		self.AssertDefaultPods(Pods)
		
	def testHasCorrectMembersMorePods(self):
		Type     = self.DLContext.CreateType('MorePods')
		Instance = Type()
		
		self.AssertNumMembers(2, Instance)
		self.AssertDefaultPods(Instance.m_Pods1)
		self.AssertDefaultPods(Instance.m_Pods2)
		
	def testHasCorrectMembersPod2InStructInStruct(self):
		Type     = self.DLContext.CreateType('Pod2InStructInStruct')
		Instance = Type()
		
		self.AssertNumMembers(1, Instance)
		self.AssertNumMembers(2, Instance.m_Pod2InStruct)
		self.AssertNumMembers(2, Instance.m_Pod2InStruct)
		self.AssertNumMembers(2, Instance.m_Pod2InStruct)
		self.assertEqual(Instance.m_Pod2InStruct.m_Pod1.__class__, Instance.m_Pod2InStruct.m_Pod2.__class__)
		self.assertEqual(0, Instance.m_Pod2InStruct.m_Pod1.m_Int1)
		self.assertEqual(0, Instance.m_Pod2InStruct.m_Pod1.m_Int2)
		self.assertEqual(0, Instance.m_Pod2InStruct.m_Pod2.m_Int1)
		self.assertEqual(0, Instance.m_Pod2InStruct.m_Pod2.m_Int2)
		
	def testHasCorrectMembersPodArray1(self):
		Type     = self.DLContext.CreateType('PodArray1')
		Instance = Type()
		
		self.AssertNumMembers(1, Instance)
		self.assertEqual(0, len(Instance.m_Array))
	
	def testHasCorrectMembersWithInlineArray(self):
		Type     = self.DLContext.CreateType('WithInlineArray')
		Instance = Type()
		
		self.AssertNumMembers(1, Instance)
		self.assertEqual(3, len(Instance.m_Array))
		self.AssertArrayHasSameType(Instance.m_Array)

	def testHasCorrectMembersWithInlineStructArray(self):
		Type     = self.DLContext.CreateType('WithInlineStructArray')
		Instance = Type()
		
		self.AssertNumMembers(1, Instance)
		self.assertEqual(3, len(Instance.m_Array))
		self.AssertArrayHasSameType(Instance.m_Array)
	
	def testHasCorrectMembersWithInlineStructStructArray(self):
		Type     = self.DLContext.CreateType('WithInlineStructStructArray')
		Instance = Type()
		
		self.AssertNumMembers(1, Instance)
		self.assertEqual(2, len(Instance.m_Array))
		self.AssertArrayHasSameType(Instance.m_Array)
		
	def testHasCorrectMembersStrings(self):
		Type     = self.DLContext.CreateType('Strings')
		Instance = Type()
		
		self.AssertNumMembers(2, Instance)
		self.assertEqual(str, type(Instance.m_Str1))
		self.assertEqual(str, type(Instance.m_Str2))
		self.assertEqual('', Instance.m_Str1)
		self.assertEqual('', Instance.m_Str2)
		
	def testHasCorrectMembersStringInlineArray(self):
		Type     = self.DLContext.CreateType('StringInlineArray')
		Instance = Type()
		
		self.AssertNumMembers(1, Instance)
		self.assertEqual(3, len(Instance.m_Strings))
		self.assertEqual(str, type(Instance.m_Strings[0]))
		self.AssertArrayHasSameType(Instance.m_Strings)
	
	def testHasCorrectMembersTestBits(self):
		Type     = self.DLContext.CreateType('TestBits')
		Instance = Type()
		
		self.AssertNumMembers(7, Instance)
		
	def testHasCorrectMembersTestBits(self):
		Type     = self.DLContext.CreateType('MoreBits')
		Instance = Type()
		
		self.AssertNumMembers(2, Instance)
		
	def testHasCorrectMembersSimplePtr(self):
		Type     = self.DLContext.CreateType('SimplePtr')
		Instance = Type()
		
		self.AssertNumMembers(2, Instance)
		self.assertEqual(None, Instance.m_Ptr1)
		self.assertEqual(None, Instance.m_Ptr2)
		
	def testHasCorrectMembersPtrChain(self):
		Type     = self.DLContext.CreateType('PtrChain')
		Instance = Type()

		self.AssertNumMembers(2, Instance)
		self.assertEqual(0, Instance.m_Int)
		self.assertEqual(None, Instance.m_Next)
		
	def testHasCorrectMembersDoublePtrChain(self):
		Type     = self.DLContext.CreateType('DoublePtrChain')
		Instance = Type()

		self.AssertNumMembers(3, Instance)
		self.assertEqual(0, Instance.m_Int)
		self.assertEqual(None, Instance.m_Next)
		self.assertEqual(None, Instance.m_Prev)

	# store/load tests
	def testWritePods(self):
		Type     = self.DLContext.CreateType('Pods')
		Instance = Type()
		
		Instance.m_int8   = 1
		Instance.m_int16  = 2
		Instance.m_int32  = 3
		Instance.m_int64  = 4
		Instance.m_uint8  = 5
		Instance.m_uint16 = 6
		Instance.m_uint32 = 7
		Instance.m_uint64 = 8
		Instance.m_fp32   = 9
		Instance.m_fp64   = 10
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('Pods', res_buffer)
		
		self.AssertInstanceEqual(Instance, LoadInstance)
		
	def testWriteMorePods(self):
		Type     = self.DLContext.CreateType('MorePods')
		Instance = Type()
		
		Instance.m_Pods1.m_int8   = 1
		Instance.m_Pods1.m_int16  = 2
		Instance.m_Pods1.m_int32  = 3
		Instance.m_Pods1.m_int64  = 4
		Instance.m_Pods1.m_uint8  = 5
		Instance.m_Pods1.m_uint16 = 6
		Instance.m_Pods1.m_uint32 = 7
		Instance.m_Pods1.m_uint64 = 8
		Instance.m_Pods1.m_fp32   = 9
		Instance.m_Pods1.m_fp64   = 10
		
		Instance.m_Pods2.m_int8   = 11
		Instance.m_Pods2.m_int16  = 12
		Instance.m_Pods2.m_int32  = 13
		Instance.m_Pods2.m_int64  = 14
		Instance.m_Pods2.m_uint8  = 15
		Instance.m_Pods2.m_uint16 = 16
		Instance.m_Pods2.m_uint32 = 17
		Instance.m_Pods2.m_uint64 = 18
		Instance.m_Pods2.m_fp32   = 19
		Instance.m_Pods2.m_fp64   = 20
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('MorePods', res_buffer)
		
		self.AssertInstanceEqual(Instance.m_Pods1, LoadInstance.m_Pods1)
		self.AssertInstanceEqual(Instance.m_Pods2, LoadInstance.m_Pods2)
	
	def testWritePod2InStruct(self):
		Type     = self.DLContext.CreateType('Pod2InStruct')
		Instance = Type()

		Instance.m_Pod1.m_Int1 = 133
		Instance.m_Pod1.m_Int2 = 7		
		Instance.m_Pod2.m_Int1 = 13
		Instance.m_Pod2.m_Int2 = 37
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('Pod2InStruct', res_buffer)
		
		self.AssertInstanceEqual(Instance.m_Pod1, LoadInstance.m_Pod1)
		self.AssertInstanceEqual(Instance.m_Pod2, LoadInstance.m_Pod2)
	
	def testWritePodArray1(self):
		Type     = self.DLContext.CreateType('PodArray1')
		Instance = Type()
		
		Instance.m_Array = [ 1337, 7331, 13, 37 ]
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('PodArray1', res_buffer)
		
		self.assertEqual(len(LoadInstance.m_Array), len(Instance.m_Array))
		self.AssertArrayEqual(Instance.m_Array, LoadInstance.m_Array)
		
	def testWritePodArray2(self):
		Type     = self.DLContext.CreateType('PodArray2')
		Instance = Type()
		
		SubType  = self.DLContext.CreateType('PodArray1')
		
		Instance.m_Array = [ SubType() ] * 3
		Instance.m_Array[0].m_Array = [ 1, 2, 3 ]
		Instance.m_Array[1].m_Array = [ 3, 4, 5 ]
		Instance.m_Array[2].m_Array = [ 6, 7, 8 ]
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('PodArray2', res_buffer)
		
		self.assertEqual(len(LoadInstance.m_Array), len(Instance.m_Array))
		self.AssertArrayEqual(Instance.m_Array[0].m_Array, LoadInstance.m_Array[0].m_Array)
		self.AssertArrayEqual(Instance.m_Array[1].m_Array, LoadInstance.m_Array[1].m_Array)
		self.AssertArrayEqual(Instance.m_Array[2].m_Array, LoadInstance.m_Array[2].m_Array)
		
	def testWriteWithInlineArray(self):
		Type     = self.DLContext.CreateType('WithInlineArray')
		Instance = Type()
		
		for i in range(0,len(Instance.m_Array)):
			Instance.m_Array[i] = i + 1
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('WithInlineArray', res_buffer)
		
		self.assertEqual(len(LoadInstance.m_Array), len(Instance.m_Array))
		
		for i in range(0,len(Instance.m_Array)):
			self.assertEqual(LoadInstance.m_Array[i], Instance.m_Array[i])
	
	def testWriteWithInlineStructArray(self):
		Type     = self.DLContext.CreateType('WithInlineStructArray')
		Instance = Type()
		
		for i in range(0,len(Instance.m_Array)):
			Instance.m_Array[i].m_Int1 = i + 1
			Instance.m_Array[i].m_Int1 = i + 2
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('WithInlineStructArray', res_buffer)
		
		self.assertEqual(len(LoadInstance.m_Array), len(Instance.m_Array))
		
		for i in range(0,len(Instance.m_Array)):
			self.AssertInstanceEqual(LoadInstance.m_Array[i], Instance.m_Array[i])

	def testWriteStrings(self):
		Type     = self.DLContext.CreateType('Strings')
		Instance = Type()
		
		Instance.m_Str1 = 'Cow'
		Instance.m_Str2 = 'Bell'
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('Strings', res_buffer)
		
		self.AssertInstanceEqual(Instance, LoadInstance)

	def testWriteStringInlineArray(self):
		Type     = self.DLContext.CreateType('StringInlineArray')
		Instance = Type()
		
		Instance.m_Strings[0] = 'Cow'
		Instance.m_Strings[1] = 'bells'
		Instance.m_Strings[2] = 'RULE!'
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('StringInlineArray', res_buffer)
		
		self.AssertInstanceEqual(Instance, LoadInstance)
		
	def testWriteStringArray(self):
		Type     = self.DLContext.CreateType('StringArray')
		Instance = Type()
		
		Instance.m_Strings = [ 'Cow', 'bells', 'RULE!', 'and', 'win the game!' ]
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('StringArray', res_buffer)
		
		self.AssertInstanceEqual(Instance, LoadInstance)
		self.AssertArrayEqual(Instance.m_Strings, LoadInstance.m_Strings)

	def testWriteTestBits(self):
		Type     = self.DLContext.CreateType('TestBits')
		Instance = Type()
		
		Instance.m_Bit1 = 1
		Instance.m_Bit2 = 2
		Instance.m_Bit3 = 3
		Instance.m_make_it_uneven = 17
		Instance.m_Bit4 = 1
		Instance.m_Bit5 = 2
		Instance.m_Bit6 = 2
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('TestBits', res_buffer)
		
		self.AssertInstanceEqual(Instance, LoadInstance)
		
	def testWriteMoreBits(self):
		Type     = self.DLContext.CreateType('MoreBits')
		Instance = Type()
		
		Instance.m_Bit1 = 512
		Instance.m_Bit2 = 17
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('MoreBits', res_buffer)
		
		self.AssertInstanceEqual(Instance, LoadInstance)
		
	def testWriteSimplePtr(self):
		Type     = self.DLContext.CreateType('SimplePtr')
		Instance = Type()
		
		Pointee = self.DLContext.CreateType('Pods')()
		Pointee.m_int32 = 1337
		
		Instance.m_Ptr1 = Pointee
		Instance.m_Ptr2 = Pointee
		
		self.assertEqual(id(Instance.m_Ptr1), id(Instance.m_Ptr2))
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('SimplePtr', res_buffer)
		
		self.assertEqual(1337, LoadInstance.m_Ptr1.m_int32)
		self.assertEqual(1337, LoadInstance.m_Ptr2.m_int32)
		self.assertEqual(id(LoadInstance.m_Ptr1), id(LoadInstance.m_Ptr2))
		
	def testWritePtrChain(self):
		Type     = self.DLContext.CreateType('PtrChain')
		Instance = Type()
		Instance2 = Type()
		
		Instance.m_Int = 1337
		Instance.m_Next = Instance2
		Instance2.m_Int = 7331
		Instance2.m_Next = None
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('PtrChain', res_buffer)
		
		self.assertEqual(Instance.m_Int,        LoadInstance.m_Int)
		self.assertEqual(Instance.m_Next.m_Int, LoadInstance.m_Next.m_Int)
		self.assertEqual(None, LoadInstance.m_Next.m_Next)
	
	def testWritePtrChain(self):
		Type     = self.DLContext.CreateType('DoublePtrChain')
		Instance  = Type()
		Instance2 = Type()
		Instance3 = Type()
		
		Instance.m_Int  = 1337
		self.assertEqual(None, Instance.m_Prev)
		Instance.m_Next = Instance2
		
		Instance2.m_Int = 7331
		Instance2.m_Prev = Instance
		Instance2.m_Next = Instance3
		
		Instance3.m_Int = 3713
		Instance3.m_Prev = Instance2
		self.assertEqual(None, Instance3.m_Next)
		
		res_buffer = self.DLContext.StoreInstance(Instance)
		LoadInstance = self.DLContext.LoadInstance('DoublePtrChain', res_buffer)
		
		self.assertEqual(None,    LoadInstance.m_Prev)
		self.assertNotEqual(None, LoadInstance.m_Next)
		self.assertNotEqual(None, LoadInstance.m_Next.m_Next)
		self.assertEqual(None,    LoadInstance.m_Next.m_Next.m_Next)
		
		self.assertEqual(id(LoadInstance),        id(LoadInstance.m_Next.m_Prev))
		self.assertEqual(id(LoadInstance.m_Next), id(LoadInstance.m_Next.m_Next.m_Prev))
		
		self.assertEqual(Instance.m_Int,               LoadInstance.m_Int)
		self.assertEqual(Instance.m_Next.m_Int,        LoadInstance.m_Next.m_Int)
		self.assertEqual(Instance.m_Next.m_Next.m_Int, LoadInstance.m_Next.m_Next.m_Int)
		
		
if __name__ == '__main__':
    unittest.main()
