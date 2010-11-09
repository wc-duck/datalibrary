BUILD_PATH = "local"

function DLTypeLibrary( tlc_file, dl_shared_lib  )
	local output_path = PathJoin( BUILD_PATH, 'generated' )
	local out_file = PathJoin( output_path, PathFilename( PathBase( tlc_file ) ) )
	local out_header    = out_file .. ".h"
	local out_cs_header = out_file .. ".cs"
	local out_lib       = out_file .. ".bin"
	local out_lib_h     = out_file .. ".bin.h"

	local DL_TLC = "python2.6 tool/dl_tlc/dl_tlc.py --dldll=local/linux64/debug/dl.so "

	AddJob( out_header, 
		"tlc " .. out_lib,
		DL_TLC .. " -o " .. out_lib .. " " .. tlc_file, 
		tlc_file )

	AddJob( out_cs_header, 
		"tlc " .. out_cs_header,
		DL_TLC .. " -cs " .. out_cs_header .. " " .. tlc_file, 
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
		"python2.6 build/bin_to_hex.py " .. out_lib .. " > " .. out_lib_h, 
		out_lib )
end

function RunUnitTest( target_name, test_file, flags )
	AddJob( target_name, "unittest", test_file, test_file )
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
	settings.cc.flags:Add("/W4", "-WX") -- settings.cc.flags:Add("/Wall", "-WX")
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

DLTypeLibrary( "tests/unittest.tld", shared_library )

build_settings.link.libs:Add( "gtest" )
build_settings.link.libs:Add( "pthread" )
dl_tests = Link( build_settings, "dl_tests", Compile( build_settings, Collect("tests/*.cpp")), static_library )

RunUnitTest( "test", dl_tests )

-- do not run unittest as default, only run
PseudoTarget( "dl_default", dl_pack, dl_tests, shared_library )
DefaultTarget( "dl_default" )
