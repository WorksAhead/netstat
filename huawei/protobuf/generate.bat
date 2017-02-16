@ECHO OFF

SET PROTOBUF_RELEASE_TAG=3.2.x
SET PROTOBUF_GIT=https://github.com/google/protobuf.git
SET PROTOBUF_DIR=%CD%\google_protobuf

SET MSBUILD_CMD="C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe"

IF NOT EXIST %MSBUILD_CMD% (
	ECHO MSBuild.exe does not exist. Visual Studio 14 2015 is required.
	EXIT /B 1
	)

IF NOT EXIST "%PROTOBUF_DIR%" (
	ECHO Clone google/protobuf ...
	git clone -b %PROTOBUF_RELEASE_TAG% %PROTOBUF_GIT% "%PROTOBUF_DIR%"
	CALL:BUILD_PROTOBUF

) ELSE (
	ECHO Pull google/protobuf ...
    PUSHD "%PROTOBUF_DIR%"
    git pull
    POPD
    CALL:BUILD_PROTOBUF
)

:BUILD_PROTOBUF
	ECHO Build google/protobuf ...

	PUSHD "%PROTOBUF_DIR%\cmake"

	IF NOT EXIST build (
		:: RD /s /q build
		MKDIR build
	)

	CD build

	cmake -G "Visual Studio 14 2015 Win64" -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_DEBUG_POSTFIX= -Dprotobuf_MSVC_STATIC_RUNTIME=OFF ..

	%MSBUILD_CMD% /property:Configuration=Release libprotoc.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug libprotoc.vcxproj

	%MSBUILD_CMD% /property:Configuration=Release libprotobuf.vcxproj
	%MSBUILD_CMD% /property:Configuration=Debug libprotobuf.vcxproj

	%MSBUILD_CMD% /property:Configuration=Release protoc.vcxproj

	POPD
GOTO:EOF
