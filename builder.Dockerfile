# escape=`

FROM microsoft/windowsservercore:1803 AS builder

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
RUN wget http://www.stackless.com/binaries/python-2.7.15150.amd64-stackless.msi -Outfile C:\stackless.msi
RUN Start-Process -filepath C:\stackless.msi -ArgumentList "/qn", "targetdir=C:\build\import\python27" -PassThru | Wait-Process

# Install Chocolatey and Git for vcpkg
SHELL ["powershell", "-Command"]
RUN Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
RUN choco install --yes --no-progress --limit-output git.install --params "'/GitAndUnixToolsOnPath /NoGitLfs /SChannel /NoAutoCrlf'"

# Install vcpkg
SHELL ["cmd", "/S", "/C"]
WORKDIR C:/build/import
RUN git clone https://github.com/Microsoft/vcpkg.git && cd vcpkg && .\bootstrap-vcpkg.bat

# Install vcpkg-based dependencies
WORKDIR C:/build/import/vcpkg
RUN vcpkg.exe install prometheus-cpp:x64-windows
RUN vcpkg.exe install gtest:x64-windows
RUN vcpkg.exe install curl:x64-windows

COPY entry.bat C:/source/entry.bat

# Entrypoint
ENTRYPOINT ["C:/source/entry.bat"]