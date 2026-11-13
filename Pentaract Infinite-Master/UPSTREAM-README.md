# Penteract Placer
> A game made by Mashpoe and Tr1NgleDev  
> For [RAMDER's](https://www.youtube.com/@RAMDER_plus) april fools [video](https://www.youtube.com/watch?v=ITbmXBFPhvE).  
> (codename 5D-Miner lol)  

[![Available on itch.io](https://img.shields.io/badge/Available%20on-itch.io-fa5c5c?style=for-the-badge)](https://mashpoe.itch.io/penteract-placer) 

----

Penteract Placer is 4D Miner, but in 5D.  
Most of the "core"/"initial setup" code was taken from [tetraxis](https://github.com/Tr1NgleDev/tetraxis).  
Rendering is based on a "ray-traced" graphics experiment 4D Miner mod I made a while ago (which is currently private).  

# Texture Format
The 4D block textures are stored in the [NDTF (N-Dimensional Texture Format)](https://github.com/Tr1NgleDev/ndtf) format. (btw the ND part of it is a lie it supports only up to 6D in the current version lol)  
While a viewer app is being developed, you can use these CLIs to convert to and from a standard image format (like .png):
- [ndtf2img](https://github.com/Tr1NgleDev/ndtf2img)
- [img2ndtf](https://github.com/Tr1NgleDev/img2ndtf)

For example:
- `ndtf2img assets/textures/blocks/stone.ndtf assets/textures/blocks/stone.png` to get a .png of the stone texture.
- `img2ndtf assets/textures/blocks/stone.png assets/textures/blocks/stone.ndtf -d 16 -i 16 -z` to convert it back into an .ndtf. (`-d` specifies the third size, `-i` specifies the fourth size, `-z` turns on compression)

# Credits
- Mashpoe - Code, Textures
- Tr1NgleDev - Code, Block Textures (based on 4D Miner), "music/pp.flac"
- Seaspooder - Music

# Links
- [4D Miner by Mashpoe](https://4dminer.com/)
- [4D-Modding by Tr1NgleDev](https://4d-modding.com/) (for 4D Miner)
