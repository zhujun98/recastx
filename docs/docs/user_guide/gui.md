
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
- *Slices*
    - *Y-Z*: Toggle the visibility of the slice in the y-z plane.
    - *X-Z*: Toggle the visibility of the slice in the x-z plane.
    - *X-Y*: Toggle the visibility of the slice in the x-y plane.
    - *Reset*: Click to reset the positions and orientations of all the slices.
- *Show volume*: Check to display the low-resolution 3D volume.
    - *Alpha*: Opacity of the low-resolution 3D volume.

## Manipulating 3D model with mouse and keyboard

- *Hold down the left mouse button and move*:
    - When a high-resolution slice is selected (the selected slice will be highlighted), the slice will move
      along its normal direction.
    - Otherwise, the model will rotate along an axis which is perpendicular to the moving direction.
- *Hold down the right mouse button and move*:
    - When a high-resolution slice is selected (the selected slice will be highlighted), the slice will rotate
      around a highlighted axis.
    - Otherwise, the model will rotate along an axis which is perpendicular to the moving direction.
- *Scroll the mouse*: Zoom in/out of the model.
- <kbd>w</kbd>/<kbd>s</kbd>: Rotate around the x-axis.
- <kbd>a</kbd>/<kbd>d</kbd>: Rotate around the y-axis.
- <kbd>space</kbd>: Reset to perspective view.