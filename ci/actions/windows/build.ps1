$ErrorActionPreference = "Continue"

if (${env:artifact} -eq 1) {
    $env:BUILD_TYPE = "Release"
    if ( ${env:BETA} -eq 1 ) {
        $env:NETWORK_CFG = "beta"
        $env:BUILD_TYPE = "RelWithDebInfo"
    }
    elseif (${env:TEST} -eq 1) {
        $env:NETWORK_CFG = "test"
    }
    else {
        $env:NETWORK_CFG = "live"
    }
    $env:NANO_SHARED_BOOST = "ON"
    $env:NANO_TEST = "-DNANO_TEST=OFF"
    $env:TRAVIS_TAG = ${env:TAG}
    
    $env:CI = "-DCI_BUILD=ON"
    $env:RUN = "artifact"
}
else {
    if ( ${env:RELEASE} -eq "true" -or ${env:TEST_USE_ROCKSDB} -eq 1 ) {
        $env:BUILD_TYPE = "RelWithDebInfo"
    }
    else { 
        $env:BUILD_TYPE = "Debug"
    }
    $env:NANO_SHARED_BOOST = "OFF"
    $env:NETWORK_CFG = "dev"
    $env:NANO_TEST = "-DNANO_TEST=ON"
    $env:CI = '-DCI_TEST="1"'
    $env:RUN = "test"
}

mkdir build
Push-Location build

#accessibility of Boost dlls for generating config samples
$ENV:PATH = "$ENV:PATH;$ENV:BOOST_ROOT\lib64-msvc-14.2"

& ..\ci\actions\windows\configure.bat
if (${LastExitCode} -ne 0) {
    throw "Failed to configure"
}

if (${env:RUN} -eq "artifact") {
    $p = Get-Location
    Invoke-WebRequest -Uri https://aka.ms/vs/16/release/vc_redist.x64.exe -OutFile "$p\vc_redist.x64.exe"
}

& ..\ci\actions\windows\build.bat
if (${LastExitCode} -ne 0) {
    throw "Failed to build ${env:RUN}"
}
$env:cmake_path = Split-Path -Path(get-command cmake.exe).Path
. "$PSScriptRoot\signing.ps1"

& ..\ci\actions\windows\run.bat
if (${LastExitCode} -ne 0) {
    throw "Failed to Pass Test ${env:RUN}"
}

Pop-Location