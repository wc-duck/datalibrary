/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;

public class CDLContext
{
    public CDLContext()
    {
        IntPtr ptr = Marshal.AllocHGlobal(8);
        CheckDLErrors(DLContextCreate(ptr, (IntPtr)0, (IntPtr)0));
        m_Context = Marshal.ReadIntPtr(ptr);
        Marshal.FreeHGlobal(ptr);
    }

    ~CDLContext()
    {
        CheckDLErrors(DLContextDestroy(m_Context));
    }

    public void LoadTypeLibrary(byte[] _DataBuffer)
    {
        CheckDLErrors(DLLoadTypeLibrary(m_Context, _DataBuffer, _DataBuffer.Length));
    }
	
	public void LoadTypeLibraryFromFile(string _File)
    {
        if (!File.Exists(_File))
            throw new System.ApplicationException(string.Format("Cant open file: {0}", _File));
        LoadTypeLibrary(File.ReadAllBytes(_File));
    }
    
    public object LoadInstance(Type _Type, byte[] _DataBuffer)
    {
        IntPtr buffer = Marshal.AllocHGlobal(_DataBuffer.Length);

        CheckDLErrors(DLLoadInstanceInplace(m_Context, buffer, _DataBuffer, _DataBuffer.Length));
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

    private void CheckDLErrors(int _Err)
    {
        if (_Err > 0)
            throw new System.ApplicationException(string.Format("DLError: {0}", DLErrorToString(_Err)));
    }

    public IntPtr m_Context;

    [DllImport("dldyn.dll", EntryPoint = "DLLoadTypeLibrary", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLLoadTypeLibrary(IntPtr _Context, byte[] _pData, int _DataSize);

    [DllImport("dldyn.dll", EntryPoint = "DLContextDestroy", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLContextDestroy(IntPtr _Context);

    [DllImport("dldyn.dll", EntryPoint = "DLContextCreate", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLContextCreate(IntPtr _pContext, IntPtr _pDLAllocFuncs, IntPtr _pInstanceAllocFuncs);

    [DllImport("dldyn.dll", EntryPoint = "DLLoadInstanceInplace", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLLoadInstanceInplace(IntPtr _Context, IntPtr _pInstance, byte[] _pData, int _DataSize);

    [DllImport("dldyn.dll", EntryPoint = "DLInstaceSizeStored", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLInstaceSizeStored(IntPtr _Context, UInt32 _TypeHash, IntPtr _pInstance, IntPtr _pDataSize);

    [DllImport("dldyn.dll", EntryPoint = "DLStoreInstace", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLStoreInstace(IntPtr _Context, UInt32 _TypeHash, IntPtr _pInstance, IntPtr _pData, int _DataSize);

    
    
    [DllImport("dldyn.dll", EntryPoint = "DLPackText", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLPackText(IntPtr _Context, string _pTxtData, byte[] _pPackedData, int _pPackedDataSize);

    [DllImport("dldyn.dll", EntryPoint = "DLRequiredTextPackSize", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLRequiredTextPackSize(IntPtr _Context, string _pTxtData, IntPtr _pPackedDataSize);

    [DllImport("dldyn.dll", EntryPoint = "DLUnpack", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLUnpack(IntPtr _Context, byte[] _pPackedData, int _PackedDataSize, byte[] _pTxtData, int _TxtDataSize);

    [DllImport("dldyn.dll", EntryPoint = "DLRequiredUnpackSize", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static int DLRequiredUnpackSize(IntPtr _Context, byte[] _pPackedData, int _PackedDataSize, IntPtr _TxtDataSize);


    
    [DllImport("dldyn.dll", EntryPoint = "DLErrorToString", ExactSpelling = false, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    private extern static string DLErrorToString(int _Err);
}
