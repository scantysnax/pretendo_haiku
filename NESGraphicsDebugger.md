# NES Graphics Debugger

The NES Graphics Debugger provides live inspection tools for background NameTables, CHR PatternTables, palettes, attribute data, tile selection, scrolling behavior, and decoded CHR bytes.

The debugger is made of three main view types:

- `NameTableView`
- `PatternTableView`
- `CHRExplorerView`

The NameTable and PatternTable windows both include an attached CHR Explorer panel. Hovering or locking a tile in either view updates the CHR Explorer with decoded tile data, palette information, raw CHR bytes, and pixel-level analysis.

---

# NameTable Viewer

The NameTable viewer displays one NES NameTable as a 256x240 bitmap.

Each NameTable window corresponds to one of the four logical NameTables:

| Window | Base Address |
|---|---|
| Name Table 1 | `$2000` |
| Name Table 2 | `$2400` |
| Name Table 3 | `$2800` |
| Name Table 4 | `$2C00` |

The viewer renders the background using:

- NameTable tile bytes
- Attribute table palette selection
- Background pattern table selection from `PPUCTRL`
- NES palette RAM
- The emulator's host palette mapping

## Controls

| Input | Action |
|---|---|
| `G` | Toggle attribute grid |
| `H` | Toggle attribute palette map |
| `B` | Enable both grid and map |
| `N` | Disable grid and map |
| `F` | Toggle attribute block overlay |
| `M` | Toggle matching tile overlay |
| `V` | Toggle PPU viewport overlay |
| `P` | Toggle follow viewport |
| `Space` | Freeze NameTable bitmap updates |
| Mouse hover | Explore tile under cursor |
| Mouse click | Lock/unlock selected tile |
| Shift-click | Lock to screen position |
| `1`-`4` | Select CHR preview palette |
| `0` | Return CHR preview to source palette |

## Hover and Lock Behavior

By default, moving the mouse over the NameTable selects the tile under the cursor and updates the CHR Explorer.

Clicking a tile locks the selection. While locked, moving the mouse does not change the selected tile.

Shift-click enables screen-position locking. In this mode, the debugger tracks the tile currently visible under the locked screen coordinate. This is useful for watching how scrolling changes which NameTable tile appears at a fixed screen position.

## Freeze Behavior

Pressing `Space` freezes NameTable bitmap updates.

When frozen:

- The background bitmap remains unchanged.
- Mouse movement and clicks may still update overlays or selection state.
- The cached NameTable bitmap is not rebuilt until updates are unfrozen.

This is useful for inspecting a specific frame without the background changing underneath the debugger overlays.

## Attribute Overlays

The NameTable viewer includes several attribute-related overlays.

### Attribute Grid

The attribute grid shows both:

- 16x16 pixel quadrant boundaries
- 32x32 pixel attribute-byte boundaries

This helps visualize how the NES attribute table divides a NameTable into palette regions.

### Attribute Map

The attribute map tints tiles according to their selected background palette. This makes it easier to see which tiles are using palette 0, 1, 2, or 3.

### Attribute Blocks

The attribute block overlay highlights attribute-controlled regions and helps identify which palette quadrant a tile belongs to.

### Active Attribute Cell

The active attribute cell overlay highlights the 4x4 tile attribute cell containing the selected tile.

### Active Attribute Quadrant

The active attribute quadrant overlay highlights the 2x2 tile quadrant that supplies the selected tile's palette bits.

## Matching Tile Overlay

The matching tile overlay highlights all tiles in the visible NameTable that use the same tile index as the active tile.

This is useful for finding repeated tiles, repeated background details, and reused graphics.

The overlay only matches tile index. It does not match palette.

---

# PatternTable Viewer

The PatternTable viewer displays one 256-tile CHR pattern table.

Each PatternTable window corresponds to one CHR pattern table:

| Window | Base Address |
|---|---|
| Pattern Table 1 | `$0000` |
| Pattern Table 2 | `$1000` |

The PatternTable viewer can display tiles in either 8x8 or 8x16 layout mode.

## Controls

| Input | Action |
|---|---|
| Window zoom | Toggle 8x8 / 8x16 mode |
| Mouse hover | Explore CHR tile |
| Mouse click | Lock/unlock selected tile |
| `1`-`4` | Select CHR preview palette |
| `0` | Return CHR preview to source palette |

## 8x8 Mode

In 8x8 mode, the pattern table is shown as a normal 16x16 grid:

```text
16 columns x 16 rows = 256 tiles
```

Each cell represents one 8x8 CHR tile.

## 8x16 Mode

In 8x16 mode, tiles are grouped as sprite pairs:

```text
even tile = top half
odd tile  = bottom half
```

The viewer displays these pairs as 8x16 sprite-shaped entries.

When hovering or locking a tile in 8x16 mode, selection is normalized to the top/even tile of the pair.

## Pattern State Panel

The Pattern State panel shows:

| Field | Meaning |
|---|---|
| `PT` | Pattern table index |
| `Base` | CHR base address |
| `Mode` | 8x8 or 8x16 view mode |
| `Tile` | Active tile index |
| `CHR` | Active CHR address |
| `State` | Hover or locked selection state |

## NameTable-to-PatternTable Highlighting

When a tile is selected in a NameTable viewer, the corresponding PatternTable window receives an external highlight.

This allows the user to see where the active NameTable tile lives inside the CHR pattern table.

---

# CHR Explorer

The CHR Explorer displays detailed information about the active tile selected from either a NameTable view or PatternTable view.

It shows:

- Decoded tile preview
- Palette previews
- Raw CHR bytes
- Tile metadata
- NameTable attribute metadata, when available
- Pixel-level bitplane information
- CHR usage analysis

## Tile Preview

The tile preview shows the decoded 2bpp CHR tile using the currently selected palette.

In 8x16 mode, both the top and bottom tiles are shown stacked vertically.

Hovering over the preview updates the pixel-level inspection area.

## Palette Preview

The palette preview shows how the current tile looks with each of the four background palettes.

| Marker | Meaning |
|---|---|
| Red outline | Selected preview palette |
| Black outline | Source palette from NameTable attribute data |
| Red + black outline | Selected palette is also the source palette |

The source palette is only available when the tile came from a NameTable context.

## Palette Controls

| Key | Action |
|---|---|
| `1` | Select palette 0 |
| `2` | Select palette 1 |
| `3` | Select palette 2 |
| `4` | Select palette 3 |
| `0` | Return to source palette |

These controls affect the CHR preview display only. They do not modify PPU palette RAM.

## Tile Info

The Tile panel shows basic information about the selected tile:

| Field | Meaning |
|---|---|
| `PT` | Pattern table index |
| `Tile` | Tile index |
| `State` | Hover or locked |
| `CHR` | CHR address |
| `CHR Top` | Top tile address in 8x16 mode |
| `CHR Bot` | Bottom tile address in 8x16 mode |

When the tile comes from a NameTable, additional fields are shown:

| Field | Meaning |
|---|---|
| `NT` | NameTable index |
| `Tile Addr` | Address of the NameTable byte |
| `Tile Index` | Tile byte read from the NameTable |
| `Attr` | Attribute table address |
| `Attr Byte` | Raw attribute byte |
| `Quadrant` | Attribute quadrant used by the tile |
| `Source Pal` | Palette selected by the attribute bits |
| `Selected` | Currently selected preview palette |

## Raw CHR Bytes

CHR data is displayed as raw hex bytes.

In 8x8 mode:

```text
16 bytes total
8 bytes for plane 0
8 bytes for plane 1
```

In 8x16 mode:

```text
32 bytes total
16 bytes for the top tile
16 bytes for the bottom tile
```

## Pixel / CHR Panel

The Pixel / CHR panel shows information about the selected pixel in the zoomed tile preview.

| Field | Meaning |
|---|---|
| `Pixel` | Pixel coordinate and decoded 2-bit value |
| `Bits` | Plane 0 and plane 1 bit values |
| `Row P0` | Raw bitplane 0 row |
| `Row P1` | Raw bitplane 1 row |
| `PalAddr` | Palette RAM address used for this pixel |
| `NES` | NES palette color index |

The highlighted bit in `Row P0` and `Row P1` corresponds to the selected pixel.

## Attribute Quadrants

When inspecting a NameTable tile, the CHR Explorer shows a 2x2 attribute quadrant diagram.

Each NES attribute byte controls a 4x4 tile region. That region is divided into four 2x2 tile quadrants:

```text
TL  TR
BL  BR
```

Each quadrant selects one of the four background palettes.

## CHR Analysis

The CHR Analysis panel summarizes how the selected tile uses its bitplanes.

| Field | Meaning |
|---|---|
| `P0` | Whether bitplane 0 contains any set bits |
| `P1` | Whether bitplane 1 contains any set bits |
| `Colors` | Decoded color indices used by the tile |
| `Opaque` | Number of nonzero pixels |

For 8x8 tiles, opaque pixels are counted out of 64.

For 8x16 tiles, opaque pixels are counted out of 128.

---

# Data Flow

## NameTable Selection Flow

```text
NameTableView hover/click
        |
        v
Compute active world tile
        |
        v
Read NameTable byte
Read attribute byte
Resolve palette quadrant
Compute CHR address
Read CHR bytes
        |
        v
Update CHRExplorerView
        |
        v
Highlight matching PatternTable tile
```

## PatternTable Selection Flow

```text
PatternTableView hover/click
        |
        v
Compute tile index
Normalize for 8x16 mode if needed
Compute CHR address
Read CHR bytes
        |
        v
Update CHRExplorerView
```

---

# Important Implementation Notes

## Pattern Table Base

Background pattern table selection comes from `PPUCTRL` bit 4:

```cpp
uint32 patternBase = (nes::ppu::ppuctrl() & 0x10) << 8;
```

This resolves to:

| Bit 4 | Background Pattern Table |
|---|---|
| 0 | `$0000` |
| 1 | `$1000` |

## NameTable Bases

The four logical NameTables are:

| Index | Base |
|---|---|
| 0 | `$2000` |
| 1 | `$2400` |
| 2 | `$2800` |
| 3 | `$2C00` |

## Attribute Table Addressing

Each NameTable has an attribute table starting at:

```cpp
nameTableBase + 0x3C0
```

The attribute byte for a tile is:

```cpp
attrAddr = nameTableBase + 0x3C0
    + ((tileY / 4) * 8)
    + (tileX / 4);
```

Each attribute byte controls a 4x4 tile area.

The selected quadrant is:

```cpp
quadrant = (((tileY >> 1) & 1) << 1)
    | ((tileX >> 1) & 1);
```

The palette index is:

```cpp
palette = (attrByte >> (quadrant * 2)) & 0x3;
```

## CHR Tile Decoding

Each 8x8 NES CHR tile is 16 bytes:

```text
bytes 0-7   = bitplane 0
bytes 8-15  = bitplane 1
```

Each pixel is decoded as:

```cpp
pixel = ((plane0 >> (7 - x)) & 1)
    | (((plane1 >> (7 - x)) & 1) << 1);
```

## Palette Lookup

Pixel value 0 uses the universal background color:

```text
$3F00
```

Pixel values 1-3 use:

```cpp
$3F00 + 1 + (palette * 4) + (pixel - 1)
```

Palette mirroring is handled by the mapper/PPU VRAM read logic.

---

# Developer Implementation Notes

## Class Responsibilities

### `NameTableView`

`NameTableView` owns the NameTable bitmap and is responsible for rendering a 256x240 background view from PPU VRAM.

It handles:

- NameTable rendering
- Attribute-table palette selection
- Mouse hover and tile locking
- Screen-position locking
- Matching tile overlays
- Attribute overlays
- PPU viewport overlay
- Forwarding selected tile data to `CHRExplorerView`
- Forwarding selected tile highlights to `PatternTableView`

### `PatternTableView`

`PatternTableView` owns the PatternTable bitmap and renders CHR data as either:

- 8x8 tile grid
- 8x16 sprite-pair grid

It handles:

- PatternTable rendering
- 8x8 / 8x16 mode switching
- Hover and lock selection
- External highlights from `NameTableView`
- Forwarding selected CHR data to `CHRExplorerView`

### `CHRExplorerView`

`CHRExplorerView` does not own PPU data directly. Instead, it receives tile/CHR data from either `NameTableView` or `PatternTableView`.

It handles:

- Decoding CHR bitplanes
- Drawing zoomed tile previews
- Drawing palette previews
- Showing NameTable/attribute context
- Showing raw CHR bytes
- Showing pixel-level bitplane inspection
- Showing CHR usage analysis

## View Communication

The debug views communicate in one direction from selector views to inspector views.

```text
NameTableView      -> CHRExplorerView
PatternTableView   -> CHRExplorerView
NameTableView      -> PatternTableView external highlight
```

`CHRExplorerView` is mostly passive. It displays whatever tile data was most recently supplied by a source view.

## PatternTable Highlighting

When a tile is selected in `NameTableView`, the view computes:

- background pattern table base
- tile index
- pattern table number

It then calls the matching `PatternTableWindow` / `PatternTableView` to show an external highlight.

This is intentionally separate from PatternTable hover/lock selection.

## Cached NameTable Bitmap

`NameTableView` uses a cached bitmap to avoid rebuilding the background image on every mouse or overlay redraw.

The bitmap should be rebuilt only when PPU/frame data changes.

Mouse movement, tile lock changes, and overlay changes should redraw overlays and panels without forcing a background bitmap rebuild.

## Screen-Position Lock

Normal tile lock stores a world tile coordinate.

Screen-position lock stores a view coordinate instead.

When the PPU scroll changes, the tile under that fixed screen position may change. Code that asks for the active tile should go through `ActiveTile()` instead of directly reading `fLockedTileX` and `fLockedTileY`.

This keeps hover, normal lock, and screen lock behavior consistent.

## Palette Selection

The selected CHR preview palette is separate from the source palette.

- Source palette comes from NameTable attribute bits.
- Selected palette is the palette currently being previewed in `CHRExplorerView`.

This is why the UI may show different values for `Source Pal` and `Selected`.

## UI Helper

Panel drawing is shared through `DebugHelpers.h`.

Use:

```cpp
::DrawDebugPanel(this, panel, "Panel Title");
```

The global namespace prefix is intentional when a class also has a member function named `DrawDebugPanel()`.

---

# Recommended Debugging Workflow

1. Open both PatternTable windows.
2. Open the desired NameTable window.
3. Enable the viewport overlay with `V`.
4. Hover tiles in the NameTable to inspect their CHR data.
5. Use `M` to find repeated matching tiles.
6. Use `H` or `G` to inspect attribute palette behavior.
7. Click to lock a tile when you want the CHR Explorer to stay fixed.
8. Use `1`-`4` to preview the tile with different palettes.
9. Use `0` to return to the source palette.
10. Use Shift-click to lock a screen position while scrolling.
11. Use `Space` to freeze NameTable bitmap updates for frame inspection.

---

# Window Summary

## NameTable Window

Best for inspecting:

- Background tile layout
- Attribute table behavior
- Palette quadrant selection
- Viewport scrolling
- Matching/reused background tiles

## PatternTable Window

Best for inspecting:

- Raw CHR layout
- 8x8 tiles
- 8x16 sprite pairs
- Tile reuse from NameTable selections

## CHR Explorer

Best for inspecting:

- Decoded CHR pixels
- Raw CHR bytes
- Palette selection
- Attribute source data
- Pixel-level bitplane behavior
