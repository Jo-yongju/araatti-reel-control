param(
    [string]$CubeIdePath = $env:STM32CUBEIDE_HOME,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$Project = 'all'
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$projects = @{
    'encoder' = 'firmware\encoder_test'
    'foc' = 'foc\STM32CubeIDE'
}
if ($Project -ne 'all' -and -not $projects.ContainsKey($Project)) {
    throw "Unknown project '$Project'. Choose: all, $($projects.Keys -join ', ')"
}
if (-not $CubeIdePath) {
    $preferred = 'C:\ST\STM32CubeIDE_1.17.0\STM32CubeIDE'
    if (Test-Path -LiteralPath (Join-Path $preferred 'headless-build.bat')) {
        $CubeIdePath = $preferred
    } elseif (Test-Path -LiteralPath 'C:\ST') {
        $candidate = Get-ChildItem -LiteralPath 'C:\ST' -Directory -Filter 'STM32CubeIDE_*' |
            Sort-Object Name -Descending |
            Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'STM32CubeIDE\headless-build.bat') } |
            Select-Object -First 1
        if ($candidate) { $CubeIdePath = Join-Path $candidate.FullName 'STM32CubeIDE' }
    }
}
if (-not $CubeIdePath -or -not (Test-Path -LiteralPath (Join-Path $CubeIdePath 'headless-build.bat'))) {
    throw 'Install STM32CubeIDE, then pass -CubeIdePath with the folder containing headless-build.bat.'
}
$CubeIdePath = (Resolve-Path -LiteralPath $CubeIdePath).Path
$plugins = Join-Path $CubeIdePath 'plugins'
$gcc = Get-ChildItem -LiteralPath $plugins -Directory -Filter 'com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.*' |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'tools\bin\arm-none-eabi-gcc.exe') } |
    Sort-Object Name -Descending | Select-Object -First 1
$make = Get-ChildItem -LiteralPath $plugins -Directory -Filter 'com.st.stm32cube.ide.mcu.externaltools.make.win32_*' |
    Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName 'tools\bin\make.exe') } |
    Sort-Object Name -Descending | Select-Object -First 1
if (-not $gcc -or -not $make) { throw 'The CubeIDE ARM GCC or Make tool was not found.' }
$originalPath = $env:PATH
$env:PATH = (Join-Path $gcc.FullName 'tools\bin') + ';' + (Join-Path $make.FullName 'tools\bin') + ';' + $originalPath
try {
    & (Join-Path $gcc.FullName 'tools\bin\arm-none-eabi-gcc.exe') --version
    if ($LASTEXITCODE -ne 0) { throw 'ARM GCC could not start.' }
    $runRoot = Join-Path $repoRoot ('build\ide-' + [guid]::NewGuid().ToString('N'))
    [void](New-Item -ItemType Directory -Path $runRoot)
    $arguments = @('-configuration', (Join-Path $runRoot 'configuration'), '-data', (Join-Path $runRoot 'workspace'))
    $expected = @()
    foreach ($key in ($projects.Keys | Sort-Object)) {
        if ($Project -ne 'all' -and $Project -ne $key) { continue }
        $projectDir = Join-Path $repoRoot $projects[$key]
        [xml]$description = Get-Content -LiteralPath (Join-Path $projectDir '.project') -Raw
        $projectName = [string]$description.projectDescription.name
        $arguments += @('-import', $projectDir, '-cleanBuild', "$projectName/$Configuration")
        $expected += Join-Path $projectDir "$Configuration\$projectName.elf"
    }
    $logPath = Join-Path $runRoot 'build.log'
    $started = [DateTime]::UtcNow
    Write-Host "Building $Project / $Configuration. Log: $logPath"
    # CubeIDE can return zero even when Make fails; also check the log and fresh ELF files.
    & (Join-Path $CubeIdePath 'headless-build.bat') @arguments *> $logPath
    $exitCode = $LASTEXITCODE
    Get-Content -LiteralPath $logPath -Tail 18
    $failed = Select-String -LiteralPath $logPath -Pattern 'fatal error:|error:|make(\[\d+\])?: \*\*\*|Cannot run program|Application error|An error has occurred|No Project matched|No Config matched|Unrecognized argument'
    if ($exitCode -ne 0 -or $failed) { throw "Build failed. See $logPath" }
    foreach ($elf in $expected) {
        if (-not (Test-Path -LiteralPath $elf) -or (Get-Item -LiteralPath $elf).LastWriteTimeUtc -lt $started) {
            throw "A fresh ELF was not generated: $elf. See $logPath"
        }
        Write-Host "Built: $elf"
        & (Join-Path $gcc.FullName 'tools\bin\arm-none-eabi-size.exe') $elf
        if ($LASTEXITCODE -ne 0) { throw "Could not inspect ELF: $elf" }
    }
} finally {
    $env:PATH = $originalPath
}

