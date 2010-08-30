--[[ 

    TODO: This file is a real hackwork and should be prio one to get working right!
    
    * Add configs
    * Add builds for more platforms
    * Build unittests and needed headers with !
    ... and more!
--]]

settings = NewSettings()

-- ugly add!
settings.cc.includes:Add("S:/Source/Engine2/external/yajl-1.0.8/include")
settings.cc.includes:Add("include")
settings.cc.Output = function(settings, path) return PathJoin("local/obj", PathFilename(PathBase(path)) .. settings.config_ext) end

settings.link.libpath:Add("local/lib", "S:/Source/Engine2/dev/local/debug/win32")
settings.link.libs:Add("yajl")
settings.link.flags:Add("/DEBUG")
settings.dll.libpath:Add("local/lib", "S:/Source/Engine2/dev/local/debug/win32")
settings.dll.libs:Add("yajl")
settings.dll.flags:Add("/DEBUG")

files = CollectRecursive("src/*.cpp")
objs = Compile(settings, files)
lib = StaticLibrary(settings, "local/lib/dl", objs)
sh  = SharedLibrary(settings, "local/dll/dldyn", objs)

-- mod settings to build dl_pack
settings.cc.includes:Add("src")
settings.link.libs:Add("dl")
dl_pack = Link(settings, "local/exe/dl_pack", Compile(settings, Collect("tool/dl_pack/*.cpp")), lib)

-- for unittests to build python-scripts need to run to generate unittest.h

-- mod settings to build dl_tests
settings.cc.includes:Add("extern/gtest-1.3.0/include")
unittests = Link(settings, "local/exe/dl_tests", Compile(settings, Collect("tests/*.cpp", "external/gtest-1.3.0/src")), lib)
