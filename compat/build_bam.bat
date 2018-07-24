call c:\"Program Files (x86)"\"Microsoft Visual Studio 12.0"\VC\vcvarsall.bat
git clone https://github.com/matricks/bam.git
cd bam
git reset --hard c021a4c0d86dd0f042bf49046ee6bb51b88b8d96
call make_win64_msvc.bat
exit
