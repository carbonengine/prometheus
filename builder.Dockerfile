# escape=`

FROM mcr.microsoft.com/windows/servercore:1809 AS builder

# Set up environment to collect install errors.
# COPY Install.cmd C:\TEMP\
ADD https://aka.ms/vscollect.exe C:\TEMP\collect.exe

# Download the Build Tools bootstrapper.
ADD https://aka.ms/vs/15/release/vs_buildtools.exe C:\TEMP\vs_buildtools.exe

# Restore the default Windows shell for correct batch processing below.
SHELL ["cmd", "/S", "/C"]

# Install Build Tools
RUN C:\TEMP\vs_buildtools.exe --quiet --wait --norestart --nocache `
    --installPath C:\BuildTools `
    --add Microsoft.VisualStudio.Component.Windows10SDK `
    --add Microsoft.VisualStudio.Component.VC.CoreBuildTools `
    --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    --add Microsoft.VisualStudio.Component.Windows81SDK `
    --add Microsoft.Component.VC.Runtime.UCRTSDK `
    --add Microsoft.VisualStudio.Component.Windows10SDK.17763 `
 || IF "%ERRORLEVEL%"=="3010" EXIT 0

RUN md C:\build\import
WORKDIR C:/build/import

# Install Stackless
SHELL ["powershell", "-Command"]
ADD http://www.stackless.com/binaries/python-2.7.15150.amd64-stackless.msi C:\stackless.msi
RUN Start-Process -filepath C:\stackless.msi -ArgumentList "/qn", "targetdir=C:\build\import\python27" -PassThru | Wait-Process

# Install Chocolatey and Git for vcpkg
SHELL ["powershell", "-Command"]
RUN Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
RUN choco install --yes --no-progress --limit-output git.install --params "'/GitAndUnixToolsOnPath /NoGitLfs /SChannel /NoAutoCrlf'"

# Install pip for pytest
ADD https://bootstrap.pypa.io/get-pip.py C:\get-pip.py
RUN C:\build\import\python27\python.exe C:\get-pip.py

# Install pytest
RUN C:\build\import\python27\Scripts\pip.exe install -U pytest

# Install vcpkg-based imports
SHELL ["cmd", "/S", "/C"]

# Install prometheus-cpp v0.7.0
WORKDIR C:/build/import
RUN git clone https://github.com/Microsoft/vcpkg.git prometheus-cpp
WORKDIR C:/build/import/prometheus-cpp
RUN git checkout 72657582cff173ce285611f32716110cc0ada989
RUN bootstrap-vcpkg.bat
RUN vcpkg.exe install prometheus-cpp:x64-windows-static

# Install gtest v1.10.0
WORKDIR C:/build/import
RUN git clone https://github.com/Microsoft/vcpkg.git gtest
WORKDIR C:/build/import/gtest
RUN git checkout 92fb473d2b1891cf5c3a9ff4da60282ac215a64f
RUN bootstrap-vcpkg.bat
RUN vcpkg.exe install gtest:x64-windows-static

# Install curl v7.66.0
WORKDIR C:/build/import
RUN git clone https://github.com/Microsoft/vcpkg.git curl
WORKDIR C:/build/import/curl
RUN git checkout d3a7830335274d2b8af203521d7314edd8019b48
RUN bootstrap-vcpkg.bat
RUN vcpkg.exe install curl:x64-windows-static

# Entrypoint
COPY entry.bat C:/source/entry.bat
ENTRYPOINT ["C:/source/entry.bat"]