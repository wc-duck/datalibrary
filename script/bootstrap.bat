if not exist "local" mkdir local

git clone https://github.com/matricks/bam.git local/bam

cd local/bam
call make_win64_msvc.bat
exit
