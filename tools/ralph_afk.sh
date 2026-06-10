#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

max_iterations="${RALPH_MAX_ITERATIONS:-0}"
sleep_seconds="${RALPH_SLEEP_SECONDS:-60}"
continue_on_failure="${RALPH_CONTINUE_ON_FAILURE:-0}"
stop_file="${RALPH_STOP_FILE:-}"
no_valid_issues_token="${RALPH_NO_VALID_ISSUES_TOKEN:-RALPH_NO_VALID_ISSUES}"

if [[ "$#" -gt 0 && "$1" =~ ^[0-9]+$ ]]; then
    max_iterations="$1"
    shift
fi

if ! [[ "${max_iterations}" =~ ^[0-9]+$ ]]; then
    echo "RALPH_MAX_ITERATIONS must be a non-negative integer" >&2
    exit 2
fi

if ! [[ "${sleep_seconds}" =~ ^[0-9]+$ ]]; then
    echo "RALPH_SLEEP_SECONDS must be a non-negative integer" >&2
    exit 2
fi

iteration=1
once_args=("$@")
iteration_output="$(mktemp -t ralph_afk.XXXXXX)"
trap 'rm -f "${iteration_output}"' EXIT

while true; do
    if [[ -n "${stop_file}" && -e "${stop_file}" ]]; then
        echo "ralph_afk: stop file exists at ${stop_file}" >&2
        exit 0
    fi

    if [[ "${max_iterations}" -ne 0 && "${iteration}" -gt "${max_iterations}" ]]; then
        exit 0
    fi

    echo "ralph_afk: starting iteration ${iteration}" >&2
    : > "${iteration_output}"

    if "${script_dir}/ralph_once.sh" "${once_args[@]}" 2>&1 | tee "${iteration_output}"; then
        echo "ralph_afk: completed iteration ${iteration}" >&2
        if grep -Fqx "${no_valid_issues_token}" "${iteration_output}"; then
            echo "ralph_afk: no valid issues remain" >&2
            exit 0
        fi
    else
        status="$?"
        echo "ralph_afk: iteration ${iteration} failed with status ${status}" >&2
        if [[ "${continue_on_failure}" != "1" ]]; then
            exit "${status}"
        fi
    fi

    iteration=$((iteration + 1))

    if [[ "${sleep_seconds}" -gt 0 ]]; then
        sleep "${sleep_seconds}"
    fi
done
