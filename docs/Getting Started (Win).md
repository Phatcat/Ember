# 🔥 **EMBER** DOCUMENTATION: GETTING STARTED (WINDOWS)

## PRE-REQUIREMENTS:

### TOOLCHAIN
- [CMake](https://cmake.org/download/) - __3.12.4__ or newer.

- [Visual Studio](https://visualstudio.microsoft.com/downloads/) - __2019 update 6 or newer__.
When installing Visual Studio make sure to select 'Desktop Development with C++'.
Make sure that you install a version of python fitting the architecture you are aiming to build ember with.
You can find it under '_Individual Components_' and then '_Compilers, build tools, and runtimes_'.
Aditionally you need to install support for python language under 'Development activities'
You can also download and install Python manually through https://www.python.org/

- [MySQL Server](https://dev.mysql.com/downloads/mysql/) - Get the 8.0.x version.

- [MySQLConnectorC++](https://dev.mysql.com/downloads/connector/cpp/8.0.html) - Get the 8.0.x version, just use the installer, and if you want to compile ember in Debug, grab the debug binaries as well.

### LIBRARIES
- [boost](https://sourceforge.net/projects/boost/files/boost-binaries) - __1.76 or newer__ (make sure it fits your version of Visual Studio)

- [botan](https://github.com/randombit/botan/releases) - __2.14.0 or newer__

- [flatbuffers](https://github.com/google/flatbuffers/releases) - If you are not interested in building ember in debug you can choose to download both the source and the flatc.exe executable. 
That way you won't have to build flatbuffers, but instead you'll have to move the flatc.exe file into a folder called 'bin' inside flatbuffers. 
If you want to build ember in debug just get the source since you will be building flatc.exe yourself.

- [pcre](https://ftp.pcre.org/pub/pcre/) - __8.39 or newer__ - ___NOT pcre2!___

- [zlib](https://github.com/madler/zlib/releases) - __1.2.8 or newer__

__Refer with CMakeList.txt for up to date toolchain and library requirements__


## STEP 1: GETTING READY

1. Download everything.

2. Unpack the contents of all the library archives into a dedicated folder 
(For this guide we are going to assume ___C:/Libs is used___)

3. Install __CMake__, __Boost__, __Python__, __Visual Studio__ and __WinMPQ__.
Optionally also install __Perl__.

4. Stage your MySQLConnectorC++ in the following way;
    Before | After
    ------ | -----
    _C:/Program Files/MySQL/Connector C++ 8.0/__libxx/vsxx___ | _C:/Program Files/MySQL/Connector C++ 8.0/__lib___
    ___C:/Libs/mysql-connector-c++-8.0.xx-winxxx/libxx/vs14___ | ___C:/Program Files/MySQL/Connector C++ 8.0/lib___
    ___C:/Libs/mysql-connector-c++-8.0.xx-winxxx/libxx__/debug_ | ___C:/Program Files/MySQL/Connector C++ 8.0/lib__/debug_
    ___C:/Libs/mysql-connector-c++-8.0.xx-winxxx/libxx/vs14__/debug_ | ___C:/Program Files/MySQL/Connector C++ 8.0/lib__/debug_

With __boost__ you need to manually add a new system environment variable with the name ___BOOST_ROOT___ pointing to your boost install location.

You can skip this if you installed __Python__ through Visual Studio, 
otherwise Python will ask to do it for you during installation, which if you had it do that, you can skip this as well.
You can also manually add it as an entry to the PATH system environment variable pointing to your python root installation directory.


## STEP 2: COMPILING AND INSTALLING BOTAN

To compile __botan__ you must have __python__ and __visual studio__ installed.

1. Open ___x86/x64 Native Tools Command Prompt for VS___ (just search for it) with administrative rights,
and change directory to your Botan path (hint: ___Libs/botan___)

__OPTIONAL__ - If you want to be able to build ember in debug, you need to build Botan in debug.
If you do not wish to build ember in debug skip to step 5.

2. Enter the following commands;
    ```
    .\configure.py --with-debug-info --debug-mode
    nmake
    botan-test
    nmake install
    ```

You should now have the debug version of botan installed locally (C:/Botan)

3. Stage the files below in the following way;
    From | To
    ---- | --
    _C:/Botan/lib/__bontan.lib___ | _C:/Botan/lib/__botand.lib___
    _C:/Botan/__bin___ | _C:/Botan/__bind___

4. Now open up the Native Tools Command Prompt for VS again at the same location.

5. Enter the following commands;
    ```
    .\configure.py
    nmake
    botan-test
    nmake install
    ```

You should now have the release version of botan installed locally (C:/Botan)


## STEP 3 (__OPTIONAL__): COMPILING FLATBUFFERS
(__Skip this part if you downloaded flatc.exe and aren't building ember in debug__)

1. Launch CMake GUI
    - Press '__Browse Source...__' and select your dedicated __FlatBuffers__ dep folder (default is ___Libs/flatbuffers___)
    - Press '__Browse Build...__' and select your dedicated __build__ folder (default is ___Libs/flatbuffers/bin___)
    - Press '__Configure__' and use default native compilers.
    - Press '__Generate__' and then '__Open Project__' once it's finished generating.

2. Select __Release__ _or_ __Debug__ in Visual Studio and build the solution.

3. Stage the files below in the following way;
    From | To
    ---- | --
    _Libs/flatbuffers/bin/__Release___ or _Libs/flatbuffers/bin/__Debug___ | _Libs/flatbuffers/bin_


## STEP 4: COMPILING ZLIB

1. Launch CMake GUI
    - Press '__Browse Source...__' and select your dedicated __zlib__ dep folder (default is ___Libs/zlib___)
    - Press '__Browse Build...__' and select your dedicated __build__ folder (default is ___Libs/zlib/build___)
    - Press '__Configure__' and use default native compilers.
    - Set the __path__ to ___LIBRARY_OUTPUT_PATH___ to ___Libs/zlib/lib___
    - Press '__Generate__' and then '__Open Project__'

2. Select __Release__ in Visual Studio and build the solution.

3. Stage the files below in the following way;
    From | To
    ---- | --
    _Libs/zlib/zlib.h_ | _Libs/zlib/__include__/zlib.h_
    _Libs/zlib/__build__/zconf.h_ | _Libs/zlib/__include__/zconf.h_
    _Libs/zlib/__build/Release___ | _Libs/zlib/__lib___

__OPTIONAL__ - If you want to be able to build ember in debug, you need to build zlib in debug as well.

4. Select __Debug__ in Visual Studio and build the solution.

5. Stage the files below in the following way;
    From | To
    ---- | --
    _Libs/zlib/__build/Debug___ | _Libs/zlib/__lib___


## STEP 5: COMPILING PCRE

1. Launch CMake GUI
    - Press '__Brose Source...__' and select your dedicated __pcre__ dep folder (default is Libs/pcre)
    - Press '__Brose Build...__' and select your dedicated __build__ folder (default is Libs/pcre/build)
    - Press '__Configure__' and use default native compilers.
    - Enable __UTF__ and __SUPPORT_JIT__ on and leave the rest at default
    - Press '__Configure__' again, then press '__Generate__' and then '__Open Project__'

2. Select __Release__ in Visual Studio and build the solution.

3. Stage the files below in the following way;
    From | To
    ---- | --
    _Libs/pcre/__build__/pcre.h_ | _Libs/pcre/__include__/pcre.h_
    _Libs/pcre/__build/Release___ | _Libs/pcre/__lib___

__OPTIONAL__ - If you want to be able to build ember in debug, you need to build pcre in debug as well.

4. Select __Debug__ in Visual Studio and build the solution.

5. Stage the files below in the following way;
    From | To
    ---- | --
    _Libs/pcre/__build/Debug___ | _Libs/pcre/__lib___


## STEP 6: GENERATING EMBER VISUAL STUDIO SOLUTION

Launch CMake GUI
- Press '__Browse Source...__' and select your dedicated Ember source folder (hint: Ember)
- Press '__Browse Build...__' and select your dedicated build folder (hint: Ember/build)
- Press '__Add entry__' and enter all of the following entries;

    Name | Type | Value
    ---- | ---- | -----
    CMAKE_INSTALL_PREFIX | PATH | C:/Ember/build/bin
    BOTAN_ROOT_DIR | PATH | C:/Botan
    MYSQLCCPP_ROOT_DIR | PATH | C:/Program Files/MySQL/Connector C++ 8.0
    FLATBUFFERS_ROOT_DIR | PATH | C:/Libs/flatbuffers
    ZLIB_ROOT_DIR | PATH | C:/Libs/zlib
    ZLIB_LIBRARY_DEBUG | C:/Libs/zlib/lib/zlibstaticd.lib
    ZLIB_LIBRARY_RELEASE | C:/Libs/zlib/lib/zlibstatic.lib
    PCRE_ROOT_DIR | PATH | C:/Libs/pcre

    OBS! Adjust the paths to fit your setup!

- Press '__Configure__' and use default native compilers.
- Press '__Generate__'


## STEP 7: COPYING OVER DEPENDENCIES AND CONFIG FILES

Stage the files below in the following way; (adjust folders to fit your setup)

### FOR RELEASE:
From | To
---- | --
___C:/Botan/bin__/botan.dll_ | ___Ember/build/bin__/botan.dll_
___C:/Program Files/MySQL/Connector C++ 8.0/lib__/mysqlcppconn9-vs14.dll_ | ___Ember/build/bin__/mysqlcppconn9-vs14.dll_

### FOR DEBUG:
From | To
---- | --
___C:/Botan/bind__/botan.dll_ | ___Ember/build/bin__/botan.dll_
___C:/Program Files/MySQL/Connector C++ 8.0/lib/debug__/mysqlcppconn9-vs14.dll_ | ___Ember/build/bin__/mysqlcppconn9-vs14.dll_

### FOR BOTH:
From | To
---- | --
___C:/Program Files/MySQL/MySQL Server 8.0/lib__/libmysql.dll_ | ___Ember/build/bin__/libmysql.dll_
_Ember/__scripts/run_win.sh___ | _Ember/__build/bin___


## STEP 8: COMPILING EMBER

1. Launch the VS solution from the dedicated ember __build__ folder from the steps above.

2. Choose your solution (__Debug/Release__) and __build__.


## STEP 9: SETTING UP EMBER DB

1. Move __run_win.sh__ from the Ember/scripts folder and place it in the Ember/build/bin folder

2. Run __run_win.sh__ from the Ember/scripts folder in a bash shell (like git bash) and follow the steps

3. Move the config files out of the ___configs/___ folder and Remove the .__dist__ extension from the configuration files so they all end in .__conf__

4. Edit the configuration files so that they match your server and platform configuration.


## STEP 10: CREATING AN ACCOUNT

Run this mysql command from cmd for an account named 'Admin' with the password 'admin';
```
mysql INSERT INTO ember_login.users (username, s, v, creation_date, subscriber, survey_request, pin_method, pin, totp_key)
VALUES ('Admin', 'E2F18CE18E7FEAD7322CB6AD5055B62D5892545452BA2EF2961FEC66B25DD4D1', 
'2D494E0B39AB678B5203C7E0CEB8E0B431D45C4D4B214ADE189F214554B433CC', UTC_TIMESTAMP, b'1', b'0', b'0', b'0', b'0');
```


## STEP 11: RUNNING EMBER

You are now all ready to start up the daemons.

- Start mdns.exe, then gateway.exe, account.exe, character.exe, login.exe, social.exe.