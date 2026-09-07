#!/usr/bin/env python3
"""Add merge request commits to CHANGELOG.md's Unreleased section."""

import re
import subprocess
import sys
from pathlib import Path


SECTIONS = ("Added", "Changed", "Fixed")
ADDED_VERBS = {"add", "added", "enable", "enabled", "implement", "implemented", "introduce", "introduced", "support", "supported"}
FIXED_VERBS = {"correct", "corrected", "fix", "fixed", "prevent", "prevented", "repair", "repaired", "resolve", "resolved"}
SKIP_PREFIXES = ("fixup!", "squash!")


def git(*args):
    return subprocess.check_output(("git", *args), text=True).strip()


def release_tag(target):
    if target.startswith("v"):
        pattern = f"{target}.*"
    else:
        latest = git("tag", "--merged", f"origin/{target}", "--list", "v*.*", "--sort=-version:refname").splitlines()
        if not latest:
            raise RuntimeError(f"No release tag is reachable from {target}")
        pattern = f"{latest[0].split('.', 1)[0]}.*"

    tags = git("tag", "--merged", f"origin/{target}", "--list", pattern, "--sort=-version:refname").splitlines()
    if not tags:
        raise RuntimeError(f"No release tag matching {pattern}")
    return tags[0]


def category(subject):
    verb = re.match(r"[A-Za-z]+", subject)
    if verb is None:
        return "Changed"
    word = verb.group(0).lower()
    if word in ADDED_VERBS:
        return "Added"
    if word in FIXED_VERBS:
        return "Fixed"
    return "Changed"


def changed_files(commit):
    return git("diff-tree", "--no-commit-id", "--name-only", "-r", commit).splitlines()


def commits_since(target):
    raw = git("log", "--no-merges", "--reverse", "--format=%H%x1f%s%x1f%B%x1e", f"{target}..HEAD")
    commits = []
    for record in raw.split("\x1e"):
        if not record.strip():
            continue
        commit, subject, body = record.strip().split("\x1f", 2)
        if subject.lower().startswith(SKIP_PREFIXES):
            continue
        if changed_files(commit) == ["CHANGELOG.md"]:
            continue
        commits.append((commit, subject.strip(), body))

    hashes = {commit for commit, _, _ in commits}
    result = []
    for commit, subject, body in commits:
        fixed_hashes = re.findall(r"(?im)^\s*(?:fix(?:es|ed)?|close[sd]?|resolve[sd]?)\s*:?\s*([0-9a-f]{7,40})\b", body)
        if any(any(candidate.startswith(fixed) for candidate in hashes) for fixed in fixed_hashes):
            continue
        result.append((category(subject), subject))
    return result


def update_changelog(path, target, entries):
    lines = path.read_text(encoding="utf-8").splitlines()
    try:
        start = lines.index("## [Unreleased]")
    except ValueError:
        start = next((i for i, line in enumerate(lines) if line.startswith("## ")), len(lines))
        lines[start:start] = ["## [Unreleased]", ""]

    end = next((i for i in range(start + 1, len(lines)) if lines[i].startswith("## ")), len(lines))
    for section in SECTIONS:
        if f"### {section}" not in lines[start:end]:
            lines[end:end] = [f"### {section}", ""]
            end += 2

    existing = set(lines[start:end])
    additions = {section: [] for section in SECTIONS}
    for section, subject in entries:
        entry = f"- {target} {subject}"
        if entry not in existing:
            additions[section].append(entry)
            existing.add(entry)

    for section in reversed(SECTIONS):
        new_entries = additions[section]
        if not new_entries:
            continue
        header = lines.index(f"### {section}", start, end)
        section_end = next((i for i in range(header + 1, end) if lines[i].startswith("### ")), end)
        insert_at = section_end
        while insert_at > header + 1 and not lines[insert_at - 1]:
            insert_at -= 1
        lines[insert_at:section_end] = new_entries + [""]
        end += len(new_entries) + 1 - (section_end - insert_at)

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    if len(sys.argv) != 3:
        sys.exit(f"Usage: {sys.argv[0]} <target-branch> <changelog>")
    target, filename = sys.argv[1:]
    # tag = release_tag(target)
    update_changelog(Path(filename), target, commits_since(target))
    print(f"Updated Unreleased entries for {target}")


if __name__ == "__main__":
    main()
