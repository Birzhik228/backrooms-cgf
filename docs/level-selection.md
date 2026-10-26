# Level selector — 26 October 2026

Press **Escape** and select **Choose a Level** in the pause menu.
The selector offers **1 / Yellow Rooms**, **2 / Level Fun**, and **3 / Parking**.
Use Up/Down and Enter or click a row. Escape or Back returns to the pause menu
without resuming play; another Escape resumes a living run. **Ctrl only
crouches** and never opens or closes a menu.

The world and audio stay paused while either menu is open, with a free mouse
cursor. Choose a Level is also available in the main menu and on Game Over.
The selector cannot interrupt a capture or its blackout. Escape in settings
returns to the menu; Escape in the controls guide dismisses the guide as before.

Selecting a level starts it at its entrance with fresh level progress, using
the same seed and preferences. Selecting the current level also starts it
again. Keys, doors, presents, circuits, stamina, creatures, ambient timers and
capture state reset. Old chunk meshes and shadow caches are cleared through
the existing transition path. The flashlight starts enabled. The panel states
that progress resets before the player selects a destination.

Normal progression through the exits remains Level 0 -> Fun -> Parking.
Level 0's key and exit are now closer: its reserved route to the exit is
approximately **122.5–137.5 metres**, including the last approach. The key is
still three to five rooms before the door. HUD distances measure straight-line
distance and can therefore be smaller. Fun and Parking keep their prior exit
positions and geometry. See [key and hiding details](key-and-hiding.md).

## Verification

`tools/verify-level-menu.ps1` exercises the actual Escape/menu callback path,
keyboard and mouse travel, returning to the pause menu, Ctrl crouching,
same-level reset and returning from later levels. It saves rendered captures
under `tests/artifacts/level-menu/` for visual inspection. The previous Ctrl-tap
handling and its test target are no longer used.

The progression suite checks seeded distance bounds and walks the connected
route through collision geometry. This includes collecting the key and
opening/crossing the exit rather than only checking coordinate distance.

The route changes were verified on Windows on 26 October 2026, before moving
the selector into the Escape menu. Progression, world, Fun, Parking and streaming
suites passed. Distance bounds were checked across 256 seeds, with eight full
collision-tested key/exit routes. Eighteen sampled Fun/Parking geometry and
collision signatures matched the previous build exactly. These route and
geometry checks cover the unchanged first-level distance adjustment.

After moving the selector into Escape, the rebuilt Windows game passed all
23 rendered cases in `verify-level-menu.ps1`, with zero OpenGL errors. These
include mouse and keyboard entry, Back/Escape returning to pause, a second
Escape resuming, Ctrl taps leaving menus unchanged, Ctrl crouching, settings
navigation, Game Over selection and fresh progress after level travel. The
pause menu and selector were visually checked at 960x640; the selector also
passed the 1919x1027 rendering case.
