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
                Throughput reaches up to 3 GB/s of 16-bit raw pixel data (a few tomograms/s) on an ordinary GPU node.
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
    <div class="feature-section left">
        <div class="slideshow-container">
            <div class="feature-image-container slide-image slide-a fade">
                <img src="recastx-docs-supplement/h1.gif"/>
            </div>
            <div class="feature-image-container slide-image slide-a fade">
                <img src="recastx-docs-supplement/foam.gif"/>
            </div>
            <a class="prev" onclick="plusDivsA(-1)">❮</a>
            <a class="next" onclick="plusDivsA(1)">❯</a>
        </div>
        <div>
            <p>On-demand slab/slice reconstruction for dynamic experiment</p>
        </div>
    </div>
    <div class="feature-section right">
        <div>
            <p>Rich graphical user interface</p>
        </div>
        <div class="feature-image-container">
            <img src="recastx-docs-supplement/overview.png"/>
        </div>
    </div>
    <div class="feature-section left">
        <div class="slideshow-container">
            <div class="feature-image-container slide-image slide-c fade">
                <img src="recastx-docs-supplement/beads_volume.gif"/>
            </div>
            <div class="feature-image-container slide-image slide-c fade">
                <img src="recastx-docs-supplement/shepp_logan_3d.gif"/>
            </div>
            <div class="feature-image-container slide-image slide-c fade">
                <img src="recastx-docs-supplement/tomophantom_3d08.gif"/>
            </div>
            <a class="prev" onclick="plusDivsC(-1)">❮</a>
            <a class="next" onclick="plusDivsC(1)">❯</a>
        </div>
        <div>
            <p>High-resolution 3D reconstruction for static experiment</p>
        </div>
    </div>
    <div class="feature-section right last">
        <div>
            <p>Scalable architecture</p>
        </div>
        <div class="feature-image-container">
            <img src="recastx-docs-supplement/recastx_architecture.jpg"/>
        </div>
    </div>
</div>

<div class="container" id="future-container">
    <div>
        <h2>And more will come</h2>
    </div>
    <div class="container future-section">
        <div class="col">
            <h3 class="blue">Real-time 3D slab reconstruction</h3>
            <p class="card-text">
                As an enhancement to 2D reconstructed slice, 3D slab will give you localised 3D information in real time.
            </p>
        </div>
        <div class="col middle">
            <h3 class="fucsia">Segmentation</h3>
            <p class="card-text">
                Segmentation is essential to better scene understanding. Machine learning will play a pivotal role here.
            </p>
        </div>
        <div class="col">
            <h3 class="red">Rendering materials</h3>
            <p class="card-text">
                Rendering voxels with real material properties will greatly enhance 3D visualization.
            </p>
        </div>
    </div>
    <hr>
    <div class="container future-section">
        <div class="col">
            <h3 class="orange">Higher throughput</h3>
            <p class="card-text">
                We use heterogeneous computing strategy to achieve the highest possible reconstruction throughput. 
                Both CPU and GPU algorithms will be further optimised.
            </p>
        </div>
        <div class="col middle">
            <h3 class="violet">Interactive 2D/3D image analysis</h3>
            <p class="card-text">
                We understand the importance of user experience to online analysis softwares.
            </p>
        </div>
        <div class="col">
            <h3 class="green">Smart data acquisition</h3>
            <p class="card-text">
                This will generate huge impact on data reduction. Funded by SDSC data science projects 
                for large-scale infrastructures.
            </p>
        </div>
    </div>
</div>

<script>
let slideIndexA = 1;
showDivsA(slideIndexA);

function plusDivsA(n) {
  showDivsA(slideIndexA += n);
}

function showDivsA(n) {
  let i;
  let x = document.getElementsByClassName("slide-a");
  if (n > x.length) { slideIndexA = 1 }
  if (n < 1) { slideIndexA = x.length }
  for (i = 0; i < x.length; i++) {
    x[i].style.display = "none";  
  }
  x[slideIndexA - 1].style.display = "flex";  
}

let slideIndexC = 1;
showDivsC(slideIndexC);

function plusDivsC(n) {
  showDivsC(slideIndexC += n);
}

function showDivsC(n) {
  let i;
  let x = document.getElementsByClassName("slide-c");
  if (n > x.length) { slideIndexC = 1 }
  if (n < 1) { slideIndexC = x.length }
  for (i = 0; i < x.length; i++) {
    x[i].style.display = "none";  
  }
  x[slideIndexC - 1].style.display = "flex";  
}
</script>
