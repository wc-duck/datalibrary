BUILD_PATH = "local"
PYTHON = "python"
-- PYTHON = "C:\\Python26\\python.exe"

function DLTypeLibrary( tlc_file, dl_shared_lib )
	local output_path = PathJoin( BUILD_PATH, 'generated' )
	local out_file = PathJoin( output_path, PathFilename( PathBase( tlc_file ) ) )
	local out_header    = out_file .. ".h"
	local out_cs_header = out_file .. ".cs"
	local out_lib       = out_file .. ".bin"
	local out_lib_h     = out_file .. ".bin.h"

	local DL_TLC = PYTHON .. " tool/dl_tlc/dl_tlc.py --dldll=" .. dl_shared_lib

	AddJob( out_header, 
		"tlc " .. out_lib,
		DL_TLC .. " -o " .. out_lib .. " " .. tlc_file, 
		tlc_file )

	AddJob( out_cs_header, 
		"tlc " .. out_cs_header,
		DL_TLC .. " -s " .. out_cs_header .. " " .. tlc_file, 
		tlc_file )

	AddJob( out_lib, 
		"tlc " .. out_header,
		DL_TLC .. " -c " .. out_header .. " " .. tlc_file, 
		tlc_file )

	AddDependency( tlc_file, Collect( "tool/dl_tlc/*.py" ) )
	AddDependency( tlc_file, Collect( "bind/python/*.py" ) )
	AddDependency( tlc_file, dl_shared_lib )

	-- dump type-lib to header
	AddJob( out_lib_h, 
		out_lib .. " -> " .. out_lib_h, 
		PYTHON .. " build/bin_to_hex.py " .. out_lib .. " > " .. out_lib_h, 
		out_lib )
end


function DefaultSettings( platform, config )

	local settings = NewSettings()
	settings.cc.includes:Add("include")
	settings.cc.includes:Add("local")
	
	settings.dll.libs:Add("yajl")
	settings.link.libs:Add("yajl")
	
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
	settings.cc.flags:Add("-Werror", "-ansi")
	return settings
end

function DefaultMSVC( config )
	local settings = DefaultSettings( "win32", config )
	--[[
		Settings here should be!
		settings.cc.flags:Add("/Wall", "-WX")
		/EHsc only on unittest
		/wd4324 = warning C4324: 'SA128BitAlignedType' : structure was padded due to __declspec(align())
	--]]
	settings.cc.flags:Add("/W4", "-WX", "/EHsc", "/wd4324")

	settings.cc.includes:Add("extern/include")
	settings.dll.libpath:Add("extern/libs/" .. platform .. "/" .. config)
	settings.link.libpath:Add("extern/libs/" .. platform .. "/" .. config)

	return settings
end

function DefaultGCCLinux32( config )
	local settings = DefaultGCC( "linux32", config )
	settings.cc.flags:Add( "-m32" )
	settings.cc.flags:Add( "-malign-double" ) -- TODO: temporary workaround, dl should support natural alignment for double
	settings.dll.flags:Add( "-m32" )
	settings.link.flags:Add( "-m32" )
	return settings
end

function DefaultGCCLinux64( config )
	local settings = DefaultGCC( "linux64", config )
	settings.cc.flags:Add( "-m64" )
	settings.dll.flags:Add( "-m64" )
	settings.link.flags:Add( "-m64" )
	return settings
end

settings = 
{
	linux32 = { debug = DefaultGCCLinux32( "debug" ), release = DefaultGCCLinux32( "release" ) },
	linux64 = { debug = DefaultGCCLinux64( "debug" ), release = DefaultGCCLinux64( "release" ) },
	win32 =   { debug = DefaultMSVC( "debug" ),       release = DefaultMSVC( "release" ) }
}

platform = ScriptArgs["platform"]
config   = ScriptArgs["config"]

if not platform then error( "platform need to be set. example \"platform=linux32\"" ) end
if not config then   error( "config need to be set. example \"config=debug\"" )       end

platform_settings = settings[platform]
if not platform_settings then              error( platform .. " is not a supported platform" )    end
build_settings = platform_settings[config]
if not build_settings then                 error( config .. " is not a supported configuration" ) end

build_settings = settings[ platform ][ config ]

lib_files = Collect( "src/*.cpp" )
obj_files = Compile( build_settings, lib_files )

-- ugly fugly, need -fPIC on .so-files!
local output_path = PathJoin( BUILD_PATH, PathJoin( platform, config ) )
if platform == "linux64" then
	build_settings.cc.Output = function(settings, path) return PathJoin(output_path .. "/dll/", PathFilename(PathBase(path)) .. settings.config_ext) end
	build_settings.cc.flags:Add( "-fPIC" )
	dll_files = Compile( build_settings, lib_files )
else
	dll_files = obj_files
end

static_library = StaticLibrary( build_settings, "dl", obj_files )
shared_library = SharedLibrary( build_settings, "dl", dll_files )

dl_pack = Link( build_settings, "dl_pack", Compile( build_settings, Collect("tool/dl_pack/*.cpp", "src/getopt/*.cpp")), static_library )

-- HACK BONANZA!
if platform == "linux32" then
	DLTypeLibrary( "tests/unittest.tld", "local/linux64/" .. config .. "/dl.so" )
else
	DLTypeLibrary( "tests/unittest.tld", shared_library )
end

build_settings.link.libs:Add( "gtest" )

if platform == "linux64" or platform == "linux32" then
	build_settings.link.libs:Add( "pthread" )
end

dl_tests = Link( build_settings, "dl_tests", Compile( build_settings, Collect("tests/*.cpp")), static_library )

dl_tests_py = "tests/dl_tests.py"

AddJob( "test",          "unittest c",               dl_tests,                   dl_tests ) -- c unit tests
AddJob( "test_valgrind", "unittest valgrind",        "valgrind -v " .. dl_tests, dl_tests ) -- valgrind c unittests
AddJob( "test_py",       "unittest python bindings", "python " .. dl_tests_py,   dl_tests_py, shared_library ) -- python bindings unittests

-- do not run unittest as default, only run
PseudoTarget( "dl_default", dl_pack, dl_tests, shared_library )
DefaultTarget( "dl_default" )
