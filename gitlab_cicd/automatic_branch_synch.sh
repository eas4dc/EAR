#!/usr/bin/env bash

set -Eeuo pipefail

for variable in CI_COMMIT_BRANCH CI_PROJECT_ID CI_API_V4_URL MERGE_REQUEST_ACCESS_TOKEN; do
    if [[ -z ${!variable:-} ]]; then
        echo "$variable not defined" >&2
        exit 1
    fi
done

if ! command -v jq >/dev/null 2>&1; then
    echo "jq is required to parse GitLab API responses" >&2
    exit 1
fi

if [[ ! $CI_COMMIT_BRANCH =~ ^v([[:digit:]]+)$ ]]; then
    echo "Major version pattern not found for $CI_COMMIT_BRANCH" >&2
    exit 1
fi

next_major="$((10#${BASH_REMATCH[1]} + 1))"
next_release_branch="v$next_major"
target_branch="development"

gitlab_api() {
    curl --fail-with-body --silent --show-error \
        --header "PRIVATE-TOKEN: $MERGE_REQUEST_ACCESS_TOKEN" \
        "$@"
}

branches="$(gitlab_api --get \
    --data-urlencode 'per_page=100' \
    "$CI_API_V4_URL/projects/$CI_PROJECT_ID/repository/branches")"
if jq -e --arg branch "$next_release_branch" \
    '.[] | select(.name == $branch)' >/dev/null <<<"$branches"; then
    target_branch="$next_release_branch"
fi

echo "Synchronizing $CI_COMMIT_BRANCH into $target_branch"

existing_merge_requests="$(gitlab_api --get \
    --data-urlencode 'state=opened' \
    --data-urlencode "source_branch=$CI_COMMIT_BRANCH" \
    --data-urlencode "target_branch=$target_branch" \
    "$CI_API_V4_URL/projects/$CI_PROJECT_ID/merge_requests")"
merge_request_iid="$(jq -r '.[0].iid // empty' <<<"$existing_merge_requests")"

if [[ -z $merge_request_iid ]]; then
    merge_request="$(gitlab_api --request POST \
        --data-urlencode "source_branch=$CI_COMMIT_BRANCH" \
        --data-urlencode "target_branch=$target_branch" \
        --data-urlencode "title=$CI_COMMIT_BRANCH synchronization" \
        "$CI_API_V4_URL/projects/$CI_PROJECT_ID/merge_requests")"
    merge_request_iid="$(jq -er '.iid' <<<"$merge_request")"
    echo "Created merge request !$merge_request_iid"
else
    echo "Reusing open merge request !$merge_request_iid"
fi

gitlab_api --request PUT \
    --data-urlencode 'auto_merge=true' \
    "$CI_API_V4_URL/projects/$CI_PROJECT_ID/merge_requests/$merge_request_iid/merge" >/dev/null

echo "Merge request !$merge_request_iid will merge when its pipeline succeeds"
