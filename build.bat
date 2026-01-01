@echo off
:start
make 3dsx
make cia_all -j
echo Press any key to rebuild.
pause
goto start 
