# CoreMLWin Distribution Guide

Complete guide for distributing CoreMLWin as a hobby/personal developer.

## Table of Contents
- [Quick Start (Recommended Path)](#quick-start-recommended-path)
- [Option 1: GitHub Releases](#option-1-github-releases)
- [Option 2: Winget Submission](#option-2-winget-submission)
- [Automation](#automation)

---

## Quick Start (Recommended Path)

For a hobby developer, this is the easiest path:

1. **Week 1**: Create GitHub Release with portable ZIP (instant)
2. **Week 2**: Submit to Winget (free, reaches millions)
3. **Optional**: Submit to Chocolatey (if you want IT admin audience)

---

## Option 1: GitHub Releases

### What You Get
- ✅ Instant distribution
- ✅ Direct download links
- ✅ Version tracking
- ✅ Release notes
- ✅ Auto-generated changelog
- ✅ Free CDN hosting

### Prerequisites
- GitHub repository (you have this: `user/WinCoreML`)
- Built MSI installer
- Portable ZIP package
- Git tag for version

### Step-by-Step Process

#### 1. Build Release Artifacts

On Windows:
```powershell
# Build the MSI installer
cd installer/wix
.\build-installer.ps1

# Creates: installer/wix/output/CoreMLWin-0.1.0-x64.msi
```

For portable ZIP:
```powershell
# Create portable package
.\installer\create-portable.ps1

# Creates: installer/portable/CoreMLWin-0.1.0-portable-x64.zip
```

#### 2. Create Git Tag

```bash
# Create annotated tag
git tag -a v0.1.0 -m "Release v0.1.0 - Initial public release"

# Push tag to GitHub
git push origin v0.1.0
```

#### 3. Create GitHub Release (Web UI)

1. Go to: https://github.com/user/WinCoreML/releases/new
2. Select tag: `v0.1.0`
3. Release title: `CoreMLWin v0.1.0 - Universal ML Runtime for Windows`
4. Description:
   ```markdown
   ## 🚀 First Official Release

   CoreMLWin brings Apple CoreML-like experience to Windows with high-performance ML inference.

   ### ✨ Features
   - ONNX Runtime integration with DirectML GPU acceleration
   - Support for NVIDIA, AMD, and Intel GPUs
   - Python SDK and C++ API
   - Production-ready with comprehensive security testing

   ### 📦 Installation

   **Option 1: MSI Installer** (Recommended for most users)
   - Download `CoreMLWin-0.1.0-x64.msi`
   - Double-click to install
   - Requires administrator privileges

   **Option 2: Portable ZIP** (No installation required)
   - Download `CoreMLWin-0.1.0-portable-x64.zip`
   - Extract anywhere
   - Run `coremlwin_service.exe`

   ### 📋 Requirements
   - Windows 10 version 1903 (build 18362) or newer
   - DirectX 12 capable GPU (for GPU acceleration)
   - Visual C++ Redistributable 2022

   ### 🔗 Quick Start
   ```powershell
   # Install Python SDK
   pip install coremlwin

   # Test installation
   python -c "from coremlwin_client import CoreMLWinClient; print(CoreMLWinClient().health())"
   ```

   ### 📊 What's New
   - Initial public release
   - Full ONNX Runtime integration
   - DirectML GPU acceleration
   - Security hardening (red team tested)
   - Comprehensive test suite

   ### 🐛 Known Issues
   - None at this time

   ### 📚 Documentation
   - [Installation Guide](../docs/INSTALLATION.md)
   - [Security Audit Report](../docs/RED_TEAM_AUDIT.md)
   ```

5. Upload binaries:
   - Drag and drop `CoreMLWin-0.1.0-x64.msi`
   - Drag and drop `CoreMLWin-0.1.0-portable-x64.zip`

6. **Set as latest release**: ✅ Check this box

7. Click **Publish release**

#### 4. Generate SHA256 Hashes

After upload, you need SHA256 hashes for Winget submission:

```powershell
# On Windows
certutil -hashfile CoreMLWin-0.1.0-x64.msi SHA256

# On Linux
sha256sum CoreMLWin-0.1.0-x64.msi
```

Save the hash - you'll need it for Winget!

#### 5. Automated Release (Alternative)

Use the included GitHub Actions workflow:

```yaml
# Automatically triggered when you push a tag
git tag v0.1.0
git push origin v0.1.0

# GitHub Actions will:
# 1. Build MSI
# 2. Create portable ZIP
# 3. Generate checksums
# 4. Create GitHub Release
# 5. Upload all artifacts
```

See: `.github/workflows/release.yml`

---

## Option 2: Winget Submission

### What You Get
- ✅ Reaches ALL Windows 10/11 users
- ✅ Users install with: `winget install CoreMLWin.UniversalMLRuntime`
- ✅ Auto-update support
- ✅ 100% free
- ✅ Official Microsoft catalog

### Prerequisites
- GitHub account
- Published GitHub Release (see Option 1)
- MSI installer with public download URL
- SHA256 hash of MSI

### Step-by-Step Process

#### 1. Fork winget-pkgs Repository

1. Go to: https://github.com/microsoft/winget-pkgs
2. Click **Fork** button (top right)
3. Wait for fork to complete

#### 2. Clone Your Fork

```bash
git clone https://github.com/YOUR_USERNAME/winget-pkgs.git
cd winget-pkgs
```

#### 3. Create Branch for Your Package

```bash
git checkout -b coremlwin-0.1.0
```

#### 4. Create Manifest Folder Structure

Winget requires a specific folder structure:

```
manifests/
  c/                           # First letter of publisher (lowercase)
    CoreMLWin/                 # Publisher name
      UniversalMLRuntime/      # Package name
        0.1.0/                 # Version
          CoreMLWin.UniversalMLRuntime.installer.yaml
          CoreMLWin.UniversalMLRuntime.locale.en-US.yaml
          CoreMLWin.UniversalMLRuntime.yaml
```

Create the folders:
```bash
mkdir -p manifests/c/CoreMLWin/UniversalMLRuntime/0.1.0
```

#### 5. Copy Your Manifest Files

You already have these prepared! Copy them:

```bash
cp /path/to/WinCoreML/installer/winget/*.yaml \
   manifests/c/CoreMLWin/UniversalMLRuntime/0.1.0/
```

#### 6. Update Manifest with Real Values

Edit `CoreMLWin.UniversalMLRuntime.installer.yaml`:

```yaml
# Line 24: Update with your actual GitHub release URL
InstallerUrl: https://github.com/YOUR_USERNAME/WinCoreML/releases/download/v0.1.0/CoreMLWin-0.1.0-x64.msi

# Line 25: Update with actual SHA256 hash
InstallerSha256: YOUR_ACTUAL_SHA256_HASH_HERE
```

**CRITICAL**: The InstallerUrl MUST be publicly accessible!

#### 7. Validate Manifest Locally

Install winget CLI and validate:

```powershell
# Install winget (if not already installed)
# It's built into Windows 11, or download from Microsoft Store

# Validate your manifest
winget validate --manifest manifests/c/CoreMLWin/UniversalMLRuntime/0.1.0/
```

Expected output:
```
Manifest validation succeeded.
```

#### 8. Test Installation Locally

```powershell
# Test install from your local manifest
winget install --manifest manifests/c/CoreMLWin/UniversalMLRuntime/0.1.0/

# Verify it works
coremlwin --version
```

#### 9. Commit and Push to Your Fork

```bash
git add manifests/c/CoreMLWin/
git commit -m "New package: CoreMLWin.UniversalMLRuntime version 0.1.0"
git push origin coremlwin-0.1.0
```

#### 10. Create Pull Request

1. Go to: https://github.com/microsoft/winget-pkgs
2. GitHub will show: "YOUR_USERNAME wants to merge from coremlwin-0.1.0"
3. Click **Compare & pull request**
4. Title: `New package: CoreMLWin.UniversalMLRuntime version 0.1.0`
5. Description (auto-generated template):
   ```markdown
   ## New Package Submission

   - Package: CoreMLWin.UniversalMLRuntime
   - Version: 0.1.0
   - Publisher: CoreMLWin Project

   ## Validation
   - [x] Manifest validated with `winget validate`
   - [x] Tested installation locally
   - [x] Package installs successfully
   - [x] Package uninstalls cleanly

   ## Additional Information
   Universal Machine Learning Runtime for Windows with DirectML GPU acceleration.
   Open source project at: https://github.com/YOUR_USERNAME/WinCoreML
   ```

6. Click **Create pull request**

#### 11. Automated Validation

Within minutes, automated bots will:
- ✅ Validate manifest syntax
- ✅ Check installer URL is accessible
- ✅ Verify SHA256 hash matches
- ✅ Scan for malware
- ✅ Test installation
- ✅ Check uninstallation

If any check fails, the bot will comment with details.

#### 12. Manual Review

After automated checks pass:
- A Microsoft moderator will manually review (1-7 days)
- They may ask questions or request changes
- Once approved, your PR will be merged

#### 13. Publication

After merge:
- Your package appears in winget catalog within 24 hours
- Users can install with: `winget install CoreMLWin.UniversalMLRuntime`

### Updating Your Package

For version 0.2.0:

1. Create new folder: `manifests/c/CoreMLWin/UniversalMLRuntime/0.2.0/`
2. Copy manifests from 0.1.0, update version and URLs
3. Submit new PR: `Update: CoreMLWin.UniversalMLRuntime version 0.2.0`

---

## Automation

### GitHub Actions for Releases

Create: `.github/workflows/release.yml`

```yaml
name: Create Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build-and-release:
    runs-on: windows-latest

    steps:
      - uses: actions/checkout@v4

      - name: Build MSI
        run: |
          cd installer/wix
          .\build-installer.ps1

      - name: Create portable ZIP
        run: |
          .\installer\create-portable.ps1

      - name: Generate checksums
        run: |
          cd installer/wix/output
          certutil -hashfile CoreMLWin-0.1.0-x64.msi SHA256 > CoreMLWin-0.1.0-x64.msi.sha256

      - name: Create Release
        uses: softprops/action-gh-release@v1
        with:
          files: |
            installer/wix/output/CoreMLWin-0.1.0-x64.msi
            installer/wix/output/CoreMLWin-0.1.0-x64.msi.sha256
            installer/portable/CoreMLWin-0.1.0-portable-x64.zip
          body: |
            ## CoreMLWin v${{ github.ref_name }}

            Download and install CoreMLWin Universal ML Runtime.

            See [INSTALLATION.md](docs/INSTALLATION.md) for details.
          draft: false
          prerelease: false
        env:
          GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### Automated Winget Submission

Use `wingetcreate` tool:

```powershell
# Install wingetcreate
winget install wingetcreate

# Create new package submission
wingetcreate new --urls https://github.com/user/WinCoreML/releases/download/v0.1.0/CoreMLWin-0.1.0-x64.msi --version 0.1.0

# Update existing package
wingetcreate update CoreMLWin.UniversalMLRuntime --urls https://github.com/user/WinCoreML/releases/download/v0.2.0/CoreMLWin-0.2.0-x64.msi --version 0.2.0 --submit
```

---

## Common Issues

### GitHub Releases

**Problem**: Download links return 404
- **Solution**: Make sure release is published (not draft)

**Problem**: Assets not showing
- **Solution**: Wait a few minutes for CDN propagation

### Winget Submission

**Problem**: Manifest validation fails
```
Error: InstallerUrl is not accessible
```
- **Solution**: GitHub release must be published (not draft)
- **Solution**: URL must be exact (case-sensitive)

**Problem**: SHA256 mismatch
```
Error: Expected hash does not match actual hash
```
- **Solution**: Re-download MSI from GitHub release and compute hash again
- **Solution**: Make sure you're hashing the exact file users will download

**Problem**: PR blocked by automation
```
Error: Package already exists
```
- **Solution**: Check if package name is unique. Search winget-pkgs repo first.

**Problem**: Manual review taking forever
- **Solution**: Typical review time is 2-7 days. Be patient.
- **Solution**: Respond quickly to any moderator questions

---

## Timeline for Hobby Developer

### Week 1: GitHub Release
- **Day 1**: Build MSI and portable ZIP
- **Day 2**: Create GitHub release and upload artifacts
- **Day 3**: Test downloads, update README with installation instructions

✅ **Users can now download your software!**

### Week 2: Winget Submission
- **Day 1**: Fork winget-pkgs, create manifest, validate
- **Day 2**: Submit PR to winget-pkgs
- **Day 3-10**: Wait for automated checks and manual review
- **Day 10-14**: Respond to any questions, make requested changes

✅ **Users can now `winget install` your software!**

### Ongoing: Updates
- For each new version:
  - Create GitHub release (automated via GitHub Actions)
  - Submit winget PR (automated via `wingetcreate update`)

---

## Metrics & Analytics

### GitHub Releases
- View download counts: https://github.com/user/WinCoreML/releases
- Per-asset downloads shown on release page
- Use GitHub Insights for traffic data

### Winget
- No public download metrics (privacy-focused)
- Track indirectly via:
  - GitHub release downloads
  - GitHub repository stars/forks
  - Issues and discussions

---

## Cost Analysis

| Platform | Setup Cost | Recurring Cost | Time to Publish |
|----------|-----------|----------------|-----------------|
| GitHub Releases | $0 | $0 | 10 minutes |
| Winget | $0 | $0 | 2-14 days |
| Chocolatey | $0 | $0 | 1-7 days |
| Microsoft Store | $19 (one-time) | $0 | 3-7 days |

**Recommendation**: Start with GitHub Releases + Winget (both free, maximum reach)

---

## Resources

- [GitHub Releases Documentation](https://docs.github.com/en/repositories/releasing-projects-on-github)
- [Winget Package Manager](https://learn.microsoft.com/en-us/windows/package-manager/)
- [Submit to Winget Repository](https://learn.microsoft.com/en-us/windows/package-manager/package/repository)
- [Winget Manifest Schema](https://learn.microsoft.com/en-us/windows/package-manager/package/manifest)
- [actions/upload-artifact](https://github.com/actions/upload-artifact)
- [softprops/action-gh-release](https://github.com/marketplace/actions/github-upload-release-artifacts)

---

## Support

- **GitHub Issues**: https://github.com/user/WinCoreML/issues
- **Winget Issues**: https://github.com/microsoft/winget-pkgs/issues
- **Discussion**: https://github.com/user/WinCoreML/discussions
