@rem Copy sources into the build directory
@rem This is done because we don't want to pollute the mounted source volume with build artifacts
@rem This also gives us a clean rebuild every time

@rem /e=all directories
@rem /NFL /NDL /NJH /NJS /nc /ns /np = be silent
robocopy C:\source C:\build /e /NFL /NDL /NJH /NJS /nc /ns /np

@rem Build the source
cd C:\build
call C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x64 10.0.17763.0 && msbuild /m prometheus_module.sln

@rem Copy prometheus_module exports
md C:\export\bin
md c:\export\include

copy C:\build\x64\release\prometheus_module.pyd C:\export\bin\
copy C:\build\x64\release\prometheus_module.pdb C:\export\bin\
copy C:\build\prometheus_module\*_interface.h C:\export\include\

@rem Copy Python dependencies
copy C:\build\import\python27\python27.dll C:\export\bin\

@rem Set up tests
md C:\export\tests
copy C:\export\bin\* C:\export\tests\
copy C:\build\test\test.py C:\export\tests\
copy C:\build\x64\release\test_native.pyd C:\export\tests\

@rem Run tests if requested
if not "%~1"=="-test" goto skiptests
cd C:\export\tests
C:\build\import\python27\Scripts\pytest.exe -v --junitxml=test_results.xml test.py
:skiptests

@rem Done