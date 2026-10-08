# Run one or more .s files on Ripes and record iret, exectime, and the host
# memory peak of the Ripes process. Results are appended to results.csv.
#
# Ripes is a GUI-subsystem exe, and once it exits Windows no longer exposes
# its peak memory to PowerShell, so we poll the process while it runs.
# PeakWorkingSet64 is monotonic, so the last sample is the peak (to within
# one 20 ms polling interval).
param(
    [Parameter(Mandatory)] [string[]] $Src,
    [string] $Proc = 'RV32_ISS',
    [int] $Reps = 3,
    [switch] $NoPoll   # timing runs: don't let the poller share the CPU
)
$ripes = 'C:\Users\LIU YEN-CHENG\Tools\Ripes\Ripes.exe'
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$csv = Join-Path $dir 'results.csv'
if (-not (Test-Path $csv)) {
    'src,proc,rep,iret,cycles,exectime_ms,peak_ws_bytes,peak_private_bytes' |
        Set-Content -Encoding ascii $csv
}

foreach ($s in $Src) {
    $path = (Resolve-Path (Join-Path $dir $s)).Path
    for ($r = 1; $r -le $Reps; ++$r) {
        $out = Join-Path $env:TEMP "ripes_out.json"
        Remove-Item $out -ErrorAction SilentlyContinue
        $argLine = "--mode cli --src `"$path`" -t asm --proc $Proc " +
                "--iret --cycles --exectime --json --output `"$out`""
        $p = Start-Process -FilePath $ripes -ArgumentList $argLine -PassThru -WindowStyle Hidden
        $ws = 0; $priv = 0
        if ($NoPoll) { $p.WaitForExit() }
        while (-not $p.HasExited) {
            try {
                $p.Refresh()
                if ($p.PeakWorkingSet64 -gt $ws) { $ws = $p.PeakWorkingSet64 }
                if ($p.PeakPagedMemorySize64 -gt $priv) { $priv = $p.PeakPagedMemorySize64 }
            } catch {}
            Start-Sleep -Milliseconds 20
        }
        $j = Get-Content $out -Raw | ConvertFrom-Json
        $line = '{0},{1},{2},{3},{4},{5},{6},{7}' -f $s, $Proc, $r,
            $j.'# instructions retired', $j.cycles, $j.'execution time (ms)', $ws, $priv
        Add-Content -Encoding ascii $csv $line
        $line
    }
}


