if not exist "local" mkdir local

git clone https://github.com/matricks/bam.git local/bam

cd local/bam

git reset --hard c021a4c0d86dd0f042bf49046ee6bb51b88b8d96

call make_win64_msvc.bat
exit
