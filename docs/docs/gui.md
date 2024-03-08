## Control panel

- *Acquire*: Click to only start acquiring the raw projection images and meta data.
- *Process*: Click to start acquiring and processing data.
- *Stop*: Click to stop acquiring and processing data.


- *Mode*
    - *Continuous*: Reconstruction takes place continuously at a given interval.
    - *Discrete*: Reconstruction takes place only after a 180-degree-scan finished.
- *Update interval*: Reconstruction in the *Continuous* mode will take place each time after receiving the specified number of projections.


- *Fix camera*: Check to disable rotating and zooming the 3D model with mouse.


- *Downsampling*
    - *Col*: Downsampling factor of columns of the raw projection image.
    - *Row*: Downsampling factor of rows of the raw projection image.
- *Ramp filter*: Select the ramp filter applied before FBP reconstruction.


- *Colormap*: Select the displayed colormap.
- *Auto Levels*: Check to enable automatically setting color levels of the displayed objects.
    - *Min*: Minimum colormap value.
    - *Max*: Maximum colormap value.
- *Slice 1/2/3*
    - *2D*: Check to display the 2D high-resolution slice.
    - *3D*: Check to display the 3D high-resolution slab (TBD).
    - *Disable*: Check to hide the slice.
- *Reset all slices*: Click to reset the positions and orientations of all the slices.
- *Show slice histograms*: Check to display the pixel value histogram for each slice.
- *Volume*
    - *Preview*: Check to use the 3D reconstructed volume as preview.
    - *Show*: Check to display the 3D reconstructed volume.
    - *Disable*: Check to disable the reconstruction of the 3D volume. As a result, 
                 there will be no preview when moving a slice.
    - *Alpha*: Opacity of the low-resolution 3D volume.
    - *Front*: Relative position of the front plane of the volume along the view direction.

## Manipulating 3D model with mouse and keyboard

### Reconstructed slices

- Select a slice on *mouse hover* (the selected slice will be highlighted).
- Move the selected slice along its normal direction by *dragging the left mouse button*.
- Rotate the selected slice around a highlighted axis by *dragging the right mouse button*.

### Camera

- Rotate the model along an axis which is perpendicular to the moving direction by *dragging
  the middle mouse button* or *dragging the left mouse button while holding the left* <kbd>Alt</kbd>.
- Zoom in/out of the model by *scrolling the mouse*.
- <kbd>w</kbd>/<kbd>s</kbd>: Rotate around the x-axis.
- <kbd>a</kbd>/<kbd>d</kbd>: Rotate around the y-axis.
- <kbd>space</kbd>: Reset to perspective view.
- <kbd>Page Up</kbd>: Move the front plane of the volume forward.
- <kbd>Page Down</kbd>: Move the front plane of the volume backward.