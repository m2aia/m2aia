THIS PAGE IS WORK IN PROGRESS

Setup for development
-----------

Supported platforms: see [https://docs.mitk.org/nightly/SupportedPlatformsPage.html](https://docs.mitk.org/nightly/SupportedPlatformsPage.html)

Requirements: 

* On Windows: Visual Studio (Community Edition) 2019 (2022 usually also works) ([link](https://visualstudio.microsoft.com/de/vs/community))
* Qt 5.12.x (select at least Qt Charts, Qt WebEngine and Qt Script) ([Qt download](https://www.qt.io/download-thank-you))
* latest version of [CMake](https://cmake.org/download)
* [Git](https://git-scm.com/download/)
* [doxygen](https://www.doxygen.nl/download.html)
* [graphviz dot](https://www.graphviz.org/download/)

Build Instructions
-------------------

`Create a new folder for M2aia` 

On Windows: due to path length restrictions, this should be at the top of your folder hierarchy, e.g.
`C:\M2aiaWorkDir`.

`Open a terminal` (on Windows: preferably the `git bash`) and follow the instructions.

```
cd C:\M2aiaWorkDir
mkdir -p build

git clone https://github.com/m2aia/mitk
git clone https://github.com/m2aia/m2aia
```

Make sure that the m2aia-main branch is selected in the m2aia/mitk repository.
```
cd mitk
git status
```

`Open the CMake-GUI`

* Set "Where is the source code" to C:/M2aiaWorkDir/mitk
* Set "Where to build the binaries" to C:/M2aiaWorkDir/build

`Click on "Configure"`

On Windows:
* A dialog box appears
* Select the compiler under `Specify the generator` (depending on which you have installed, e.g. "Visual Studio 16 2019").
* Select "x64" under "Optional platform for generator" (usually this is the default)
* `click "Finish"`

An error may occur that Qt5 has not been found: In this case, you have to set `Qt5_DIR` in CMake (type `Qt5` in the search bar of CMake). Set it to the directory of your Qt installation that contains the file `Qt5Config.cmake`, e.g., on Windows: `C:/Qt/5.12.x/msvc2017_64/lib/cmake/Qt5` (WARNING: Make sure that you use / as path separator ALSO ON WINDOWS!)

Additional changes to the CMake configuration (use the search bar of CMake to find the options):

* MITK_EXTENSION_DIRS: add the path of your m2aia checkout. On Windows, if you use the path name as in the example above: 'C:/M2aiaWorkDir/m2aia' (again the WARNING: Make sure that you use / as path separator ALSO ON WINDOWS!)

`Click on "Configure"`

* set MITK_BUILD_CONFIGURATION to M2aiaRelease

`Click on "Generate"`

`Click on "Open Project"`

On Windows, this will open Visual Studio:
* select Debug or Release in the toolbar
* `Build the ALL_BUILD target` (this will take some time)
In rare occasions (depending on the internet load) the downloads of additional packages (e.g. VTK) may fail. In this case, you can try to build the targets of the failed packages by right-clicking and selecting `Build` in the solution explorer. On success, build the ALL_BUILD target again (build, not rebuild, of course).

`Open CMake again`

* Set "Where to build the binaries" to C:/M2aiaWorkdir/build/MITK-build
* enable MITK_BUILD_APP_M2aiaWorkbench
* optionally disable MITK_BUILD_APP_Workbench (this will remove the target for the standard MITK Workbench)

`Click on "Generate"`

`Click on "Open Project"`

On Windows: In the Visual Studio `Solution Explorer` (usually the right pane), select m2aia/Applications/M2aiaWorkbench. Right-click on M2aiaWorkbench and choose `Set as Startup Projekt`from the popup menu. Now you can run M²aia.

Post-Build Configuration
-------------------
To configure the M2aia registration plugin:
- Start M²aia 
- from the main menuof M²aia choose Window/Preferences - the Preferences dialog will appear.
- on the left pane of the Preferences dialog select M²aia
- add the paths to the local installation of elastix and transformix executables: 
  - These can be found in M2aiaWorkDir/build/ep/bin 
  - or you can install them from the [elastix-repository](https://github.com/SuperElastix/elastix/releases) to a directory of your choice.

Extending M²aia
-------------------
M²aia is based on [MITK](https://www.mitk.org).

[Documentation on first steps of MITK development](https://docs.mitk.org/nightly/FirstSteps.html)
