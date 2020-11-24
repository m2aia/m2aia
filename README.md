M<sup>2</sup>aia - MSI applications for interactive analysis in MITK
=========================

M<sup>2</sup>aia provides modules and plugins for viewing and analysing mass spectrometry images in MITK.
You can download version M<sup>2</sup>aia 2020.11.x [here](https://github.com/jtfcordes/m2aia/releases).

Manuscript in preparation.


Setup
-----------

Configure MITK-Superbuild (https://docs.mitk.org/nightly/DeveloperManualPortal.html)

MITK-Superbuild cmake configuration
- MITK_USE_Boost_LIBRARIES add 'filesystem'
- MITK_EXTENSION_DIRS add '<M2aia_Directory_Path>'
- enable MITK_BUILD_APP_M2aia 

M<sup>2</sup>aia in app preferences
- add path to local installation of elastix and transformix executabels (https://github.com/SuperElastix/elastix/releases)



