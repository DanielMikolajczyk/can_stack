# Path to your specific Zig version
$Zig = "C:\Users\dmiko\Downloads\zig-x86_64-windows-0.15.2\zig-x86_64-windows-0.15.2\zig.exe"

# --- MANUAL CONFIGURATION ---
$IncDir  = "inc"
$Output  = "build/output/cantp_node.exe"

# Explicitly list your source files here
$Sources = @(
    "src/Can.c",
    "src/CanIf.c",
    "src/CanSM.c",
    "src/CanTp.c",
    "src/CanTrcv.c",
    "main.c"
)
# ----------------------------

Write-Host "Starting manual build for CanTp..." -ForegroundColor Cyan

# Join the array into a single string for the command
$SourceFiles = $Sources -join " "

# Run zig cc
# -I: adds include directory
# -g: includes debug symbols
& $Zig cc "-I$IncDir" $Sources -o $Output -g -Wall -Wextra

if ($LASTEXITCODE -eq 0) {
    Write-Host "Successfully built: $Output" -ForegroundColor Green
} else {
    Write-Host "Build failed." -ForegroundColor Red
}