#!/bin/sh
glslangValidator -o vertexdisplay.vert.spv    -V vertexdisplay.vert 
glslangValidator -o vertexdisplay.frag.spv    -V vertexdisplay.frag 
glslangValidator -o nodeupdate.comp.spv       -V nodeupdate.comp
glslangValidator -o nodetopology.comp.spv     -V nodetopology.comp
glslangValidator -o vertexgeneration.comp.spv -V vertexgeneration.comp

