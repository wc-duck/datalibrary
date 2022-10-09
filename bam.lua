BUILD_PATH = "local"

EXTERNALS_PATH = 'external'
GTEST_PATH = PathJoin( EXTERNALS_PATH, 'gtest' )

function dl_type_lib( tlc_file, dltlc, dl_tests )
	local output_path = PathJoin( BUILD_PATH, 'generated' )
	local out_file = PathJoin( output_path, PathFilename( PathBase( tlc_file ) ) )
	local out_header    = out_file .. ".h"
	local out_lib       = out_file .. ".bin"
	local out_lib_h     = out_file .. ".bin.h"
	local out_lib_txt_h = out_file .. ".txt.h"

	local BIN2HEX  = _bam_exe .. " -e tool/bin2hex.lua"

	if family == "windows" then
		dltlc = string.gsub( dltlc, "/", "\\" )
	end

	AddJob( out_lib,       "tlc " .. out_lib,    dltlc   .. " -o "    .. out_lib    .. " "     .. tlc_file,  tlc_file )
	AddJob( out_lib_h,     "tlc " .. out_lib_h,  BIN2HEX .. " dst="   .. out_lib_h  .. " src=" .. out_lib,   out_lib )
	AddJob( out_lib_txt_h, "tlc " .. out_lib_h,  BIN2HEX .. " dst="   .. out_lib_txt_h  .. " src=" .. tlc_file, tlc_file )
	AddJob( out_header,    "tlc " .. out_header, dltlc   .. " -c -o " .. out_header .. " "     .. tlc_file,  tlc_file )

	AddDependency( tlc_file, dltlc )
	AddDependency( dl_tests, out_lib_h )
end

function DefaultSettings( platform, config, compiler )
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
	local output_path = PathJoin( BUILD_PATH, PathJoin( PathJoin( platform, compiler ), config ) )
	local output_func = function(settings, path) return PathJoin(output_path, PathFilename(PathBase(path)) .. settings.config_ext) end
	settings.cc.Output = output_func
	settings.lib.Output = output_func
	settings.dll.Output = output_func

	settings.link.Output = output_func

	return settings
end

function get_compiler()
  compiler = ScriptArgs["compiler"]

  if family == "windows" then
    local has_msvs = {}
    for i=8,30 do
      has_msvs[i] = os.getenv('VS' .. i .. '0COMNTOOLS') ~= nil
    end

    if compiler == nil then
      for i=30,8,-1 do
        if has_msvs[i] then
          compiler = 'msvs' .. i
          break
        end
      end
    else
      for i=8,30 do
        if compiler == ('msvs' .. i) then
          if not has_msvs[i] then
            print( compiler  .. ' is not installed on this machine' )
            os.exit(1)
          end
          break
        end
      end
    end
    if compiler == nil then
      compiler = 'gcc'
    end
  end

  return compiler
end

function DefaultGCCLike( platform, config, compiler )
	local settings = DefaultSettings( platform, config, compiler )

	if compiler == 'gcc' then
	 SetDriversGCC(settings)
	elseif compiler == 'clang' then
	 SetDriversClang(settings)
	else
	 return
	end

	if config == "debug" then
		settings.cc.flags:Add("-O0", "-g")
	elseif config == "coverage" then
	  settings.cc.flags:Add("-O0", "-g", "--coverage")
	  settings.link.flags:Add("--coverage")
	elseif config == "release" then
		settings.cc.flags:Add("-O2")
	elseif config == "sanitizer" then
		settings.cc.flags:Add("-O0", "-g", "-fno-omit-frame-pointer", "-fsanitize=undefined", "-fno-sanitize-recover=all")
		settings.link.flags:Add("-fsanitize=undefined", "-fno-sanitize-recover=all")
	else
		print( config  .. ' is not a valid configuration' )
		os.exit(1)
	end

	local arch = platform == 'linux_x86' and '-m32' or '-m64'
	settings.cc.flags:Add( arch )
	settings.dll.flags:Add( arch )
	settings.link.flags:Add( arch )
	settings.link.libs:Add( 'rt' )

	return settings
end

function DefaultMSVC( build_platform, config, compiler )
	local settings = DefaultSettings( build_platform, config, compiler )
	SetDriversCL(settings)

	if config == "debug" then
		settings.cc.flags:Add("/Od", "/MDd", "/Z7", "/D \"_DEBUG\"", "/EHsc", "/GS-")
		settings.dll.flags:Add("/DEBUG")
		settings.link.flags:Add("/DEBUG")
	elseif config == "release" then
		settings.cc.flags:Add("/Ox", "/Ot", "/MD", "/D \"NDEBUG\"", "/EHsc")
	else
		print( config  .. ' is not a valid configuration' )
		os.exit(1)
	end

	return settings
end

function make_gtest_settings( base_settings )
	local settings = TableDeepCopy( base_settings )

	settings.cc.includes:Add( GTEST_PATH )
	settings.cc.includes:Add( PathJoin( GTEST_PATH, 'include' ) )

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
		settings.cc.flags_cxx:Add("-std=c++11") -- enabled to test out sized enums.
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

local compiler = get_compiler()
print( 'compiler used "' .. compiler .. '"')

settings =
{
	linux_x86 = {
	 debug     = DefaultGCCLike( "linux_x86", "debug",     compiler ),
	 release   = DefaultGCCLike( "linux_x86", "release",   compiler ),
	 sanitizer = DefaultGCCLike( "linux_x86", "sanitizer", compiler ),
  },
	linux_x86_64 = {
    debug     = DefaultGCCLike( "linux_x86_64", "debug",      compiler ),
    coverage  = DefaultGCCLike( "linux_x86_64", "coverage",   compiler ),
    release   = DefaultGCCLike( "linux_x86_64", "release",    compiler ),
    sanitizer = DefaultGCCLike( "linux_x86_64", "sanitizer",  compiler )
  },
	win32 = {
    debug   = DefaultMSVC( "win32", "debug",   compiler ),
    release = DefaultMSVC( "win32", "release", compiler )
  },
	winx64 = {
    debug   = DefaultMSVC( "winx64", "debug",   compiler ),
    release = DefaultMSVC( "winx64", "release", compiler )
  }
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
gtest_settings = make_gtest_settings( build_settings )

gtest_lib = StaticLibrary( gtest_settings, "gtest", Compile( gtest_settings, Collect( PathJoin( GTEST_PATH, "src/gtest-all.cc" ) ) ) )
dl_lib    = StaticLibrary( dl_settings,    "dl",    Compile( dl_settings,    Collect( "src/*.cpp" ) ) )
dl_shared = SharedLibrary( dl_settings,    "dlsh",  Compile( dl_so_settings, Collect( "src/*.cpp" ) ) )

dl_settings.cc.includes:Add('tool/dlpack')
getopt   = Compile( dl_settings, CollectRecursive( "tool/dlpack/*.c" ) )
dl_pack  = Link( build_settings, "dlpack",   Compile( dl_settings, CollectRecursive("tool/dlpack/*.cpp") ), getopt, dl_lib )
dltlc    = Link( build_settings, "dltlc",    Compile( dl_settings, CollectRecursive("tool/dltlc/*.cpp") ), getopt, dl_lib )
dl_tests = Link( test_settings,  "dl_tests", Compile( test_settings, Collect("tests/*.cpp") ), dl_lib, gtest_lib )
dlbench  = Link( test_settings,  "dlbench",  Compile( test_settings, Collect("benchmark/*.cpp") ), dl_lib )

tl1 = dl_type_lib( "tests/sized_enums.tld", dltlc, dl_tests )
tl2 = dl_type_lib( "tests/unittest.tld",    dltlc, dl_tests )
tl3 = dl_type_lib( "tests/unittest2.tld",   dltlc, dl_tests )
tl4 = dl_type_lib( "tests/small.tld",       dltlc, dl_tests )
tlbench = dl_type_lib( "benchmark/dlbench.tld", dltlc, dl_tests )

dl_test_valid_c = Compile( dl_settings, Collect( "tests/*.c" ), tl1, tl2, tl3, tl4, tlbench )

local    test_args = ""
local py_test_args = ""
if ScriptArgs["test_filter"] then
	   test_args = " --gtest_filter=" .. ScriptArgs["test_filter"]
	py_test_args = " " .. ScriptArgs["test_filter"]
end

if family == "windows" then
	AddJob( "test",          "unittest c",        string.gsub( dl_tests, "/", "\\" ) .. test_args, dl_tests,    "local/generated/unittest.bin" )

	SkipOutputVerification("test")
else
	local valgrind_flags = " -v --leak-check=full --track-origins=yes "

	AddJob( "test",          "unittest c",        dl_tests .. test_args,                                 dl_tests,    "local/generated/unittest.bin" )
	AddJob( "test_valgrind", "unittest valgrind", "valgrind" .. valgrind_flags .. dl_tests .. test_args, dl_tests,    "local/generated/unittest.bin" )
	AddJob( "test_gdb",      "unittest gdb",      "gdb --args " .. dl_tests .. test_args,                dl_tests,    "local/generated/unittest.bin" )

	SkipOutputVerification("test")
	SkipOutputVerification("test_valgrind")
	SkipOutputVerification("test_gdb")
end

AddJob( "benchmark", "benchmark", dlbench, dlbench )
SkipOutputVerification("benchmark")


PYTHON = "python"
if family == "windows" then -- hackery hack
    PYTHON = "C:\\Python27\\python.exe"
end

dl_tests_py = "tests/python_bindings/dl_tests.py"
AddJob( "test_py",
		"unittest python",
		PYTHON .. " " .. dl_tests_py .. " " .. dl_shared .. " " .. py_test_args,
		dl_tests_py,
		dl_shared, "local/generated/unittest.bin" )

-- do not run unittest as default, only run
PseudoTarget( "dl_default", dl_pack, dltlc, dl_tests, dl_shared, dlbench, dl_test_valid_c )
DefaultTarget( "dl_default" )
