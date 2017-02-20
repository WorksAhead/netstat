@ECHO OFF

SET PROTOBUF_ROOT_DIR=%CD%\protobuf
SET PROTOBUF_DIR=%PROTOBUF_ROOT_DIR%\google_protobuf
SET PROTOBUF_PROTOS_DIR=%PROTOBUF_ROOT_DIR%\protos
SET PROTOBUF_PROTOS_CPP_OUT_DIR=%PROTOBUF_PROTOS_DIR%\cpp_out

SET PROTOBUF_PROTOC_CMD="%PROTOBUF_DIR%\cmake\build\Release\protoc.exe"
SET MSBUILD_CMD="C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe"

IF NOT EXIST %PROTOBUF_PROTOC_CMD% (
	ECHO protoc.exe does not exist. Please run %PROTOBUF_ROOT_DIR%\generate.bat first.
	PAUSE
	EXIT /B 1
	)

IF NOT EXIST %MSBUILD_CMD% (
	ECHO MSBuild.exe does not exist. Visual Studio 14 2015 is required.
	PAUSE
	EXIT /B 1
	)

IF NOT EXIST "%PROTOBUF_PROTOS_CPP_OUT_DIR%" (
	MKDIR "%PROTOBUF_PROTOS_CPP_OUT_DIR%"
)

PUSHD "%PROTOBUF_PROTOS_CPP_OUT_DIR%"

IF EXIST build (
	RD /s /q build
)
MKDIR build & CD build

cmake -G "Visual Studio 14 2015 Win64" -DProtobuf_SRC_ROOT_FOLDER="%PROTOBUF_DIR%" -DProtobuf_PROTOC_EXECUTABLE="%PROTOBUF_DIR%\cmake\build\Release\protoc.exe" -DProtobuf_LIBRARIES="libprotoc.lib;libprotobuf.lib" ..\..

%MSBUILD_CMD% /property:Configuration=Release protos.vcxproj
%MSBUILD_CMD% /property:Configuration=Debug protos.vcxproj

POPD
