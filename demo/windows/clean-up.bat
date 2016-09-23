rmdir /s /q bin
rmdir /s /q obj
rmdir /s /q lib

del /q msvc2012\*.sdf
del /q msvc2012\*.opensdf
del /q msvc2012\*.VC.db
del /q /a:h msvc2012\*.suo

del /q msvc2013\*.sdf
del /q msvc2013\*.opensdf
del /q msvc2013\*.VC.db
del /q /a:h msvc2013\*.suo

del /q msvc2015\*.sdf
del /q msvc2015\*.opensdf
del /q msvc2015\*.VC.db
del /q /a:h msvc2015\*.suo

pause
