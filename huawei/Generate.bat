@ECHO OFF

SET DEP_DIR=%CD%\huawei_api_server\dependencies

SET CURL_DIR=%DEP_DIR%\curl
SET JSON_DIR=%DEP_DIR%\json
SET HMAC_DIR=%DEP_DIR%\hmac
SET B64C_DIR=%DEP_DIR%\b64.c

SET PROTOBUF_ROOT_DIR=%CD%\protobuf
SET PROTOBUF_DIR=%PROTOBUF_ROOT_DIR%\google_protobuf

SET CURL_GIT=https://github.com/curl/curl.git
SET JSON_GIT=https://github.com/nlohmann/json.git

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

PUSHD %DEP_DIR%

IF NOT EXIST %CURL_DIR% (
	ECHO Clone curl ...
	git clone %CURL_GIT%

	PUSHD %CURL_DIR%

	ECHO Build curl ...
	MKDIR build & CD build
	cmake -G "Visual Studio 14 2015 Win64" -DCURL_STATICLIB=1 -DBUILD_CURL_EXE=0 -DBUILD_TESTING=0 ..
	%MSBUILD_CMD% /property:Configuration=Release lib\libcurl.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug lib\libcurl.vcxproj

	POPD
)

IF NOT EXIST %JSON_DIR% (
	ECHO Clone nlohmann/json ...
	git clone %JSON_GIT%
)

:: Build hmac
PUSHD %HMAC_DIR%

IF NOT EXIST build (
	ECHO Build hmac ...
	MKDIR build & CD build
	cmake -G "Visual Studio 14 2015 Win64" ..
	%MSBUILD_CMD% /property:Configuration=Release hmac.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug hmac.vcxproj
)

POPD

:: Build b64.c
PUSHD %B64C_DIR%

IF NOT EXIST build (
	ECHO Build b64.c ...
	MKDIR build & CD build
	cmake -G "Visual Studio 14 2015 Win64" ..
	%MSBUILD_CMD% /property:Configuration=Release b64c.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug b64c.vcxproj
)

POPD

POPD

IF EXIST build (
	RD /s /q build
)
ECHO Generate Visual Studio project ...
MKDIR build & CD build
cmake -G "Visual Studio 14 2015 Win64" -DProtobuf_SRC_ROOT_FOLDER="%PROTOBUF_DIR%" -DProtobuf_PROTOC_EXECUTABLE="%PROTOBUF_DIR%\cmake\build\Release\protoc.exe" -DProtobuf_LIBRARIES="libprotoc.lib;libprotobuf.lib" -DCURL_INCLUDE_DIR="%CURL_DIR%\include" -DCURL_LIBRARY="%CURL_DIR%\build\lib" -DJSON_INCLUDE_DIR="%JSON_DIR%\src" -DHMAC_INCLUDE_DIR="%HMAC_DIR%" -DHMAC_LIBRARY="%HMAC_DIR%\build\lib" -DB64C_INCLUDE_DIR="%B64C_DIR%" -DB64C_LIBRARY="%B64C_DIR%\build\lib" ..
