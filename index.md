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

|Installers and packages for M²aia||
|---|---|
|**Latest/Nightly** (April 2023)|[[M²aia:latest](http://data.jtfc.de/latest/)]|
|**Releases**|[[M²aia:v2022.08](https://github.com/m2aia/M2aia/releases/tag/v2022.08.00)]|
||[[M²aia:v2021.07](https://github.com/m2aia/M2aia/releases/tag/v2021.07.00)]|
||[[M²aia:Archive](https://github.com/m2aia/M2aia/releases)]|

Using registration utilities requires [elastix 5.0.0](https://github.com/SuperElastix/elastix/releases/tag/5.0.0).

Cite M²aia
----------

Cordes J; Enzlein T; Marsching C; Hinze M; Engelhardt S; Hopf C; Wolf
I (July, 2021): M²aia - Interactive, fast and memory efficient analysis of 2D and 3D multi-modal mass spectrometry imaging
data [https://doi.org/10.1093/gigascience/giab049](https://doi.org/10.1093/gigascience/giab049)

[biotools:m2aia](https://bio.tools/m2aia) 

[RRID:SCR_019324](https://scicrunch.org/resolver/RRID:SCR_019324)

ImzML access using M²aia in Python 
----------------------------------

[pyM²aia](https://github.com/m2aia/pym2aia) is a memory-efficient Python interface for mass spectrometry imaging with focus on Deep Learning utilizing M²aia's I/O libraries.
With this project we provide an open-source and extensible set of functions for ImzML metadata parsing, spectral and ion image processing/access.
Examples of different use cases ([I: meta-data](https://github.com/m2aia/pym2aia-examples/blob/main/Example_I.ipynb); 
[II: signal processing](https://github.com/m2aia/pym2aia-examples/blob/main/Example_II.ipynb); 
[III: ion image generation](https://github.com/m2aia/pym2aia-examples/blob/main/Example_III.ipynb); 
[IV](https://github.com/m2aia/pym2aia-examples/blob/main/Example_IV.ipynb),
[V](https://github.com/m2aia/pym2aia-examples/blob/main/Example_V.ipynb),
[VI](https://github.com/m2aia/pym2aia-examples/blob/main/Example_VI.ipynb): pyM²aia's use in various unsupervised deep learning scenarios) can be found in a separate repository: [pyM²aia-examples](https://github.com/m2aia/pym2aia-examples)

[pym2aia documentation](https://data.jtfc.de/pym2aia/sphinx-build/html/m2aia.html#module-m2aia.ImageIO)

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
