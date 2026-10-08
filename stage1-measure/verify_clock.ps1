# Two checks on the host CPU clock, independent of Ripes' own behaviour.
#
# 1. Native control: run the native minirubik solver (WSL) back to back while
#    sampling Windows' "% Processor Performance" (100 = base 2.8 GHz). If the
#    native run time tracks the counter the same way the Ripes rate did, the
#    variation belongs to the machine, not to Ripes.
# 2. Warm-up: run the same short RV32_SS program from an idle CPU ("cold",
#    after 3 s of idle) and right after 2 s of busy work ("warm"). If warm
#    runs are faster, short runs are penalised by clock ramp-up, which would
#    explain why longer SS programs showed a higher rate.
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ripes = 'C:\Users\LIU YEN-CHENG\Tools\Ripes\Ripes.exe'

"=== 1. native solver x150, with % Processor Performance ==="
$job = Start-Job { (Get-Counter '\Processor Information(_Total)\% Processor Performance' -SampleInterval 1 -MaxSamples 25).CounterSamples | ForEach-Object { [math]::Round($_.CookedValue) } }
$wslDir = wsl.exe -d Ubuntu-24.04 -- wslpath -a ($dir -replace '\\', '/')
$native = wsl.exe -d Ubuntu-24.04 -- bash "$wslDir/native_loop.sh" 150
"native ms (150 runs, in order): " + ($native -join ' ')
"perf %   : " + ((Receive-Job $job -Wait) -join ' ')

"=== 2. RV32_SS alu_20000.s, cold vs warm ==="
function RunSS {
    $o = Join-Path $env:TEMP 'ss.json'
    $p = Start-Process $ripes -ArgumentList "--mode cli --src `"$dir\alu_20000.s`" -t asm --proc RV32_SS --iret --exectime --json --output `"$o`"" -PassThru -WindowStyle Hidden
    $p.WaitForExit()
    (Get-Content $o -Raw | ConvertFrom-Json).'execution time (ms)'
}
$cold = @(); $warm = @()
for ($i = 0; $i -lt 6; ++$i) {
    Start-Sleep -Seconds 3
    $cold += RunSS
    $t = (Get-Date).AddSeconds(2); while ((Get-Date) -lt $t) { }   # busy 2 s
    $warm += RunSS
}
"cold ms: " + ($cold -join ' ')
"warm ms: " + ($warm -join ' ')
