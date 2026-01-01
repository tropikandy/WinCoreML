# Master Release Script
# Orchestrates the entire release process

param(
    [Parameter(Mandatory=$true)]
    [string]$Version,

    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$SkipGitTag,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host $Message -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
}

function Write-Success {
    param([string]$Message)
    Write-Host "✓ $Message" -ForegroundColor Green
}

function Write-Error {
    param([string]$Message)
    Write-Host "✗ $Message" -ForegroundColor Red
}

function Write-Warning {
    param([string]$Message)
    Write-Host "⚠ $Message" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Cyan
Write-Host "║   CoreMLWin Release Automation        ║" -ForegroundColor Cyan
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Cyan
Write-Host ""
Write-Host "Version: $Version" -ForegroundColor White

if ($DryRun) {
    Write-Warning "DRY RUN MODE - No changes will be made"
}

Write-Host ""

# Step 1: Pre-flight checks
Write-Step "Step 1: Pre-flight Checks"

# Check git status
$gitStatus = git status --porcelain
if ($gitStatus) {
    Write-Error "Working directory is not clean!"
    Write-Host "Uncommitted changes:" -ForegroundColor Yellow
    Write-Host $gitStatus
    Write-Host ""
    Write-Host "Please commit or stash changes before releasing" -ForegroundColor Yellow
    exit 1
}
Write-Success "Git working directory is clean"

# Check if on correct branch
$currentBranch = git branch --show-current
if ($currentBranch -ne "main" -and $currentBranch -notlike "claude/*") {
    Write-Warning "Not on main branch (current: $currentBranch)"
    $continue = Read-Host "Continue anyway? (y/N)"
    if ($continue -ne "y") {
        exit 1
    }
}
Write-Success "On branch: $currentBranch"

# Check for tag conflicts
$existingTag = git tag -l "v$Version"
if ($existingTag -and -not $SkipGitTag) {
    Write-Error "Tag v$Version already exists!"
    Write-Host "Delete it with: git tag -d v$Version" -ForegroundColor Yellow
    exit 1
}
Write-Success "Tag v$Version is available"

# Step 2: Run tests
if (-not $SkipTests) {
    Write-Step "Step 2: Running Tests"

    # Start service for testing
    Write-Host "Starting service..." -ForegroundColor Yellow
    if (-not $DryRun) {
        $serviceProcess = Start-Process -FilePath "build\bin\Release\coremlwin_service.exe" -PassThru -WindowStyle Hidden
        Start-Sleep -Seconds 5
    }

    try {
        # Run integration tests
        Write-Host "Running integration tests..." -ForegroundColor Yellow
        if (-not $DryRun) {
            $env:PYTHONPATH = "sdk/python"
            pytest tests/test_integration.py -v --tb=short

            if ($LASTEXITCODE -ne 0) {
                Write-Error "Tests failed!"
                exit 1
            }
        }
        Write-Success "All tests passed"

    } finally {
        if ($serviceProcess) {
            Stop-Process -Id $serviceProcess.Id -Force -ErrorAction SilentlyContinue
        }
    }
} else {
    Write-Warning "Skipping tests"
}

# Step 3: Build packages
if (-not $SkipBuild) {
    Write-Step "Step 3: Building Packages"

    # Build MSI
    Write-Host "Building MSI installer..." -ForegroundColor Yellow
    if (-not $DryRun) {
        Push-Location installer\wix
        .\build-installer.ps1
        Pop-Location
    }
    Write-Success "MSI installer built"

    # Build portable ZIP
    Write-Host "Building portable package..." -ForegroundColor Yellow
    if (-not $DryRun) {
        .\installer\create-portable.ps1 -Version $Version
    }
    Write-Success "Portable package built"

} else {
    Write-Warning "Skipping build"
}

# Step 4: Generate checksums
Write-Step "Step 4: Generating Checksums"

$msiPath = "installer\wix\output\CoreMLWin-$Version-x64.msi"
$zipPath = "installer\portable\CoreMLWin-$Version-portable-x64.zip"

if (Test-Path $msiPath) {
    $msiHash = (Get-FileHash $msiPath -Algorithm SHA256).Hash
    Write-Host "MSI SHA256:" -ForegroundColor Cyan
    Write-Host "  $msiHash" -ForegroundColor White

    if (-not $DryRun) {
        $msiHash | Out-File "$msiPath.sha256" -Encoding ASCII
    }
} else {
    Write-Error "MSI not found: $msiPath"
    exit 1
}

if (Test-Path $zipPath) {
    $zipHash = (Get-FileHash $zipPath -Algorithm SHA256).Hash
    Write-Host "ZIP SHA256:" -ForegroundColor Cyan
    Write-Host "  $zipHash" -ForegroundColor White
} else {
    Write-Error "ZIP not found: $zipPath"
    exit 1
}

# Step 5: Create git tag
if (-not $SkipGitTag) {
    Write-Step "Step 5: Creating Git Tag"

    $tagMessage = "Release v$Version`n`nPackages:`n- MSI Installer`n- Portable ZIP`n`nSHA256 Checksums:`nMSI: $msiHash`nZIP: $zipHash"

    if (-not $DryRun) {
        git tag -a "v$Version" -m $tagMessage
        Write-Success "Created tag v$Version"
    } else {
        Write-Host "Would create tag: v$Version" -ForegroundColor Yellow
    }
} else {
    Write-Warning "Skipping git tag creation"
}

# Step 6: Summary and next steps
Write-Step "Release Preparation Complete!"

Write-Host ""
Write-Host "📦 Built Packages:" -ForegroundColor Cyan
Write-Host "  • $msiPath" -ForegroundColor White
Write-Host "  • $zipPath" -ForegroundColor White
Write-Host ""

Write-Host "🔐 Checksums:" -ForegroundColor Cyan
Write-Host "  MSI: $msiHash" -ForegroundColor White
Write-Host "  ZIP: $zipHash" -ForegroundColor White
Write-Host ""

if (-not $DryRun) {
    Write-Host "📋 Next Steps:" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "1. Push the tag to GitHub:" -ForegroundColor White
    Write-Host "   git push origin v$Version" -ForegroundColor Gray
    Write-Host ""
    Write-Host "2. GitHub Actions will automatically:" -ForegroundColor White
    Write-Host "   • Build packages" -ForegroundColor Gray
    Write-Host "   • Run tests" -ForegroundColor Gray
    Write-Host "   • Create GitHub Release" -ForegroundColor Gray
    Write-Host "   • Upload artifacts" -ForegroundColor Gray
    Write-Host ""
    Write-Host "3. After GitHub Release is published:" -ForegroundColor White
    Write-Host "   • Users can download directly from:" -ForegroundColor Gray
    Write-Host "     https://github.com/user/WinCoreML/releases/tag/v$Version" -ForegroundColor Gray
    Write-Host ""
    Write-Host "4. Submit to Winget (optional):" -ForegroundColor White
    Write-Host "   .\tools\prepare-winget-submission.ps1 ``" -ForegroundColor Gray
    Write-Host "     -Version $Version ``" -ForegroundColor Gray
    Write-Host "     -InstallerUrl https://github.com/user/WinCoreML/releases/download/v$Version/CoreMLWin-$Version-x64.msi ``" -ForegroundColor Gray
    Write-Host "     -SHA256Hash $msiHash ``" -ForegroundColor Gray
    Write-Host "     -CreatePR" -ForegroundColor Gray
    Write-Host ""

    Write-Host "Ready to push? (y/N): " -ForegroundColor Yellow -NoNewline
    $push = Read-Host

    if ($push -eq "y") {
        Write-Host ""
        Write-Host "Pushing tag..." -ForegroundColor Cyan
        git push origin "v$Version"

        if ($LASTEXITCODE -eq 0) {
            Write-Success "Tag pushed successfully!"
            Write-Host ""
            Write-Host "Monitor the release at:" -ForegroundColor Cyan
            Write-Host "https://github.com/user/WinCoreML/actions" -ForegroundColor White
        } else {
            Write-Error "Failed to push tag"
        }
    } else {
        Write-Host ""
        Write-Host "Tag not pushed. Push manually when ready:" -ForegroundColor Yellow
        Write-Host "  git push origin v$Version" -ForegroundColor Gray
    }
} else {
    Write-Host "DRY RUN - No changes made" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "╔════════════════════════════════════════╗" -ForegroundColor Green
Write-Host "║         Release Ready! 🚀              ║" -ForegroundColor Green
Write-Host "╚════════════════════════════════════════╝" -ForegroundColor Green
Write-Host ""
