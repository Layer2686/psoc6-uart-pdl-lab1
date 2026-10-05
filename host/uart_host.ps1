param(
    [string]$Port = 'COM4',
    [ValidatePattern('^[A-Z]{1,63}$')]
    [string]$Message = 'LAZOR',
    [ValidateRange(1, 1000)]
    [int]$Repeat = 1,
    [ValidateRange(100, 30000)]
    [int]$TimeoutMs = 2000,
    [switch]$Loopback
)

$ErrorActionPreference = 'Stop'
$encoding = [System.Text.Encoding]::ASCII
$baudrate = 38400
$packet = $encoding.GetBytes($Message + "`r")
$expectedAck = 'ACK:' + $Message + "`n"
$serial = $null

function ConvertTo-HexString([byte[]]$Bytes) {
    return (($Bytes | ForEach-Object { $_.ToString('X2') }) -join ' ')
}

Write-Output 'Lab 1, variant 8: 38400 baud, 8N1, no flow control'
Write-Output ('Bit time: {0:F3} us; frame time: {1:F3} us' -f
    (1e6 / $baudrate), (10e6 / $baudrate))

try {
    if ($Loopback) {
        Write-Output 'Mode: software loopback (no COM port or board involved)'
    }
    else {
        $serial = [System.IO.Ports.SerialPort]::new(
            $Port, $baudrate, [System.IO.Ports.Parity]::None,
            8, [System.IO.Ports.StopBits]::One)
        $serial.Handshake = [System.IO.Ports.Handshake]::None
        $serial.DtrEnable = $false
        $serial.RtsEnable = $false
        $serial.ReadTimeout = $TimeoutMs
        $serial.WriteTimeout = $TimeoutMs
        $serial.Open()
        # Consume the terminal banner before starting the request/ACK protocol.
        Start-Sleep -Milliseconds 100
        $serial.DiscardInBuffer()
        Write-Output "Port: $Port"
    }

    for ($exchange = 1; $exchange -le $Repeat; ++$exchange) {
        Write-Output "--- exchange $exchange/$Repeat ---"
        Write-Output "TX text: '$Message'"
        Write-Output "TX bytes ($($packet.Length)): $(ConvertTo-HexString $packet)"

        if ($Loopback) {
            $stream = [System.IO.MemoryStream]::new()
            try {
                $stream.Write($packet, 0, $packet.Length)
                $stream.Position = 0
                $received = [byte[]]::new($packet.Length)
                $count = $stream.Read($received, 0, $received.Length)
                if (($count -ne $packet.Length) -or
                    ($encoding.GetString($received) -cne ($Message + "`r"))) {
                    throw 'Software loopback did not return the complete packet.'
                }
            }
            finally {
                $stream.Dispose()
            }
            Write-Output "RX bytes ($count): $(ConvertTo-HexString $received)"
            Write-Output 'Verify: OK (software loopback only)'
            continue
        }

        $serial.Write($packet, 0, $packet.Length)
        $received = [System.Collections.Generic.List[byte]]::new()
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        while ($true) {
            $remaining = $TimeoutMs - [int]$timer.ElapsedMilliseconds
            if ($remaining -le 0) {
                throw 'Timed out waiting for ACK. Check firmware, COM port and 38400/8N1.'
            }
            $serial.ReadTimeout = $remaining
            $byte = $serial.ReadByte()
            $received.Add([byte]$byte)
            if ($received.Count -gt 128) {
                throw 'Response exceeds the expected packet size.'
            }
            if ($byte -eq 10) { break }
        }
        $timer.Stop()
        $response = $encoding.GetString($received.ToArray())
        Write-Output "RX text: '$($response.TrimEnd([char]10))'"
        Write-Output "RX bytes ($($received.Count)): $(ConvertTo-HexString $received.ToArray())"
        Write-Output ('Calculated wire times: TX {0:F4} ms, RX {1:F4} ms' -f
            ($packet.Length * 10e3 / $baudrate),
            ($received.Count * 10e3 / $baudrate))
        if ($response -cne $expectedAck) {
            throw "Unexpected response; expected ACK:$Message followed by LF."
        }
        Write-Output 'Verify: OK'
    }
}
finally {
    if ($null -ne $serial) { $serial.Dispose() }
}
