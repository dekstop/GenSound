# GenSound — TODO

## Milestone 7 (not yet implemented)
- [ ] Offline render / export path
  - Render a voice (or all voices) to a WAV/AIFF file without real-time constraint
  - Useful for tracker-style sample baking from procedural instruments
  - Candidate approach: headless render mode driven by a MIDI clip + duration parameter

## Nice-to-haves / future work
- [ ] Replace polling FileWatcher with kqueue / FSEvents for lower latency and CPU use
- [ ] Pitch bend MIDI handling (populate VoiceContext::freq from pitchwheel message)
- [ ] Per-script parameter label customisation (script exports a `paramLabel(int)` fn)
- [ ] Multiple simultaneous scripts / instrument slots
- [ ] AU code-signing documentation / build script
- [ ] CMake `COPY_PLUGIN_AFTER_BUILD` auto-install option
- [ ] Unit tests for VoiceManager, ExternalCompiler, DynamicLibraryLoader
- [ ] CI pipeline (GitHub Actions) that builds the plugin on macOS runners
- [ ] Example script: hi-hat (short noise burst)
- [ ] Example script: multi-operator FM with envelope per operator
- [ ] Example script: additive synthesis using `VoiceState` for partial phases
- [ ] Investigate `__builtin_expect` / SIMD for inner sample loop if profiling warrants it
- [ ] Consider a small `gs::Delay<N>` helper in SDK to make Karplus-Strong easier to write
- [ ] Header dependency tracking for script directory includes: pass `-MMD` to clang++, parse the emitted `.d` file after each build, and watch exactly the headers it lists rather than polling the whole directory
