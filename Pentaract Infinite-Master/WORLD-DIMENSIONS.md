# World Dimensions

7D is now available as an experimental world mode with axis T. The default
remains 5D. See `experiments/world7d/README.md` for controls, separate formats
and performance limits. The 3D rendering path remains available in every world from 3D to 7D.

The world creation screen offers 2D through 7D. Each visit defaults to 5D.
The chosen dimension is stored in the world's `info.json` as `dimensions`.
`renderType` is stored independently in the same file: 2 or 3. Worlds with
2 dimensions always use render type 2; 3D through 7D worlds offer either type.
Existing worlds without the field retain their
previous rendering. Creation still defaults to a 5D world with 3D rendering.
6D uses its own game state and renderer; existing worlds use the original state.

| World | Active axes | Hidden movement |
| --- | --- | --- |
| 2D | A/Y, B/X | None (side view) |
| 3D | A/Y, B/X, C/Z | None |
| 4D | A/Y, B/X, C/Z, D/W | Q/E |
| 5D | A/Y, B/X, C/Z, D/W, E/V | Q/E and R/F |
| 6D | A/Y, B/X, C/Z, D/W, E/V, F/U | Q/E, R/F and T/G |

With 3D rendering, TAB plus two axis letters selects a horizontal hyperplane, but only among
the world's active axes. Extra-dimensional mouse rotations follow the same
restriction. Position arguments in console commands take exactly the active
coordinates, in A, B, C, D, E order.

Inactive axes have one voxel coordinate (zero), no neighboring chunks, no
movement and no circuit neighbors. The player's inactive coordinates remain
at the center of that voxel. Terrain generation and cave noise sampling skip
inactive columns. The existing five-component CPU/GPU buffer layout is kept;
this change does not compact the memory allocated to each chunk.

## 2D Side View

2D worlds use an orthographic A/X view. Each square
uses a fixed slice of its C-positive texture face, with full extended block IDs.
The cursor selects squares within 12 blocks: left click breaks, right click
interacts or places into an empty square (or beside the nearest occupied edge).
Q/D move horizontally; Space jumps or flies upward, Shift flies downward.
The existing flight/noclip toggles still apply. The camera tracks the player;
mouse movement does not rotate the view. Terrain, circuits and structures use A/X.

The player sprite is 1.8 blocks tall, with its feet at the collision bottom.
Its 5x10 proportions give a visual width of 0.9 blocks; collision is unchanged.
Nearest-neighbor filtering preserves the original pixels and transparency.
Horizontal movement alternates Walk1 and directional Walk2 every 160 ms;
stopping returns to the idle frame. Animation follows actual horizontal movement.
The five source images live in `assets/textures/player2d/`; Jump is used in the air.
The 6D/7D sprite follows those engines' 1.75-block vertical body instead.

With render type 2, the visible plane passes through the player and includes A.
TAB plus ONE axis letter (X/Z/W/V/U/T, if present) selects its horizontal axis.
Controls use actual keyboard letters, including AZERTY:

| Movement | Keys |
| --- | --- |
| Visible horizontal axis | Q / D |
| First hidden axis | Z / S |
| Second hidden axis | A / E |
| Third hidden axis | R / F |
| Fourth hidden axis | T / G |
| Fifth hidden axis | Y / H |

After alignment, hidden axes are ordered by their world-axis index, skipping
the visible axis. Missing axes do not move. Middle-button horizontal dragging
rotates the visible axis toward the first hidden axis. Shift selects the second,
Alt the third, Ctrl the fourth, and Ctrl+Shift the fifth (7D). TAB alignment is exact.
Picking and rendering share the same projection, including non-cardinal cuts;
Collision uses only the current slice, without thickness along hidden axes.
The sprite stays screen-facing. The complete orientation is saved (`sideBasis`
in legacy player saves, `cameraFrame` in the 6D/7D player saves).

Streaming traverses the horizontal line of the slice, plus the vertical layers
intersecting the view and collision/active-circuit/structure margins. It does not
allocate a full N-dimensional box. Terrain, physics, redstone and structures
retain the actual world dimension and its separate structure library.

Cardinal X/Z views use exact basis vectors to keep rendering, picking and
collision on the same voxel slice at integer coordinates. In side views,
hidden-axis position cycles through the world's texture faces with smooth
interpolation (one face per block, 25% blend over the main face). The variation
is position-based, not time-based, and preserves the main face's alpha.

## Structures

Each dimension has its own independent library and generation configuration:

- `Structure/2D/*.bin` and `Structure/2D/generation.json`
- `Structure/3D/*.bin` and `Structure/3D/generation.json`
- `Structure/4D/*.bin` and `Structure/4D/generation.json`
- `Structure/5D/*.bin` and `Structure/5D/generation.json`
- `Structure/6D/*.bin` and `Structure/6D/generation.json`

The structure block shows only active axes. Save, file selection, manual Load,
automatic generation and the live configuration reload all use that world's
library. Generation still uses the existing frequency, height, biome, grounding
and occupied-block exclusion rules, evaluated along the active axes only.

Structure binary version 2 stores the dimension in header byte 11 (formerly
reserved). Coordinates and block IDs keep their existing encoding. Readers
reject a dimension mismatch, even if someone moves a binary into another
dimension's directory. World catalog snapshots also retain this dimension.

Older structure binaries and files directly in `Structure/` are not migrated
or automatically imported. Save new structures from a world of the desired
dimension. Existing files are not deleted.

## Verification

See the 6D-specific section below for differences from the original engine.

- `tests/run-chunk-streaming.cmd`: 3D/4D/5D generation, asynchronous jobs,
  inactive-axis exclusion, collisions, circuits, chunk diffs and existing 5D tests.
- `tests/run-structures.cmd`: dimension-tagged capture/binary round trips,
  foreign-library rejection, natural placement and existing structure tests.
- `StructureSmoke.vcxproj`, then `StructureSmoke.exe 3`, `4` or `5` from its
  test runtime directory: OpenGL rendering, dimension selector, structure Save
  and Load, movement, actual world creation and reload.

## 6D Integration

TAB plus two letters from X/Z/W/V/U selects the visible horizontal axes without
moving the player. The three hidden movement pairs follow the remaining axes
in order. The mouse looks within the selected 3D slice. V toggles flight,
Space/Shift move vertically in flight, C toggles noclip, Ctrl sprints, and `!`
opens the block selection inventory. Ctrl+Shift+S saves; Escape opens the menu.

Hold the middle mouse button to rotate the slice into the first hidden direction,
Alt+middle for the second, or Ctrl+middle for the third (initially W/V/U).
Movement and streaming follow the rotated frame; TAB alignment resets it.
The orthonormal horizontal frame is saved with the player. Placing a block
overlapping the player's feet lifts them just above it, unless a ceiling or an
unloaded chunk prevents a collision-free destination.

Supported sand is excluded from the falling queue when chunks arrive. Missing
vertical neighbors park sand until that neighbor loads instead of polling every
500 ms. Support edits wake adjacent sand/anti-sand; actual motion still uses the
500 ms cadence. Terrain and biome generation rules are unchanged.

6D/7D generation runs in the existing worker pools. Storage uses 32-block
vertical columns, one voxel wide on EVERY horizontal axis. X/Z/W/V/U/T share
the same streaming policy and physical visible radius (20 surface, 12 below
A=8, plus one guard block). Visible columns intersect the actual rotated slice.
GPU uploads aggregate full block IDs and a hashed column index in two buffers,
without allocating a texture per column. Chunks outside the visible and pinned
regions are released; edits survive eviction and regeneration.
An independent body-sized collision region with one block of look-ahead is
prefetched and retained even outside the visible slice. Crossing a hidden-axis
chunk boundary therefore cannot repeatedly evict the neighbor needed to move.
These off-screen collision chunks do not require GPU uploads.
Streaming reuses its working set while the visible region and collision halo
are unchanged, and re-sorts requests only when that set changes. Aligned slices
use direct rectangular enumeration and direct GPU lookup on ANY horizontal
axis pair; rotated slices retain polygon clipping and hashed lookup.
Terrain noise permutations are reused per worker/seed. Cave lattice corners
with zero interpolation weight are not evaluated; the generated blocks do not
change.
Generation requests are grouped in batches of at most 32 columns. Each batch
uses one worker task and one shared contiguous column allocation; only requested
columns are generated. Individual columns can still be edited and unloaded.
A partially retained batch keeps its allocation until its last column is released.
Extended-block tracking is an inline 32-bit occupancy mask rather than a hash
table per column. Sand/circuit cleanup runs once per group of evictions.
The HUD now says `Columns`: this is the logical column count, not the job count.
The player's horizontal footprint lies strictly in the current slice; hidden
axes add no body thickness. Hidden-direction motion refreshes the slice
immediately. Circuit blocks retain their immediate horizontal face neighbors.

### Structures

6D structures use the separate `PSTR6D` binary signature, six relative
coordinates, a 16-bit block ID and 12-bit redirector face flags. The structure
block exposes six offsets and sizes. Its selected region remains loaded before,
during and after Save, including when the player leaves. Arrows select a saved
file for manual Load. Foreign-dimensional files are rejected.

Save registers the structure in `Structure/6D/generation.json`. Rules use
`frequency` (0..1), `spacing`, `height` (two integers), `biome` (`default`,
`test`, or both), and `enabled`. Natural placement grounds the lowest occupied
layer only; overlap exclusion compares occupied blocks instead of bounding boxes.
This first 6D natural placement targets the surface, not cave floors.

Saving a structure or choosing **Reload Structures** in the pause menu updates
rules for new chunk coordinates. Immutable catalog revisions are retained in
the world, so returning to old chunks or reloading does not change their
generation. The older binary/version-2 discussion above applies to 3D/4D/5D.

### Saves and Rendering

Aligned 6D/7D slices now render an exposed-face triangle mesh on every horizontal
axis pair. The mesh is partitioned into 32-high by 4-by-4 tiles in slice space.
Immutable snapshots with a one-cell border are meshed on the rendering worker
pool; edits and arriving/evicted columns invalidate their tile and neighbors.
Each frame schedules at most eight tile jobs, with at most sixteen pending.
Unchanged tiles keep their GPU buffers. Neighbor faces are removed only when
the neighbor is opaque: opacity is checked from the actual loaded texture alpha,
including both redirector states. Open doors have no mesh; cutout texels discard
without writing depth. Textures, face orientation, targeting and lighting follow
the existing NDTF renderer. Structure bounds are drawn as depth-tested lines.
Freely rotated slices automatically use the previous raycaster. Returning to an
aligned slice rebuilds its mesh progressively. No save format changes are needed.

`blocks6d.bin` stores only edited voxels with all six signed coordinates, IDs
and face metadata. `player6d.json` stores position, hyperplane, selection,
structure regions and the current catalog revision. Files are written to
temporary paths and replaced after writing.
Generation references live in `generation6d.bin` / `generation7d.bin`, not JSON.
The little-endian format is: 8-byte `PGENHST\n` magic, version u16 (1),
dimension u16, column count u64, legacy count u64, then both groups of records.
Each record contains N signed 32-bit coordinates followed by a u64 revision.
Old JSON histories are converted on world load, after validation. The original
player JSON is kept as `playerNd.before-binary.json.bak`; binary data is replaced
before the small player JSON switches to `historyBinaryVersion: 1`.
Missing or invalid binary history is an error, never an empty-history fallback.
Edited world coordinates and structure formats are unchanged.

The 6D renderer preserves the original NDTF face coordinate order and orientation.
When U is visible, it replaces the highest hidden horizontal texture axis;
otherwise textures are extruded along U. Free rotations use the least visible
horizontal axis for substitution. No shear or blended coordinates are used.
It currently
has a black sky, direct face lighting reduced by 10.7 percent and distance fog;
the older renderer's AO/shadow/water options are not implemented in this state.
The 3D/4D/5D renderer is unchanged.

### 8D Worlds

World creation now accepts 2 through 8 dimensions; 5D remains the default.
8D uses the same column batching, asynchronous visibility planning, cold cache,
exposed-face mesh and rotated-slice raycaster as 6D/7D. Only columns intersecting
the active view and its collision margins are streamed, not an 8D bounding box.

The additional horizontal axis is S (Y, X, Z, W, V, U, T, S). TAB plus two
axis letters selects a 3D view; in 2D rendering TAB plus one letter suffices.
I/K controls the last hidden direction. Existing movement pairs are unchanged.
Middle mouse with Ctrl+Alt+Shift rotates into the last hidden direction in 3D;
in 2D rendering use middle mouse with Ctrl+Alt for that direction.

8D has independent binary worlds and `Structure/8D` templates with JSON generation
rules. All sixteen redstone faces use metadata on the existing block IDs.
Circuits retain the same offscreen propagation and stable-state sleeping behavior.
Hidden S varies internal texture sections within a voxel without rotating face UVs.
The center crosshair is removed from 6D, 7D and 8D; block targeting still works.
The separate Model Maker is unchanged.

`tests/run-world8d.cmd` checks addressing, S terrain/picking, all sixteen circuit
faces, binary structures, generation, plane coverage and off-slice collisions.
`World8DSmoke.vcxproj` exercises native creation, rendering and save/reload.
The `--textures` mode covers the 6D/7D/8D shader mappings and real NDTF pixels.

### 6D Tests

- `tests/run-slice-columns.cmd`: equal coverage across all horizontal planes,
  rotated/corner enumeration, off-slice collision exclusion in 6D/7D, terrain,
  circuits and natural structures with column storage.
- `tests/run-column-perf.cmd`: CPU timings for aligned/rotated enumeration and
  400 underground columns, with a generated-block fingerprint. The native 7D
  smoke test reports stationary update/render timings and checks XZ/XT/UT plus
  a rotated view. These small tests do not predict FPS in every user world.
- `tests/run-generation-history.cmd`: 6D/7D binary round trips, signed limits,
  legacy entries, bad revisions/dimensions and truncated input. Native 7D smoke
  additionally checks automatic JSON migration, backup and world reload.
- `tests/run-column-batches.cmd`: task grouping, contiguous allocation, edits
  during generation, partial eviction, cancellation and extended-block masks.
- `tests/run-slice-mesh.cmd`: exposed/internal faces, texture cutouts, open doors,
  tile boundary neighbors and UT face indices. The native 7D smoke test checks
  mesh updates after edits across tile boundaries and rotated-slice fallback.

- `tests/run-world6d.cmd`: addressing, terrain/caves, background jobs,
  twelve-face circuits, binary structures and natural grounded placement.
  Offscreen tests cover propagation/removal on every face, stable-powered sleep,
  repeaters, redirector metadata, paired doors and saved structure circuits.

6D/7D keep sparse circuit records independently of terrain residency. Placed,
saved and discovered structure circuits continue propagating changes after their
chunks unload. Results are saved as block edits without generating neighboring
terrain. After 30 seconds without changes or repeater cooldowns, simulation sleeps
even if the stable circuit is powered; a circuit edit wakes it again. This does
not discover structures in terrain that has never been generated.
The HUD counts actual rendered frames using wall-clock time, independent of the
100 Hz simulation. It shows average FPS and the longest frame in each 0.5-second
sampling window. Visibility enumeration runs on a worker with only one plan in
flight. Streaming runs once per rendered frame, not once per simulation step.
6D/7D retain at most 8192 detached cold columns per world in a FIFO cache; edits
made while unloaded are reapplied when a column returns. Submitted generation
jobs remain counted until completion, even after leaving the view, to prevent
unbounded background queues during rotation. Column dimensions remain unchanged.

Streaming now consumes column-publication events instead of scanning all resident
columns on every publication. Render updates consume changed-column events, also
covering cached columns and redstone edits. Visibility workers prepare the vertical
column set and distance-sorted request order; the main thread advances a bounded
request cursor. Collision requests remain prioritized independently of that cursor.
`World7DSmoke.exe --profile` creates an isolated, fixed-seed cave world and reports
frame, streaming and upload timings while moving along visible X and hidden T.

6D/7D textures recycle the four tangent coordinates of existing 5D NDTF assets.
When hidden, U traverses both internal section coordinates in the same direction;
T traverses them in opposite directions. Horizontal and vertical face coordinates
are never rotated or shifted, preserving directional grass and wood faces.
The mapping repeats per block, including negative coordinates, and uses one
texture sample. Visible extra axes retain their existing planar substitution.
`World7DSmoke.exe --textures` checks the actual GPU mapping in all four shaders:
hidden U/T variation, periodicity, fixed face orientation and mesh/raycast agreement.
It also samples actual stone, grass-side, wood-side and sand pixels at fractional
positions 0.1 through 0.9 within the same voxel, rather than only checking coordinates.
- `World6DSmoke.vcxproj`: native hidden-window creation/load, GPU/CPU chunk
  parity, textured rendering, structure Save/Load, U circuits, TAB XU,
  movement, resizing and world save/reload in an isolated runtime directory.
