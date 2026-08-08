# RefactorTactics UI Asset Pack — extracted from HUD concept

This pack was created from the supplied HUD screenshot as a **prototype/reference UI kit**.

## Contents
- `panels/`: major HUD panel crops
- `buttons/`: confirm/undo/valid controls
- `tiles/`: universal action and hero-kit tiles
- `warnings/`: warning chips
- `icons/`: isolated luminous glyphs with transparent background + reference crops
- `manifest.json`: pixel bounds, sizes, and suggested 9-slice margins
- `source/HUD_reference.png`: original source reference

## Unreal Engine use
1. Import PNGs under `Content/RefactorTactics/UI/Prototype/`.
2. Set UI textures to the project's UI texture group / compression convention.
3. Use `Draw As: Box` for panel/button assets intended for 9-slice.
4. Start from margins in `manifest.json`, then tune visually.
5. Use isolated `icons/*.png` as Image brushes.
6. Do not ship this pack as final art without rebuilding the frames cleanly.

## Important limitation
The screenshot already contains text, portraits, gradients, glow, and composited panel fills.
Therefore the large panel/button crops are excellent for **prototype composition and visual matching**,
but they are not true source-layer assets. For production, reconstruct:
- empty panel frames,
- scalable 9-slice corners/borders,
- neutral button states,
- hover/selected/disabled states,
- text-free warning chips,
- vector/SDF-style icons where possible.

## Recommended next production pass
Create clean assets from this visual language:
- `RT_UI_Frame_Large_9S`
- `RT_UI_Frame_Small_9S`
- `RT_UI_Button_Primary_9S`
- `RT_UI_Button_Secondary_9S`
- `RT_UI_Button_Warning_9S`
- `RT_UI_Tile_Action_9S`
- `RT_UI_Tile_Ability_9S`
- `RT_UI_PortraitFrame_9S`
- `RT_UI_Separator_H`
- `RT_UI_Timeline_Node`
- `RT_UI_Timeline_Line`
- icon set as transparent monochrome/mask textures
