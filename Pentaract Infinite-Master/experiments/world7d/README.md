# Experimental 7D Worlds

The first 7D integration uses the working 6D pipeline in an isolated state,
`StateGame7D`, so testing T does not alter the stable 6D implementation.
The model editor remains unchanged (up to 6D).

Coordinates are A/Y, B/X, C/Z, D/W, E/V, F/U, G/T. Chunk storage is
32 x 1 x 1 x 1 x 1 x 1 x 1. All horizontal axes use independent columns.
Terrain and cave noise include T. Generation uses bounded worker queues.
GPU block IDs and column lookup keys are uploaded in aggregate buffers.
Aligned views use a direct rectangular lookup for every axis pair; only freely
rotated slices need the general hashed lookup. Streaming skips unchanged working
sets, and noise permutations are cached per worker without changing terrain.
Up to 32 requested columns share a generation task and contiguous allocation.
Only requested columns are filled. Partial eviction preserves remaining columns,
and sand/circuit eviction cleanup is performed in one pass per streaming update.

## Controls

- TAB + two letters from X/Z/W/V/U/T: align the horizontal view.
- The four hidden directions use Q/E, R/F, T/G, Y/H in axis order.
  In the default XZ view, Y/H moves along T.
- Middle mouse rotates into hidden direction 1; Alt+middle direction 2;
  Ctrl+middle direction 3; Ctrl+Alt+middle direction 4.
- Flight, collision, inventory, sand, anti-sand and placement underfoot use
  the same behavior as the 6D state.

## Persistence and Structures

7D worlds use blocks7d.bin (PWRL7D, version 1, 32-byte edited-block records)
and player7d.json. All seven coordinates, orientation and pinned regions persist.
The world selector defaults to 5D and offers 3D through 7D.

Structure/7D is separate from every other dimension. Structure files have the
PSTR7D signature and dimension 7. They store seven relative coordinates, the
16-bit block ID and a separate 14-face mask. generation.json remains JSON.
Manual capture/load, natural surface placement, foundations and occupied-block
exclusion follow the existing 6D implementation. Existing files are not migrated.

Redstone operates over 14 neighbors, with the existing 250 ms tick. Redirectors
retain a single canonical ID; the T faces are mask bits 12 and 13. No combination
of enabled faces requires a new ID. Sand still moves along A every 500 ms.

## Rendering and Cost

All aligned horizontal pairs use an exposed-face triangle mesh. Tile meshes are
built asynchronously from immutable snapshots, with local invalidation on edits
and column changes. Free rotations keep the raycaster; switching back rebuilds
the aligned mesh progressively. Open doors and alpha-cutout textures preserve
visibility through their empty areas.

The 7D shader reuses NDTF assets. Visible U/T replace distinct hidden texture
coordinates without shearing. There are no new 6D texture assets. Sky remains
black and lighting uses the current 6D intensity. AO/shadows remain unported.

All horizontal axes share a visible radius of 20 at the surface or 12 below A=8,
plus one guard block. Only columns intersecting the slice are rendered. A small
body-centered halo prefetches hidden movement; circuits retain face neighbors.
The body footprint lies in the actual slice, including free rotations, and does
not collide with blocks merely adjacent along a hidden axis. Aligned hyperplanes
have equal column counts; rotated views and different terrain can still cost more.
Existing world edits retain their world-coordinate format and old generation
catalog references are resolved through legacy chunk keys.

## Checks

tests/run-world7d.cmd checks addressing, T-dependent terrain, ray picking,
14-face circuits, metadata, structure binaries, natural placement and camera math.
World7DSmoke.vcxproj checks native creation, rendering in XZ/XT, T movement,
alignment, persistence and T-face metadata in an isolated test runtime.
tests/run-slice-columns.cmd additionally checks equal coverage of all 15
horizontal planes, rotated/corner geometry and off-slice collisions in 6D/7D.
