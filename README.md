# CITS 3003 Project README
## Team Members:
*Tobias Camille - 24214277*
*Etienne Vinton Horn - 24217857*
## Build Details
Project built and tested on Windows 11 with CMake 3.12.
`CMakeLists.txt` was modified to change path formatting due to initial build errors. Library dependencies also needed to be changed to a higher version as building with the older versions caused errors. 
## Features
All required features have been implemented.
Additional features include:
- Skybox rendering
- Normal mapping
- Parallax occlusion mapping
- Post-processing renderer
- A custom scene with new models, textures, and animations

## Asset Sources
The polycam public library was used for the car model:
https://poly.cam/capture/9B4F02B7-C618-422A-9E18-CF51A78DE4D3?


Polyhaven was used for many of the textures and models:
https://polyhaven.com/a/asphalt_02
https://polyhaven.com/a/rebar_reinforced_concrete
https://polyhaven.com/a/propane_tank
https://polyhaven.com/a/plaster_stone_wall_02
https://polyhaven.com/a/plank_flooring_03
https://polyhaven.com/a/concrete_road_barrier
https://polyhaven.com/a/church_bricks_03
https://polyhaven.com/a/utility_box_02

The city skybox was sourced from https://www.jeffrey-martin.com/nyc360skyline-panorama-gigapixel and modified with Gemini to fill in the empty areas.