M²aia (MSI applications for interactive analysis in MITK) is a software tool enabling interactive signal processing and visualisation of [mass spectrometry imaging (MSI)](https://en.wikipedia.org/wiki/Mass_spectrometry_imaging) datasets. M²aia extends the open source [Medical Imaging and Interaction Toolkit (MITK)](https://www.mitk.org) [1,2] and provides powerful methods that the MSI community can adopt, exploit and improve further.

M²aia provides features for

* Read/write access to the open standard imzML
* Image interaction and visualization
* Signal processing: Normalization, baseline correction, smoothing and peak picking
* Image segmentation
* Dimensionality reduction
* Multi-modal 2D registration
* 3D MSI reconstruction

Downloads
---------
[ubuntu2004]: https://github.com/m2aia/m2aia/releases/download/v2024.01/M2aia-v2024.01.1285f33-Ubuntu20.04-linux-x86_64.tar.gz
[ubuntu2204]: https://github.com/m2aia/m2aia/releases/download/v2024.01/M2aia-v2024.01.1285f33-Ubuntu22.04-linux-x86_64.tar.gz
[windows]: https://github.com/m2aia/m2aia/releases/download/v2024.01/M2aia-v2024.01.1285f33-windows-x86_64.exe
[windows-zip]: https://github.com/m2aia/m2aia/releases/download/v2024.01/M2aia-v2024.01.1285f33-windows-x86_64.zip
[pym2aia-github]: https://github.com/m2aia/pym2aia
[pym2aia-pypi]: https://pypi.org/project/m2aia/
[pym2aia-examples]: https://github.com/m2aia/pym2aia-examples
[pym2aia-gettingstarted]: https://m2aia.de/pym2aia.html

|M²aia       |  Packages|
|---         |---      |
| **v2024.01**<br>[Github](https://github.com/m2aia/m2aia/releases/tag/v2024.01)| [ubuntu-20.04][ubuntu2004] <br> [ubuntu-22.04][ubuntu2204] <br> [windows-installer][windows] <br> [windows-zip][windows-zip]| 
| **archive** | http://data.jtfc.de/latest/|

Install elastix for image-based registration utilities: [elastix 5.0.0](https://github.com/SuperElastix/elastix/releases/tag/5.0.0)

|pyM²aia     |  |
|---         |---      |
| [getting started][pym2aia-gettingstarted] | [Github][pym2aia-github] <br> [PyPi][pym2aia-pypi] <br> [Examples][pym2aia-examples]  |


Cite M²aia
----------

Cordes, Jonas, Thomas Enzlein, Christian Marsching, Marven Hinze, Sandy Engelhardt, Carsten Hopf, and Ivo Wolf. “M²aia—Interactive, Fast, and Memory-Efficient Analysis of 2D and 3D Multi-Modal Mass Spectrometry Imaging Data.” GigaScience 10, no. 7 (July 20, 2021): giab049. https://doi.org/10.1093/gigascience/giab049.


Cordes, Jonas, Thomas Enzlein, Carsten Hopf, and Ivo Wolf. “pyM2aia: Python Interface for Mass Spectrometry Imaging with Focus on Deep Learning.” Bioinformatics 40, no. 3 (March 1, 2024): btae133. https://doi.org/10.1093/bioinformatics/btae133.



[biotools:m2aia](https://bio.tools/m2aia) 

[RRID:SCR_019324](https://scicrunch.org/resolver/RRID:SCR_019324)


Multi-modal 3D mouse brain data
------------------------------

Cordes J; Enzlein T; Marsching C; Hinze M; Engelhardt S; Hopf C; Wolf I (2021): Supporting data for "M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data" GigaScience Database. [http://dx.doi.org/10.5524/100909](http://dx.doi.org/10.5524/100909)


Supporting Material
-------------------

Supporting protocol for use-case 1: N-linked glycan m/z candidate detection in "M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data" [dx.doi.org/10.17504/protocols.io.brw2m7ge](http://dx.doi.org/10.17504/protocols.io.brw2m7ge)

Supporting protocol for use-case 1: Dimensionality reduction in "M2aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data" [https://dx.doi.org/10.17504/protocols.io.bwybpfsn](https://dx.doi.org/10.17504/protocols.io.bwybpfsn)

Supporting capsule for use-case 1: R-based processing in "M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data"[doi.org/10.24433/CO.2384502.v1](https://doi.org/10.24433/CO.2384502.v1)

Supporting capsule for use-case 1: Command-line application based pre-processing in "M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging data" [doi.org/10.24433/CO.7662658.v1](https://doi.org/10.24433/CO.7662658.v1)

External references
-------------------

[1] Nolden, M., et al. 2013. “The Medical Imaging Interaction Toolkit: Challenges and Advances.” International Journal of Computer Assisted Radiology and Surgery 8 (4): 607–20. doi:10.1007/s11548-013-0840-8.

[2] Wolf, I., et al. 2005. “The Medical Imaging Interaction Toolkit.” Medical Image Analysis 9 (6): 594–604. doi:10.1016/j.media.2005.04.005.

[3] [https://github.com/SuperElastix/elastix (v5.0.0)](https://github.com/SuperElastix/elastix)
