# Plant Simulator

A project to simulate plant growth accurately and efficiently. Plant Simulator is designed to take advantage of the GPU as much as possible, drastically speeding up computation. The program is in pure C and has dependencies on Vulkan and GLFW. This project is compatible with most C compilers, including clang, gcc and tcc. To build, 

```bash
$ cd assets/shaders
$ ./compile.sh
$ cd build
$ make
```

Run from the project root directory.

```bash
$ ./build/plant-simulator
```

Part of this project is based upon 
<https://vulkan-tutorial.com>

In addition, it uses the linmath.h library, which can be found at <https://github.com/datenwolf/linmath.h>
