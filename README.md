M<sup>2</sup>aia - MSI applications for interactive analysis in MITK
=========================

M<sup>2</sup>aia provides modules and plugins for viewing and analysing mass spectrometry images in MITK.
You can download M<sup>2</sup>aia [here](https://github.com/jtfcordes/m2aia/releases).

Manuscript in preparation.


Setup for development (Windows)
-----------
System requirments: https://docs.mitk.org/nightly/SupportedPlatformsPage.html

```

git clone https://github.com/jtfcordes/m2aia.git

git clone https://github.com/mitk/mitk.git

cd mitk

git checkout snapshot/2020-11-19

git apply ../m2aia/Patch/patchMitk.diff

mkdir ../MITK-superbuild

cd ../MITK-superbuild

```

Now run CMake and follow the build Instructions: https://docs.mitk.org/nightly/BuildInstructionsPage.html

Additional changes to the CMake configuration of the MITK-Superbuild:
- MITK_USE_Boost_LIBRARIES add 'filesystem'
- MITK_EXTENSION_DIRS add '<M2aia_Directory_Path>'
- enable MITK_USE_MatchPoint

Once the MITK-superbuild was build successfully:
CMake MITK-build 
- enable MITK_BUILD_APP_M2aia 

Set MitkM2aia as the startup project.

M<sup>2</sup>aia in app preferences
- add path to local installation of elastix and transformix executabels (https://github.com/SuperElastix/elastix/releases)



