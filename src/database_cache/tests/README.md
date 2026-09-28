# Reporting tests for an existing EARDBD

This directory builds two synthetic report plugins and a TCP client. It never
builds, starts, stops or configures EARDBD. Use your already compiled daemon,
with the desired recording plugins loaded, and manage its lifecycle yourself.

## Build

After updating `Makefile.am`, regenerate/configure using your usual project
options, then run `make post` from the project root. For an existing configured
tree whose common library is already built:

```sh
autoreconf -i
./config.status --recheck
./config.status
make post
```

Automake's `check` builds `reporting_client`, the existing `test_status` and
`test_types` tools, and `.libs/record_a.so` and `.libs/record_b.so` through
Libtool. The `.la` files are metadata, not loadable plugins. The module link
flags include `-rpath` so these check-only libraries produce shared objects
instead of only static convenience archives.
It does not execute any tests. The focused build command is:

```sh
make -C src/database_cache/tests check
```

The client uses the `eardbd_api.c`, keeping its object inside this tests
directory with Automake `subdir-objects` enabled.
It links the project's existing `libcommon.a`. Build with the same headers,
feature flags and `SEC_KEY` as the running daemon. `CONSTANTS` is inherited
from `make post`.

## Configure the instance you manage

Copy the desired plugins to the daemon's `<InstDir>/lib/plugins/reports`, then
load one or both with your normal configuration/start procedure:

```ini
EARDBDReportPlugins=record_a.so:record_b.so
```

An already running instance must have loaded these plugins before the test.
Each plugin appends to `<TmpDir>/A.tsv` or `<TmpDir>/B.tsv` (`<TmpDir>` configured in the `ear.conf`). The daemon user must
be able to write these files, and the verification user must be able to read
them. No MySQL replacement, dummy plugin or failing-initialization plugin is
built. The two plugins differ only in their recorded identity.

For an installation at `/opt/ear`, copy the newly built shared objects with
executable permissions (the EAR plugin loader checks `X_OK`):

```sh
install -m 755 src/database_cache/tests/.libs/record_a.so /opt/ear/lib/plugins/reports/
install -m 755 src/database_cache/tests/.libs/record_b.so /opt/ear/lib/plugins/reports/
```

Use the privileges appropriate for your installation. Restart the test daemon
you manage after installing/configuring plugins; an existing process does not
pick them up merely because files or `ear.conf` changed.

Initialization records the identity and daemon PID. Application callbacks
record job/step IDs, node, application name, MPI/learning flags and frequency.
Periodic-metric callbacks record job/step IDs, node, timestamps and energy.
Each record is flushed before the callback returns; I/O failures return
`EAR_ERROR`. Other report callbacks record their name/count.

## Send and verify

From the project root, replace the endpoint and directory with your instance's
values:

```sh
make -C src/database_cache/tests check-reporting \
  REPORTING_ARGS='--host localhost --port 50002 --records-dir /path/to/ear-tmp --plugins A B'
```

The runner sends three applications (MPI, non-MPI and learning) and two periodic
metrics through `eardbd_connect`/`eardbd_send_*`/`eardbd_disconnect`. It requires
each selected plugin to record the exact five payloads once and prints one
`PASS` line per plugin. Both plugin initialization and actual delivery are
checked. Send success alone is not proof of reporting success.

Use `--plugins A` or `--plugins B` when only that recorder is configured. To
check a different configuration, change/restart the daemon yourself and repeat
the command with the corresponding identities. This checks the requested
recorders' delivery, not the absence of every other loaded plugin.

The runner generates a fresh job ID, prints it, ignores unrelated/older records,
and never truncates the TSV files. `--job-id NUMBER` allows an explicit unused
ID. It polls files only: additional connections from the same source IP can
displace the sender in EARDBD. The test's own connection can also displace an
existing client from that IP, so run it from a dedicated client host/address
when other clients are active.

`--timeout` defaults to 90 seconds for delivery plus `--settle` (default two
seconds) to observe duplicate delivery. Set timeout above the daemon's insertion
interval plus the settling duration; for example `--timeout 100 --settle 60`
with a 30-second insertion interval. Duplicate detection is bounded by this
observation period. The sender has a separate `--client-timeout` of ten seconds.

For a remote daemon, run the verifier where its output directory is readable,
such as through a shared filesystem. The client also works independently:

```sh
src/database_cache/tests/reporting_client send HOST PORT UNUSED_JOB_ID
src/database_cache/tests/reporting_client status HOST PORT
```

The send command only sends; it does not verify TSV output. Do not run the status
command concurrently with sending from the same IP.

Python 3 is required only for verification. You can run it directly without
invoking the build:

```sh
python3 src/database_cache/tests/reporting_tests.py \
  --client src/database_cache/tests/reporting_client \
  --host HOST --port PORT --records-dir /path/to/ear-tmp --plugins A B
```

If verification fails, report the printed job ID, command output and the
matching rows from the selected TSV files, together with relevant daemon logs.
Startup failures, mutation tests, database persistence and daemon lifecycle
management are outside this suite's scope.

## Missing recording files

The plugin creates its TSV file during `report_init`, before the sender runs.
The runner checks for that file first; a missing-file error means it has not
sent any test traffic yet. Do not create empty TSV files to work around it.

Check the configuration actually selected by the daemon's `EAR_ETC`, especially
`EARDBDReportPlugins`, `InstDir` and `TmpDir`. For example, `TmpDir=/tmp/ear`
requires `--records-dir /tmp/ear`, not `/tmp`. Check that the installed plugins
are `.so` files and that the daemon can load them and write its output directory.
New plugin diagnostics report their output path or the reason opening it failed.

Run Bash deployment scripts with `bash` (or their shebang), rather than `sh`,
when they use `&>` redirection. Inspect the startup output from the same launch;
an older running daemon does not prove that a newly configured launch succeeded.
The runner reads the local filesystem: for containers, remote hosts or private
temporary directories, expose the daemon's recording directory to the verifier.
