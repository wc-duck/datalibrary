****  DEPENDENCIES  ****

This libraries is needed to build DL and the libraries need to be avaliable in the default include- and lib-paths.

- yajl  https://github.com/lloyd/yajl
- gtest http://code.google.com/p/googletest ( only needed to build unittests )
- bam   https://github.com/matricks/bam ( used as buildtool so if you do not want to use dl:s buildscripts, ignore )


**** HOWTO BUILD DL ****

Bam buildscripts assumes to find its dependencies installed on the system on platforms that support it, on the other
systems dependencies are assumed to be found in the folder "external/" with includes under "include/DEP" and library-files 
under "libs/PLATFORM/CONFIGURATION".
They also assumes to find Python2.6 in the path.

example:
external/
    include/
      yajl/
        headers.h
      gtest/
        headers.h
    libs/
      win32/
        debug/
          yajl.lib
          gtest.lib
        release/
          yajl.lib
          gtest.lib
		

the buildscripts define different targets and platforms.
command-line parameters:
	platform=PLATFORM ( supported linux32, linux64, win32, win64, ps3, xenon )
	config=CONFIG ( supported debug, release )

targets:
	test          - build and run c-unittests
	test_valgrind - build and run c-unittests under valgrind
	test_py       - build and run python-unittests
	test_cs       - build and run C#-unittests


****    EXAMPLE     ****

build and run linux 32bit unittest in debug:
bam platform=linux32 config=debug test
