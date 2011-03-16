// replace me with real tests later!

using System.IO;

public class DLTest
{
    public static void Main()
    {
    	DLContext dl_ctx = new DLContext();
        
        dl_ctx.LoadTypeLibraryFromFile("local/generated/unittest.bin");
    }
}