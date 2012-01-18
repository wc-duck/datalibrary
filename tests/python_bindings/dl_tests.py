''' copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info '''

import unittest

import sys, os
sys.path.append(os.path.dirname(__file__) + '/../../bind/python/')
from libdl import DLContext

g_DLLPath = None

class DL(unittest.TestCase):
	def setUp(self):
		self.dl_ctx = DLContext( dll_path = g_DLLPath )
		self.dl_ctx.LoadTypeLibraryFromFile(os.path.dirname(__file__) + '/../../local/generated/unittest.bin')
		
	def tearDown(self):
		self.dl_ctx = None

class TestLibDL( DL ):
	# helper asserts
	def AssertTypeEqual(self, name, val1, val2):
		t1 = type(val1)
		t2 = type(val2)
		self.assertTrue( ( t1 in [ int, long ] and t2 in [ int, long ] ) # would like to check this but it should be possible to write the_int = 1 even if unit64 
						 or 
						 t1 == t2, 
						 'type of member %s differs, %s != %s' % ( name, t1, t2 ) )
		
		if isinstance( val1, (list, tuple) ):
			for i in range(len(val1)):
				self.AssertTypeEqual( name + '[%u]' % i, val1[i], val2[i] )
	
	def AssertInstanceEqual(self, I1, I2):
		self.assertEqual(type(I1),      type(I2))
		self.assertEqual(dir(I1),       dir(I2))
		self.assertEqual(I1.TYPE_ID,    I2.TYPE_ID)
		self.assertEqual(I1.__class__,  I2.__class__)
		self.assertEqual(I1.__dict__,   I2.__dict__)
		
		for key in I1.__dict__.keys():
			self.AssertTypeEqual(key, I1.__dict__[key], I2.__dict__[key])
				
	def AssertArrayHasSameType(self, Array):
		FirstType = type(Array[0])
		for item in Array:
			self.assertEqual(FirstType, type(item))
	
	def AssertArrayEqual(self, A1, A2):
		self.assertEqual(len(A1), len(A2))
		
		for i in range(0, len(A1)):
			self.assertEqual(A1[i], A2[i])
	
	def AssertDefaultPods(self, Pods):		
		self.assertEqual(0, Pods.i8)
		self.assertEqual(0, Pods.i16)
		self.assertEqual(0, Pods.i32)
		self.assertEqual(0, Pods.i64)
		self.assertEqual(0, Pods.u8)
		self.assertEqual(0, Pods.u16)
		self.assertEqual(0, Pods.u32)
		self.assertEqual(0, Pods.u64)
		self.assertEqual(0, Pods.f32)
		self.assertEqual(0, Pods.f64)
	
	
	# functionality tests
	def testCantAddExtraMember(self):
		Pods     = self.dl_ctx.types.Pods()
		
		'''def ShouldRaiseException():
			Pods.m_Whooo = 1
		
		self.assertRaises(AttributeError, ShouldRaiseException)
		
		# test that a real set will not raise an exception!
		Pods.m_i32 = 1337
		self.assertEqual(1337, Pods.m_i32)'''
	
	
	# structure tests
	def testHasCorrectMembersPod1(self):
		Pods = self.dl_ctx.types.Pods()
		self.AssertDefaultPods(Pods)
		
	def testHasCorrectMembersMorePods(self):
		Instance = self.dl_ctx.types.MorePods()
		
		self.AssertDefaultPods(Instance.Pods1)
		self.AssertDefaultPods(Instance.Pods2)
		
	def testHasCorrectMembersPod2InStructInStruct(self):
		Instance = self.dl_ctx.types.Pod2InStructInStruct()
		
		self.assertEqual(Instance.p2struct.Pod1.__class__, Instance.p2struct.Pod2.__class__)
		self.assertEqual(0, Instance.p2struct.Pod1.Int1)
		self.assertEqual(0, Instance.p2struct.Pod1.Int2)
		self.assertEqual(0, Instance.p2struct.Pod2.Int1)
		self.assertEqual(0, Instance.p2struct.Pod2.Int2)
		
	def testHasCorrectMembersPodArray1(self):
		Instance = self.dl_ctx.types.PodArray1()
		self.assertEqual(0, len(Instance.u32_arr))
	
	def testHasCorrectMembersWithInlineArray(self):
		Instance = self.dl_ctx.types.WithInlineArray()
		
		self.assertEqual(3, len(Instance.Array))
		self.AssertArrayHasSameType(Instance.Array)

	def testHasCorrectMembersWithInlineStructArray(self):
		Instance = self.dl_ctx.types.WithInlineStructArray()
		
		self.assertEqual(3, len(Instance.Array))
		self.AssertArrayHasSameType(Instance.Array)
	
	def testHasCorrectMembersWithInlineStructStructArray(self):
		Instance = self.dl_ctx.types.WithInlineStructStructArray()
		
		self.assertEqual(2, len(Instance.Array))
		self.AssertArrayHasSameType(Instance.Array)
		
	def testHasCorrectMembersStrings(self):
		Instance = self.dl_ctx.types.Strings()
		
		self.assertEqual(str, type(Instance.Str1))
		self.assertEqual(str, type(Instance.Str2))
		self.assertEqual('', Instance.Str1)
		self.assertEqual('', Instance.Str2)
		
	def testHasCorrectMembersStringInlineArray(self):
		Instance = self.dl_ctx.types.StringInlineArray()
		
		self.assertEqual(3, len(Instance.Strings))
		self.assertEqual(str, type(Instance.Strings[0]))
		self.AssertArrayHasSameType(Instance.Strings)
		
	'''def testHasCorrectMembersSimplePtr(self):
		Type     = self.dl_ctx.CreateType('SimplePtr')
		Instance = Type()
		
		self.AssertNumMembers(2, Instance)
		self.assertEqual(None, Instance.m_Ptr1)
		self.assertEqual(None, Instance.m_Ptr2)
		
	def testHasCorrectMembersPtrChain(self):
		Type     = self.dl_ctx.CreateType('PtrChain')
		Instance = Type()

		self.AssertNumMembers(2, Instance)
		self.assertEqual(0, Instance.m_Int)
		self.assertEqual(None, Instance.m_Next)
		
	def testHasCorrectMembersDoublePtrChain(self):
		Type     = self.dl_ctx.CreateType('DoublePtrChain')
		Instance = Type()

		self.AssertNumMembers(3, Instance)
		self.assertEqual(0, Instance.m_Int)
		self.assertEqual(None, Instance.m_Next)
		self.assertEqual(None, Instance.m_Prev)'''

	# store/load tests
	def do_the_round_about(self, typename, instance):
		loaded = self.dl_ctx.LoadInstance( typename, self.dl_ctx.StoreInstance(instance) )
		self.AssertInstanceEqual( instance, loaded )
		
		via_text = self.dl_ctx.LoadInstanceFromString( typename, self.dl_ctx.StoreInstanceToString( instance ) )
		self.AssertInstanceEqual( instance, via_text )
	
	def testWritePods(self):
		Instance = self.dl_ctx.types.Pods()
		
		Instance.i8  = 1
		Instance.i16 = 2
		Instance.i32 = 3
		Instance.i64 = 4
		Instance.u8  = 5
		Instance.u16 = 6
		Instance.u32 = 7
		Instance.u64 = 8
		Instance.f32 = 9.0
		Instance.f64 = 10.0
		
		self.do_the_round_about('Pods', Instance)
		
	def testWriteMorePods(self):
		Instance = self.dl_ctx.types.MorePods()
		
		Instance.Pods1.i8  = 1
		Instance.Pods1.i16 = 2
		Instance.Pods1.i32 = 3
		Instance.Pods1.i64 = 4
		Instance.Pods1.u8  = 5
		Instance.Pods1.u16 = 6
		Instance.Pods1.u32 = 7
		Instance.Pods1.u64 = 8
		Instance.Pods1.f32 = 9.0
		Instance.Pods1.f64 = 10.0
		
		Instance.Pods2.i8  = 11
		Instance.Pods2.i16 = 12
		Instance.Pods2.i32 = 13
		Instance.Pods2.i64 = 14
		Instance.Pods2.u8  = 15
		Instance.Pods2.u16 = 16
		Instance.Pods2.u32 = 17
		Instance.Pods2.u64 = 18
		Instance.Pods2.f32 = 19.0
		Instance.Pods2.f64 = 20.0
		
		self.do_the_round_about('MorePods', Instance)

	def testWritePod2InStruct(self):
		Instance = self.dl_ctx.types.Pod2InStruct()

		Instance.Pod1.Int1 = 133
		Instance.Pod1.Int2 = 7		
		Instance.Pod2.Int1 = 13
		Instance.Pod2.Int2 = 37
		
		self.do_the_round_about('Pod2InStruct', Instance)
	
	def testWritePodArray1(self):
		Instance = self.dl_ctx.types.PodArray1()
		
		Instance.u32_arr = [ 1337, 7331, 13, 37 ]
		
		self.do_the_round_about('PodArray1', Instance)
		
	def testWritePodArray2(self):
		Instance = self.dl_ctx.types.PodArray2()
		
		Instance.sub_arr = [ self.dl_ctx.types.PodArray1() for i in range(3) ]
		Instance.sub_arr[0].u32_arr = [ 1, 2, 3 ]
		Instance.sub_arr[1].u32_arr = [ 3, 4, 5 ]
		Instance.sub_arr[2].u32_arr = [ 6, 7, 8 ]
		
		self.do_the_round_about('PodArray2', Instance)
		
	def testWriteWithInlineArray(self):
		Instance = self.dl_ctx.types.WithInlineArray()
		
		Instance.Array = [ i for i in range(1,len(Instance.Array) + 1) ]
		
		self.do_the_round_about('WithInlineArray', Instance)
	
	def testWriteWithInlineStructArray(self):
		Instance = self.dl_ctx.types.WithInlineStructArray()
		
		for i in range(0,len(Instance.Array)):
			Instance.Array[i].Int1 = i + 1
			Instance.Array[i].Int2 = i + 2
		
		self.do_the_round_about('WithInlineStructArray', Instance)

	def testWriteStrings(self):
		Instance = self.dl_ctx.types.Strings()
		
		Instance.Str1 = 'Cow'
		Instance.Str2 = 'Bell'
		
		self.do_the_round_about('Strings', Instance)

	def testWriteStringInlineArray(self):
		Instance = self.dl_ctx.types.StringInlineArray()
		
		Instance.Strings = [ 'Cow', 'bells', 'RULE!' ]
		
		self.do_the_round_about('StringInlineArray', Instance)
		
	def testWriteStringArray(self):
		Instance = self.dl_ctx.types.StringArray()
		
		Instance.Strings = [ 'Cow', 'bells', 'RULE!', 'and', 'win the game!' ]
		
		self.do_the_round_about('StringArray', Instance)

	def testWriteTestBits(self):
		Instance = self.dl_ctx.types.TestBits()
		
		Instance.Bit1 = 1
		Instance.Bit2 = 2
		Instance.Bit3 = 3
		Instance.make_it_uneven = 17
		Instance.Bit4 = 1
		Instance.Bit5 = 2
		Instance.Bit6 = 2
		
		self.do_the_round_about('TestBits', Instance)
		
	def testWriteMoreBits(self):
		Instance = self.dl_ctx.types.MoreBits()
		
		Instance.Bit1 = 512
		Instance.Bit2 = 17
		
		self.do_the_round_about('MoreBits', Instance)
		
	'''def testWriteSimplePtr(self):
		Type     = self.dl_ctx.CreateType('SimplePtr')
		Instance = Type()
		
		Pointee = self.dl_ctx.CreateType('Pods')()
		Pointee.m_i32 = 1337
		
		Instance.m_Ptr1 = Pointee
		Instance.m_Ptr2 = Pointee
		
		self.assertEqual(id(Instance.m_Ptr1), id(Instance.m_Ptr2))
		
		res_buffer = self.dl_ctx.StoreInstance(Instance)
		LoadInstance = self.dl_ctx.LoadInstance('SimplePtr', res_buffer)
		
		self.assertEqual(1337, LoadInstance.m_Ptr1.m_i32)
		self.assertEqual(1337, LoadInstance.m_Ptr2.m_i32)
		self.assertEqual(id(LoadInstance.m_Ptr1), id(LoadInstance.m_Ptr2))
		
	def testWritePtrChain(self):
		Type     = self.dl_ctx.CreateType('PtrChain')
		Instance = Type()
		Instance2 = Type()
		
		Instance.m_Int = 1337
		Instance.m_Next = Instance2
		Instance2.m_Int = 7331
		Instance2.m_Next = None
		
		res_buffer = self.dl_ctx.StoreInstance(Instance)
		LoadInstance = self.dl_ctx.LoadInstance('PtrChain', res_buffer)
		
		self.assertEqual(Instance.m_Int,        LoadInstance.m_Int)
		self.assertEqual(Instance.m_Next.m_Int, LoadInstance.m_Next.m_Int)
		self.assertEqual(None, LoadInstance.m_Next.m_Next)
	
	def testWritePtrChain(self):
		Type     = self.dl_ctx.CreateType('DoublePtrChain')
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
		
		res_buffer = self.dl_ctx.StoreInstance(Instance)
		LoadInstance = self.dl_ctx.LoadInstance('DoublePtrChain', res_buffer)
		
		self.assertEqual(None,    LoadInstance.m_Prev)
		self.assertNotEqual(None, LoadInstance.m_Next)
		self.assertNotEqual(None, LoadInstance.m_Next.m_Next)
		self.assertEqual(None,    LoadInstance.m_Next.m_Next.m_Next)
		
		self.assertEqual(id(LoadInstance),        id(LoadInstance.m_Next.m_Prev))
		self.assertEqual(id(LoadInstance.m_Next), id(LoadInstance.m_Next.m_Next.m_Prev))
		
		self.assertEqual(Instance.m_Int,               LoadInstance.m_Int)
		self.assertEqual(Instance.m_Next.m_Int,        LoadInstance.m_Next.m_Int)
		self.assertEqual(Instance.m_Next.m_Next.m_Int, LoadInstance.m_Next.m_Next.m_Int)'''
		
	def testWriteEnum(self):
		Instance = self.dl_ctx.types.TestingEnum()
		Instance.TheEnum = self.dl_ctx.enums.TestEnum1.TESTENUM1_VALUE2
		self.do_the_round_about('TestingEnum', Instance)
		
	def testWriteEnumInlineArray(self):
		Instance = self.dl_ctx.types.InlineArrayEnum()
		Instance.EnumArr = [ self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE2,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE4,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE3,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE1 ]
		self.do_the_round_about('InlineArrayEnum', Instance)
		
	def testWriteEnumArray(self):
		Instance = self.dl_ctx.types.ArrayEnum()
		Instance.EnumArr = [ self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE2,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE4,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE3,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE1,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE3,
						     self.dl_ctx.enums.TestEnum2.TESTENUM2_VALUE1 ]
		self.do_the_round_about('ArrayEnum', Instance)
		
if __name__ == '__main__':
    g_DLLPath = sys.argv[1]
    del sys.argv[1]
    unittest.main()
