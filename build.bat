@echo off

rmdir /s /q export
md export
docker build -t eve-monolith-prometheus-builder -f builder.Dockerfile .

if "%~1"=="-test" (set buildflags="-test") else (set buildflags="")

docker run --rm --mount type=bind,source="%cd%",target="c:\source" --mount type=bind,source="%cd%\export",target="c:\export" eve-monolith-prometheus-builder %buildflags%
