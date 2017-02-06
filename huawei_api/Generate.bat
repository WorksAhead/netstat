@ECHO OFF

SET DEP_DIR=%CD%\dependencies

SET CURL_DIR=%DEP_DIR%\curl
SET JSON_DIR=%DEP_DIR%\json
SET HMAC_DIR=%DEP_DIR%\hmac
SET B64C_DIR=%DEP_DIR%\b64.c

SET CURL_GIT=https://github.com/curl/curl.git
SET JSON_GIT=https://github.com/nlohmann/json.git

SET MSBUILD_CMD="C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe"

PUSHD %DEP_DIR%

IF NOT EXIST %CURL_DIR% (
	git clone %CURL_GIT%

	PUSHD %CURL_DIR%

	MKDIR build
	CD build
	cmake -G "Visual Studio 14 2015 Win64" -DCURL_STATICLIB=1 -DBUILD_CURL_EXE=0 -DBUILD_TESTING=0 ..
	%MSBUILD_CMD% /property:Configuration=Release lib\libcurl.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug lib\libcurl.vcxproj

	POPD
)

IF NOT EXIST %JSON_DIR% (
	git clone %JSON_GIT%
)

:: Build hmac
PUSHD %HMAC_DIR%

IF NOT EXIST build (
	MKDIR build
	CD build
	cmake -G "Visual Studio 14 2015 Win64" ..
	%MSBUILD_CMD% /property:Configuration=Release hmac.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug hmac.vcxproj
)

POPD

:: Build b64.c
PUSHD %B64C_DIR%

IF NOT EXIST build (
	MKDIR build
	CD build
	cmake -G "Visual Studio 14 2015 Win64" ..
	%MSBUILD_CMD% /property:Configuration=Release b64c.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug b64c.vcxproj
)

POPD

POPD

IF EXIST build (
	RD /s /q build
)
MKDIR build
CD build
cmake -G "Visual Studio 14 2015 Win64" -DCURL_INCLUDE_DIR="%CURL_DIR%\include" -DCURL_LIBRARY="%CURL_DIR%\build\lib" -DJSON_INCLUDE_DIR="%JSON_DIR%\src" -DHMAC_INCLUDE_DIR="%HMAC_DIR%" -DHMAC_LIBRARY="%HMAC_DIR%\build\lib" -DB64C_INCLUDE_DIR="%B64C_DIR%" -DB64C_LIBRARY="%B64C_DIR%\build\lib" ..

PAUSE