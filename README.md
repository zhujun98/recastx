# RECAST-X

**REC**onstruction of **A**rbitrary **S**labs in **T**omography - **X**

**Authors**: Jun Zhu <zhujun981661@gmail.com>

---

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
been heavily optimized and it currently reaches more than 2 GB/s on a ordinary GPU 
node. In addition, a modern GUI has been developed to offer better visualization 
of the data and more flexible control of the reconstruction pipeline.

Currently, the software is dedicated for the DAQ interface at the TOMCAT beamline. 
However, there is a plan to make it more flexible in order to work with other DAQ 
interfaces.

*References*

[1] Buurlage, JW., Marone, F., Pelt, D.M. et al. Real-time reconstruction and visualisation towards dynamic feedback control during time-resolved tomography experiments at TOMCAT. Sci Rep 9, 18379 (2019). https://doi.org/10.1038/s41598-019-54647-4
