# Stage 1 measurements

Environment for the recorded data:

- Ripes `v2.2.6-106-g5b8a616` (continuous prerelease, commit 5b8a616), Windows build
- Windows 10 Home 19045, Intel i7-1165G7 (4C/8T, base 2.8 GHz), 15.7 GB RAM
- AC power, power plan "ASUS Recommended", power-mode slider at default
- minirubik fork base: upstream 231796cc48868f4ea276f652139b6bebbad0cd02
- RV32 toolchain: Ubuntu 24.04 `gcc-riscv64-unknown-elf` 13.2 + picolibc 1.8.6,
  unpacked into `~/opt/rv` with `apt-get download` + `dpkg -x`

The PowerShell scripts run on Windows and expect Ripes at
`C:\Users\LIU YEN-CHENG\Tools\Ripes\Ripes.exe`; edit `$ripes` to match.

## Files

| File | Purpose |
| :--- | :--- |
| `gen.ps1` | generates every `.s` program below |
| `run.ps1` | runs `.s` files on Ripes, appends iret, cycles, exectime, host peak memory to `results.csv` |
| `analyze.ps1` | memory slope (least squares) and simulation rates (medians) |
| `results.csv` | raw data, one row per run |
| `fill_sw_<KiB>.s`, `fill_sb_<KiB>.s` | write N fresh bytes with `sw` / `sb` |
| `readfresh_<KiB>.s` | `lw` over N bytes that were never written |
| `alu_<K>.s` | K iterations of `addi`/`bne`, no memory access |
| `rewrite_<K>.s`, `reread_<K>.s` | K stores / loads cycling over the same 4 KiB |
| `empty.s` | exit only: fixed cost of `--exectime` |
| `unroll_<K>.s`, `nottaken_<K>.s` | taken vs. not-taken branch cost on `RV32_5S` |
| `bigmap_readbig.s`, `bigmap_readsmall.s` | same loads after touching 16 MiB, over 16 MiB vs. 4 KiB |
| `verify_clock.ps1`, `native_loop.sh` | CPU clock checks: native solver timing, SS cold vs. warm |
| `baseline_ripes.c` | wrapper that runs the unmodified `solver.c` on Ripes |
| `build_baseline.sh` | builds it in WSL, with a Ripes-compatible linker script (`ripes.ld`) |
| `baseline_21345671111111.dis` | disassembly of that build; source of the instruction counts |
| `predict_iret.py` | predicts the BFS loop's instruction count from the disassembly |
| `run_timeline.ps1` | long run with host memory logged once a second |
| `baseline_21345671111111_RV32_ISS.*` | console output, Ripes report and memory timeline of the baseline run |
| `baseline_hung_run.timeline.csv` | memory timeline of the first run, which spun in crt0 after `main` |
| `bad_report.json` | Ripes report of an invalid-input build: it stops after 1,029 instructions, confirming `_exit` ends the run |

## Reproduce

```powershell
.\gen.ps1
# A. memory (polled, so host peak is captured)
.\run.ps1 -Reps 3 -Src fill_sw_1024.s,fill_sw_1536.s,fill_sw_2048.s,fill_sw_3072.s,fill_sw_4096.s
# B. rate (no polling, so the poller does not share the CPU)
.\run.ps1 -NoPoll -Reps 5 -Src alu_2000000.s,fill_sw_4096.s,rewrite_1000000.s,reread_1000000.s
.\run.ps1 -NoPoll -Reps 5 -Proc RV32_5S -Src alu_20000.s,fill_sw_64.s,rewrite_20000.s
.\analyze.ps1
# baseline: build in WSL, run on Windows (about 15-20 min on RV32_ISS)
wsl -d Ubuntu-24.04 -- bash ./build_baseline.sh
.\run_timeline.ps1 -Src baseline_21345671111111.elf
```

Notes:

- Host peak is `PeakWorkingSet64` (resident RAM) and `PeakPagedMemorySize64`
  (private commit), sampled every 20 ms while Ripes runs; Windows no longer
  exposes them once the process exits.
- Host memory is 48 B per guest byte plus 16 B per hash bucket, with the
  bucket count a power of two: 64-80 B per guest byte. The linear slope
  (80.3 B/B over power-of-two sizes) is therefore an upper bound.
- The CPU clock on this laptop varies by up to 5x under sustained load
  (`alu_4000000.s` fell from 12.7 M/s to 2.4 M/s over 12 runs while
  `% Processor Performance` fell to 23-33%), so timings are medians of 5
  with min-max ranges, and comparisons are interleaved.
