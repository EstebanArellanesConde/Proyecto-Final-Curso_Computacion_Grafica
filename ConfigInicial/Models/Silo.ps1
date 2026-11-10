Add-Type -AssemblyName System.Drawing

Get-ChildItem "Silo_*.jpg" | ForEach-Object {
    try {
        $img = [System.Drawing.Image]::FromFile($_.FullName)

        [PSCustomObject]@{
            Archivo = $_.Name
            TamanoMB = [math]::Round($_.Length / 1MB, 2)
            Ancho = $img.Width
            Alto = $img.Height
            Formato = $img.RawFormat.Guid
            Estado = "OK"
        }

        $img.Dispose()
    }
    catch {
        [PSCustomObject]@{
            Archivo = $_.Name
            TamanoMB = [math]::Round($_.Length / 1MB, 2)
            Ancho = "-"
            Alto = "-"
            Formato = "-"
            Estado = "ERROR: $($_.Exception.Message)"
        }
    }
}