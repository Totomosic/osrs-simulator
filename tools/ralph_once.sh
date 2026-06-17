#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"
prompt_file="${script_dir}/ralph_prompt.md"

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
    if [[ ! -f "${prompt_file}" ]]; then
        echo "ralph_once: prompt file not found: ${prompt_file}" >&2
        exit 2
    fi

    if ! [[ "${issue_limit}" =~ ^[1-9][0-9]*$ ]]; then
        echo "RALPH_ISSUE_LIMIT must be a positive integer" >&2
        exit 2
    fi

    prompt="$(<"${prompt_file}")"
    prompt="${prompt//RALPH_ISSUE_LIMIT/${issue_limit}}"
    prompt="${prompt//RALPH_NO_VALID_ISSUES_TOKEN/${no_valid_issues_token}}"
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
