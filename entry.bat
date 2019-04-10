@rem Set up build directory
md C:\build

@rem /e=all directories
@rem /xo=don't overwrite newer files
@rem /NFL /NDL /NJH /NJS /nc /ns /np = be silent
robocopy C:\source C:\build /e /NFL /NDL /NJH /NJS /nc /ns /np
robocopy C:\import C:\build\import /e /xo /NFL /NDL /NJH /NJS /nc /ns /np

@rem Build the source
cd C:\build
call C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x64 10.0.17763.0 && msbuild /m prometheus_module.sln

@rem Copy prometheus_module exports
md C:\export\bin
md c:\export\include

copy C:\build\x64\release\prometheus_module.pyd C:\export\bin\
copy C:\build\prometheus_module\*_interface.h C:\export\include\

@rem Copy Python dependencies
copy C:\build\import\python27\python27.dll C:\export\bin\

@rem Copy vcpkg dependencies
copy C:\build\import\vcpkg\installed\x64-windows\bin\zlib1.dll C:\export\bin\

@rem Set up tests
md C:\export\tests
copy C:\export\bin\* C:\export\tests\
copy C:\build\test\test.py C:\export\tests\
copy C:\build\x64\release\test_native.pyd C:\export\tests\
copy C:\build\import\vcpkg\installed\x64-windows\bin\gtest.dll C:\export\tests\
copy C:\build\import\vcpkg\installed\x64-windows\bin\libcurl.dll C:\export\tests\

@rem Run tests if requested
if not "%~1"=="-test" goto skiptests
cd C:\export\tests
C:\build\import\python27\python.exe -m unittest discover -v -p test.py
:skiptests

@rem Done