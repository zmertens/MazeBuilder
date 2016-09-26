![](resources/demo.gif)

# About
Shoot at sprites. Don't get shot it slows you down. Get powerups. Find the exit as fast as you can. <br />

## Build
 - CMake 3.0+
 - C++11 compiler
 - GLM 9.7.0+
 - SDL2
 - SDL2 Mixer (audio)
 - GPU capable of OpenGL 4.3+

If all dependencies are installed and available in the system path, then create a build directory and change into it and run `cmake .. && make -j8` <br />
On Ubuntu based systems, you can get the dependencies like such `apt-get install cmake g++ libglm-dev libsdl2-dev libsdl2-mixer-dev libglu1-mesa-dev` <br />

### References
Draws inspiration from [BennyBox's Wolfenstein 3D clone](https://github.com/BennyQBD/Wolfenstein3DClone). <br />

