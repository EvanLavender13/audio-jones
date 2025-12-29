# Drawable Reorder Buttons

Add ▲/▼ buttons to reorder drawables in the list, controlling render order.

## Current State

- `src/ui/imgui_drawables.cpp:273-302` - drawable list UI with selection
- `src/ui/imgui_drawables.cpp:254-268` - Delete button pattern (operates on selected item)
- `src/render/drawable.cpp:114-155` - renders drawables in array order

## Phase 1: Add Reorder Buttons

**Goal**: Users can move the selected drawable up or down in the list.

**Modify**:
- `src/ui/imgui_drawables.cpp` - add ▲/▼ buttons next to Delete button

**Implementation**:
- Add "▲" button after Delete, disabled when `*selected == 0`
- Add "▼" button after ▲, disabled when `*selected == *count - 1`
- On ▲ click: swap `drawables[*selected]` with `drawables[*selected - 1]`, decrement `*selected`
- On ▼ click: swap `drawables[*selected]` with `drawables[*selected + 1]`, increment `*selected`
- Use `Drawable` copy constructor (already handles union correctly)

**Done when**: Clicking ▲/▼ moves selected drawable and updates render order visually.
