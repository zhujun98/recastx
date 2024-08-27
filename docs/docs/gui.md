#### MAIN CONTROL

- *Acquire*: Click to only start acquiring the raw projection images and meta data.
- *Process*: Click to start acquiring and processing data.
- *Stop*: Click to stop acquiring and processing data.

#### SCAN MODE

- *Mode*
    - *Static*: The intention is to use it for static experiments (TBD).
    - *Dynamic*: Reconstruction takes place only after all the projections in a scan have arrived.
    - *Continuous*: Reconstruction takes place continuously at a given interval.
- *Update interval*: Reconstruction in the *Continuous* mode will take place each time after receiving the specified number of projections.

#### CAMERA

- *Fix camera*: Check to disable rotating and zooming the 3D model with mouse.

#### GEOMETRY

- *Column Count*: Number of columns of a projection.
- *Row Count*: Number of rows of a projection.
- *Angle Count*: Number of projections per scan.
- *180 degree* / *360 degree*: Angle range per scan.
- *Slice Size*: Size of the reconstructed slice. Default to *Column Count*.
- *Volume Size*: Size of the reconstructed volume. Default to 128 (for preview).
- *X range*: X range of the reconstruction. 
- *Y range*: Y range of the reconstruction.
- *Z range*: Z range of the reconstruction.

#### PREPROCESSING

- *Downsample*
    - *Col*: Downsampling factor of columns of the raw projection image.
    - *Row*: Downsampling factor of rows of the raw projection image.
- *Minus Log*: Uncheck if the data is already linearized.
- *Offset*: Offset of the rotation center.
- *Ramp filter*: Select the ramp filter applied before FBP reconstruction.

#### PROJECTION

- *Display*: Uncheck to hide the projection.
    - *Projection ID*: The requested projection ID (within a scan). If the ID of the displayed
                       projection is different from the requested one. The displayed one will also
                       be shown.
- *Colormap*: Colormap for displaying the projection.
- *Auto Levels*: Check to enable automatically setting color levels of the displayed objects.
    - *Min*: Minimum colormap value.
    - *Max*: Maximum colormap value.
- *Keep Aspect Ratio*: Uncheck to not keep the aspect ratio of the image. It can
                       be used to improve the visualization if the image has a large
                       aspect ratio.

#### SLICE 1/2/3

- *2D*: Check to display the 2D high-resolution slice.
- *3D*: Check to display the 3D high-resolution slab (TBD).
- *Disable*: Check to hide the slice.
- *Reset all slices*: Click to reset the positions and orientations of all the slices.
- *Colormap*: Colormap for rendering the slices.
- *Auto Levels*: Check to enable automatically setting color levels of the displayed objects.
    - *Min*: Minimum colormap value.
    - *Max*: Maximum colormap value.

#### VOLUME

- *Preview*: Check to use the 3D reconstructed volume as preview.
- *Show*: Check to display the 3D reconstructed volume.
- *Disable*: Check to disable the reconstruction of the 3D volume. As a result, 
             there will be no preview when moving a slice.
  
- *Quality*: Volume rendering quality. 1 is the worst and 5 is the best.
- *Volume Shadow*: Check to enable volumetric lighting effect.
- *Threshold*: Voxels with intensities smaller than the threshold will not be rendered.
- *View front*: Relative position of the front plane of the volume along the view direction.
- *Colormap*: Colormap for rendering the volume (shared with the slices).
- *Auto Levels*: Check to enable automatically setting color levels of the displayed objects (shared with the slices).
    - *Min*: Minimum colormap value (shared with the slices).
    - *Max*: Maximum colormap value (shared with the slices).

#### LIGHTING

- *Show*: Check to display the light source object.
- *Position*: The position of the point light source.
- *Color*: The light color.
- *Ambient*: Ambient component of lighting.
- *Diffuse*: Diffuse component of lighting.
- *Specular*: Specular component of lighting.

#### MOUSE AND KEYBOARD

- Reconstructed slices

    - Select a slice on *mouse hover* (the selected slice will be highlighted).
    - Move the selected slice along its normal direction by *dragging the left mouse button*.
    - Rotate the selected slice around a highlighted axis by *dragging the right mouse button*.

- Camera

    - Rotate the model along an axis which is perpendicular to the moving direction by *dragging
      the middle mouse button* or *dragging the left mouse button while holding the left* <kbd>Alt</kbd>.
    - Zoom in/out of the model by *scrolling the mouse*.
    - <kbd>w</kbd>/<kbd>s</kbd>: Rotate around the x-axis.
    - <kbd>a</kbd>/<kbd>d</kbd>: Rotate around the y-axis.
    - <kbd>space</kbd>: Reset to perspective view.