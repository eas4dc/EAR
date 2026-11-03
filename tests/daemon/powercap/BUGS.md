# EARD powercap: defects found by this suite

Every entry is pinned by at least one `XFAIL("PC-nn", ...)` check (or, for
defects first reported in an issue, `XFAIL("#nnn", ...)`) that states
the **correct** behaviour. Today these checks fail as expected (XFAIL). Once a
fix lands, the check reports XPASS and fails the run on purpose: turn that
XFAIL into a CHECK in the same change.

Paths are relative to `src/`. Severity: **crash** (EARD dies), **high** (a
node ends up uncapped or over its cap, or EARGM's accounting goes wrong),
**medium** (power distributed wrongly, but the node stays within its cap),
**low** (cosmetic, or a latent path).

| ID | Issue | Sev. | Where | Defect | Shown by |
|---|---|---|---|---|---|

## Existing issues pinned by tag

Checks tagged with an issue number pin defects that were already filed:

| Tag | What the check shows | Test |
|---|---|---|

## Hardware power limits (#788)

Implemented: device caps are clamped to RAPL POWER_INFO / NVML ranges, the node
never applies a cap under its working minimum (max of the hardware minimum and
ear.conf `MIN_WORKING_POWER`) and offers back what the devices can never use
(rigs `limits-*`). Still open, for the improvement track: an explicit "cannot
comply" flag to EARGM, which today only sees the raised cap.

## Found by review, not asserted

Filed in #776 (items 9-11) and #770/#774 where noted. The busy-spin locks,
the intel63 reset, the gpu.c return codes, the greedy entry sizes and the
missing `<pthread.h>` are fixed; still open:

- `metrics/common/nvml.h`: EAR's fallback `nvmlMemory_v2_t` has no `version` field; real NVML answers `NVML_ERROR_ARGUMENT_VERSION_MISMATCH`, so GPU memory size is never known.
