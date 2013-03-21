## Quick Start
![screenshot](https://raw.github.com/Fizzadar/opengl-graphing/master/screenshot.png)

This was originally created for an assignment in our "Advanced Graphics" module. To start:

	sh build.sh && ./graph

## Controls+ arrow keys :: rotate the graph/axis around the x/y axis+ wasd :: offset the graph/axis along the x/y axis+ q + e :: offset the graph/axis along the z-axis+ 0 :: display no graphs (axis only)+ 1 :: display both graphs+ 2 :: display 2D graph only (repositions accordingly)+ 3 :: display 3D graph only+ r :: reset everything+ \- :: reduce sample count+ =+ :: increase sample count
## Notes+ Build & tested on OSX, untested on Windows/Linux/etc+ Build in c
+ OpenGL 3.2 core profile
+ Manual perspective, shader based rotations+ Uses glfw for input/window management+ All the y values are scaled (x50), since they otherwise oscillate between -1 & 1  - Could have divided the z/x values also+ Compiled (OSX) using: **gcc graph.c -I/usr/local/include -I/opt/X11/include -L/usr/local/lib -I/opt/X11/lib -w -framework OpenGL -framework Cocoa -framework IOKit -lglfw -o graph**## Features+ No noticeable lag up to 590 samples (after which it ran out of memory, 34,8690 polygons)  - CPU usage also appears low at all times+ ‘Camera’ movement by moving & rotating the entire graph/axis  - Rotations/offsets done in-shader+ Both graphs drawn in 3D space, both can be rotated/moved in the same ways, both can be displayed together or separately