
Setup for development
-----------

Supported Platforms:
- System requirments: ([https://docs.mitk.org/nightly/SupportedPlatformsPage.html](https://docs.mitk.org/nightly/SupportedPlatformsPage.html))


Build Instructions:
-------------------

Windows requirements: 

* Visual Studio 2019
* Qt 5.12.x
* latest CMake
* Git

`Create a new folder` (due to path length restrictions on windows it should be placed very low in the folder hierarchy)

```
C:\M2aiaWorkDir
```

`Open a terminal` and follow the instructions.

```
cd C:\M2aiaWorkDir
mkdir -p build

git clone https://github.com/mitk/mitk
git clone https://github.com/jtfcordes/m2aia

cd mitk
git checkout v2021-10
git apply ../m2aia/Patch/mitk.diff
```

`Open CMake`

* Set "Where is the source code" to C:/M2aiaWorkDir/mitk
* Set "Where to build the binaries" to C:/M2aiaWorkDir/build

`Run "Configure"`

Specify the generator and set it to "Visual Studio 15 2017".
Set the optional platform "x64"

`Click "Finish"`

An error may occur. You may be ask for the Qt directory. Set it e.g. to C:/Qt/5.12.x/msvc2017_64/lib/cmake/Qt5

Now set the additional changes to the CMake configuration.

* MITK_EXTENSION_DIRS add 'C:/M2aiaWorkDir/m2aia'

`Run "Configure"`

* set MITK_BUILD_CONFIGURATION to M2aiaRelease

`Run "Generate"`

`Click "Open Project"`

`Build the BUILD_ALL target` (may take some time)

`Open CMake`

* Set "Where to build the binaries" to C:/M2aiaWorkdir/build/MITK-build

* and enable MITK_BUILD_APP_M2aia

Run "Generate"

`Click "Open Project"`

Now you can start the development. https://docs.mitk.org/nightly/FirstSteps.html

Post-Build Configuration:
-------------------
Download elastix: ([https://github.com/SuperElastix/elastix/releases](https://github.com/SuperElastix/elastix/releases))

Start M²aia, go to in app preferences:
- add path to local installation of elastix and transformix executables
