// replace me with real tests later!

using System;
using NUnit.Framework; 

[TestFixture]
public class DLTest
{
	private DLContext ctx;
	
	private object DoTheRoundAbout( Type type, object obj )
	{
		string str_from_obj = ctx.StoreInstanceToString( obj );
		object obj_from_str = ctx.LoadInstanceFromString( type, str_from_obj ); 
		byte[] buf_from_obj = ctx.StoreInstance( obj_from_str );
    	return ctx.LoadInstance( type, buf_from_obj );
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
    	
    	Pods p2 = (Pods)DoTheRoundAbout( typeof( Pods ), p1 );
    	
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
    	
    	MorePods p2 = (MorePods)DoTheRoundAbout( typeof( MorePods ), p1 );
    	
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
}
