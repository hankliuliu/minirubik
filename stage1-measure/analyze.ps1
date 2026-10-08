# Summarize results.csv: memory slope (measurement A) and simulation rate
# (measurement B). Medians are used throughout; min-max shows the spread.
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rows = Import-Csv (Join-Path $dir 'results.csv')

function Median($xs) {
    $s = @($xs | Sort-Object { [double]$_ })
    if ($s.Count % 2) { [double]$s[($s.Count - 1) / 2] }
    else { ([double]$s[$s.Count / 2 - 1] + [double]$s[$s.Count / 2]) / 2 }
}

# Least-squares slope and intercept of y on x.
function Fit($pts) {
    $n = $pts.Count
    $mx = ($pts | Measure-Object x -Average).Average
    $my = ($pts | Measure-Object y -Average).Average
    $sxy = 0; $sxx = 0; $syy = 0
    foreach ($p in $pts) {
        $sxy += ($p.x - $mx) * ($p.y - $my)
        $sxx += ($p.x - $mx) * ($p.x - $mx)
        $syy += ($p.y - $my) * ($p.y - $my)
    }
    $b = $sxy / $sxx
    [pscustomobject]@{ slope = $b; intercept = $my - $b * $mx; r2 = $sxy * $sxy / ($sxx * $syy) }
}

"=== A. Host bytes per guest byte (RV32_ISS, polled runs) ==="
foreach ($op in 'sw', 'sb') {
    $pts = foreach ($g in ($rows | Where-Object { $_.src -like "fill_${op}_*" -and $_.proc -eq 'RV32_ISS' -and $_.peak_ws_bytes -ne '0' } | Group-Object src)) {
        $kib = [int]($g.Name -replace '\D', '')
        if ($kib -lt 1024) { continue }   # 64 KiB runs are too short to poll reliably
        [pscustomobject]@{
            kib = $kib; x = $kib * 1024
            y = Median ($g.Group.peak_ws_bytes); priv = Median ($g.Group.peak_private_bytes)
        }
    }
    $pts = $pts | Sort-Object kib
    $ws = Fit $pts
    $pv = Fit ($pts | ForEach-Object { [pscustomobject]@{ x = $_.x; y = $_.priv } })
    "{0}: sizes(KiB)={1}" -f $op, (($pts.kib) -join ',')
    "    working set : {0:N2} B/B  intercept {1:N1} MiB  r2={2:N6}" -f $ws.slope, ($ws.intercept / 1MB), $ws.r2
    "    private     : {0:N2} B/B  intercept {1:N1} MiB  r2={2:N6}" -f $pv.slope, ($pv.intercept / 1MB), $pv.r2
    if ($op -eq 'sw') { $wsSlope = $ws.slope; $pvSlope = $pv.slope }
}
$peak = 18405414
"Linear projection of the 18,405,414-byte baseline peak (an upper bound; see README):"
"    working set : {0:N0} B = {1:N2} GiB" -f ($peak * $wsSlope), ($peak * $wsSlope / 1GB)
"    private     : {0:N0} B = {1:N2} GiB" -f ($peak * $pvSlope), ($peak * $pvSlope / 1GB)

""
"=== B. Retired instructions per second (timing runs, no polling) ==="
'{0,-18} {1,-9} {2,10} {3,10} {4,8} {5,22} {6,12} {7,14}' -f 'src', 'proc', 'iret', 'cycles', 'med ms', 'instr/s med (range)', 'cycles/s', '1e9 instr at med'
foreach ($g in ($rows | Where-Object { $_.peak_ws_bytes -eq '0' } | Group-Object proc, src)) {
    $r0 = $g.Group[0]; $iret = [double]$r0.iret; $cyc = [double]$r0.cycles
    $ms = @($g.Group.exectime_ms | ForEach-Object { [double]$_ })
    $med = Median $ms
    $lo = $iret / (($ms | Measure-Object -Maximum).Maximum / 1000)
    $hi = $iret / (($ms | Measure-Object -Minimum).Minimum / 1000)
    $rate = $iret / ($med / 1000)
    $t = 1e9 / $rate
    $tstr = if ($t -ge 3600) { '{0:N1} h' -f ($t / 3600) } else { '{0:N1} min' -f ($t / 60) }
    '{0,-18} {1,-9} {2,10} {3,10} {4,8} {5,22} {6,12} {7,14}' -f $r0.src, $r0.proc, $iret, $cyc, $med,
        ('{0:N0} ({1:N0}-{2:N0})' -f $rate, $lo, $hi), ('{0:N0}' -f ($cyc / ($med / 1000))), $tstr
}
