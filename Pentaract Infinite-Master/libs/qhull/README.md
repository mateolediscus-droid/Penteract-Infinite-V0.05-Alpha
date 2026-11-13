# Qhull 2020.2 (8.0.2)

Unmodified reentrant C sources from the official 2020.2 release:
https://github.com/qhull/qhull/tree/2020.2/src/libqhull_r

License: COPYING.txt. Compiled statically into the game, no extra runtime DLL.
Model5DHull.cpp uses Qt triangulation and Qx facet merging, without QJ jitter.
Only the model editor uses this library; world generation is unchanged.

The vendor C files are compiled without MSVC /sdl (the rest of the game keeps
it). Qhull's unused command-line trace parser triggers C4703 under /sdl;
the editor only passes the fixed `qhull Qt Qx` options, never user flags.
