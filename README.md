# RECASTX

[![documentation](https://github.com/zhujun98/recastx/actions/workflows/docs.yml/badge.svg)](https://github.com/zhujun98/recastx/actions/workflows/docs.yml)

---

[comment]: # (Any modification to the following content should also be implemented in 'docs/docs/index.md')

RECASTX is a GPU-accelerated software written in modern C++(17). 
This project has been developed based on a successful proof-of-principle test [1] 
using [RECAST3D](https://github.com/cicwi/RECAST3D.git) in 2019 at the 
[TOMCAT](https://www.psi.ch/en/sls/tomcat), [Swiss Light Source](https://www.psi.ch/en/sls). 
It aims at providing a near real-time streaming data processing and visualization 
tool to allow guide [tomoscopy](https://doi.org/10.1002/adma.202104659) 
experiments effectively. It also serves as the foundation of building a smart 
data acquisition system which has the potential to reduce the recorded data size 
by removing trivial or repetitive data while preserving the important scientific 
information which could lead to scientific discoveries.

Besides various new features, the performance of the reconstruction pipeline has 
been heavily optimized and it currently reaches more than 3 GB/s (raw data) on 
our testing GPU node. In addition, a modern GUI has been developed to offer better 
visualization of the 3D data and more flexible control of the reconstruction pipeline.

**References**

[1] Buurlage, JW., Marone, F., Pelt, D.M. et al. Real-time reconstruction and visualisation towards dynamic feedback control during time-resolved tomography experiments at TOMCAT. [Sci Rep 9, 18379 (2019)](https://doi.org/10.1038/s41598-019-54647-4)

Learn more about RECASTX, please check the [documentation](https://zhujun98.github.io/recastx/).