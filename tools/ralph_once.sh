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

prompt="${RALPH_PROMPT:-}"
uses_default_prompt=0
if [[ "$#" -gt 0 ]]; then
    prompt="$*"
fi

if [[ -z "${prompt}" ]]; then
    uses_default_prompt=1
    prompt="$(cat <<'PROMPT'
You are Ralph, an AFK coding agent loop running through Codex.

Run exactly one autonomous implementation pass in this repository:
1. Read AGENTS.md, docs/agents/issue-tracker.md, docs/agents/triage-labels.md, and docs/agents/domain.md.
2. Use gh to find one open GitHub issue labelled ready-for-agent.
3. If no issue is ready, report that and exit successfully without changing files.
4. For the selected issue, inspect the relevant code and docs, make the smallest coherent implementation, and run focused verification.
5. When the issue is complete and verification has passed, create a local git commit for the issue.
6. Do not push or open a PR.
7. Leave a concise final report that includes the issue number, commit hash, changed files, and verification result.
PROMPT
)"
fi

if [[ "${github_preflight}" == "1" && "${uses_default_prompt}" == "1" ]]; then
    if ! command -v gh >/dev/null 2>&1; then
        echo "ralph_once: gh is required for the default GitHub issue workflow" >&2
        exit 3
    fi

    if ! gh issue list --state open --label ready-for-agent --limit 1 --json number >/dev/null; then
        echo "ralph_once: unable to connect to GitHub issues with gh" >&2
        echo "ralph_once: check network access and run 'gh auth status' or 'gh auth refresh -h github.com'" >&2
        exit 3
    fi
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
