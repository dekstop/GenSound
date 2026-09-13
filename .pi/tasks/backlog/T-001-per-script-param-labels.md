## T-001 — Per-script parameter label customisation

Allow scripts to export a `paramLabel(int)` function so the plugin UI can display meaningful labels for each parameter knob instead of the generic P0–P7.

## Acceptance criteria
- [ ] Script can optionally export `GS_EXPORT const char* paramLabel(int index)` returning a label string for each parameter
- [ ] Plugin UI reads the labels from the compiled dylib and displays them next to the 8 knobs
- [ ] If `paramLabel` is not exported, fall back to P0–P7 labels
- [ ] Labels update when the script recompiles
- [ ] README.md updated with documentation on the `paramLabel` export
- [ ] All example scripts updated to export meaningful parameter labels

### Next
Inspect `source/ui/PluginEditor.cpp` to understand the current knob rendering, then design the dylib symbol lookup and UI update mechanism.

### Notes
- TODO.md item: "Per-script parameter label customisation (script exports a `paramLabel(int)` fn)"
- Currently parameters are unlabelled knobs (P0–P7)
- Labels should be automatable in the DAW (existing behaviour)
