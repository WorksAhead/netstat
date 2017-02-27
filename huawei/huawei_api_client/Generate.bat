@ECHO OFF

SET PROTOBUF_ROOT_DIR=%CD%\..\protobuf
SET PROTOBUF_DIR=%PROTOBUF_ROOT_DIR%\google_protobuf

IF EXIST build (
	RD /s /q build
)
ECHO Generate Visual Studio project ...
MKDIR build & CD build
cmake -G "Visual Studio 14 2015 Win64" -DProtobuf_SRC_ROOT_FOLDER="%PROTOBUF_DIR%" -DProtobuf_PROTOC_EXECUTABLE="%PROTOBUF_DIR%\cmake\build\Release\protoc.exe" -DProtobuf_LIBRARIES="libprotoc.lib;libprotobuf.lib" ..
