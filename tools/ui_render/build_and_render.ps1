# 在 PC 上编译并运行固件 UI 代码，导出全部界面 PNG
# 用法（在仓库根目录）： powershell -ExecutionPolicy Bypass -File tools/ui_render/build_and_render.ps1
$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Resolve-Path (Join-Path $here "..\..")
Push-Location $here
try {
    New-Item -ItemType Directory -Force out | Out-Null
    & gcc -std=gnu11 -O1 -w `
        -I "shim" -I "$root\Hardware" -I "$root\System" `
        "$root\Hardware\OLED.c" "$root\Hardware\OLED_Data.c" `
        "$root\Hardware\menu.c" "$root\Hardware\menu_sub.c" `
        "$root\Hardware\weather.c" "$root\Hardware\SetTime.c" `
        "stubs.c" "harness.c" -o "render.exe" -lm
    if ($LASTEXITCODE -ne 0) { throw "gcc failed" }
    & ".\render.exe"
    python "render_ui.py"
    python "render_sheet.py"
    Write-Host "OK -> tools/ui_render/png/*.png 与 00_overview.png"
}
finally { Pop-Location }
