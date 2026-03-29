# Path to your specific Zig version
$Zig = "C:\Users\dmiko\Downloads\zig-x86_64-windows-0.15.2\zig-x86_64-windows-0.15.2\zig.exe"

# --- MANUAL CONFIGURATION ---
$Output  = "build/output/cantp_node.exe"

$IncludeDirs = @(
    "inc",
    "external/inc"
)

# Explicitly list your source files here
$Sources = @(
    "src/Can_cfg.c",
    "src/Can.c",
    "src/CanIf.c",
    "src/CanSM.c",
    "src/CanTp.c",
    "src/CanTrcv.c",
    # "external/CanDrv.c",
    "main.c"
)
# --- BUILD LOGIC ---
Write-Host "Starting manual build for CAN Stack..." -ForegroundColor Cyan

# Ensure build directory exists
if (!(Test-Path "build/output")) { New-Item -ItemType Directory -Path "build/output" -Force }

# Map the include directories to "-I" flags
$IncludeFlags = $IncludeDirs | ForEach-Object { "-I$_" }

# Define compiler flags (Senior tip: -Werror ensures you don't ignore warnings)
$CompileFlags = @(
    "-g",
    # "-Wall",
    # "-Wextra",
    # "-Werror",
    "-std=c11"
)

# Run zig cc using the array of arguments (PowerShell handles the splatting)
& $Zig cc $IncludeFlags $CompileFlags $Sources -o $Output

if ($LASTEXITCODE -eq 0) {
    Write-Host "Successfully built: $Output" -ForegroundColor Green
} else {
    Write-Host "Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
}