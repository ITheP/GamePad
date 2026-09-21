# Display information
$info = @"
rrefontgen [pbm file] [char w] [char h] <fontName> <fontMode> <overlap> <optim>

pbm file       = source file of .pbm image
w              = width of character in pixels
h              = height of character in pixels
fileMode       = 1 (0-header,1-pbm,2-xml)
fontMode       = 0 (0-rect,1-vert,2-horiz)
overlap        = 0 (0-none,1-normal,2-less/optimal)
optim          = 0 (0-none,1-optimize wd/ht)

PBM file should be B&W, 1-bit.

The file should contain 16x6 grid or 32x3 grid of characters (96 characters mode),
16x8 grid (128 characters mode),
16x14 grid (224 characters mode - as big as you are going to get - 0x20 to 0xFF ascii code - RREFont starts at 0x20)
or 1 line for digits and following characters.

Use IrfanView to convert to bitmaps from other formats.

IMPORTANT: Photoshop doesn't create a valid .pbm file.
Open and re-save in IrfanView to fix.

"@
Write-Host $info

# Check if the required parameters are passed (PBM file and Font Name)
if ($args.Count -lt 2) {
    Write-Error "Usage: $PSCommandPath <PBM_FILE_PATH> <FONT_NAME>"
    exit 1
}

# Get parameters from the command line arguments
$SrcFile = $args[0]
$FontName = $args[1]

# Define paths
$tempFile = "$($SrcFile).temp"
$outputFile = [System.IO.Path]::ChangeExtension($SrcFile, ".h")

Write-Host "Processing file: $SrcFile..."

# Run the font generator
& .\rrefontgen.exe "$SrcFile" 16 16 "$FontName" 0 2 0 | Out-File -Encoding ASCII $tempFile

$searchString = "----><--------><--------><--------><--------><--------><----"
$replacement = "$searchString`r`n*/"

# Read file contents
$fileContents = Get-Content $tempFile

# Initialize modified content with #pragma once and comment block start
$modifiedContents = @("#pragma once", "/*")

# Iterate through lines and add them until the marker is found
foreach ($line in $fileContents) {
    $modifiedContents += $line
    if ($line -eq $searchString) {
        $modifiedContents += "*/"
    }
}

# Save the modified contents to the output file
$modifiedContents | Set-Content $outputFile

# Clean up
Remove-Item -Path $tempFile

Write-Host "Processing CustomIcons complete! Modified file saved as '$outputFile'."
