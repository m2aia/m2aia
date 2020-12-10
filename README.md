M<sup>2</sup>aia - MSI applications for interactive analysis in MITK
=========================

M<sup>2</sup>aia provides modules and plugins for viewing and analysing mass spectrometry images in MITK.
You can download M<sup>2</sup>aia [here](https://github.com/jtfcordes/m2aia/releases).

Manuscript in preparation.

Features
--------

<ul>
  <li> Images: imzML, NRRD, DICOM
  <li> Signal processing: Normalization, baseline correction, smoothing and peak picking
  <li> Image registration: 3D reconstruction and multi-modal image fusion   
  <li> Semi-automatic segmentation
  <li> Image interaction and visualization in 2D and 3D
</ul>



Setup for development
-----------
System requirments: https://docs.mitk.org/nightly/SupportedPlatformsPage.html

```

git clone https://github.com/jtfcordes/m2aia.git

git clone https://github.com/mitk/mitk.git

cd mitk

git checkout snapshots/2020-11-19

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

Projects that use M<sup>2</sup>aia:
-----------------------------------

Enzlein, Thomas, Jonas Cordes, Bogdan Munteanu, Wojciech Michno, Lutgarde Serneels, Bart De Strooper, Jörg Hanrieder, Ivo Wolf, Lucía Chávez-Gutiérrez, und Carsten Hopf. „Computational Analysis of Alzheimer Amyloid Plaque Composition in 2D- and Elastically Reconstructed 3D-MALDI MS Images“. Analytical Chemistry, 14. Oktober 2020, acs.analchem.0c02585. https://doi.org/10.1021/acs.analchem.0c02585.


Contact:
-------
If you have questions about M<sup>2</sup>aia or if you would like to give feedback, write an e-mail at j.cordes@hs-mannheim.de.


Gallery:
--------
![](https://github.com/jtfcordes/m2aia-supporting-materials/raw/master/recon3d.gif) 
<br/>
Manual pre-alignment and 3D reconstruction
<br/><br/><br/>
