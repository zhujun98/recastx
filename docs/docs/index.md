
## RECASTX

[comment]: # (Any modification to the following content should also be implemented in '../../README.md')

RECASTX (**REC**onstruction of **A**rbitrary **S**labs in **T**omography **X**) 
is a GPU-accelerated software written in modern C++(17).
This project has been developed based on a successful proof-of-principle test [1]
using [RECAST3D](https://github.com/cicwi/RECAST3D.git) in 2019 at the
[TOMCAT](https://www.psi.ch/en/sls/tomcat), [Swiss Light Source](https://www.psi.ch/en/sls).
It aims at providing a near real-time streaming data analysis and visualization
tool to allow monitoring [tomoscopy](https://doi.org/10.1002/adma.202104659)
experiments effectively. It also serves as the foundation of building a smart
data acquisition system which has the potential to reduce the recorded data size
by removing trivial or repetitive data while preserving the important scientific
information which could lead to scientific discoveries.

Besides various new features, the performance of the reconstruction pipeline has
been heavily optimized and it currently reaches **more than 2 GB/s (a few tomograms/s)** 
on an ordinary GPU node, which makes it possible to monitor a fast dynamic process
in near real time. In addition, a modern GUI has been developed to offer 
better visualization of the data and more flexible control of the reconstruction 
pipeline.

<figure markdown>
  <img src="recastx-docs-supplement/overview.jpg" width="98.5%"/>
</figure>

<p float="left">
  <img src="recastx-docs-supplement/beads_volume.gif" width="49%" />
  <img src="recastx-docs-supplement/fuelcell_slices.gif" width="49%" />
</p>

<figure markdown>
  ![Architecture](recastx-docs-supplement/recastx_architecture.jpg)
</figure>

**References**

[1] Buurlage, JW., Marone, F., Pelt, D.M. et al. Real-time reconstruction and visualisation towards dynamic feedback control during time-resolved tomography experiments at TOMCAT. [Sci Rep 9, 18379 (2019)](https://doi.org/10.1038/s41598-019-54647-4)

## Authors

- Jun Zhu <zhujun981661@gmail.com>
