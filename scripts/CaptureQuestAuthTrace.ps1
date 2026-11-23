[CmdletBinding()]
param(
    [string]$AdbPath = "adb",
    [string]$OutputPath = (Join-Path (Get-Location) ("quest-auth-" + (Get-Date -Format "yyyyMMdd-HHmmss") + ".log"))
)

$ErrorActionPreference = "Stop"

if ($AdbPath -eq "adb" -and -not (Get-Command adb -ErrorAction SilentlyContinue)) {
    $localAdb = Join-Path $env:TEMP "beattogether-1453-tools/platform-tools/adb.exe"
    if (Test-Path -LiteralPath $localAdb -PathType Leaf) { $AdbPath = $localAdb }
}
$state = @(& $AdbPath get-state)
if ($LASTEXITCODE -ne 0 -or $state -notcontains "device") {
    throw "Connect and authorize one Quest over USB before recording."
}

$logPath = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
if (Test-Path -LiteralPath $logPath) { throw "The output file already exists. Choose a new path." }
Set-Content -LiteralPath $logPath -Value "Quest authentication trace started $((Get-Date).ToString('o'))"

Write-Host "Recording filtered Quest events to $logPath"
Write-Host "Start Beat Saber, open Multiplayer, and try Create Server. Press Ctrl+C when the result appears."

$logLine = '^(?<time>\d\d-\d\d\s+\d\d:\d\d:\d\d\.\d+)\s+(?<pid>\d+)\s+\d+\s+[VDIWEF]\s+(?<tag>[^:]+):\s*(?<body>.*)$'
$systemClues = '(?i)\b(?:DSAT|UnknownHostException|SSLHandshakeException|SocketTimeoutException|ProtocolException|ECONNREFUSED|ENETUNREACH|ETIMEDOUT|DNS lookup failed|certificate verify failed|TLS handshake failed|permission denied|binder died|DeadObjectException)\b'

& $AdbPath logcat -b main -b system -b events -v threadtime -T 1 |
    ForEach-Object {
        $parsed = [regex]::Match($_, $logLine)
        if (!$parsed.Success) { return }

        $tag = $parsed.Groups['tag'].Value.Trim()
        $body = $parsed.Groups['body'].Value
        $event = $null
        if ($tag -eq 'MultiplayerCore' -and $body -match '\[QUEST-AUTH\] (.*)$') {
            $event = "[QUEST-AUTH] $($Matches[1])"
        } elseif ($tag -eq 'MultiplayerCore' -and $body -match 'Connection Failed Reason: (CFR-\d+ - \w+)') {
            $event = $Matches[1]
        } elseif ($tag -eq 'BeatTogether' -and $body -match 'BeatTogether placement returned (code|HTTP) (\d+)') {
            $event = "BeatTogether placement: $($Matches[1]) $($Matches[2])"
        } elseif ($tag -eq 'BeatTogether' -and $body -match 'Requesting a lobby using the legacy BeatTogether placement API') {
            $event = 'BeatTogether lobby request sent'
        } elseif ($tag -match '(?i)^(?:OsSdkLoader|OculusPlatform|OVRPlatform|PlatformService|ConnectivityService|DnsResolver|Unity|scotland2)(?:[|_-]\S+)?$' -or
                  $body -match '(?i)\b(?:oculus|meta platform|graph\.oculus\.com|user proof)\b') {
            $clues = [regex]::Matches($body, $systemClues) | ForEach-Object Value | Select-Object -Unique
            if ($clues) { $event = "System clues: $($clues -join ', ')" }
        }

        if ($event) {
            $line = "$($parsed.Groups['time'].Value) PID $($parsed.Groups['pid'].Value) $event"
            Add-Content -LiteralPath $logPath -Value $line
            Write-Output $line
        }
    }
