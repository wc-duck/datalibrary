BUILD_PATH = "local"

PYTHON = "python"
if family == "windows" then -- hackery hack
    PYTHON = "C:\\Python27\\python.exe"
end

EXTERNALS_PATH = 'external'
GTEST_PATH = PathJoin( EXTERNALS_PATH, 'gtest' )
YAJL_PATH  = PathJoin( EXTERNALS_PATH, 'yajl' )

function dl_type_lib( tlc_file, dltlc )
	local output_path = PathJoin( BUILD_PATH, 'generated' )
	local out_file = PathJoin( output_path, PathFilename( PathBase( tlc_file ) ) )
	local out_header    = out_file .. ".h"
	local out_lib       = out_file .. ".bin"
	local out_lib_h     = out_file .. ".bin.h"

	local HAXX   = PYTHON .. " ../wc_games/wc_engine/units/util/tools/bin2hex.py"

	AddJob( out_lib,       "tlc " .. out_lib,       dltlc  .. " -o " .. out_lib .. " " .. tlc_file,       tlc_file )
	AddJob( out_lib_h,     "tlc " .. out_lib_h,	HAXX   .. " -o " .. out_lib_h .. " " .. out_lib,      out_lib )
	AddJob( out_header,    "tlc " .. out_header,    dltlc .. " -c -o " .. out_header .. " " .. tlc_file,  tlc_file )

	AddDependency( tlc_file, dltlc )
	AddDependency( tlc_file, CollectRecursive( "tool/dl_tlc/*.py" ) )
	AddDependency( tlc_file, CollectRecursive( "bind/python/*.py" ) )
end

function DefaultSettings( platform, config )
	local settings = {}
	
	settings.debug = 0
	settings.optimize = 0
	settings._is_settingsobject = true
	settings.invoke_count = 0
	settings.config = config
	settings.platform = platform
	
	-- SetCommonSettings(settings)
	settings.config_name = ""
	settings.config_ext  = ""
	settings.labelprefix = ""
	
	-- add all tools
	for _, tool in pairs(_bam_tools) do
		tool(settings)
	end

	-- lock the table and return
	TableLock(settings)	

	settings.cc.includes:Add("include")
	settings.cc.includes:Add("local")
		
	local output_path = PathJoin( BUILD_PATH, PathJoin( platform, config ) )
	local output_func = function(settings, path) return PathJoin(output_path, PathFilename(PathBase(path)) .. settings.config_ext) end
	settings.cc.Output = output_func
	settings.lib.Output = output_func
	settings.dll.Output = output_func
	settings.link.Output = output_func

	return settings
end

function DefaultGCC( platform, config )
	local settings = DefaultSettings( platform, config )
	SetDriversGCC(settings)
	
	if config == "debug" then
		settings.cc.flags:Add("-O0", "-g")
	else
		settings.cc.flags:Add("-O2")
	end
	
	local arch = '-m64'
	if platform == "linux_x86" then
		settings.cc.flags:Add( "-malign-double" ) -- TODO: temporary workaround, dl should support natural alignment for double
		arch = '-m32'
	end
	settings.cc.flags:Add( arch )
	settings.dll.flags:Add( arch )
	settings.link.flags:Add( arch )
	
	return settings
end

function SetupMSVCBinaries( settings, compiler )
	if family ~= "windows" then
		return
	end

	local has_msvs10 = os.getenv('VS100COMNTOOLS') ~= nil
	local has_msvs8  = os.getenv('VS80COMNTOOLS') ~= nil
	
	if compiler == nil then
		-- set default compiler
		if     has_msvs10 then compiler = 'msvs10'
		elseif has_msvs8  then compiler = 'msvs8'
		else   compiler = nil
		end
	elseif compiler == 'msvs10' then
		if not has_msvs10 then
			print( compiler  .. ' is not installed on this machine' )
			os.exit(1)
		end
	elseif compiler == 'msvs8' then
		if not has_msvs8 then
			print( compiler  .. ' is not installed on this machine' )
			os.exit(1)
		end
	else
		print( compiler  .. ' is not installed on this machine' )
		os.exit(1)
	end
	
	local wrapper_path  = "compat/" .. compiler .. "/" .. settings.platform
	settings.cc.exe_c   = wrapper_path .. "/cl.bat"
	settings.cc.exe_cxx = wrapper_path .. "/cl.bat"
	settings.lib.exe    = wrapper_path .. "/lib.bat"
	settings.dll.exe    = wrapper_path .. "/link.bat"
	settings.link.exe   = wrapper_path .. "/link.bat"
end

function DefaultMSVC( build_platform, config )
	local settings = DefaultSettings( build_platform, config )
	SetDriversCL(settings)
	
	if config == "debug" then
		settings.cc.flags:Add("/Od", "/MDd", "/Z7", "/D \"_DEBUG\"")
		settings.dll.flags:Add("/DEBUG")
		settings.link.flags:Add("/DEBUG")
	else
		settings.cc.flags:Add("/Ox", "/Ot", "/MD", "/D \"NDEBUG\"")
	end

	SetupMSVCBinaries( settings, ScriptArgs["compiler"] )

	return settings
end

function make_gtest_settings( base_settings )
	local settings = TableDeepCopy( base_settings )
	
	settings.cc.includes:Add( GTEST_PATH )
	settings.cc.includes:Add( PathJoin( GTEST_PATH, 'include' ) )
	
	return settings
end

function make_yajl_settings( base_settings )
	local settings = TableDeepCopy( base_settings )
	
	settings.cc.includes:Add( PathJoin( YAJL_PATH, 'include' ) )
	
	if settings.platform == 'linux_x86_64' then
		settings.cc.flags:Add( "-fPIC" ) -- HAXX
	end
	
	return settings
end

function make_dl_settings( base_settings )
	local settings = TableDeepCopy( base_settings )
	
	-- build dl on high warning level!
	if settings.platform == 'linux_x86' or settings.platform == 'linux_x86_64' then
		settings.cc.flags:Add("-Wall","-Werror", "-Wextra", "-Wconversion", "-Wstrict-aliasing=2")
	else
		--[[
			/wd4324 = warning C4324: 'SA128BitAlignedType' : structure was padded due to __declspec(align())
			/wd4127 = warning C4127: conditional expression is constant.
		--]]
		settings.cc.flags:Add("/W4", "/WX", "/wd4324", "/wd4127")
		settings.cc.flags_c:Add("/TP")
	end

	settings.cc.includes:Add( PathJoin( YAJL_PATH, 'include' ) )
	
	return settings
end

function make_dl_so_settings( base_settings )
	local settings = make_dl_settings( base_settings )
	
	if settings.platform == 'linux_x86_64' then
		settings.cc.flags:Add( "-fPIC" )
	end
	
	local output_path = PathJoin( BUILD_PATH, PathJoin( settings.platform, settings.config ) )
	local dll_path    = PathJoin( output_path, 'dll' )
	
	settings.cc.Output = function(settings, path) return PathJoin( dll_path, PathFilename(PathBase(path)) .. settings.config_ext) end
	
	return settings
end

function make_test_settings( base_settings )
	local settings = TableDeepCopy( base_settings )
	
	if settings.platform == 'linux_x86' or settings.platform == 'linux_x86_64' then
		settings.cc.flags:Add("-Wall","-Werror", "-Wextra", "-Wconversion", "-Wstrict-aliasing=2")
	else
		--[[
			/EHsc only on unittest
			/wd4324 = warning C4324: 'SA128BitAlignedType' : structure was padded due to __declspec(align())
			/wd4127 = warning C4127: conditional expression is constant.
		--]]
		settings.cc.flags:Add("/W4", "/WX", "/EHsc", "/wd4324", "/wd4127")
		settings.cc.flags_c:Add("/TP")
	end

	settings.cc.includes:Add( PathJoin( PathJoin( EXTERNALS_PATH, 'gtest' ), 'include' ) )
	if settings.platform == "linux_x86_64" or settings.platform == "linux_x86" then
		settings.link.libs:Add( "pthread" )
	end

	return settings
end


------------------------ BUILD ------------------------

settings = 
{
	linux_x86    = { debug = DefaultGCC(  "linux_x86",    "debug" ), release = DefaultGCC(  "linux_x86",    "release" ) },
	linux_x86_64 = { debug = DefaultGCC(  "linux_x86_64", "debug" ), release = DefaultGCC(  "linux_x86_64", "release" ) },
	win32        = { debug = DefaultMSVC( "win32",        "debug" ), release = DefaultMSVC( "win32",        "release" ) },
	winx64       = { debug = DefaultMSVC( "winx64",       "debug" ), release = DefaultMSVC( "winx64",       "release" ) }
}

build_platform = ScriptArgs["platform"]
config         = ScriptArgs["config"]

if not build_platform then error( "platform need to be set. example \"platform=linux_x86\"" ) end
if not config then         error( "config need to be set. example \"config=debug\"" )       end

platform_settings = settings[build_platform]
if not platform_settings then error( build_platform .. " is not a supported platform" ) end
build_settings = platform_settings[config]
if not build_settings then error( config .. " is not a supported configuration" ) end

build_settings = settings[ build_platform ][ config ]

dl_settings    = make_dl_settings   ( build_settings )
dl_so_settings = make_dl_so_settings( build_settings )
test_settings  = make_test_settings ( build_settings )
yajl_settings  = make_yajl_settings ( build_settings )
gtest_settings = make_gtest_settings( build_settings )

yajl_lib  = StaticLibrary( yajl_settings,  "yajl",  Compile( yajl_settings,  Collect( PathJoin( YAJL_PATH, "src/*.c" ) ) ) )
gtest_lib = StaticLibrary( gtest_settings, "gtest", Compile( gtest_settings, Collect( PathJoin( GTEST_PATH, "src/gtest-all.cc" ) ) ) )
dl_lib    = StaticLibrary( dl_settings,    "dl",    Compile( dl_settings,    Collect( "src/*.cpp" ) ), yajl_lib )
dl_shared = SharedLibrary( dl_settings,    "dl",    Compile( dl_so_settings, Collect( "src/*.cpp" ) ), yajl_lib )

dl_settings.cc.includes:Add('tool/dl_pack')
getopt   = Compile( dl_settings, CollectRecursive( "tool/dl_pack/*.c" ) )
dl_pack  = Link( build_settings, "dl_pack",  Compile( dl_settings, CollectRecursive("tool/dl_pack/*.cpp") ), getopt, yajl_lib, dl_lib )
dltlc    = Link( build_settings, "dltlc",    Compile( dl_settings, CollectRecursive("tool/dl_tlc/*.cpp") ), getopt, yajl_lib, dl_lib )
dl_tests = Link( test_settings,  "dl_tests", Compile( test_settings, Collect("tests/*.cpp") ), dl_lib, gtest_lib, yajl_lib )

-- HACK BONANZA!
tl_build_so = dl_shared
if build_platform == "linux_x86" then
	tl_build_so = "local/linux_x86_64/" .. config .. "/dl.so"
elseif build_platform == "winx64" then
	tl_build_so = "local/win32/" .. config .. "/dl.dll"
end

tl1 = dl_type_lib( "tests/unittest.tld",  dltlc )
tl2 = dl_type_lib( "tests/unittest2.tld", dltlc ) 
tl3 = dl_type_lib( "tests/small.tld",     dltlc ) 

local    test_args = ""
local py_test_args = ""
local cs_test_args = ""
if ScriptArgs["test_filter"] then
	   test_args = " --gtest_filter=" .. ScriptArgs["test_filter"]
	py_test_args = " " .. ScriptArgs["test_filter"]
	cs_test_args = " -run=" .. ScriptArgs["test_filter"]
end

if family == "windows" then
	AddJob( "test",          "unittest c",        string.gsub( dl_tests, "/", "\\" ) .. test_args, dl_tests,    "local/generated/unittest.bin" )
else
	local valgrind_flags = " -v --leak-check=full --track-origins=yes "

	AddJob( "test",          "unittest c",        dl_tests .. test_args,                                 dl_tests,    "local/generated/unittest.bin" )
	AddJob( "test_valgrind", "unittest valgrind", "valgrind" .. valgrind_flags .. dl_tests .. test_args, dl_tests,    "local/generated/unittest.bin" )
	AddJob( "test_gdb",      "unittest gdb",      "gdb --args " .. dl_tests .. test_args,                dl_tests,    "local/generated/unittest.bin" )
end

dl_tests_py = "tests/python_bindings/dl_tests.py"
AddJob( "test_py", 
		"unittest python", 
		PYTHON .. " " .. dl_tests_py .. " " .. dl_shared .. " " .. py_test_args, 
		dl_tests_py, 
		dl_shared, "local/generated/unittest.bin" )

-- do not run unittest as default, only run
PseudoTarget( "dl_default", dl_pack, dltlc, dl_tests, dl_shared )
DefaultTarget( "dl_default" )
