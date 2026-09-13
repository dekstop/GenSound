---
name: task-tracker
description: Lightweight task tracking and development workflow for non-trivial coding work. Use for multi-step work, subtasks, dependencies, blockers, discoveries, or work that may span agent sessions.
---

# Pi Task Management

Use `.pi/tasks/` to manage tasks across three lifecycle states:

```text
.pi/tasks/
├── backlog/
├── active/
├── archive/
└── ready.md
```

## Task files

Each task is a single Markdown file with a meaningful filename:

```text
T-014-oauth-callback.md
```

The filename should contain:
- the stable task ID T-014
- a short, meaningful kebab-case label

A task's **directory is its state**:
- `backlog/` — not currently being worked on
- `active/` — currently being implemented
- `archive/` — completed, cancelled, or otherwise no longer active
- `ready.md` — index of backlog tasks ready to be picked up

Do **not** duplicate a task when its state changes. Move the existing file between directories.

## Task structure

A task file uses the following structure:

```markdown
## T-001 — Short title

Short summary of the task.

## Dependencies
- T-021-other-task — Name of other task

## Acceptance criteria
- [ ] Concrete, testable requirement
- [ ] Concrete, testable requirement

### Next
Single most useful next action.

### Notes
Durable facts, discoveries, decisions, or constraints only.

### Unknowns
Important unresolved facts or constraints.

### Hypothesis
Optional belief to investigate or test.
```

Preserve that structure when moving or updating tasks.

Tasks may additionally contain useful information appropriate to their state.
- More complex tasks might contain an ordered list of subtasks after `Dependencies`.
- For `backlog/` tasks, additional information may include implementation notes, constraints, dependencies, acceptance criteria, or useful context.
- For `active/` tasks, additionally record useful implementation details, progress, discoveries, or decisions, and a `Next` action where relevant.
- For `archive/` tasks, preserve key decisions, outcomes, and other information useful for historical reference.

Keep task information concise and implementation-focused.

## Reading tasks

**Do not routinely load complete backlog or archive files.** Review only the **first 10 lines** unless a specific task needs deeper investigation. Those lines must contain enough information to judge relevance.

`active/` tasks may be loaded in full.

**Archive is cold storage. Do not consult it during routine planning.** Inspect it only when specific historical context is relevant.

## Ready index

`.pi/tasks/ready.md` is a lightweight index of ready backlog tasks, not a second source of truth. Add tasks when ready; remove them when they move to `active/` or are no longer ready.

## Planning

During routine planning:
1. Read `ready.md`.
2. Prioritise work using the ready index and relevant backlog summaries. Review only the first 10 lines of relevant backlog tasks. Do not inspect archive unless historical context is required.
3. Select one primary task.
4. Move it from `backlog/` to `active/` and load it in full.
5. Use the Development Methodology below to decide how to proceed and keep the task current.

For complex work, use subtasks as bounded implementation increments, not competing primary tasks. Give each a derived ID such as `T-014.1`, its own file, criteria, and `Next`. Keep the parent active as the overall objective; archive completed subtasks. Do not create subtasks merely for administration.

## Development methodology

Use an iterative, context-efficient development loop:
`understand → implement → validate → learn → improve`

The loop is a feedback system, not a rigid sequence. Choose the action that most usefully advances or de-risks the task.

Before substantial changes, establish the minimum safe context: understand the task and criteria, inspect relevant code/tests, identify dependencies, blockers, and unknowns, and resolve high-impact uncertainty. For complex work, create subtasks to test hypotheses if it can help to improve planning certainty.

Do not explore the repository exhaustively when less context is sufficient.

Make the smallest coherent change that advances the task. Prefer simple, incremental solutions over speculative architecture or large batches of changes.

After each meaningful step, validate with available tests, checks, builds, or runtime behaviour. Treat failures and unexpected results as information and adapt.

Use `Unknowns` and, where useful, `Hypothesis` to make important uncertainty explicit. Use `Next` for the single most useful next action for advancing or de-risking the task; it may be implementation, investigation, testing, profiling, or another evidence-seeking action.

Record conclusions rather than an activity log: preserve discoveries, decisions, rejected approaches, useful measurements, and information that changes what to do next. Do not record every command or transient thought.

Keep the primary task focused on its objective. Use subtasks for bounded stages of complex work; use separate backlog tasks for genuinely out-of-scope work. Do not let exploratory work silently expand the parent task.

Before completing a task, verify acceptance criteria, run appropriate validation, record important outcomes and decisions, and identify follow-up work.

## Completing work

When a task is completed, or when it is impossible to complete (WONTFIX):
1. Record the important outcome, decisions, and implementation notes in the task file.
2. Move the task from `active/` to `archive/`.
3. Remove it from `ready.md`.

## Creating tasks

Before creating a task, check existing filenames/IDs to avoid duplicates.

Use the next stable ID and a meaningful filename, e.g.:

```text
T-022-webhook-retries.md
```

Put new tasks in `backlog/` unless they are immediately being worked on.

If a newly created task is ready to be picked up, add it to `ready.md`.

## Important constraints
- Never duplicate task files when changing state; move them.
- Treat directory location as the authoritative lifecycle state.
- `ready.md` is an index, not a second source of truth.
- Do not routinely load backlog/archive task bodies.
- When reviewing backlog/archive, load only the first 10 lines unless deeper inspection is justified.
- Active tasks may be loaded in full.
- Archive is cold storage and should not normally participate in planning.
- Keep work in progress deliberately small.
- Resolve important uncertainty early and cheaply.
- Validate meaningful changes continuously.
- Preserve useful conclusions and implementation knowledge in active tasks, not activity logs.
- Use subtasks only for bounded stages of complex work; keep one primary task.
- Keep unrelated discoveries as separate tasks.
- Use Git and repository state as authoritative when resuming after compaction or restart.
