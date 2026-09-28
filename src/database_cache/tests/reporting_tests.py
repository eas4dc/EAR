#!/usr/bin/env python3
"""Send synthetic records to an existing EARDBD and verify its recorder output.

Never starts, stops, configures or rebuilds the daemon or modifies its recordings.
"""

import argparse
from collections import Counter
import math
from pathlib import Path
import secrets
import subprocess
import sys
import time


def positive_int(value):
    number = int(value)
    if not 0 < number <= 2147483647:
        raise argparse.ArgumentTypeError("must be between 1 and 2147483647")
    return number


def positive_seconds(value):
    number = float(value)
    if not math.isfinite(number) or number <= 0:
        raise argparse.ArgumentTypeError("must be a finite positive number")
    return number


def records(directory, identity):
    path = directory / f"{identity}.tsv"
    try:
        text = path.read_text()
    except FileNotFoundError as exc:
        raise RuntimeError(
            f"{path} is missing. EARDBD's record_{identity.lower()}.so creates this file "
            "during plugin initialization, before any samples are sent. Check that "
            "--records-dir matches the running daemon's TmpDir, the .so is installed "
            "under <InstDir>/lib/plugins/reports, and EARDBDReportPlugins selected it "
            "when that daemon started. Inspect the daemon's plugin-loading logs."
        ) from exc
    except PermissionError as exc:
        raise RuntimeError(f"Cannot read {path}; check the verifier user's file/directory permissions") from exc
    # Ignore an incomplete trailing line while EARDBD is writing.
    return [tuple(line.split("\t")) for line in text.split("\n")[:-1]]


def payload_records(directory, identity, job):
    # Old runs and unrelated traffic must neither satisfy nor fail this run.
    return Counter(row for row in records(directory, identity)
                   if len(row) >= 3 and row[0] in ("app", "metric") and row[2] == str(job))


def expected_records(identity, job):
    rows = []
    for i in range(3):
        rows.append(("app", identity, str(job), str(i + 1), "report-test",
                     "report-regression", str(int(i != 1)), str(int(i == 2)), str(112 + i)))
    for i in range(2):
        rows.append(("metric", identity, str(job), str(10 + i), "report-test",
                     str(1700000000 + i * 10), str(1700000010 + i * 10), str(100 + i)))
    return Counter(rows)


def run(args):
    directory = args.records_dir.resolve()
    for identity in args.plugins:
        rows = records(directory, identity)
        if not any(len(row) == 3 and row[:2] == ("init", identity) for row in rows):
            raise RuntimeError(f"{identity}: no initialization marker in {directory / (identity + '.tsv')}")
        if payload_records(directory, identity, args.job_id):
            raise RuntimeError(f"job {args.job_id} already exists in {identity}.tsv; use a fresh job ID")

    print(f"Sending job {args.job_id} to {args.host}:{args.port}; reading {directory}", flush=True)
    # No status polling: another connection from the same IP displaces the sender.
    subprocess.run([str(args.client.resolve()), "send", args.host, str(args.port), str(args.job_id)],
                   check=True, timeout=args.client_timeout)
    expected = {identity: expected_records(identity, args.job_id) for identity in args.plugins}
    deadline = time.monotonic() + args.timeout
    complete_since = None
    while True:
        actual = {identity: payload_records(directory, identity, args.job_id) for identity in args.plugins}
        for identity in args.plugins:
            extra = actual[identity] - expected[identity]
            if extra:
                raise RuntimeError(f"{identity}: unexpected or duplicate records: {dict(extra)}")
        now = time.monotonic()
        if actual == expected:
            if complete_since is None:
                complete_since = now
            if now - complete_since >= args.settle:
                for identity in args.plugins:
                    print(f"PASS {identity}: all five records received exactly once for job {args.job_id}")
                return
        else:
            complete_since = None
        if now >= deadline:
            missing = {identity: dict(expected[identity] - actual[identity]) for identity in args.plugins}
            raise RuntimeError(f"delivery/settling deadline exceeded; missing records: {missing}")
        time.sleep(min(0.1, deadline - now))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path, default=Path.cwd() / "reporting_client")
    parser.add_argument("--host", required=True, help="existing EARDBD host")
    parser.add_argument("--port", required=True, type=positive_int, help="EARDBD metrics TCP port")
    parser.add_argument("--records-dir", required=True, type=Path, help="readable EARDBD TmpDir containing TSV files")
    parser.add_argument("--plugins", required=True, nargs="+", choices=("A", "B"), help="expected recorder identities")
    parser.add_argument("--job-id", type=positive_int, default=secrets.randbelow(2147483647) + 1)
    parser.add_argument("--timeout", type=positive_seconds, default=90, help="delivery plus settling deadline in seconds")
    parser.add_argument("--settle", type=positive_seconds, default=2, help="observe duplicates after delivery for this many seconds")
    parser.add_argument("--client-timeout", type=positive_seconds, default=10)
    args = parser.parse_args()
    if args.port > 65535:
        parser.error("--port must be between 1 and 65535")
    if args.settle >= args.timeout:
        parser.error("--timeout must exceed --settle")
    args.plugins = list(dict.fromkeys(args.plugins))
    try:
        run(args)
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as exc:
        print(f"FAIL job {args.job_id}: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
