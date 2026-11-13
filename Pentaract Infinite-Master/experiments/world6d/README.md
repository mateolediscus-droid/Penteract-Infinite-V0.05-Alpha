# 6D World Prototype

These modules now back the playable `StateGame6D` mode. The creation menu offers
6D with 5D still the default. Existing 3D/4D/5D worlds use the original state.
See `WORLD-DIMENSIONS.md` for controls, save formats and current limitations.

## Implemented

- Six independent integer coordinates: A/Y, B/X, C/Z, D/W, E/V, F/U.
- Uniform column addressing with extents `32 x 1 x 1 x 1 x 1 x 1`.
- `ColumnChunk` stores 32 full 16-bit IDs. Every horizontal coordinate identifies
  an independent column; U has no special loading or allocation policy.
- Twelve neighboring faces, including both signs of U.
- Six-axis voxel ray traversal, negative coordinates, simultaneous crossings
  at corners, solid-block collision and non-generating selection queries.
- Background chunk generation using the existing bounded worker queue;
  publication remains on the caller thread, and edits wait for their own
  pending chunk so that a late result cannot overwrite them.
- Continuous U-dependent terrain and cave fields, sampled through distinct
  OpenSimplex projections. There is no per-U seed change. Natural surface stays
  in the existing height range, deepstone fills underground, caves extend below
  A=20, and underground generation skips surface/biome evaluation.
- A two-block cave cache over A-E at the current integer U coordinate.
- CPU circuit logic over all twelve neighbors, including U boundaries. Inputs
  are read from the previous tick; ticks are 250ms apart. An isolated circuit
  block does not activate the scheduler. After 30 seconds without signal the
  scheduler sleeps until a circuit edit or publication of a circuit chunk.
- Repeater input-face locking and five-second reacquisition cooldown, paired
  vertical doors, and redirectors with a separate 12-bit face mask. Existing
  block IDs and the production 5D block registry are not changed.
- Six-coordinate structure capture/placement and an occupied-voxel exclusion
  index (not bounding-box exclusion). The library path is `Structure/6D`.
- An isolated binary structure codec: `PSTR6D\r\n`, version 1, dimension 6,
  reserved byte, six 32-bit extents, 64-bit block count, 16-bit name length,
  name, then records of six 32-bit relative coordinates, 16-bit block ID and
  16-bit face flags. All integers are little endian. This codec rejects 5D
  structures even if moved into the 6D directory.

## Game Integration

- `StateGame6D.cpp`: controls, collision/selection, worker-based streaming,
  structure UI/pinned regions, circuits, world edits, saves and catalog revisions.
- `assets/shaders/renderer6d.frag`: six-axis traversal, full IDs, twelve faces,
  redirector metadata and structure bounds.
- `Generation6D.h`: immutable catalogs, deterministic grounded placement,
  occupied-voxel exclusion and generated structure face metadata.
- World creation/load menus route only 6D worlds through the new state.

Slice/voxel clipping identifies visible columns, including freely rotated planes.
The body footprint uses the same geometry and has no hidden-axis thickness.
Generation uses bounded worker queues. GPU publication aggregates IDs and a
sparse hashed column index instead of allocating one texture per column.
Equal aligned view distances now require equal column counts on every axis.
Aligned views enumerate rectangles and use direct GPU lookup; rotated slices
keep exact clipping and hashed lookup. Unchanged streaming regions are reused.
Actual frame time can still vary with terrain, structures and rotation.
Aligned slices use an exposed-face triangle mesh with asynchronous 4x4x32
tile rebuilds. Freely rotated slices retain the raycaster. Actual texture alpha
controls neighbor occlusion; open doors emit no faces. See WORLD-DIMENSIONS.md.
Requests now share tasks and allocations in groups of up to 32 columns. Sparse
loading is preserved: no full off-slice hypervolume is generated. The HUD counts
logical columns, while each generation job can service 32 of them.

## Test

Run `tests/run-world6d.cmd` from the repository. Tests cover all six coordinate
axes, positive/negative boundaries, independent U blocks, all twelve hit faces,
corner rays, unloaded selection, asynchronous generation, edit precedence,
U-dependent surface/caves and synchronous/asynchronous parity.

The tests compile into `tests/bin/world6d/World6DTests.exe`. They do not alter
the installed game, user worlds, models or structures.

`tests/run-world6d.cmd` also builds and executes `World6DLogicTests.exe`.
This covers all twelve circuit directions, tick timing, sleeping/waking,
repeater feedback prevention, twelve independent redirector flags, doors across
vertical chunk boundaries powered from U, generated circuit registration,
binary roundtrips, U-separated placement, exact voxel overlap, truncated input,
and refusal to capture missing chunks as air.

`World6DSmoke.vcxproj` exercises the actual native game integration, including
creation/load, GPU column data, structures and circuits. CPU-only tests are
not a substitute for this graphical verification.

`tests/run-slice-columns.cmd` covers the shared 6D/7D column and collision
geometry, including rotated planes and corner crossings.
