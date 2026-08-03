$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$repository = Split-Path -Parent $PSScriptRoot
$source = Join-Path $repository 'apps\radio\assets\toribio-brand-source.png'

function Export-Rgba {
    param(
        [string]$Destination,
        [int]$Width,
        [int]$Height
    )

    $inputImage = [System.Drawing.Image]::FromFile($source)
    try {
        $bitmap = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.Clear([System.Drawing.Color]::Transparent)
                $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.DrawImage($inputImage, 0, 0, $Width, $Height)
            }
            finally { $graphics.Dispose() }

            $bytes = New-Object byte[] ($Width * $Height * 4)
            $position = 0
            for ($y = 0; $y -lt $Height; ++$y) {
                for ($x = 0; $x -lt $Width; ++$x) {
                    $pixel = $bitmap.GetPixel($x, $y)
                    $bytes[$position++] = $pixel.R
                    $bytes[$position++] = $pixel.G
                    $bytes[$position++] = $pixel.B
                    $bytes[$position++] = $pixel.A
                }
            }
            [System.IO.File]::WriteAllBytes($Destination, $bytes)
        }
        finally { $bitmap.Dispose() }
    }
    finally { $inputImage.Dispose() }
}

Export-Rgba (Join-Path $repository 'apps\radio\assets\toribio_watermark_tv.rgba') 300 87
Export-Rgba (Join-Path $repository 'apps\radio\assets\toribio_watermark_drc.rgba') 200 58
