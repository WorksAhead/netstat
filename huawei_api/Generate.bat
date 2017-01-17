IF NOT EXIST curl (
	git clone https://github.com/curl/curl.git
	cd curl
	mkdir build
	cd build
	cmake -G "Visual Studio 14 2015 Win64" -DCURL_STATICLIB=1 ..
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" /property:Configuration=Release lib\libcurl.vcxproj
	"C:\Program Files (x86)\MSBuild\14.0\Bin\MSBuild.exe" /property:Configuration=Debug lib\libcurl.vcxproj
	cd ..\..\
)

IF EXIST build rd /s /q build
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" -DCURL_INCLUDE_DIR="%cd%\..\curl\include" -DCURL_LIBRARY="%cd%\..\curl\build\lib" ..