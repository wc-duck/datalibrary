// replace me with real tests later!

using System;
using NUnit.Framework; 

[TestFixture]
public class DLTest
{
	private DLContext ctx;
	
	private T DoTheRoundAbout<T>( T obj )
	{
		string str_from_obj = ctx.StoreInstanceToString( obj );
		object obj_from_str = ctx.LoadInstanceFromString<T>( str_from_obj ); 
		byte[] buf_from_obj = ctx.StoreInstance( obj_from_str );
    	return ctx.LoadInstance<T>( buf_from_obj );
	}
	
	[SetUp]
    public void Init()
    {
    	ctx = new DLContext();
        ctx.LoadTypeLibraryFromFile("../../local/generated/unittest.bin");
    }
    
    [TearDown] public void Dispose() { ctx = null; }

    [Test]
    public void testPods()
    {
    	Pods p1 = new Pods();
    	p1.i8  = 1;
    	p1.i16 = 2;
    	p1.i32 = 3;
    	p1.i64 = 4;
    	p1.u8  = 5;
    	p1.u16 = 6;
    	p1.u32 = 7;
    	p1.u64 = 8;
    	p1.f32 = 9.0f;
    	p1.f64 = 1337.0;
    	
    	Pods p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.i8,  p2.i8 );
    	Assert.AreEqual( p1.i16, p2.i16 );
    	Assert.AreEqual( p1.i32, p2.i32 );
    	Assert.AreEqual( p1.i64, p2.i64 );
    	Assert.AreEqual( p1.u8,  p2.u8 );
    	Assert.AreEqual( p1.u16, p2.u16 );
    	Assert.AreEqual( p1.u32, p2.u32 );
    	Assert.AreEqual( p1.u64, p2.u64 );
    	Assert.AreEqual( p1.f32, p2.f32 );
    	Assert.AreEqual( p1.f64, p2.f64 );
    }
    
    [Test]
    public void testStructInStruct()
    {
    	MorePods p1  = new MorePods();
    	p1.Pods1.i8  = 1;    p1.Pods1.i16 = 2;     p1.Pods1.i32 = 3; p1.Pods1.i64 = 4;
    	p1.Pods1.u8  = 5;    p1.Pods1.u16 = 6;     p1.Pods1.u32 = 7; p1.Pods1.u64 = 8;
    	p1.Pods1.f32 = 9.0f; p1.Pods1.f64 = 10.0f;
    	
    	p1.Pods2.i8  = 11;    p1.Pods2.i16 = 12;    p1.Pods2.i32 = 13; p1.Pods2.i64 = 14;
    	p1.Pods2.u8  = 15;    p1.Pods2.u16 = 16;    p1.Pods2.u32 = 17; p1.Pods2.u64 = 18;
    	p1.Pods2.f32 = 19.0f; p1.Pods2.f64 = 20.0f;
    	
    	MorePods p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.Pods1.i8,  p2.Pods1.i8 );
    	Assert.AreEqual( p1.Pods1.i16, p2.Pods1.i16 );
    	Assert.AreEqual( p1.Pods1.i32, p2.Pods1.i32 );
    	Assert.AreEqual( p1.Pods1.i64, p2.Pods1.i64 );
    	Assert.AreEqual( p1.Pods1.u8,  p2.Pods1.u8 );
    	Assert.AreEqual( p1.Pods1.u16, p2.Pods1.u16 );
    	Assert.AreEqual( p1.Pods1.u32, p2.Pods1.u32 );
    	Assert.AreEqual( p1.Pods1.u64, p2.Pods1.u64 );
    	Assert.AreEqual( p1.Pods1.f32, p2.Pods1.f32 );
    	Assert.AreEqual( p1.Pods1.f64, p2.Pods1.f64 );
    	
    	Assert.AreEqual( p1.Pods2.i8,  p2.Pods2.i8 );
    	Assert.AreEqual( p1.Pods2.i16, p2.Pods2.i16 );
    	Assert.AreEqual( p1.Pods2.i32, p2.Pods2.i32 );
    	Assert.AreEqual( p1.Pods2.i64, p2.Pods2.i64 );
    	Assert.AreEqual( p1.Pods2.u8,  p2.Pods2.u8 );
    	Assert.AreEqual( p1.Pods2.u16, p2.Pods2.u16 );
    	Assert.AreEqual( p1.Pods2.u32, p2.Pods2.u32 );
    	Assert.AreEqual( p1.Pods2.u64, p2.Pods2.u64 );
    	Assert.AreEqual( p1.Pods2.f32, p2.Pods2.f32 );
    	Assert.AreEqual( p1.Pods2.f64, p2.Pods2.f64 );
    }
    
    [Test]
    public void testStrings()
    {
    	Strings s1 = new Strings();
    	s1.Str1 = "cow";
    	s1.Str2 = "bells";
    	
    	Strings s2 = DoTheRoundAbout( s1 );
    	
    	Assert.AreEqual( s1.Str1, s2.Str1 );
    	Assert.AreEqual( s1.Str2, s2.Str2 );
    }
    
    [Test]
    public void testEnum()
    {
    	TestingEnum e1 = new TestingEnum();
    	e1.TheEnum = TestEnum1.TESTENUM1_VALUE3;
    	
    	TestingEnum e2 = DoTheRoundAbout( e1 );
    	
    	Assert.AreEqual( e1.TheEnum, e2.TheEnum );
    }
    
    [Test]
    public void testInlineArrayPod()
    {
    	WithInlineArray p1 = new WithInlineArray();
    	p1.Array = new uint[3]{ 1337, 1338, 1339 };
    	
    	WithInlineArray p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.Array, p2.Array );
    } 
    
    [Test]
    public void testInlineArrayStruct()
    {
    	WithInlineStructArray p1 = new WithInlineStructArray();
    	p1.Array[0].Int1 = 1337;
		p1.Array[0].Int2 = 1338;
		p1.Array[1].Int1 = 1339;
		p1.Array[1].Int2 = 1340;
		p1.Array[2].Int1 = 1341;
		p1.Array[2].Int2 = 1342;
		
		WithInlineStructArray p2 = DoTheRoundAbout( p1 );
		
		Assert.AreEqual( p1.Array, p2.Array );
    }
    
    [Test]
    public void testInlineArrayStructInStruct()
    {
    	WithInlineStructStructArray p1 = new WithInlineStructStructArray();
    	p1.Array[0].Array[0].Int1 = 1337;
    	p1.Array[0].Array[0].Int2 = 1338;
    	p1.Array[0].Array[1].Int1 = 1339;
    	p1.Array[0].Array[1].Int2 = 1340;
    	p1.Array[0].Array[2].Int1 = 1341;
    	p1.Array[0].Array[2].Int2 = 1342;
    	
    	p1.Array[1].Array[0].Int1 = 1343;
    	p1.Array[1].Array[0].Int2 = 1344;
    	p1.Array[1].Array[1].Int1 = 1345;
    	p1.Array[1].Array[1].Int2 = 1346;
    	p1.Array[1].Array[2].Int1 = 1347;
    	p1.Array[1].Array[2].Int2 = 1348;
    
    	Assert.IsTrue( false );
    	/* broken test!
    		
    	WithInlineStructStructArray p2 = (WithInlineStructStructArray)DoTheRoundAbout( typeof( WithInlineStructStructArray ), p1 );
    	
    	Assert.AreEqual( p1.Array[0].Array, p2.Array[0].Array );
    	Assert.AreEqual( p1.Array[1].Array, p2.Array[1].Array );
    	Assert.AreEqual( p1.Array[2].Array, p2.Array[2].Array );
    	*/
    }
    
    [Test]
    public void testInlineArrayString()
    {
    	StringInlineArray p1 = new StringInlineArray();
    	p1.Strings = new string[3] { "cow", "bells", "rules" };
    	
    	StringInlineArray p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.Strings, p2.Strings );
    }
    
    [Test]
    public void testInlineArrayEnum()
    {
    	InlineArrayEnum p1 = new InlineArrayEnum();
    	p1.EnumArr = new TestEnum2[] { TestEnum2.TESTENUM2_VALUE1, TestEnum2.TESTENUM2_VALUE2, TestEnum2.TESTENUM2_VALUE3, TestEnum2.TESTENUM2_VALUE4 };
    	
    	InlineArrayEnum p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.EnumArr, p2.EnumArr );
    }
    
    [Test]
    public void testArrayPod()
    {
    	PodArray1 p1 = new PodArray1();
    	p1.u32_arr = new uint[] { 1337, 7331, 73, 31, 13, 37 };
    	p1.u32_arrArraySize = (uint)p1.u32_arr.Length;
    	
    	PodArray1 p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.u32_arr, p2.u32_arr );
    }
    
    [Test]
    public void testArrayWithSubArray()
    {
    	PodArray2 p1 = new PodArray2();
    	p1.sub_arr = new PodArray1[] { new PodArray1(), new PodArray1() };
    	
    	p1.sub_arr[0].u32_arr = new uint[] { 1337, 7313 };
    	p1.sub_arr[1].u32_arr = new uint[] { 1, 3, 3, 7 };
    	
    	Assert.IsTrue( false );
    	/* broken test!

    	PodArray2 p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.sub_arr[0].u32_arr, p2.sub_arr[0].u32_arr );
    	Assert.AreEqual( p1.sub_arr[1].u32_arr, p2.sub_arr[1].u32_arr );
    	*/
    }
    
    [Test]
    public void testArrayString()
    {
    	StringArray p1 = new StringArray();
    	p1.Strings = new string[5] { "cow", "bells", "are", "cool", "stuff" };

    	Assert.IsTrue( false );
    	/* broken test!
    	
    	StringArray p2 = DoTheRoundAbout( p1 );
    	
    	Assert.AreEqual( p1.Strings, p2.Strings );
    	*/
    }
    
    [Test]
    public void testArrayStruct()
    {
    	// write me!
    }
    
    [Test]
    public void testArrayEnum()
    {
    	// write me!
    }
    
    [Test]
    public void testBitfield()
    {
    	TestBits tb1 = new TestBits();
    	tb1.Bit1 = 0;
    	tb1.Bit2 = 2;
    	tb1.Bit3 = 4;
    	tb1.make_it_uneven = 17;
		tb1.Bit4 = 1;
		tb1.Bit5 = 0;
		tb1.Bit6 = 5;
		
		TestBits tb2 = DoTheRoundAbout( tb1 );
		
		Assert.AreEqual( tb1.Bit1,           tb2.Bit1 );
		Assert.AreEqual( tb1.Bit2,           tb2.Bit2 );
		Assert.AreEqual( tb1.Bit3,           tb2.Bit3 );
		Assert.AreEqual( tb1.make_it_uneven, tb2.make_it_uneven );
		Assert.AreEqual( tb1.Bit4,           tb2.Bit4 );
		Assert.AreEqual( tb1.Bit5,           tb2.Bit5 );
		Assert.AreEqual( tb1.Bit6,           tb2.Bit6 );
    }
    
    // add tests for pointers here!!!
    
    // add tests for empty arrays here!!!
}
