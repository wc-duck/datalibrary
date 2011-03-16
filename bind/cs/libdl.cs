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
        
        DLContext.dl_create_params p = new DLContext.dl_create_params();
        
        IntPtr params_ptr = Marshal.AllocHGlobal(Marshal.SizeOf(p));
        Marshal.StructureToPtr( p, params_ptr, false );
        
        CheckDLErrors( dl_context_create( ptr, params_ptr ) );
        ctx = Marshal.ReadIntPtr(ptr);
        
        Marshal.FreeHGlobal(params_ptr);
    }

    ~DLContext() { CheckDLErrors( dl_context_destroy(ctx) ); }
    
    public void LoadTypeLibrary( byte[] tl_buffer )    { CheckDLErrors( dl_context_load_type_library( ctx, tl_buffer, tl_buffer.Length ) ); }
	public void LoadTypeLibraryFromFile( string file ) { LoadTypeLibrary( File.ReadAllBytes( file ) ); }
    
    /*
    
    public object LoadInstance( Type _Type, byte[] _DataBuffer )
    {
        IntPtr buffer = Marshal.AllocHGlobal(_DataBuffer.Length);

        CheckDLErrors(DLLoadInstanceInplace(m_Context, buffer, _DataBuffer, _DataBuffer.Length, _DataBuffer.Length));
        object obj = Marshal.PtrToStructure(buffer, _Type);

        Marshal.FreeHGlobal(buffer);

        return obj;
    }

    public object LoadInstanceFromFile(Type _Type, string _File)
    {
        if (!File.Exists(_File))
            throw new System.ApplicationException(string.Format("Cant open file: {0}", _File));
        return LoadInstance(_Type, File.ReadAllBytes(_File));
    }
	
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
	
	public byte[] StoreInstace(object _Instance)
    {
        byte[] buffer = new byte[InstaceSizeStored(_Instance)];

        FieldInfo info = _Instance.GetType().GetField("TYPE_ID");
        UInt32 hash = (UInt32)info.GetValue(_Instance);

        IntPtr inst_ptr = Marshal.AllocHGlobal(Marshal.SizeOf(_Instance));
        Marshal.StructureToPtr(_Instance, inst_ptr, false);

        IntPtr size_ptr = Marshal.AllocHGlobal(8);
        CheckDLErrors(DLInstaceSizeStored(m_Context, hash, inst_ptr, size_ptr));
        int size = Marshal.ReadInt32(size_ptr);

        GCHandle pinned_array = GCHandle.Alloc(buffer, GCHandleType.Pinned);
        IntPtr data_ptr = pinned_array.AddrOfPinnedObject();
        CheckDLErrors(DLStoreInstace(m_Context, hash, inst_ptr, data_ptr, (int)size));

        pinned_array.Free();
        Marshal.FreeHGlobal(inst_ptr);
        Marshal.FreeHGlobal(size_ptr);

        return buffer;
    }

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
    public void StoreInstaceToTextFile(object _Instance, string _File) { File.WriteAllText(_File, StoreInstanceToString(_Instance)); }

    // private

    private int InstaceSizeStored(object _Instance)
    {
        FieldInfo info = _Instance.GetType().GetField("TYPE_ID");
        UInt32 hash = (UInt32)info.GetValue(_Instance);

        IntPtr inst_ptr = Marshal.AllocHGlobal(Marshal.SizeOf(_Instance));
        Marshal.StructureToPtr(_Instance, inst_ptr, false);

        IntPtr size_ptr = Marshal.AllocHGlobal(8);
        CheckDLErrors(DLInstaceSizeStored(m_Context, hash, inst_ptr, size_ptr));
        int size = Marshal.ReadInt32(size_ptr);

        Marshal.FreeHGlobal(inst_ptr);
        Marshal.FreeHGlobal(size_ptr);

        return size;
    }
    */

    public IntPtr ctx;

    [DllImport("dl.dll")] private extern static string dl_error_to_string( int error_code );
    [DllImport("dl.dll")] private extern static int    dl_context_create( IntPtr dl_ctx, IntPtr create_params );
    [DllImport("dl.dll")] private extern static int    dl_context_destroy( IntPtr dl_ctx );
    [DllImport("dl.dll")] private extern static int    dl_context_load_type_library( IntPtr dl_ctx, byte[] tl_data, int tl_data_size );

    /*
    [DllImport("dldyn.dll", EntryPoint = "dl_instance_load", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLLoadInstanceInplace(IntPtr _Context, IntPtr _pInstance, int _InstanceSize, byte[] _pData, int _DataSize);

    [DllImport("dldyn.dll", EntryPoint = "dl_Instace_calc_size", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLInstaceSizeStored(IntPtr _Context, UInt32 _TypeHash, IntPtr _pInstance, IntPtr _pDataSize);

    [DllImport("dldyn.dll", EntryPoint = "dl_instace_store", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLStoreInstace(IntPtr _Context, UInt32 _TypeHash, IntPtr _pInstance, IntPtr _pData, int _DataSize);    
    
    [DllImport("dldyn.dll", EntryPoint = "dl_txt_pack", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLPackText(IntPtr _Context, string _pTxtData, byte[] _pPackedData, int _pPackedDataSize);

    [DllImport("dldyn.dll", EntryPoint = "dl_txt_pack_calc_size", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLRequiredTextPackSize(IntPtr _Context, string _pTxtData, IntPtr _pPackedDataSize);

    [DllImport("dldyn.dll", EntryPoint = "dl_txt_unpack", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLUnpack(IntPtr _Context, byte[] _pPackedData, int _PackedDataSize, byte[] _pTxtData, int _TxtDataSize);

    [DllImport("dldyn.dll", EntryPoint = "dl_txt_unpack_calc_size", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLRequiredUnpackSize(IntPtr _Context, byte[] _pPackedData, int _PackedDataSize, IntPtr _TxtDataSize);
    */
}
