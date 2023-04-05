REM --- General clean-up ---
del /a:h yahel.suo
del C:\WINDOWS\yahel.ini
rd /s /q debug ipch release "Release MFC 4.2" "Debug in RAMdisk"
rd /s /q r:\ipch r:\yahel
attrib -h -s /d "%~dp0\.vs"
rd /s /q "%~dp0\.vs"

REM --- Yahel project clean-up ---
cd Yahel
del res\resource.aps
rd /s /q debug ipch release "Release MFC 4.2" "Debug in RAMdisk"
cd..
