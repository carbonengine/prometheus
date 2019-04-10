# eve-monolith-prometheus

Prometheus module for monolith.

This wraps the native [prometheus-cpp](https://github.com/jupp0r/prometheus-cpp) client and exposes it to Python as a native module (PYD).

## Building

### Docker

* Install [Docker for Windows](https://docs.docker.com/docker-for-windows/install/)
* Run `build.bat` (or `build.bat -test` if you want to run tests as well)
* Resulting artifacts will appear in `.\export`

### Manual

* Install VS2017 if you don't have it already
* Install [Windows 10 SDK 10.0.17763.132](https://go.microsoft.com/fwlink/p/?LinkID=2033908) or newer if you don't have it already
* Install dependencies. See build.Dockerfile for functional steps.
    * Create an import folder in the source root directory
    * Copy (or link) the [Stackless](http://www.stackless.com/binaries/python-2.7.15150.amd64-stackless.msi) Python27 folder into the import folder (`.\import\Python27`)
    * Install [vcpkg](https://github.com/Microsoft/vcpkg) into the import folder (`.\import\vcpkg`)
    * Open a command prompt inside the vcpkg folder. Use locally-installed packages instead of the global option in vcpkg.
    * Install [prometheus-cpp](https://github.com/jupp0r/prometheus-cpp): `vcpkg install prometheus-cpp:x64-windows`
    * Install [Googletest](https://github.com/google/googletest) (for tests only): `vcpkg install gtest:x64-windows`
    * Install [curl](https://github.com/curl/curl) (for tests only): `vcpkg install curl:x64-windows`
* Open prometheus_module.sln and build it.

## Running tests

### Docker

* Run `build.bat -test`

### Manual

* After building the solution,
* `cd test`
* `copy ..\x64\release\*.pyd`
* `copy ..\import\vcpkg\x64-windows\bin\*.dll`
* `python test.py`