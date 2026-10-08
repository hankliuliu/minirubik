# Run one long Ripes job and log the host memory of the Ripes process once a
# second, so that growth and any release (e.g. after the guest's free()) show
# up over time, not only as a final peak.
param(
    [Parameter(Mandatory)] [string] $Src,
    [string] $Type = 'elf',
    [string] $Proc = 'RV32_ISS'
)
$ripes = 'C:\Users\LIU YEN-CHENG\Tools\Ripes\Ripes.exe'
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$name = [IO.Path]::GetFileNameWithoutExtension($Src) + "_$Proc"
$path = (Resolve-Path (Join-Path $dir $Src)).Path
$report = Join-Path $dir "$name.report.json"
$console = Join-Path $dir "$name.console.txt"
$log = Join-Path $dir "$name.timeline.csv"

't_s,ws_bytes,peak_ws_bytes,private_bytes' | Set-Content -Encoding ascii $log
$argLine = "--mode cli --src `"$path`" -t $Type --proc $Proc " +
           "--iret --cycles --exectime --json --output `"$report`""
$t0 = Get-Date
$p = Start-Process -FilePath $ripes -ArgumentList $argLine -PassThru `
        -RedirectStandardOutput $console -WindowStyle Hidden
while (-not $p.HasExited) {
    try {
        $p.Refresh()
        '{0:F1},{1},{2},{3}' -f ((Get-Date) - $t0).TotalSeconds, $p.WorkingSet64,
            $p.PeakWorkingSet64, $p.PrivateMemorySize64 |
            Add-Content -Encoding ascii $log
    } catch {}
    Start-Sleep -Milliseconds 1000
}
"wall clock: {0:N1} s" -f ((Get-Date) - $t0).TotalSeconds
"--- console ---"; Get-Content $console
"--- report ---"; Get-Content $report
