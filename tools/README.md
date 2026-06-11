# Tools

Support scripts and developer tooling.

Keep tooling separate from core engine and simulator code unless it becomes part of a supported runtime surface.

## Ralph Agent Loop

`ralph_once.sh` runs one non-interactive Codex pass against the repo. By default it finds one open non-PRD GitHub issue labelled `ready-for-agent`, asks Codex to implement it, runs focused verification, creates a local commit, closes the issue, and reports the result without pushing or opening a PR.

PRD issues are planning records, not implementation tasks. Ralph skips PRD issues, and when no non-PRD issues remain it closes open PRD issues and outputs `RALPH_NO_VALID_ISSUES`. `ralph_afk.sh` exits when it sees that token.

```sh
tools/ralph_once.sh
```

`ralph_afk.sh` repeats `ralph_once.sh` for unattended runs:

```sh
RALPH_MAX_ITERATIONS=3 tools/ralph_afk.sh
tools/ralph_afk.sh 3
```

Useful environment variables:

- `CODEX_BIN`: Codex executable to run. Defaults to `codex`.
- `RALPH_MODEL`: Codex model to run. Defaults to `gpt-5.5`.
- `RALPH_REASONING_EFFORT`: Codex reasoning effort. Defaults to `medium`.
- `RALPH_PROFILE`: optional Codex profile override.
- `RALPH_SANDBOX`: Codex sandbox mode. Defaults to `danger-full-access` so the AFK issue loop can reach GitHub through `gh`.
- `RALPH_APPROVAL_POLICY`: Codex approval mode. Defaults to `never`.
- `RALPH_GITHUB_PREFLIGHT`: set to `0` to skip the default prompt's `gh issue list` connectivity check.
- `RALPH_ISSUE_LIMIT`: max open GitHub issues to inspect. Defaults to `200`.
- `RALPH_NO_VALID_ISSUES_TOKEN`: token emitted by `ralph_once.sh` and watched by `ralph_afk.sh`. Defaults to `RALPH_NO_VALID_ISSUES`.
- `RALPH_STATUS_FILE`: optional status file written by `ralph_once.sh` for wrapper-readable states.
- `RALPH_PROMPT`: prompt for `ralph_once.sh`. Positional arguments also replace the default prompt.
- `ralph_afk.sh` treats a leading numeric argument as `RALPH_MAX_ITERATIONS`; remaining positional arguments replace the default prompt.
- `RALPH_EXTRA_ARGS`: additional whitespace-separated arguments passed to `codex exec`.
- `RALPH_MAX_ITERATIONS`: max AFK iterations. Defaults to `0`, meaning unlimited.
- `RALPH_SLEEP_SECONDS`: delay between AFK iterations. Defaults to `60`.
- `RALPH_CONTINUE_ON_FAILURE`: set to `1` to keep looping after a failed iteration.
- `RALPH_STOP_FILE`: optional path; if the file exists before an iteration, `ralph_afk.sh` exits cleanly.
