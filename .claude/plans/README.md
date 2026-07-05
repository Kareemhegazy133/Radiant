# Implementation Plans

Per-story implementation plans produced by the `/plan-feature` skill.

- **One file per story**, named `RAD-XX-<slug>.md` (e.g. `RAD-25-fixed-timestep-loop.md`).
- The file is the **readable, git-tracked record** of how a story was planned — it renders
  cleanly in a markdown preview (metadata table, ASCII file-tree, phased task lists, AC
  verification table).
- When the plan is agreed, `/plan-feature` posts the same content as a **dated comment on the
  Jira story** (`addCommentToJiraIssue`, markdown) so the record also lives on the issue.
  Re-planning a story appends a new dated comment — it never overwrites the previous one.
- During implementation, the `- [ ]` step checkboxes are checked off in place, keeping the
  file a live progress record.

These are planning artifacts, not formal design docs — formal docs live in `Docs/`.
