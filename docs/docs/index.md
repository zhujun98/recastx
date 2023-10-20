---
hide:
    - navigation
    - toc
---

# RECASTX

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


**References**

[1] Buurlage, JW., Marone, F., Pelt, D.M. et al. Real-time reconstruction and visualisation towards dynamic feedback control during time-resolved tomography experiments at TOMCAT. [Sci Rep 9, 18379 (2019)](https://doi.org/10.1038/s41598-019-54647-4)

<link rel="stylesheet" href="stylesheets/homepage.css" />

<div class="container" id="button-container">
    <a href="installation" class="button" id="get-started">Get Started</a>
    <a href="tutorial/plastic_beads" class="button">Follow Tutorials</a>
    <a href="faq" class="button">Learn More</a>
</div>

<div class="container" id="highlight-container">
    <div class="card">
        <div class="content">
            <h3 class="card-title">Low-latency</h3>
            <p class="card-text">
                Slab can be defined and reconstructed on-the-fly almost instantly without reconstructing the whole 3D volume.
            </p>
        </div>
    </div>
    <div class="card">
        <div class="content">
            <h3 class="card-title">High-throughput</h3>
            <p class="card-text">
                Pipeline throughput reaches more than 2 GB/s (a few tomograms/s) on an ordinary GPU node.
            </p>
        </div>
    </div>
    <div class="card">
        <div class="content">
            <h3 class="card-title">Flexible</h3>
            <p class="card-text">
                Different scan modes as well as configurations of slabs/slices and data processing pipeline are provided.
            </p>
        </div>
    </div>
</div>

<div class="container" id="feature-container">
    <div class="section feature left">
        <img class="feature-image" src="recastx-docs-supplement/fuelcell_slices.gif"/>
        <p>On-demand slab/slice reconstruction for dynamic experiment</p>
    </div>
    <div class="section feature right">
        <p>Rich graphical user interface</p>
        <img class="feature-image" src="recastx-docs-supplement/overview.jpg"/>
    </div>
    <div class="section feature left">
        <img class="feature-image" src="recastx-docs-supplement/beads_volume.gif"/>
        <p>High-resolution 3D reconstruction for static experiment</p>
    </div>
    <div class="section feature right">
        <p>Scalable architecture</p>
        <img class="feature-image" src="recastx-docs-supplement/recastx_architecture.jpg"/>
    </div>
</div>
