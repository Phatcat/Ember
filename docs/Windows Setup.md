EMBER DOCUMENTATION: WINDOWS INSTALLATION PROCESS
-------------------------------------------------

PRE-REQUIREMENTS:

TOOLCHAIN
CMake - https://cmake.org/download/ (3.10.3 or later)

Visual Studio - https://visualstudio.microsoft.com/downloads/ (2017 update 6 or later)
When installing Visual Studio make sure to select 'Desktop Development with C++'
Also make sure that you install a version of python fitting the architecture you are aiming to build ember with
You can find it under 'Individual Components' and then 'Compilers, build tools, and runtimes'
Also install support for python language under 'Development activities'
You can also download and install Python manually through https://www.python.org/

WinMPQ - http://sfsrealm.hopto.org/downloads/WinMPQ.html

(!OPTIONALS) - The following is only needed if you want to compile mysqlccpp yourself, for ember debug
Perl - http://strawberryperl.com/ OR https://www.activestate.com/products/activeperl/downloads/
nasm - https://www.nasm.us/pub/nasm/releasebuilds/
openssl - https://github.com/openssl/openssl/releases


LIBRARIES
boost - https://sourceforge.net/projects/boost/files/boost-binaries
1.69 or later, make sure it fits your version of Visual Studio

botan - https://github.com/randombit/botan/releases
2.4.0 or later

flatbuffers - https://github.com/google/flatbuffers/releases
If you are not interested in building ember in debug you can choose to download both the source and the flatc.exe executable
That way you won't have to build flatbuffers, but instead you'll have to move the flatc.exe file into a folder called 'bin' inside flatbuffers
If you want to build ember in debug just get the source

pcre - ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre
8.39 or later - NOT pcre2!

zlib - https://github.com/madler/zlib/releases
1.2.8 or later

Pick the source as well as perl, openssl and nasm if you want to be able to run ember in debug, otherwise pick the pre-compiled
mysqlcppconn (Source) - https://github.com/mysql/mysql-connector-cpp/releases (Make sure you grab the 1.x.x and not the 8.x.x version)
mysqlcppconn (Pre-compiled) - https://dev.mysql.com/downloads/connector/cpp/1.1.html


(refer with CMakeList.txt for up to date toolchain and library requirements)

-------------------------------

STEP 1: GETTING READY

Download everything.

Unpack the contents of all the library archives into a dedicated folder 
(For this guide we are going to assume C:/Libs is used)

Install CMake, Boost, Python, Visual Studio and WinMPQ.
Optionally also install Perl and nasm.

With boost you need to manually add a new system environmental variable with the name BOOST_ROOT pointing to your boost install location.

You can skip this if you installed Python through Visual Studio, otherwise Python will ask to do it for you during installation.
You can also manually add it as an entry to the PATH system environmental variable pointing to your root python root installation directory.

(!OPTIONALS) - The following is only needed if you want to compile mysqlccpp yourself, for debugging
With perl the system environmental path variable should be set automatically during installation of Strawberry Perl.
With nasm you need to manually add another entry to the PATH system environmental variable pointing to your root nasm install location.
With openSSL the system environmental path variable should be set automatically during nmake install of openssl.

--------------------------------

STEP 2: COMPILING AND INSTALLING BOTAN

To compile botan you must have python and visual studio installed.

Open x86/x64 Native Tools Command Prompt for VS 2017 (just search for it) with administrative rights,
and change directory to your Botan path (hint: Libs/botan)

Enter the following commands;

$ .\configure.py --debug
$ nmake
$ botan-test.exe
$ nmake install

You should now have the debug version of botan installed locally (C:/Botan)

Rename the folder lib inside C:/Botan to libd

Now open up the Native Tools Command Prompt for VS 2017 again at the same location.

Enter the following commands;

$ .\configure.py --release
$ nmake
$ botan-test.exe
$ name install

You should now have the release version of botan installed locally (C:/Botan)

copy/move the contents of Botan/include/botan-2 and place them in Botan/include

-------------------------------

STEP 3 (OPTIONAL): COMPILING AND INSTALLING OPENSSL
(Skip this part if you are not compiling mysqlccpp yourself)

To compile OpenSSL you must have perl (either ActivePerl or StrawberryPerl) installed.

Open x86/x64 Native Tools Command Prompt for VS 2017 (just search for it) with administrative rights,
and change directory to your OpenSSL path (hint: Libs/openssl)

Enter the following commands;
(Only pick a single VC option, the one that fits what you are aiming to build for)

$ perl Configure ( VC-WIN32 | VC-WIN64A | VC-WIN64I | VC-CE ) --debug
$ nmake
$ nmake test
$ nmake install

-------------------------------

STEP 4 (OPTIONAL): COMPILING MYSQLCCPP
(Skip this part if you downloaded the pre-compiled and not source files)

To compile MySQL-Connector-C++ you must have an SSL solution (OpenSSL or WolfSSL)

Launch CMake GUI

Press 'Browse Source...' and select your dedicated MySQL-Connector-C++ folder (default is Libs/mysql-connector-cpp)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/mysql-connector-c++/build)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.
Press 'Generate' and then 'Open Project'

Select release in Visual Studio and build the solution.

move all the contents of the folder Libs/mysql-connector-c++/Build/driver/Release and place it all in Libs/mysql-connector-c++/lib

Open up the solution in Visual studio again and build the solution in debug mode this time

move all the contents of the folder Libs/mysql-connector-c++/Build/driver/Debug and place it all in Libs/mysql-connector-c++/libd

-------------------------------

STEP 5 (OPTIONAL): COMPILING FLATBUFFERS
(Skip this part if you downloaded flatc.exe and aren't building ember in debug)

Launch CMake GUI

Press 'Brose Source...' and select your dedicated FlatBuffers dep folder (default is Libs/flatbuffers)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/flatbuffers/bin)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.
Press 'Generate' and then 'Open Project'

Select release or debug in Visual Studio and build the solution.

Move the file flatc.exe from Libs/flatbuffers/bin/Release or Libs/flatbuffers/bin/Debug to Libs/flatbuffers/bin

-------------------------------

STEP 6: COMPILING ZLIB

Launch CMake GUI

Press 'Brose Source...' and select your dedicated zlib dep folder (default is Libs/zlib)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/zlib/build)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.
Press 'Generate' and then 'Open Project'

Select release in Visual Studio and build the solution.

Copy/move the files zconf.h and zlib.pc from the Libs/zlib/build folder and place in Libs/zlib
Copy/move the files from the folder Libs/zlib/build/Release and place them in Libs/zlib/lib

To build ember in debug open up the solution in Visual studio again and build the solution in debug mode this time
then copy/move the files from the folder Libs/zlib/Build/Debug and place them in Libs/zlib/lib

-------------------------------

STEP 7: COMPILING PCRE

Launch CMake GUI

Press 'Brose Source...' and select your dedicated pcre dep folder (default is Libs/pcre)
Press 'Brose Build...' and select your dedicated build folder (default is Libs/pcre/build)

Press 'Configure' and select Visual Studio 2017 and use default native compilers.
Select UTF and SUPPORT_JIT on and leave the rest at default
Press 'Configure' again, then press 'Generate' and then 'Open Project'

Select release in Visual Studio and build the solution.

Copy/move the file 'pcre.h' from the Libs/pcre/build folder and place it in Libs/pcre
Copy/move the files from the folder Libs/pcre/build/Release and place them in Libs/pcre/lib

To build ember in debug open up the solution in Visual studio again and build the solution in debug mode this time
then copy/move the files from the folder Libs/pcre/build/Debug and place them in Libs/pcre/lib

-------------------------------

STEP 8: GENERATING EMBER VS SOLUTION

Launch CMake GUI

Press 'Browse Source...' and select your dedicated Ember source folder (hint: Ember)
Press 'Browse Build...' and select your dedicated build folder (hint: Ember/build)

Press 'Add entry' and enter all of the following entries BEFORE configuring;

Name                            Type        Value
BOTAN_ROOT_DIR                  PATH        C:/Botan
MYSQLCCPP_ROOT_DIR              PATH        C:/Libs/mysql-connector-c++
FLATBUFFERS_INCLUDE_DIR         PATH        C:/Libs/flatbuffers/include
FLATBUFFERS_FLATC_EXECUTABLE    FILEPATH    C:/Libs/flatbuffers/bin/flatc.exe
ZLIB_ROOT_DIR                   PATH        C:/Libs/zlib
PCRE_ROOT_DIR                   PATH        C:/Libs/pcre

If you have built mysqlccpp yourself you have to set another entry
MYSQLCCPP_INCLUDE_DIR           PATH        C:/Libs/mysql-connector-c++/driver

OBS! Adjust the paths to fit your setup!

Press 'Configure', select your version of Visual Studio 
and the architecture you built the dependencies with,
and your prefered compilers, then press 'Generate'.

--------------------------------

STEP 9: COMPILING EMBER

Launch the VS solution from the dedicated build folder from before

Choose your solution (debug/release) and build.

--------------------------------

STEP 10: COPYING OVER DEPENDENCIES AND CONFIG FILES

Copy in the following library files and place them in Ember/build/bin:

pcre.dll from the Libs/pcre/lib folder
zlib.dll from the Libs/zlib/build/Release folder
mysqlcppconn.dll from the Libs/mysql-connector-c++/lib or Libs/mysql-connector-c++/Build/driver/Debug folder

Copy in config files from the Ember/configs folder and place them in Ember/build/bin

--------------------------------

STEP 11: EXTRACTING CLIENT DBC FILES

Locate your game client and extract all .dbc files from the mpq files mentioned below using WinMPQ

dbc.MPQ
patch.MPQ
patch-2.MPQ

(normally located in World of Warcraft/Data), and place them all in Ember/build/bin/dbcs

--------------------------------

STEP 12: SETTING UP EMBER DB

Run create.sql from the Ember/sql/mysql folder

Run full.sql from the Ember/sql/mysql folder

--------------------------------

STEP 13: CREATING AN ACCOUNT

[WIP]
