@echo off

echo Extracting textures from all models in current folder ...

echo __________________________________________________

for %%I in (*.mdl) do pvr2mdl.exe extract ^"%%I^"

echo __________________________________________________

echo Done. Press any key to exit ...

pause