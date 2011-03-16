/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

public class DLContext
{
    [StructLayout(LayoutKind.Sequential, Size=48, CharSet=CharSet.Ansi)]
    private class dl_create_params
    {
    	public dl_create_params()
    	{
    		alloc_func     = (IntPtr)0;
    		free_func      = (IntPtr)0;
    		alloc_ctx      = (IntPtr)0;
    		error_msg_func = (IntPtr)0;
    		error_msg_ctx  = (IntPtr)0;
    	}
    	
        public IntPtr alloc_func;
	    public IntPtr free_func;
	    public IntPtr alloc_ctx;
	    public IntPtr error_msg_func;
	    public IntPtr error_msg_ctx;
    }
    
    private void CheckDLErrors( int error_code )
    {
        if (error_code > 0)
            throw new System.ApplicationException( string.Format( "DLError: {0}({1})", dl_error_to_string( error_code ), error_code ) );
    }

    public DLContext()
    {
        IntPtr ptr = Marshal.AllocHGlobal(8);
        
        dl_create_params p = new dl_create_params();
        
        IntPtr params_ptr = Marshal.AllocHGlobal(Marshal.SizeOf(p));
        Marshal.StructureToPtr( p, params_ptr, false );
        
        CheckDLErrors( dl_context_create( ptr, params_ptr ) );
        ctx = Marshal.ReadIntPtr(ptr);
        
        Marshal.FreeHGlobal(params_ptr);
    }

    ~DLContext() { CheckDLErrors( dl_context_destroy(ctx) ); }
    
    public void LoadTypeLibrary( byte[] tl_buffer )    { CheckDLErrors( dl_context_load_type_library( ctx, tl_buffer, tl_buffer.Length ) ); }
	public void LoadTypeLibraryFromFile( string file ) { LoadTypeLibrary( File.ReadAllBytes( file ) ); }
    
    public object LoadInstance( Type type, byte[] packed_instance )
    {
        IntPtr buffer = Marshal.AllocHGlobal( packed_instance.Length );

		CheckDLErrors( dl_instance_load( ctx, TypeIDOf( type ), buffer, (uint)packed_instance.Length, packed_instance, (uint)packed_instance.Length, (IntPtr)0 ) );
        object obj = Marshal.PtrToStructure( buffer, type );

        Marshal.FreeHGlobal( buffer );
        return obj;
    }

    public object LoadInstanceFromFile( Type type, string file ) { return LoadInstance( type, File.ReadAllBytes( file ) ); }
	
	/*
	public object LoadInstanceFromTextFile(Type _Type, string _File)
	{
 		if (!File.Exists(_File))
             throw new System.ApplicationException(string.Format("Cant open file: {0}", _File));
 
        string Text = File.ReadAllText(_File);
 
 		IntPtr size_ptr = Marshal.AllocHGlobal(8);
        CheckDLErrors(DLRequiredTextPackSize(m_Context, Text, size_ptr));
        int size = Marshal.ReadInt32(size_ptr);
 		Marshal.FreeHGlobal(size_ptr);
 		
 		byte[] Packed = new byte[size];
 		CheckDLErrors(DLPackText(m_Context, Text, Packed, size));
 		
 		return LoadInstance(_Type, Packed);
	}
	*/
	
	public byte[] StoreInstance( object instance )
    {
        byte[] buffer = new byte[ InstaceSizeStored( instance ) ];

        IntPtr inst_ptr = Marshal.AllocHGlobal( Marshal.SizeOf( instance ) );
        Marshal.StructureToPtr( instance, inst_ptr, false );
        
        CheckDLErrors( dl_instance_store( ctx, TypeIDOf( instance ), inst_ptr, buffer, (uint)buffer.Length, (IntPtr)0 ) );
        
        Marshal.FreeHGlobal(inst_ptr);

        return buffer;
    }

	/*
    public string StoreInstanceToString(object _Instance)
    {
        byte[] PackedData = StoreInstace(_Instance);

        IntPtr size_ptr = Marshal.AllocHGlobal(8);
        CheckDLErrors(DLRequiredUnpackSize(m_Context, PackedData, PackedData.Length, size_ptr));
        int size = Marshal.ReadInt32(size_ptr);

        byte[] TextData = new byte[size];

        CheckDLErrors(DLUnpack(m_Context, PackedData, PackedData.Length, TextData, TextData.Length));

        Marshal.FreeHGlobal(size_ptr);

        System.Text.ASCIIEncoding enc = new System.Text.ASCIIEncoding();
        return enc.GetString(TextData);
    }

    public void StoreInstaceToFile(object _Instance, string _File)     { File.WriteAllBytes(_File, StoreInstace(_Instance)); }
    public void StoreInstaceToTextFile(object _Instance, string _File) { File.WriteAllText(_File, StoreInstanceToString(_Instance)); }*/

    // private
    
    private uint TypeIDOf( Type type )       { return (uint)type.GetField("TYPE_ID").GetValue(null); }
    private uint TypeIDOf( object instance ) { return TypeIDOf( instance.GetType() ); }

    private uint InstaceSizeStored( object instance )
    {
        IntPtr inst_ptr = Marshal.AllocHGlobal( Marshal.SizeOf( instance ) );
        Marshal.StructureToPtr( instance, inst_ptr, false );

        IntPtr size_ptr = Marshal.AllocHGlobal(8);
        CheckDLErrors( dl_instance_store( ctx, TypeIDOf( instance ), inst_ptr, new byte[0], 0, size_ptr ) );
        
        uint size = (uint)Marshal.ReadInt32(size_ptr);

        Marshal.FreeHGlobal(inst_ptr);
        Marshal.FreeHGlobal(size_ptr);

        return size;
    }

    public IntPtr ctx;

    [DllImport("dl.dll")] private extern static string dl_error_to_string( int error_code );
    [DllImport("dl.dll")] private extern static int    dl_context_create( IntPtr dl_ctx, IntPtr create_params );
    [DllImport("dl.dll")] private extern static int    dl_context_destroy( IntPtr dl_ctx );
    [DllImport("dl.dll")] private extern static int    dl_context_load_type_library( IntPtr dl_ctx, byte[] tl_data, int tl_data_size );
    
    [DllImport("dl.dll")] private extern static int    dl_instance_load( IntPtr dl_ctx,          uint type_id,
                                           								 IntPtr instance,        uint instance_size,
                                           								 byte[] packed_instance, uint packed_instance_size,
                                           								 IntPtr consumed );
                                           								 
	[DllImport("dl.dll")] private extern static int    dl_instance_store( IntPtr dl_ctx,        uint type_id, 
																		  IntPtr instance,
																		  byte[] out_buffer,    uint out_buffer_size, 
																		  IntPtr produced_bytes );
}
