<a href="https://m2aia.de/">
  <img src="https://github.com/jtfcordes/M2aia/raw/master/Applications/M2aiaWorkbench/icons/icon.png" alt="M2aia logo" title="M2aia" align="right" height="80" />
</a>

## Welcome to *[M²aia][m2aia]*: a desktop application for mass spectrometry imaging.

M²aia is open source software, based on the well-known [Medical Imaging Interaction Toolkit (MITK)](https://mitk.org/). The software enables interactive visualization, signal-processing, 3D reconstruction and multi-modal image registration of mass spectrometry imaging (MSI) datasets.


## More information ##

**[github.io/m2aia][m2aia]**<br>
[Build instructions (windows/Linux)][m2aia-build] <br>
[Downloads][download]<br>

[Protocols][m2aia]<br>

## Cite M²aia ##

Cordes J; Enzlein T; Marsching C; Hinze M; Engelhardt S; Hopf C; Wolf I (July, 2021): M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data https://doi.org/10.1093/gigascience/giab049

[biotools:m2aia][biotools-m2aia]

## Docker ##

To use M²aia in a browser run:

```docker pull ghcr.io/m2aia/m2aia:latest```<br>
```docker run --rm -v /local/data/path:/data -p 8888:80 ghcr.io/m2aia/m2aia:latest```

with password:

```docker run --rm -v /local/data/path:/data -e VNC_PASSWORD=<password> -p 8888:80 m2aia/m2aia:latest```

Start M²aia by accessing http://localhost:8888

The container will shut down if M²aia is closed (File->Exit).
The results written to the mapped volume have root user ownership


## Python ##

We provide ImzML Reader and the Signal Processing utilities for Python.

[https://github.com/m2aia/pym2aia](https://github.com/m2aia/pym2aia)<br>
[https://github.com/m2aia/pym2aia-examples](https://github.com/m2aia/pym2aia-examples)


## Contact ##
If you have questions about the application or if you would like to give us feedback, don't hesitate to open a [new issue][contribute] or contact me [directly](mailto:j.cordes@hs-mannheim.de).


[![DOI](https://zenodo.org/badge/314852965.svg)](https://zenodo.org/badge/latestdoi/314852965)

[logo]: https://github.com/jtfcordes/M2aia/raw/master/Applications/M2aiaWorkbench/icons/icon.png
[mitk]: http://mitk.org
[m2aia-build]: https://m2aia.de/development.html
[m2aia]: http://m2aia.de
[itk]: https://itk.org
[vtk]: https://vtk.org
[mitk-usermanual]: http://docs.mitk.org/nightly/UserManualPortal.html
[biotools-m2aia]: https://bio.tools/m2aia
[scicrunch-m2aia]: https://scicrunch.org/resolver/RRID:SCR_019324

[download]: https://m2aia.de
[contribute]: https://github.com/jtfcordes/M2aia/issues
[cmake]: https://www.cmake.org


