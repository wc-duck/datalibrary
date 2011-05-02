'''
    Unittest using both 32 bit and 64 bit dl_pack to try converting between ptrsizes
'''

import unittest, json, os, tempfile

BINARY_TYPELIBRARY = 'local/generated/unittest.bin'
PODS_TEXT          = '''{
        "type" : "Pods",
        "data" : {
                "i8"  : -1,
                "i16" : -2,
                "i32" : 3,
                "i64" : 4,
                "u8"  : 5,
                "u16" : 5,
                "u32" : 6,
                "u64" : 7,
                "f32" : 6.111,
                "f64" : 7.222
        }
}'''

class DLPackUnpackTest(unittest.TestCase):
    def setUp(self):
        self.verbose = False
            
    def do_the_round_about_stdin_and_stdout(self, test_text, test_settings ):
        in_file,  in_name  = tempfile.mkstemp()
        out_file, out_name = tempfile.mkstemp()
        
        open( in_name, 'wb' ).write( test_text )
        
        cmd = 'cat %s | %s > %s' % ( in_name, '%(pack_exe)s | %(unpack_exe)s' % test_settings, out_name )
        os.system( cmd )
        if self.verbose: print cmd
        
        in_data  = json.loads( test_text )
        out_data = json.loads( open( out_name, 'rb' ).read()[:-1] ) # TODO: why this -1 here?
        
        os.unlink( in_name )
        os.unlink( out_name )
        
        self.assertEqual( in_data, out_data )
        
    def do_the_round_about_file(self, test_text, test_settings ):
        in_file,       in_name       = tempfile.mkstemp()
        storage_file,  storage_name  = tempfile.mkstemp()
        out_file,      out_name      = tempfile.mkstemp()
        
        open( in_name, 'wb' ).write( test_text )
        
        cmd = '%s -o %s %s' % ( test_settings['pack_exe'], storage_name, in_name )
        os.system( cmd )
        if self.verbose: print cmd
        
        cmd = '%s -o %s %s' % ( test_settings['unpack_exe'], out_name, storage_name )
        os.system( cmd )
        if self.verbose: print cmd
        
        in_data  = json.loads( test_text )
        out_data = json.loads( open( out_name, 'rb' ).read()[:-1] ) # TODO: why this -1 here?
        
        os.unlink( in_name )
        os.unlink( storage_name )
        os.unlink( out_name )
        
        self.assertEqual( in_data, out_data )
            
    def test_32bit_to_64bit(self):
        settings = { 
                     'pack_exe'   : 'local/linux_x86/debug/dl_pack -l %s -p 8'  % BINARY_TYPELIBRARY,
                     'unpack_exe' : 'local/linux_x86_64/debug/dl_pack -l %s -u' % BINARY_TYPELIBRARY
                   } 
        self.do_the_round_about_file( PODS_TEXT, settings )
        self.do_the_round_about_stdin_and_stdout( PODS_TEXT, settings )
        
    def test_64bit_to_32bit(self):
        settings = {
                     'pack_exe'   : 'local/linux_x86_64/debug/dl_pack -l %s -p 4' % BINARY_TYPELIBRARY,
                     'unpack_exe' : 'local/linux_x86/debug/dl_pack -l %s -u'      % BINARY_TYPELIBRARY
                   }
        self.do_the_round_about_file( PODS_TEXT, settings )
        self.do_the_round_about_stdin_and_stdout( PODS_TEXT, settings )
        
    def test_64bit_read_on_32bit(self):
        settings = {
                     'pack_exe'   : 'local/linux_x86_64/debug/dl_pack -l %s' % BINARY_TYPELIBRARY,
                     'unpack_exe' : 'local/linux_x86/debug/dl_pack -l %s -u' % BINARY_TYPELIBRARY
                   }
        self.do_the_round_about_stdin_and_stdout( PODS_TEXT, settings )
        
    def test_32bit_read_on_64bit(self):
        settings = {
                     'pack_exe'   : 'local/linux_x86/debug/dl_pack -l %s' % BINARY_TYPELIBRARY,
                     'unpack_exe' : 'local/linux_x86_64/debug/dl_pack -l %s -u' % BINARY_TYPELIBRARY
                   }
        self.do_the_round_about_stdin_and_stdout( PODS_TEXT, settings )
        
if __name__ == '__main__':
    unittest.main()