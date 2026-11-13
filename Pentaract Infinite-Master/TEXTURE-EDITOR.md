# Texture Editor (prototype)

In the common ND engine, authored profile coordinates follow the visible plane:
A remains vertical, X/Z follow its two horizontal axes, and W/V use two other
axes from the selected control group (with distinct fallback axes if needed).
Face selection, painted pixels, regions, grouping and hidden slices use this
same mapping in both ray and mesh rendering. Saved profiles are not rewritten.

Main menu > Texture Editor. Profiles apply to blocks in 2D and 3D rendering,
in worlds from 2D to 15D. Existing block icons are unchanged.

## Controls

- `<` / `>` selects the block. Unsaved drafts stay in memory while switching blocks.
- Each face has its own palette of 1-16 colors. RGB is 0-255; Proportion is a
  percentage from 0 to 100 (two decimal places). Editing a percentage redistributes
  the remaining percentage across other colors, keeping a total of 100%.
  Apply commits values to selected faces. Swatch arrows browse colors in groups of eight.
- + Color / - Color add or remove a color on selected faces. Removing a color clears
  its painted pixels and redistributes its probability. One color must remain.
- Face checkboxes define the scope of weight edits. All, Sides and Top select groups.
  RGB colors, seed and V/interpolation settings also belong to individual faces.
  Top selects only Y+: edits cannot recolor the sides or underside.
- W -/+ selects one of 16 slices. All W applies weight edits and painting to all slices.
  Copy W copies the current slice's weights to all slices of selected faces.
- The editor starts at W=V=0.500 (also when changing block). W/V numeric fields
  accept coordinates from 0 to 1; Go applies them and 0.5 returns to the center.
  +/- moves by 1/16. Slice 8 is sampled at exactly 0.5, not 0.53125.
- New profiles use the center as their base distribution. Editing another W slice
  makes it an explicit anchor; untouched slices interpolate between authored anchors
  and the center, and extend the nearest anchor outside that range. Editing the
  center preserves explicit variations. Copy W makes a uniform base again.
  Existing profiles preserve all their authored values (all slices remain anchors).
  The corrected W sampling moves their anchor locations by half a former slice;
  saved proportions, painted pixels and colors are not reset.
- + Zone creates a rectangle on the selected faces, initially inheriting their
  proportions and XYZ preferences. Up to eight zones per face are available.
  The zone arrows switch between Whole face and existing zones. - Zone removes
  the selected zone on checked faces only. The highest numbered overlapping zone wins.
- U/V min/max delimit the rectangle on the face's 16x16 grid (0 <= min < max <= 16).
  Apply or Set commits bounds and color settings. A yellow outline marks the
  selected zone. U/V here are face texture coordinates, not extra world axes.
  The same rectangle can be applied to several checked faces.
- Zone proportions are independent per W slice. XYZ preferences are per color,
  per face or zone: 50 is neutral, above 50 favors the positive half-axis, below
  50 favors the negative half. Y+ favors height. Preference transitions smoothly
  around the middle; 0/100 retain a small nonzero chance in the opposite half.
  Axes perpendicular to the face have no spatial variation and are ignored.
  Preferences multiply and locally renormalize the base probabilities; they are
  not strict quotas of actual pixels. Zero base probability remains zero.
  RGB colors, seed and V/interpolation settings remain shared across a face's zones.
- Groupement % controls the fraction assigned to authored groups for the selected
  color and face/zone. Increasing it without two models opens Highlight mode.
  Pixels / group is the desired pixel count (1-256). Draw a connected silhouette
  directly over the cube's pixels, then Add. Repeat at least twice. Each sample must match the
  target within one pixel, and sample counts must differ by at most one pixel.
  New clears the draft; arrows review models; Del removes the selected model.
  Erase removes highlighted cells. Highlight is only a yellow overlay on the
  existing 3D preview: same camera, geometry, texture and brush hit testing.
  Selecting pixels never writes paint pins or changes W/V. Each selection is
  limited to its starting face and W slice; New starts another selection.
  Samples retain the underlying palette colors, including forced paint and
  procedural pixels. Toggling Highlight with no selection changes no cube pixels.
  Save persists all models, including an unfinished set with only one sample.
  Models belong to the clicked face and current zone; their source position is
  irrelevant after capture. Their silhouettes and captured color arrangements
  are reused without random distortion.
  Placement is deterministic, respects zone boundaries, and avoids clipping or
  merging same-color groups. A background color must retain a nonzero ungrouped
  proportion to separate groups; a fully grouped palette falls back to grain.
  Percentages are placement targets, not strict quotas when shapes cannot fit.
  Partial grouping also retains scattered pixels. Zero keeps independent grain.
  Four layouts per face/W slice are baked on profile changes, not per frame.
  Hidden coordinates choose among these layouts. Grouped layouts use discrete W
  slices; smooth W interpolation still applies to the ordinary grain path.
  Forced painting wins, including where V shifts to a different grouped layout.
  Old profiles without drawn models keep their colors and settings but use grain
  until at least two group models are authored.
- Transparency turns the selected palette entry into a fully transparent pixel.
  It is a per-color, per-face property like RGB, shared by that face's zones.
  Its percentage, regions, grouping and XYZ preferences determine where holes
  are generated; painting that color forces a transparent pixel. Transparent
  swatches display a checkerboard. Erase restores the procedural result instead.
  This is binary cutout transparency, not partial opacity. It affects rendering
  only: collisions remain unchanged. The 6D-15D mesh retains neighboring faces
  behind blocks whose edited palettes contain transparent colors.
- Smooth W interpolates weights. V influence (0-100) shifts the W distribution
  with the V coordinate, giving a deliberately simple fifth-dimensional extension.
- Left drag paints the clicked face with the selected palette color at probability 100%.
  Fast mouse movements are interpolated to avoid holes between input events;
  GPU profile updates are batched once per mouse event. Releasing over UI controls
  also ends the stroke.
  The face checkboxes also restrict the brush. Erase restores procedural generation.
  A selected zone restricts the brush to its rectangle. Forced pixels override
  both region probabilities and XYZ preferences.
  Pins use a fixed 16x16 grid, per face and W slice, and take priority over V variation.
  With All W unchecked, paint belongs to the W section where it was drawn. In game,
  hyperplane alignment centers every active horizontal coordinate at floor(value)+0.5,
  without changing Y; this returns hidden axes to the central section. Use All W
  when a painted pattern must also appear in the other W sections.
- Right drag orbits; wheel zooms. XZ cycles the six preview planes containing Y.
  V -/+ changes the hidden V slice (unless V is visible in the selected plane).
- Undo swaps the last edit or paint stroke. Reset restores selected faces only.
- Save writes and applies the current block profile. Reload reads its saved version.
  Preview edits are applied live in memory; only Save persists them between sessions.

## Storage And Rendering

Profiles: `TextureProfiles/<canonical-block-id>.txp`, beside the executable, independent
of the working directory and world saves. Binary little-endian uint32 format:
magic `TXP1`, version 8, word count, then four reserved/header words, 10x16 RGB colors,
10 active color counts, 10x3 face settings (seed, V, interpolation), 10x16x16 weights
(hundredths of a percent), 10x16x256 pins, 10x16x3 XYZ preferences and 10x8 region
records. Each region stores enabled/min/max, 16x16 weights and 16x3 preferences.
Version 4 appends 10x9x16x2 grouping settings (whole face plus eight regions;
strength and cell size per color). Versions 1-3 are upgraded on read with missing
features disabled: neutral biases, no new regions, grouping zero and size four.
Version 5 appends 10x16 binary transparency flags. Earlier profiles remain opaque.
Version 6 appends 10x9 slice-override masks. Earlier profiles treat all slices as
explicitly authored. Interpolated slice weights are stored for direct GPU access.
Version 7 appends a model count and variable model records: face, color, region
(zero means whole face), and eight uint32 mask words for a normalized 16x16 shape.
The former cell-size field now holds the target pixel count. Existing versions
remain readable. Generated layouts are GPU-only caches, not saved in the file.
Version 8 adds 256 palette-index bytes to each model (zero retains the old
single-color behavior). Captured colors move with their pixels during normalization.
Zero pin means procedural; 1-16 selects a palette color. Save uses a temporary file
and atomic replacement. Invalid profiles are logged and skipped.

The upper-left state button cycles lever OFF/ON, wire/repeater levels 0-15, crystal door
Closed/Open, or redirector Face OFF/ON. Face checkboxes still select directions
within the currently edited profile. Save, Reload and Undo target only that
state. Each logical state uses its own profile. An uncustomized state keeps its
own procedural material instead of borrowing the OFF/zero-state texture. Open
doors default to transparent and stay non-solid even with an authored texture.
Lever OFF and ON have independent files; existing authored files are unchanged.
ND worlds show the targeted lever state or wire power in the coordinate HUD.
Redirector profiles are chosen per face, including faces beyond the first mask
word. No additional inventory blocks or save-world IDs are needed.
The editor authors the ten faces of a 5D block. In higher-dimensional worlds,
additional face pairs reuse V-/V+; their grain still hashes every active axis
independently. Grain is deterministic and world-anchored, not regenerated each frame.
No changes to collision, generation, simulation or world serialization.

GPU profiles use SSBO binding 6 and are uploaded only when edited. Both preview
and world shaders use the generated `profileGrain` function. Run
`node tools/texture-profile-shaders.mjs` after changes to its generator.

## Verification

`TextureEditorSmoke.vcxproj`: binary roundtrip, compilation of all 21 world shaders,
W probability selection, forced pixels, native preview, painting, orbit and compact
viewport. Run the resulting executable again with `--reload` to check loading in
a separate process. Test profiles live beside the test executable, not the game.
