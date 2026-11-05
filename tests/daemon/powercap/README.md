# EARD powercap test suite

Tests for `src/daemon/powercap/` in isolation: EARGM is simulated, and the
hardware is faked with [ear-fake-devices](../../../../ear-fake-devices)
(`$FAKEDEV`, default `~/ear-fake-devices`).

```sh
make unit          # needs only a configured tree (config_install.h)
make integration   # needs a built tree + ear-fake-devices built (make there)
make check         # both
PC_ONLY=greedy ./test_node   # run the tests whose name contains "greedy"
```

Checks print `PASS`, `FAIL`, `XFAIL [#nnn]` or `XPASS [#nnn]`. An XFAIL is a
known defect: the check states the correct behaviour and is expected to fail
today, tagged with the issue that tracks it. A binary exits non-zero on any
FAIL or XPASS (XPASS means a bug was fixed: promote the XFAIL to a CHECK).
Every test runs in a forked child, so the file-scope state of the code under
test starts clean each time, and crashes are caught and reported.

## Layout

```
            EARGM (simulated: eargm_sim.c)
              | get_status / set_opt / release_idle / set_powerlimit / red / inc / reset
              v
  powercap.c           node state machine   <- test_node, test_monitor (fake_pmgt below it)
              |
  powercap_mgt.c       split between domains <- test_mgt (fake plugins + fake discovery)
              |
  cpu_generic.so  gpu.so                     <- test_cpu_plugin, test_gpu_plugin (fake device APIs)
              |
  libmgt / libmetrics  intel63 RAPL, NVML
              |
  ear-fake-devices     libfakesys (MSRs, 2 sockets), libfakenvml (per-GPU caps/util)
                                             <- test_integration: everything real above this line
```

| Binary | Code under test | Stands in for the rest |
|---|---|---|
| `test_status` | `powercap_status.c`, `common/types/powercap.c` | nothing (pure functions) |
| `test_node` | `powercap.c` | `fake_pmgt.c`, `fake_powermon.c`, `eargm_sim.c` |
| `test_monitor` | `powercap.c` built with `POWERCAP_MON=1` | same |
| `test_mgt` | `powercap_mgt.c` | `fake_hw_mgt.c`: four in-process plugins, topology, TDPs, GPU readings |
| `test_gpu_plugin` | `gpu.c` | `fake_dev_api.c` (`FAKE_GPU_SIDE`) |
| `test_cpu_plugin[_asan]` | `cpu_generic.c` | `fake_dev_api.c` (`FAKE_CPU_SIDE`); ASan build catches reads past the P-state list |
| `test_dvfs_plugin` | `dvfs.c`, the **default** CPU plugin, closed loop (power follows P-state) | `fake_dev_api.c` (`FAKE_CPU_SIDE`) |
| `test_gpu_dvfs_plugin[_asan]` | `gpu_dvfs.c`, the **default** GPU plugin, closed loop (power follows clock) | `fake_dev_api.c` (`FAKE_GPU_SIDE`) |
| `test_inm_plugin`, `test_grace_hopper_plugin`, `test_gpu_dummy_plugin` | `inm.c`, `grace_hopper.c`, `gpu_dummy.c`, called through the manager's function-pointer types | `execute()` stub records the commands |
| `test_integration` | all of the above plus the real `.so` plugins and EAR libs | fake hardware, fake power monitor, simulated EARGM |

`common_stubs.c` replaces the monitor thread with a recorder:
subscriptions are stored and the tests fire `call_main` by hand, so the
util-detection and plugin threads are deterministic in the unit tier.
`fake_clock_ns` freezes time where power is computed from energy.

`eargm_sim.c` performs, for one node, what the EARD side of each EARGM request
does (the `dyncon_*` handlers of `daemon/remote_api/dynamic_configuration.c`).
`process_pmgt_status()` and `dyncon_filter_opt()` are verbatim copies; keep
them in sync.

## Integration rigs

`run_integration.sh` runs `test_integration` once per rig. Each rig gets its
own fake hardware: 2 sockets (PKG TDP 200 W, DRAM TDP 40 W) and

| Rig | GPUs | Utilization |
|---|---|---|
| `gpu4-mixed` | 4 | 100 / 50 / 25 / 0 % |
| `gpu4-equal` | 4 | 80 % each |
| `gpu2` | 2 | 90 / 30 % |
| `cpu-only` | 0 | - |
| `gpu4-busy-at-start` | 4 | 80 % already when EARD starts |
| `dvfs-gpu4` | 4 | 80 %, with the default plugins `dvfs.so` + `gpu_dvfs.so` |
| `dvfs-cpu-only` | 0 | `dvfs.so` alone |
| `enforce-cpu-only` | 0 | `cpu_generic.so`, RAPL enforcing its caps |
| `enforce-gpu4` | 4 | 80 %, `cpu_generic.so` + `gpu.so`, RAPL and GPU limits enforced |
| `limits-cpu-only`, `limits-gpu4` | 0 / 4 | as `enforce-*`, plus the hardware power ranges: PKG 60..250 W, DRAM 10..60 W, GPU 100..400 W; caps below the minimum / above the maximum (#788) |

The first five rigs use `cpu_generic.so` + `gpu.so`. The `dvfs-*` rigs turn on
fakesys's cpufreq sysfs (the acpi-cpufreq files EAR's frequency layer drives)
with RAPL PKG power falling 50 W per GHz, and fakenvml's `power_model = clock`
(50 W + 0.2 W/MHz). Those plugins cap by frequency, so their oracle is the
measured power (RAPL energy counters and NVML power usage) rather than a
programmed cap. The `enforce-*` rigs add fakesys `rapl_enforce` (PKG/DRAM power
held at the PL1 limit EAR programmed, CPUs slowed accordingly) and fakenvml
`enforce_limit`, so `cpu_generic.so` and `gpu.so` run closed-loop: the tests
check their status against measured power and that the measured node power
stays under the node cap through EARGM periods. `RIGS="dvfs-gpu4 ..."` runs a
subset.

The GPUs start idle and load up after 6 s, like a job starting on a node where
EARD is already up. The oracle is the fake hardware itself: after each action
the test reads the RAPL power-limit MSRs (`/dev/cpu/N/msr` 0x610/0x618) and the
per-GPU NVML limits back, independently of EAR, and prints them:

```
hw after util detection   PKG   23   23  DRAM   4   4  GPU  400  100  400  100   (total 1054)
```

Requirements on the EAR tree:

- configured with `--with-cuda=$FAKEDEV/fake-cuda` (a minimal `nvml.h` that
  compiles EAR's NVML support; `libfakenvml.so` is loaded at run time through
  `HACK_NVML_FILE`) and `CC_FLAGS=-march=native`;
- inm.so and grace_hopper.so have no integration rig: they shell out to
  `ipmitool` / `nvidia-smi`, and until their ABI is fixed (#768, #769) a rig
  would show nothing the unit tests don't;
- built with plain `make` (parallel builds of this tree are racy);
- fakesys runs with `fake_uid = 0`, because EAR only exposes GPU caps to root
  (`nvml_is_privileged`).

`IT_VERB=4` raises EAR verbosity; `IT_LOGDIR=<dir>` keeps each rig's stderr.
