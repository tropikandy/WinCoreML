# CoreMLWin Release Checklist

Quick reference for releasing a new version of CoreMLWin.

## 🚀 One-Command Release

```powershell
# Complete release automation (recommended)
.\tools\release.ps1 -Version 0.1.0
```

This script will:
- ✅ Check git status is clean
- ✅ Run all tests
- ✅ Build MSI installer
- ✅ Build portable ZIP
- ✅ Generate SHA256 checksums
- ✅ Create git tag
- ✅ Prompt to push to GitHub

Then GitHub Actions automatically:
- ✅ Builds packages in CI
- ✅ Runs tests
- ✅ Creates GitHub Release
- ✅ Uploads all artifacts

---

## 📦 Manual Release Process

If you prefer step-by-step control:

### 1. Pre-Release Checks

```powershell
# Ensure working directory is clean
git status

# Update version in all files
# - installer/wix/CoreMLWin.wxs (line 3)
# - installer/msix/AppxManifest.xml (line 6)
# - installer/chocolatey/coremlwin.nuspec (line 4)
# - installer/winget/*.yaml (PackageVersion)
```

### 2. Run Tests

```powershell
# Start service
start build\bin\Release\coremlwin_service.exe

# Run integration tests
$env:PYTHONPATH = "sdk/python"
pytest tests/test_integration.py -v

# Stop service
taskkill /IM coremlwin_service.exe /F
```

### 3. Build Packages

```powershell
# Build MSI installer
cd installer/wix
.\build-installer.ps1
cd ..\..

# Build portable ZIP
.\installer\create-portable.ps1 -Version 0.1.0
```

### 4. Create Git Tag and Push

```powershell
# Create annotated tag
git tag -a v0.1.0 -m "Release v0.1.0"

# Push tag (triggers GitHub Actions)
git push origin v0.1.0
```

### 5. Wait for GitHub Actions

Monitor at: https://github.com/user/WinCoreML/actions

GitHub Actions will:
- Build packages in CI
- Run tests
- Create GitHub Release
- Upload artifacts

### 6. Verify GitHub Release

Check: https://github.com/user/WinCoreML/releases

Should have:
- ✅ Release v0.1.0 published
- ✅ `CoreMLWin-0.1.0-x64.msi` uploaded
- ✅ `CoreMLWin-0.1.0-x64.msi.sha256` uploaded
- ✅ `CoreMLWin-0.1.0-portable-x64.zip` uploaded
- ✅ `CoreMLWin-0.1.0-portable-x64.zip.sha256` uploaded

---

## 📲 Submit to Winget (Optional but Recommended)

After GitHub Release is published:

### Automated Method (Easiest)

```powershell
# Get SHA256 from release
$sha256 = Get-Content "installer\wix\output\CoreMLWin-0.1.0-x64.msi.sha256"

# Prepare winget submission (creates PR automatically)
.\tools\prepare-winget-submission.ps1 `
  -Version 0.1.0 `
  -InstallerUrl https://github.com/user/WinCoreML/releases/download/v0.1.0/CoreMLWin-0.1.0-x64.msi `
  -SHA256Hash $sha256 `
  -CreatePR
```

### Manual Method

1. **Fork winget-pkgs**:
   - Go to: https://github.com/microsoft/winget-pkgs
   - Click Fork

2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/winget-pkgs.git
   cd winget-pkgs
   ```

3. **Run preparation script**:
   ```powershell
   ..\WinCoreML\tools\prepare-winget-submission.ps1 `
     -Version 0.1.0 `
     -InstallerUrl https://github.com/user/WinCoreML/releases/download/v0.1.0/CoreMLWin-0.1.0-x64.msi `
     -SHA256Hash YOUR_HASH_HERE `
     -WingetPkgsRepo .\winget-pkgs
   ```

4. **Push to your fork**:
   ```bash
   git push -u origin coremlwin-0.1.0
   ```

5. **Create PR**:
   - Go to: https://github.com/microsoft/winget-pkgs
   - Click "Compare & pull request"
   - Fill out template
   - Submit

6. **Wait for review** (2-7 days)

---

## 🗓️ Release Timeline

### Immediate (Day 1)
- ✅ Run `.\tools\release.ps1`
- ✅ Push git tag
- ✅ GitHub Release published automatically
- **Users can download MSI and ZIP**

### Week 1 (Days 1-7)
- ✅ Submit to Winget
- ⏳ Automated validation (minutes)
- ⏳ Manual review (2-7 days)

### Week 2 (Days 7-14)
- ✅ Winget PR merged
- ✅ Package appears in winget catalog (24 hours after merge)
- **Users can `winget install CoreMLWin.UniversalMLRuntime`**

---

## 📊 Post-Release

### Monitor Downloads

**GitHub Releases**:
- View at: https://github.com/user/WinCoreML/releases
- Download counts shown per asset

**Winget** (no public metrics):
- Track via GitHub release downloads
- Monitor GitHub stars/forks

### Announce Release

Consider posting to:
- GitHub Discussions
- Reddit: r/machinelearning, r/LocalLLaMA
- Twitter/X
- LinkedIn

### Template Announcement

```markdown
🚀 CoreMLWin v0.1.0 Released!

Universal ML Runtime for Windows with DirectML GPU acceleration.

✨ Features:
- ONNX Runtime integration
- DirectML GPU support (NVIDIA/AMD/Intel)
- Python SDK
- Production-ready security

📦 Install:
winget install CoreMLWin.UniversalMLRuntime

Or download: https://github.com/user/WinCoreML/releases/tag/v0.1.0

🔗 GitHub: https://github.com/user/WinCoreML
```

---

## 🐛 Troubleshooting

### GitHub Actions Fails

**Check logs**: https://github.com/user/WinCoreML/actions

Common issues:
- ONNX Runtime download failed → Retry workflow
- Tests failed → Fix and push new commit, delete tag, retry
- MSI build failed → Check WiX installation in CI

### Winget Validation Fails

**Check PR comments**: Automated bot will comment with errors

Common issues:
- SHA256 mismatch → Re-download MSI from release and compute hash
- Installer URL 404 → Ensure release is published (not draft)
- Manifest syntax error → Run `winget validate` locally

### Release Not Appearing

**GitHub Release**:
- Check it's published (not draft)
- Wait a few minutes for CDN propagation

**Winget**:
- Takes 24 hours after PR merge
- Update winget: `winget upgrade --all`
- Clear cache: `winget source reset`

---

## 📚 Resources

- [Distribution Guide](docs/DISTRIBUTION_GUIDE.md) - Detailed guide
- [GitHub Releases Docs](https://docs.github.com/en/repositories/releasing-projects-on-github)
- [Winget Package Manager](https://learn.microsoft.com/en-us/windows/package-manager/)
- [GitHub Actions Workflow](.github/workflows/release.yml)

---

## ✅ Quick Checklist

Before running `.\tools\release.ps1`:

- [ ] All tests passing locally
- [ ] Version updated in all manifest files
- [ ] CHANGELOG.md updated with changes
- [ ] No uncommitted changes in git
- [ ] On correct branch (main or claude/*)
- [ ] ONNX Runtime available
- [ ] Clean CMake build completed

The script will verify most of these automatically!

---

**Need help?** Open an issue: https://github.com/user/WinCoreML/issues
