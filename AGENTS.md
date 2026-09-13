# Instructions

## Context

GenSound is a macOS AU/VST3 instrument plugin for fast sketching of generative and tracker-friendly instruments in ordinary C++. Users write small C++ scripts that define oscillators, envelopes, and filters, and the plugin hot-reloads them at runtime without restarting the DAW.

JUCE 8.x provides the plugin shell, UI, and audio processing infrastructure. CMake fetches JUCE automatically.

### Source layout

| Directory | Purpose |
|---|---|
| `source/` | Plugin source code |
| `source/PluginProcessor.cpp|h` | JUCE AudioProcessor implementation |
| `source/audio/` | Voice engine (`Voice`, `VoiceManager`) |
| `source/compile/` | External compilation, file watching, diagnostics |
| `source/loader/` | Dynamic library loading and versioning |
| `source/ui/` | Plugin editor UI |
| `source/scripting/` | Script type definitions |
| `sdk/` | SDK header (`GenSoundSDK.h`) for user scripts |
| `scripts/` | Example scripts (sine, kick, snare, fm_bass, pluck, gated_pad) |

### Architecture notes

- **Voice model**: Polyphonic voice engine with 256-byte per-voice scratch memory (`VoiceState`). Scripts implement `initVoice()` and `synth()`.
- **Hot-swap**: Compiled scripts are loaded as dynamic libraries (`DynamicLibraryLoader`). New voices use the latest dylib; old voices continue with their version until they finish. Failed compilations are transparent.
- **Compilation pipeline**: `ExternalCompiler` invokes clang++ to compile the user's `.cpp` into a dylib. `FileWatcher` polls for changes. `ScriptManager` coordinates loading/unloading.
- **SDK**: `GenSoundSDK.h` provides `gs::` namespace helpers (oscillators, envelopes, filters, noise, clipping, state casting). ABI version is currently `1`.
- **Parameters**: 8 unlabelled knobs (`P0`–`P7`) mapped to `ctx->p[]` in scripts.

## Development process

You have limited context memory, so always work in stages. Make a plan first before you dive in, splitting requests into discrete tasks. This is especially important for larger changes. Then work on one task at a time.

Keep track of your progress so that another agent can pick up the work if you get interrupted.

Keep `README.md` updated with any major changes to the project scope. It should only contain information useful to end users, any details regarding the implementation plan and progress should live in task files, and any instructions to agents in `AGENTS.md`.

## Task tracking

For non-trivial or multi-step coding work, use the `task-tracker` Pi skill:

```
/skill:task-tracker
```

It maintains `.pi/tasks/{backlog,active,archive}/` plus `.pi/tasks/ready.md`. Task files use stable IDs and meaningful filenames such as `T-014-oauth-callback.md`; the directory is the task's authoritative state, and tasks are moved between directories rather than duplicated. Keep the first 10 lines of backlog tasks concise enough for routine review; active tasks contain detailed current work and are the normal task-context files; archive is cold storage and should not normally be loaded. Use `ready.md` as a lightweight index of backlog tasks ready to be picked up, not as a second source of truth. Use explicit dependencies/blockers, acceptance criteria, and a `Next` action. Work on one primary task at a time, record out-of-scope discoveries as separate tasks, and use Git/repository state as authoritative when resuming after compaction or restart.

## Version control

The project directory is under Git version control. Create a repository if needed, using a "main" branch.

Commit every successful feature implementation with a descriptive message. Use the git identity "Coding Agent <agent@auto.local>" to indicate that it came from an automated coding agent. Don't include any system details or personal information about the user, including any file or directory paths outside the project root.

## Miscellaneous

Keep `README.md` and `AGENTS.md` below 7k bytes at all times.

We use British spelling, date formats, and measurement units.

## Development process

You have limited context memory, so always work in stages. Make a plan first before you dive in, splitting requests into discrete tasks. This is especially important for larger changes. Then work on one task at a time. 

Keep track of your progress so that another agent can pick up the work if you get interrupted.

Keep `README.md` updated with any major changes to the project scope. It should only contain information useful to end users, any details regarding the implementation plan and progress should live in task files, and any instructions to agents in `AGENTS.md`. 

## Task tracking

For non-trivial or multi-step coding work, use the `task-tracker` Pi skill:

```text
/skill:task-tracker
```

It maintains `.pi/tasks/{backlog,active,archive}/` plus `.pi/tasks/ready.md`. Task files use stable IDs and meaningful filenames such as `T-014-oauth-callback.md`; the directory is the task's authoritative state, and tasks are moved between directories rather than duplicated. Keep the first 10 lines of backlog tasks concise enough for routine review; active tasks contain detailed current work and are the normal task-context files; archive is cold storage and should not normally be loaded. Use `ready.md` as a lightweight index of backlog tasks ready to be picked up, not as a second source of truth. Use explicit dependencies/blockers, acceptance criteria, and a `Next` action. Work on one primary task at a time, record out-of-scope discoveries as separate tasks, and use Git/repository state as authoritative when resuming after compaction or restart.

## Version control

The project directory is under Git version control. Create a repository if needed, using a "main" branch. 

Commit every successful feature implementation with a descriptive message. Use the git identity "Coding Agent <agent@auto.local>" to indicate that it came from an automated coding agent. Don't include any system details or personal information about the user, including any file or directory paths outside the project root. 

## Miscellaneous

Keep `README.md` and `AGENTS.md` below 7k bytes at all times.

We use British spelling, date formats, and measurement units.
