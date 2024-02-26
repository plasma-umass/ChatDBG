cl /EHsc /MD /Zi /c /I. /std:c++20 chatdbg.cpp
link /DLL chatdbg.obj "C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\arm64\dbgeng.lib" "C:\Program Files (x86)\Windows Kits\10\Debuggers\lib\arm64\dbghelp.lib" libcurl.lib
