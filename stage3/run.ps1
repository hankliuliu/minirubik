# Run ELFs on Ripes (CLI) and append iret, cycles and console output to
# ripes-results.csv.  Usage: .\run.ps1 build\v1_worst.elf [-Proc RV32_5S]
param(
    [Parameter(Mandatory)] [string[]] $Elf,
    [string] $Proc = 'RV32_ISS'
)
$ripes = 'C:\Users\LIU YEN-CHENG\Tools\Ripes\Ripes.exe'
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$csv = Join-Path $dir 'ripes-results.csv'
if (-not (Test-Path $csv)) {
    'elf,proc,iret,cycles,exectime_ms,console' | Set-Content -Encoding ascii $csv
}
foreach ($e in $Elf) {
    # Ripes gets a local copy: the repo may sit on a \\wsl.localhost path.
    $path = Join-Path $env:TEMP (Split-Path -Leaf $e)
    Copy-Item -Force (Join-Path $dir $e) $path
    $json = Join-Path $env:TEMP 'ripes_s3.json'
    $con = Join-Path $env:TEMP 'ripes_s3.txt'
    Remove-Item $json, $con -ErrorAction SilentlyContinue
    $p = Start-Process -FilePath $ripes -PassThru -WindowStyle Hidden `
        -RedirectStandardOutput $con -ArgumentList ("--mode cli --src `"$path`" -t elf " +
        "--proc $Proc --iret --cycles --exectime --json --output `"$json`"")
    $p.WaitForExit()
    $j = Get-Content $json -Raw | ConvertFrom-Json
    $out = ((Get-Content $con) -join ' | ') -replace ',', ';'
    $line = '{0},{1},{2},{3},{4},{5}' -f (Split-Path -Leaf $e), $Proc,
        $j.'# instructions retired', $j.cycles, $j.'execution time (ms)', $out
    Add-Content -Encoding ascii $csv $line
    $line
}
