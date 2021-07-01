M²aia - MSI applications for interactive analysis in MITK
=========================

M²aia provides modules and plugins for viewing and analysing mass spectrometry images in MITK.

[You can download M²aia here](https://github.com/jtfcordes/m2aia/releases).

Cite M²aia
----------

[![DOI](https://zenodo.org/badge/314852965.svg)](https://zenodo.org/badge/latestdoi/314852965)
Please reference to [RRID:SCR_019324](https://scicrunch.org/resolver/RRID:SCR_019324).

Manuscript submitted. M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data

Features
--------

<ul>
  <li> Images: imzML, NRRD, DICOM
  <li> Signal processing: Normalization, baseline correction, smoothing and peak picking 
  <li> Image registration: 3D reconstruction
  <li> Image fusion: Multi-modal MSI (multi polarity, multi matrix)
  <li> Semi-automatic segmentation
  <li> Image interaction and visualization in 2D and 3D
</ul>

Supporting Material
-------------------

Supporting protocol for use-case 1: N-linked glycan m/z candidate detection in "M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data" [dx.doi.org/10.17504/protocols.io.brw2m7ge](http://dx.doi.org/10.17504/protocols.io.brw2m7ge)

Supporting capsule for use-case 1: R-based processing in "M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data"[doi.org/10.24433/CO.2384502.v1](https://doi.org/10.24433/CO.2384502.v1)

Supporting capsule for use-case 1: Command-line application based pre-processing in "M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data" [doi.org/10.24433/CO.7662658.v1](https://doi.org/10.24433/CO.7662658.v1)


Setup for development
-----------

M²aia in app preferences
- add path to local installation of elastix and transformix executabels (https://github.com/SuperElastix/elastix/releases)

Supported Platforms:
- System requirments: https://docs.mitk.org/nightly/SupportedPlatformsPage.html

Projects that use M<sup>2</sup>aia:
-----------------------------------

Enzlein, Thomas, Jonas Cordes, Bogdan Munteanu, Wojciech Michno, Lutgarde Serneels, Bart De Strooper, Jörg Hanrieder, Ivo Wolf, Lucía Chávez-Gutiérrez, und Carsten Hopf. „Computational Analysis of Alzheimer Amyloid Plaque Composition in 2D- and Elastically Reconstructed 3D-MALDI MS Images“. Analytical Chemistry, 14. Oktober 2020, acs.analchem.0c02585. https://doi.org/10.1021/acs.analchem.0c02585.


Build Instructions:
-------------------

Windows requirements: 

* Visual Studio 2017
* >= Qt 5.12.9
* >= CMake 3.19.2
* Git

Unix packages:
build-essential libglu1-mesa-dev libtiff5-dev libwrap0-dev libxcomposite1 libxcursor1 libxi-dev libxkbcommon-x11-0 libxt-dev mesa-common-dev ca-certificates curl ninja-build  libasound2 libnss3-dev libnss3 libnspr4-dev libxtst6 file qtbase5-dev qtscript5-dev libqt5svg5-dev libqt5opengl5-dev libqt5xmlpatterns5-dev qtwebengine5-dev qttools5-dev libqt5x11extras5-dev qtxmlpatterns5-dev-tools libqt5webengine-data libfontconfig1-dev libdbus-1-3 doxygen libopenslide-dev

`Create a new folder` (due to path length restrictions on windows it should be placed very low in the folder hierarchy)

```
C:\M2aiaWorkdir
```

`Open a terminal` and follow the instructions.

```
cd C:\M2aiaWorkdir
mkdir M2aia-superbuild

git clone https://github.com/mitk/mitk
git clone https://github.com/jtfcordes/m2aia

cd mitk
git checkout v2021-02
git apply ../m2aia/Patch/patchMitk-v2021-02.diff
```

`Open CMake`

* Set "Where is the source code" to C:/M2aiaWorkdir/mitk
* Set "Where to build the binaries" to C:/M2aiaWorkdir/MITK-superbuild

`Run "Configure"`

Specify the generator and set it to "Visual Studio 15 2017".
Set the optional platform "x64"

`Click "Finish"`

An error may occur. You may be ask for the Qt directory. Set it e.g. to C:/Qt/5.12.9/msvc2017_64/lib/cmake/Qt5

Now set the additional changes to the CMake configuration.

* MITK_EXTENSION_DIRS add 'C:/M2aiaWorkdir/m2aia'

`Run "Configure"`

* set MITK_BUILD_CONFIGURATION to M2aiaRelease

`Run "Generate"`

`Click "Open Project"`

`Build the BUILD_ALL target` (may take some time)

`Open CMake`

* Set "Where to build the binaries" to C:/M2aiaWorkdir/M2aia-superbuild/MITK-build

* and enable MITK_BUILD_APP_M2aia

Run "Generate"

`Click "Open Project"`

Now you can start development. https://docs.mitk.org/nightly/FirstSteps.html


Contact:
-------
If you have questions about M<sup>2</sup>aia or if you would like to give feedback, write an e-mail at j.cordes@hs-mannheim.de.


Gallery:
--------
![](https://github.com/jtfcordes/m2aia-supporting-materials/raw/master/recon3d.gif) 
<br/>
Manual pre-alignment and 3D reconstruction
<br/><br/><br/>
