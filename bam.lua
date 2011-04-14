BUILD_PATH = "local"

PYTHON = "python"
if family == "windows" then -- hackery hack
    PYTHON = "C:\\Python26\\python.exe"
end

function DLTypeLibrary( tlc_file, dl_shared_lib )
	local output_path = PathJoin( BUILD_PATH, 'generated' )
	local out_file = PathJoin( output_path, PathFilename( PathBase( tlc_file ) ) )
	local out_header    = out_file .. ".h"
	local out_cs_header = out_file .. ".cs"
	local out_lib       = out_file .. ".bin"
	local out_lib_h     = out_file .. ".bin.h"

	local DL_TLC = PYTHON .. " tool/dl_tlc/dl_tlc.py --dldll=" .. dl_shared_lib

	AddJob( out_lib,       "tlc " .. out_lib,       DL_TLC .. " -o " .. out_lib .. " " .. tlc_file,       tlc_file )
	AddJob( out_lib_h,     "tlc " .. out_lib_h,	DL_TLC .. " -x " .. out_lib_h .. " " .. tlc_file,     tlc_file )
	AddJob( out_cs_header, "tlc " .. out_cs_header,	DL_TLC .. " -s " .. out_cs_header .. " " .. tlc_file, tlc_file )
	AddJob( out_header,    "tlc " .. out_header,    DL_TLC .. " -c " .. out_header .. " " .. tlc_file,    tlc_file )

	AddDependency( tlc_file, CollectRecursive( "tool/dl_tlc/*.py" ) )
	AddDependency( tlc_file, CollectRecursive( "bind/python/*.py" ) )
	AddDependency( tlc_file, dl_shared_lib )
end

function CSharpSettings()
	local settings = {}
	settings.exe = {}
	settings.lib = {}

	if family == "unix" then
		settings.exe.exe = "gmcs "
		settings.exe.fix_path = function( path ) return path end
	else
		settings.exe.exe = "c:/WINDOWS/Microsoft.NET/Framework/v3.5/csc /nologo " -- path here need to be fetched from system!
		settings.exe.fix_path = function( path ) return string.gsub( path, "/", "\\" ) end
	end

	settings.lib.exe      = settings.exe.exe .. "/target:library "
	settings.lib.fix_path = settings.exe.fix_path

	settings.exe.flags      = NewFlagTable()
	settings.exe.libpaths   = NewFlagTable()
	settings.exe.references = NewFlagTable()
	settings.exe.outpath    = PathJoin( BUILD_PATH, 'csharp' )
	settings.exe.output     = function( settings, path ) return PathJoin( settings.outpath, PathFilename( PathBase( path ) ) .. ".exe" ) end
	settings.lib.flags      = NewFlagTable()
	settings.lib.libpaths   = NewFlagTable()
	settings.lib.references = NewFlagTable()
	settings.lib.outpath    = PathJoin( BUILD_PATH, 'csharp' )
	settings.lib.output     = function( settings, path ) return PathJoin( settings.outpath, PathFilename( PathBase( path ) ) .. ".dll" ) end

	-- these settings might go in a separate function
	settings.exe.flags:Add( "/warnaserror" )
	settings.lib.flags:Add( "/warnaserror" )

	return settings
end

function CSharpCompile( settings, src )
	local compiled = settings.output( settings, src )
	local flags = settings.flags:ToString() .. settings.references:ToString() .. settings.libpaths:ToString()
	AddJob( compiled, "C# " .. compiled, settings.exe .. flags .. " /out:" .. settings.fix_path( compiled ) .. " " .. settings.fix_path( src ), src )
	return compiled
end

function CSharpExe( settings, src ) return CSharpCompile( settings.exe, src ) end 
function CSharpLibrary( settings, src ) return CSharpCompile( settings.lib, src ) end 

function DefaultSettings( platform, config )
	local settings = {}
	
	settings.debug = 0
	settings.optimize = 0
	settings._is_settingsobject = true
	settings.invoke_count = 0
	
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
	settings.cc.includes:Add("extern/include")
	settings.cc.includes:Add("local")
	
	settings.dll.libs:Add("yajl")
	settings.link.libs:Add("yajl")

	settings.dll.libpath:Add("extern/libs/" .. platform .. "/" .. config)
	settings.link.libpath:Add("extern/libs/" .. platform .. "/" .. config)
	
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
	
	settings.cc.flags:Add("-Werror", "-ansi", "-Wstrict-aliasing=2")
	if config == "debug" then
		settings.cc.flags:Add("-O0", "-g")
	else
		settings.cc.flags:Add("-O2")
	end
	
	if platform == "linux_x86" then
		settings.cc.flags:Add( "-malign-double" ) -- TODO: temporary workaround, dl should support natural alignment for double
		settings.cc.flags:Add( "-m32" )
		settings.dll.flags:Add( "-m32" )
		settings.link.flags:Add( "-m32" )
	elseif platform == "linux_x86_64" then
		settings.cc.flags:Add( "-m64" )
		settings.dll.flags:Add( "-m64" )
		settings.link.flags:Add( "-m64" )
	end
	
	return settings
end

function SetupMSVCBinaries( settings, build_platform )
	if family ~= "windows" then
		return
	end

	vs8_path = os.getenv("VS80COMNTOOLS")
	
	if vs8_path == nil then error("Visual Studio 8 is not installed on this machine!") end
	
	local vs_install_dir = Path( vs8_path .. "/../../../" )
	local vc_inc_path = Path( vs_install_dir .. "/VC/include" )
	local vs_ide_path = Path( vs_install_dir .. "/Common7/IDE" )
	local vs_define_name = ""
	
	local lib_path_sufix = ""
	
	if build_platform == "win32" then
		vs_define_name = "DL_VS32_PATH"
		EnvironmentSet( vs_define_name, Path( "\"" .. vs_install_dir .. "/VC/bin/\"" ) )
	elseif build_platform == "winx64" then
		lib_path_sufix = "/amd64"
		vs_define_name = "DL_VS64_PATH"
		if platform == "win32" then
			EnvironmentSet( vs_define_name, Path( "\"" .. vs_install_dir .. "/VC/bin/x86_amd64/\"" ) )
		else
			EnvironmentSet( vs_define_name, Path( "\"" .. vs_install_dir .. "/VC/bin/amd64/\"" ) )
		end
	end
	
	EnvironmentSet("PATH", vs_ide_path)
	
	settings.cc.exe_c = "%" .. vs_define_name .. "%cl"
	settings.lib.exe  = "%" .. vs_define_name .. "%lib"
	settings.dll.exe  = "%" .. vs_define_name .. "%link"
	settings.link.exe = "%" .. vs_define_name .. "%link"
	
	local lib_path  = Path( vs_install_dir .. "/VC/lib" .. lib_path_sufix )
	local psdk_path = Path( vs_install_dir .. "/VC/PlatformSDK/lib" .. lib_path_sufix )
	settings.cc.includes:Add(vc_inc_path)
	settings.dll.libpath:Add(lib_path)
	settings.dll.libpath:Add(psdk_path)
	settings.link.libpath:Add(lib_path)
	settings.link.libpath:Add(psdk_path)
end

function DefaultMSVC( build_platform, config )
	local settings = DefaultSettings( build_platform, config )
	SetDriversCL(settings)
	
	--[[
		/EHsc only on unittest
		/wd4324 = warning C4324: 'SA128BitAlignedType' : structure was padded due to __declspec(align())
		/wd4127 = warning C4127: conditional expression is constant.
	--]]
	settings.cc.flags:Add("/W4", "/WX", "/EHsc", "/wd4324", "/wd4127") -- add /Wall?
	
	if config == "debug" then
		settings.cc.flags:Add("/Od", "/MTd", "/Z7", "/D \"_DEBUG\"")
		settings.dll.flags:Add("/DEBUG")
		settings.link.flags:Add("/DEBUG")
	else
		settings.cc.flags:Add("/Ox", "/Ot", "/MT", "/D \"NDEBUG\"")
	end
	
	SetupMSVCBinaries( settings, build_platform )

	return settings
end

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
if not platform_settings then              error( build_platform .. " is not a supported platform" )    end
build_settings = platform_settings[config]
if not build_settings then                 error( config .. " is not a supported configuration" ) end

build_settings = settings[ build_platform ][ config ]

lib_files = Collect( "src/*.cpp" )
obj_files = Compile( build_settings, lib_files )

-- ugly fugly, need -fPIC on .so-files!
local output_path = PathJoin( BUILD_PATH, PathJoin( build_platform, config ) )
if build_platform == "linux_x86_64" then
	build_settings.cc.Output = function(settings, path) return PathJoin(output_path .. "/dll/", PathFilename(PathBase(path)) .. settings.config_ext) end
	build_settings.cc.flags:Add( "-fPIC" )
	dll_files = Compile( build_settings, lib_files )
else
	dll_files = obj_files
end

static_library = StaticLibrary( build_settings, "dl", obj_files )
shared_library = SharedLibrary( build_settings, "dl", dll_files )

dl_pack = Link( build_settings, "dl_pack", Compile( build_settings, CollectRecursive("tool/dl_pack/*.cpp", "src/getopt/*.cpp")), static_library )

-- HACK BONANZA!
if build_platform == "linux_x86" then
	DLTypeLibrary( "tests/unittest.tld", "local/linux_x86_64/" .. config .. "/dl.so" )
	DLTypeLibrary( "tests/unittest2.tld", "local/linux_x86_64/" .. config .. "/dl.so" )
else
	DLTypeLibrary( "tests/unittest.tld", shared_library )
	DLTypeLibrary( "tests/unittest2.tld", shared_library )
end

build_settings.link.libs:Add( "gtest" )

if build_platform == "linux_x86_64" or build_platform == "linux_x86" then
	build_settings.link.libs:Add( "pthread" )
end

dl_tests = Link( build_settings, "dl_tests", Compile( build_settings, Collect("tests/*.cpp")), static_library )

local    test_args = ""
local py_test_args = ""
local cs_test_args = ""
if ScriptArgs["test_filter"] then
	   test_args = " --gtest_filter=" .. ScriptArgs["test_filter"]
	py_test_args = " " .. ScriptArgs["test_filter"]
	cs_test_args = " -run=" .. ScriptArgs["test_filter"]
end

cs_settings     = CSharpSettings()
cs_libdl_lib    = CSharpLibrary( cs_settings, "bind/cs/libdl.cs" )
cs_unittest_lib = CSharpLibrary( cs_settings, "local/generated/unittest.cs" )

cs_settings.lib.references:Add("/reference:libdl")
cs_settings.lib.references:Add("/reference:unittest")
cs_settings.lib.references:Add("/reference:nunit.framework")
cs_settings.lib.libpaths:Add("/lib:local/csharp")
cs_settings.lib.libpaths:Add("/lib:/usr/lib/cli/nunit.framework-2.4")

cs_test_lib     = CSharpLibrary( cs_settings, "tests/csharp_bindings/dl_tests.cs", "local/generated/unittest.cs" )
AddDependency( cs_test_lib, cs_libdl_lib, cs_unittest_lib )

if family == "windows" then
	AddJob( "test",          "unittest c",        string.gsub( dl_tests, "/", "\\" ) .. test_args, dl_tests,    "local/generated/unittest.bin" )
else
	AddJob( "test",          "unittest c",        dl_tests .. test_args,                           dl_tests,    "local/generated/unittest.bin" )
	AddJob( "test_valgrind", "unittest valgrind", "valgrind -v --leak-check=full " .. dl_tests,    dl_tests,    "local/generated/unittest.bin" )
	AddJob( "test_cs",       "unittest c#",       "nunit-console " .. cs_test_lib .. cs_test_args, cs_test_lib, "local/generated/unittest.bin" )
end

dl_tests_py = "tests/python_bindings/dl_tests.py"
AddJob( "test_py", "unittest python", PYTHON .. " " .. dl_tests_py .. " " .. shared_library .. " " .. py_test_args, dl_tests_py, shared_library, "local/generated/unittest.bin" )

-- do not run unittest as default, only run
PseudoTarget( "dl_default", dl_pack, dl_tests, shared_library )
DefaultTarget( "dl_default" )
