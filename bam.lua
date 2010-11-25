BUILD_PATH = "local"
-- PYTHON = "python"
PYTHON = "C:\\Python26\\python.exe"

function DLTypeLibrary( tlc_file, dl_shared_lib )
	local output_path = PathJoin( BUILD_PATH, 'generated' )
	local out_file = PathJoin( output_path, PathFilename( PathBase( tlc_file ) ) )
	local out_header    = out_file .. ".h"
	local out_cs_header = out_file .. ".cs"
	local out_lib       = out_file .. ".bin"
	local out_lib_h     = out_file .. ".bin.h"

	local DL_TLC = PYTHON .. " tool/dl_tlc/dl_tlc.py --dldll=" .. dl_shared_lib

	AddJob( out_lib, 
		"tlc " .. out_lib,
		DL_TLC .. " -o " .. out_lib .. " " .. tlc_file, 
		tlc_file )

	AddJob( out_lib_h, 
		"tlc " .. out_lib_h,
		DL_TLC .. " -x " .. out_lib_h .. " " .. tlc_file, 
		tlc_file )

	AddJob( out_cs_header, 
		"tlc " .. out_cs_header,
		DL_TLC .. " -s " .. out_cs_header .. " " .. tlc_file, 
		tlc_file )

	AddJob( out_header, 
		"tlc " .. out_header,
		DL_TLC .. " -c " .. out_header .. " " .. tlc_file, 
		tlc_file )

	AddDependency( tlc_file, Collect( "tool/dl_tlc/*.py" ) )
	AddDependency( tlc_file, Collect( "bind/python/*.py" ) )
	AddDependency( tlc_file, dl_shared_lib )
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
	if config == "debug" then
		settings.cc.flags:Add("-O0", "-g")
	else
		settings.cc.flags:Add("-O2")
	end
	return settings
end

function SetupMSVCBinaries( settings, build_platform )
	path = os.getenv("PATH")
	vs8_path = os.getenv("VS80COMNTOOLS")
	
	if vs8_path == nil then error("Visual Studio 8 is not installed on this machine!") end
	
	local vs_install_dir = Path( vs8_path .. "/../../../" )
	local vc_inc_path = Path( vs_install_dir .. "/VC/include" )
	local vs_ide_path = Path( vs_install_dir .. "/Common7/IDE" )
	local vs_bin_path = ""
	local vs_lib_path = ""
	local vs_define_name = ""
	
	if build_platform == "win32" then
		vc_lib_path = Path( vs_install_dir .. "/VC/lib" )
		vs_bin_path = Path( "\"" .. vs_install_dir .. "/VC/bin/\"" )
		vs_define_name = "DL_VS32_PATH"
	elseif build_platform == "winx64" then
		vc_lib_path = Path( vs_install_dir .. "/VC/lib/amd64" )
		vs_define_name = "DL_VS64_PATH"
		if platform == "win32" then
			vs_bin_path = Path( "\"" .. vs_install_dir .. "/VC/bin/x86_amd64/\"" )
		else
			vs_bin_path = Path( "\"" .. vs_install_dir .. "/VC/bin/amd64/\"" )
		end
	end
	
	EnvironmentSet("PATH", vs_ide_path)
	EnvironmentSet(vs_define_name, vs_bin_path)
	
	settings.cc.exe_c = "%" .. vs_define_name .. "%cl"
	settings.lib.exe  = "%" .. vs_define_name .. "%lib"
	settings.dll.exe  = "%" .. vs_define_name .. "%link"
	settings.link.exe = "%" .. vs_define_name .. "%link"
	
	settings.cc.includes:Add(vc_inc_path)
	settings.dll.libpath:Add(vc_lib_path)
	settings.link.libpath:Add(vc_lib_path)
end

function DefaultMSVC( build_platform, config )
	local settings = DefaultSettings( build_platform, config )
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

function DefaultMSVS_Win32( config )
	local settings = DefaultMSVC( "win32", config )
	SetupMSVCBinaries( settings, "win32" )
	return settings
end

function DefaultMSVS_Winx64( config )
	local settings = DefaultMSVC( "winx64", config )
	SetupMSVCBinaries( settings, "winx64" )
	return settings
end

function DefaultGCCLinux_x86( config )
	local settings = DefaultGCC( "linux_x86", config )
	settings.cc.flags:Add( "-m32" )
	settings.cc.flags:Add( "-malign-double" ) -- TODO: temporary workaround, dl should support natural alignment for double
	settings.dll.flags:Add( "-m32" )
	settings.link.flags:Add( "-m32" )
	return settings
end

function DefaultGCCLinux_x86_64( config )
	local settings = DefaultGCC( "linux_x86_64", config )
	settings.cc.flags:Add( "-m64" )
	settings.dll.flags:Add( "-m64" )
	settings.link.flags:Add( "-m64" )
	return settings
end

settings = 
{
	linux_x86    = { debug = DefaultGCCLinux_x86( "debug" ),    release = DefaultGCCLinux_x86( "release" ) },
	linux_x86_64 = { debug = DefaultGCCLinux_x86_64( "debug" ), release = DefaultGCCLinux_x86_64( "release" ) },
	win32        = { debug = DefaultMSVS_Win32( "debug" ),      release = DefaultMSVS_Win32( "release" ) },
	winx64       = { debug = DefaultMSVS_Winx64( "debug" ),     release = DefaultMSVS_Winx64( "release" ) }
}

build_platform = ScriptArgs["platform"]
config   = ScriptArgs["config"]

if not build_platform then error( "platform need to be set. example \"platform=linux32\"" ) end
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

dl_pack = Link( build_settings, "dl_pack", Compile( build_settings, Collect("tool/dl_pack/*.cpp", "src/getopt/*.cpp")), static_library )

-- HACK BONANZA!
if build_platform == "linux_x86" then
	DLTypeLibrary( "tests/unittest.tld", "local/linux_x86_64/" .. config .. "/dl.so" )
else
	DLTypeLibrary( "tests/unittest.tld", shared_library )
end

build_settings.link.libs:Add( "gtest" )

if build_platform == "linux_x86_64" or build_platform == "linux_x86" then
	build_settings.link.libs:Add( "pthread" )
end

dl_tests = Link( build_settings, "dl_tests", Compile( build_settings, Collect("tests/*.cpp")), static_library )

dl_tests_py = "tests/dl_tests.py"

if family == "windows" then
	AddJob( "test", "unittest c", string.gsub( dl_tests, "/", "\\" ), dl_tests )
else
	AddJob( "test",          "unittest c",        dl_tests,                   dl_tests )
	AddJob( "test_valgrind", "unittest valgrind", "valgrind -v " .. dl_tests, dl_tests ) -- valgrind c unittests
end

AddJob( "test_py",       "unittest python bindings", "python " .. dl_tests_py .. " -v", dl_tests_py, shared_library ) -- python bindings unittests

-- do not run unittest as default, only run
PseudoTarget( "dl_default", dl_pack, dl_tests, shared_library )
DefaultTarget( "dl_default" )
