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
status_file="${RALPH_STATUS_FILE:-}"

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
2. If the wrapper selected an issue below, work only on that GitHub issue.
3. If the wrapper did not select an issue, use gh to find one open non-PRD GitHub issue labelled ready-for-agent.
4. For the selected issue, inspect the relevant code and docs, use /tdd to make the smallest coherent implementation, and run focused verification.
5. When the issue is complete and verification has passed, create a local git commit for the issue.
6. Do not push or open a PR.
7. Leave a concise final report that includes the issue number, commit hash, changed files, and verification result.
8. After completing the issue close the GitHub issue with a comment linking to the commit and report that in the final report.
9. Do not work on PRD issues. A PRD issue is planning context, not an implementation task.
PROMPT
)"
    prompt="${prompt}

If no open non-PRD GitHub issue labelled ready-for-agent exists, output exactly this token on its own line and exit successfully without changing files:
${no_valid_issues_token}"
fi

if [[ "${github_preflight}" == "1" && "${uses_default_prompt}" == "1" ]]; then
    if ! command -v gh >/dev/null 2>&1; then
        echo "ralph_once: gh is required for the default GitHub issue workflow" >&2
        exit 3
    fi

    if ! command -v jq >/dev/null 2>&1; then
        echo "ralph_once: jq is required for the default GitHub issue workflow" >&2
        exit 3
    fi

    if ! [[ "${issue_limit}" =~ ^[1-9][0-9]*$ ]]; then
        echo "RALPH_ISSUE_LIMIT must be a positive integer" >&2
        exit 2
    fi

    open_issues_json="$(gh issue list --state open --limit "${issue_limit}" --json number,title,labels)" || {
        echo "ralph_once: unable to connect to GitHub issues with gh" >&2
        echo "ralph_once: check network access and run 'gh auth status' or 'gh auth refresh -h github.com'" >&2
        exit 3
    }

    jq_prd_predicate='
        (
            (.title | test("(^|[^A-Za-z])PRD([^A-Za-z]|$)"; "i"))
            or ([((.labels // [])[]).name | ascii_downcase] | any(. == "prd" or . == "type:prd" or . == "type: prd"))
        )
    '

    selected_issue_number="$(
        jq -r '
            map(select(
                ((.labels // []) | any(.name == "ready-for-agent"))
                and ('"${jq_prd_predicate}"' | not)
            ))
            | first
            | .number // empty
        ' <<<"${open_issues_json}"
    )"

    if [[ -z "${selected_issue_number}" ]]; then
        open_non_prd_count="$(
            jq -r '
                map(select('"${jq_prd_predicate}"' | not))
                | length
            ' <<<"${open_issues_json}"
        )"

        if [[ "${open_non_prd_count}" == "0" ]]; then
            while IFS= read -r prd_issue_number; do
                [[ -z "${prd_issue_number}" ]] && continue
                gh issue close "${prd_issue_number}" --comment "Closing this PRD because all implementation issues are complete."
            done < <(
                jq -r '
                    map(select('"${jq_prd_predicate}"'))
                    | .[].number
                ' <<<"${open_issues_json}"
            )
        fi

        if [[ -n "${status_file}" ]]; then
            printf '%s\n' "no_valid_issues" > "${status_file}"
        fi

        echo "${no_valid_issues_token}"
        exit 0
    fi

    prompt="${prompt}

Issue selection has already been done by the wrapper. Work only on GitHub issue #${selected_issue_number}.

Do not work on PRD issues. A PRD issue is planning context, not an implementation task. If the selected issue turns out to be a PRD, make no code changes, report the mismatch, and exit successfully."
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
