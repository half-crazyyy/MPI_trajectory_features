param(
    [string]$Executable = ".\build\Debug\mpi_trajectory_features.exe",
    [string]$InputFile = "data\sample_air_objects.csv",
    [int]$MaxProcesses = 12,
    [switch]$Oversubscribe
)

$OutputDir = "docs\performance"
$Results = Join-Path $OutputDir "benchmark_results.csv"

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
Remove-Item $Results -ErrorAction SilentlyContinue
Remove-Item (Join-Path $OutputDir "benchmark_summary.csv") -ErrorAction SilentlyContinue
Remove-Item (Join-Path $OutputDir "features_*.csv") -ErrorAction SilentlyContinue

foreach ($N in 1..$MaxProcesses) {
    Write-Host "Запуск бенчмарка на $N MPI-процесс(ах)"

    $args = @()
    if ($Oversubscribe) {
        # Для OpenMPI. В MS-MPI этот параметр обычно не нужен.
        $args += "--oversubscribe"
    }
    $args += @("-n", $N, $Executable,
        "--input", $InputFile,
        "--output", (Join-Path $OutputDir "features_$N.csv"),
        "--repetitions", "3",
        "--verify")

    mpiexec @args
    Write-Host ""
}

python scripts\plot_results.py --input $Results --output-dir $OutputDir
