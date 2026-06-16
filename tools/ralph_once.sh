#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

codex_bin="${CODEX_BIN:-codex}"
model="${RALPH_MODEL:-gpt-5.5}"
reasoning_effort="${RALPH_REASONING_EFFORT:-medium}"
sandbox="${RALPH_SANDBOX:-danger-full-access}"
approval_policy="${RALPH_APPROVAL_POLICY:-never}"
github_preflight="${RALPH_GITHUB_PREFLIGHT:-1}"
issue_limit="${RALPH_ISSUE_LIMIT:-200}"
no_valid_issues_token="${RALPH_NO_VALID_ISSUES_TOKEN:-RALPH_NO_VALID_ISSUES}"

prompt="${RALPH_PROMPT:-}"
uses_default_prompt=0
if [[ "$#" -gt 0 ]]; then
    prompt="$*"
fi

if [[ -z "${prompt}" ]]; then
    uses_default_prompt=1
    if ! [[ "${issue_limit}" =~ ^[1-9][0-9]*$ ]]; then
        echo "RALPH_ISSUE_LIMIT must be a positive integer" >&2
        exit 2
    fi

    prompt="$(cat <<'PROMPT'
You are Ralph, an AFK coding agent loop running through Codex.

Run exactly one autonomous implementation pass in this repository:
1. Read AGENTS.md, docs/agents/issue-tracker.md, docs/agents/triage-labels.md, and docs/agents/domain.md.
2. Use the gh CLI to find one open non-PRD GitHub issue labelled ready-for-agent, then work only on that issue.
3. Use this command as the starting point for issue selection: gh issue list --state open --label ready-for-agent --limit RALPH_ISSUE_LIMIT --json number,title,body,labels,comments
4. For the selected issue, inspect the relevant code and docs, use /tdd to make the smallest coherent implementation, and run focused verification.
5. When the issue is complete and verification has passed, create a local git commit for the issue.
6. Do not push or open a PR.
7. Leave a concise final report that includes the issue number, commit hash, changed files, and verification result.
8. After completing the issue close the GitHub issue with a comment linking to the commit and report that in the final report.
9. Do not work on PRD issues. A PRD issue is planning context, not an implementation task.
10. If no ready-for-agent implementation issue exists, and no open non-PRD issues remain, close open PRD issues with a comment explaining that all implementation issues are complete.
PROMPT
)"
    prompt="${prompt}

If no open non-PRD GitHub issue labelled ready-for-agent exists, output exactly this token on its own line and exit successfully without changing files:
${no_valid_issues_token}"
    prompt="${prompt/RALPH_ISSUE_LIMIT/${issue_limit}}"
fi

if [[ "${github_preflight}" == "1" && "${uses_default_prompt}" == "1" ]]; then
    if ! command -v gh >/dev/null 2>&1; then
        echo "ralph_once: gh is required for the default GitHub issue workflow" >&2
        exit 3
    fi

    gh issue list --state open --limit 1 --json number >/dev/null || {
        echo "ralph_once: unable to connect to GitHub issues with gh" >&2
        echo "ralph_once: check network access and run 'gh auth status' or 'gh auth refresh -h github.com'" >&2
        exit 3
    }
fi

codex_args=(
    exec
    --cd "${repo_root}"
    --model "${model}"
    --config "model_reasoning_effort=\"${reasoning_effort}\""
    --config "approval_policy=\"${approval_policy}\""
    --sandbox "${sandbox}"
)

if [[ -n "${RALPH_PROFILE:-}" ]]; then
    codex_args+=(--profile "${RALPH_PROFILE}")
fi

if [[ "${RALPH_SEARCH:-0}" == "1" ]]; then
    codex_args+=(--search)
fi

if [[ "${RALPH_JSON:-0}" == "1" ]]; then
    codex_args+=(--json)
fi

if [[ -n "${RALPH_OUTPUT_LAST_MESSAGE:-}" ]]; then
    codex_args+=(--output-last-message "${RALPH_OUTPUT_LAST_MESSAGE}")
fi

if [[ -n "${RALPH_EXTRA_ARGS:-}" ]]; then
    # shellcheck disable=SC2206
    extra_args=(${RALPH_EXTRA_ARGS})
    codex_args+=("${extra_args[@]}")
fi

exec "${codex_bin}" "${codex_args[@]}" "${prompt}"
