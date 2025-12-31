# CoreMLWin Deployment & Distribution

This directory contains all installation packages and deployment configurations for CoreMLWin.

## 📦 Available Packages

### 1. WiX MSI Installer (`wix/`)
Traditional Windows Installer package for enterprise deployment.

**Build**:
```powershell
cd wix
.\build-installer.ps1
```

**Output**: `wix/output/CoreMLWin-0.1.0-x64.msi`

**Use Cases**:
- Enterprise deployment via Group Policy
- SCCM/Intune deployment
- Offline installation
- Custom installation paths

---

### 2. MSIX Package (`msix/`)
Modern Windows app package for Microsoft Store.

**Build**:
```powershell
cd msix
makeappx pack /d PackageFiles /p CoreMLWin.msix
signtool sign /fd SHA256 /a /f certificate.pfx CoreMLWin.msix
```

**Use Cases**:
- Microsoft Store distribution
- Automatic updates
- Sandboxed installation
- Consumer-facing installation

---

### 3. Chocolatey Package (`chocolatey/`)
Package for Chocolatey package manager.

**Build**:
```powershell
cd chocolatey
choco pack
```

**Test Locally**:
```powershell
choco install coremlwin -source .
```

**Publish**:
```powershell
choco push coremlwin.0.1.0.nupkg --source https://push.chocolatey.org/ --api-key YOUR_API_KEY
```

**Use Cases**:
- IT admin deployments
- Developer installations
- Automated CI/CD
- Mass deployment

---

### 4. Winget Manifest (`winget/`)
Windows Package Manager manifest files.

**Submit to winget-pkgs repository**:
```powershell
# Fork https://github.com/microsoft/winget-pkgs
# Add manifests to manifests/c/CoreMLWin/UniversalMLRuntime/0.1.0/
# Create pull request
```

**Use Cases**:
- Modern Windows installations
- Developer installations
- Command-line deployment

---

### 5. Portable ZIP (`portable/`)
Standalone package requiring no installation.

**Build**:
```powershell
# Copy build artifacts
.\create-portable.ps1
```

**Use Cases**:
- No admin rights
- USB drive deployment
- Multiple versions side-by-side
- Testing

---

## 🚀 Quick Build All Packages

```powershell
# Run from installer/ directory
.\build-all.ps1

# Outputs:
# - wix/output/CoreMLWin-0.1.0-x64.msi
# - msix/CoreMLWin.msix
# - chocolatey/coremlwin.0.1.0.nupkg
# - portable/CoreMLWin-0.1.0-portable-x64.zip
```

---

## 📋 Pre-Release Checklist

Before building release packages:

### Code
- [ ] All tests passing (`pytest tests/ -v`)
- [ ] Security audit complete
- [ ] Version updated in all manifests
- [ ] CHANGELOG.md updated
- [ ] Documentation up to date

### Build
- [ ] Clean build on Windows (`cmake --build . --config Release`)
- [ ] ONNX Runtime bundled correctly
- [ ] DirectML.dll included
- [ ] Dependencies verified

### Packaging
- [ ] Update version in `wix/CoreMLWin.wxs`
- [ ] Update version in `msix/AppxManifest.xml`
- [ ] Update version in `chocolatey/coremlwin.nuspec`
- [ ] Update version in `winget/*.yaml`
- [ ] Generate SHA256 hashes
- [ ] Sign all packages (MSI, MSIX)

### Testing
- [ ] Test MSI install/uninstall
- [ ] Test MSIX install/uninstall
- [ ] Test Chocolatey install/upgrade/uninstall
- [ ] Test Winget install
- [ ] Test portable extraction
- [ ] Verify service starts correctly
- [ ] Verify Python SDK works
- [ ] Test on clean Windows 10 VM
- [ ] Test on Windows 11

### Distribution
- [ ] Create GitHub release
- [ ] Upload MSI to release
- [ ] Upload portable ZIP to release
- [ ] Submit MSIX to Microsoft Store
- [ ] Push Chocolatey package
- [ ] Submit Winget manifest PR
- [ ] Update documentation links

---

## 📦 Package Comparison

| Feature | MSI | MSIX | Chocolatey | Winget | Portable |
|---------|-----|------|------------|--------|----------|
| **Auto-update** | ❌ | ✅ | ✅ | ✅ | ❌ |
| **Requires admin** | ✅ | ❌ | ✅ | ✅ | ❌ |
| **Offline install** | ✅ | ❌ | ❌ | ❌ | ✅ |
| **GPO deployment** | ✅ | ❌ | ✅ | ❌ | ❌ |
| **Build time** | 5 min | 10 min | 2 min | N/A | 1 min |
| **Distribution** | Manual | Store | Auto | Auto | Manual |

---

## 🔑 Code Signing

All packages should be signed for production release.

### Get Code Signing Certificate

1. **EV Code Signing Certificate** (recommended for driver/system software)
   - DigiCert, Sectigo, SSL.com
   - ~$300-500/year
   - Required for kernel-mode drivers
   - Bypasses SmartScreen warnings immediately

2. **Standard Code Signing Certificate**
   - Same vendors
   - ~$100-200/year
   - Builds reputation over time

### Sign Packages

**MSI**:
```powershell
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 `
  /a /f certificate.pfx /p PASSWORD CoreMLWin-0.1.0-x64.msi
```

**MSIX**:
```powershell
signtool sign /fd SHA256 /a /f certificate.pfx /p PASSWORD CoreMLWin.msix
```

**EXE** (portable):
```powershell
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 `
  /a /f certificate.pfx /p PASSWORD coremlwin_service.exe
```

### Verify Signatures

```powershell
signtool verify /pa /v CoreMLWin-0.1.0-x64.msi
```

---

## 📊 Distribution Analytics

Track installation metrics:

### Chocolatey
- View at: https://community.chocolatey.org/packages/coremlwin
- Metrics: Downloads, version distribution

### Winget
- No public analytics (privacy-focused)
- Track via GitHub release downloads

### Microsoft Store
- Partner Center dashboard
- Installs, acquisitions, ratings

### GitHub Releases
- View download counts
- Track by version

---

## 🐛 Troubleshooting

### MSI Build Fails
- Check WiX Toolset installed: `candle.exe --version`
- Verify all source files exist
- Check file paths in CoreMLWin.wxs

### MSIX Packaging Fails
- Verify AppxManifest.xml syntax
- Check all required assets present
- Ensure version format: X.Y.Z.0

### Chocolatey Validation Fails
- Run `choco pack --validate`
- Check nuspec syntax
- Verify dependencies exist

### Signing Fails
- Check certificate validity: `certutil -dump certificate.pfx`
- Verify password correct
- Ensure timestamp server reachable

---

## 📞 Support

- **Build Issues**: Open issue on GitHub
- **Distribution Questions**: Check Microsoft/Chocolatey docs
- **Code Signing**: Contact certificate authority

---

## 🔗 Resources

- [WiX Toolset](https://wixtoolset.org/)
- [MSIX Packaging](https://docs.microsoft.com/en-us/windows/msix/)
- [Chocolatey Docs](https://docs.chocolatey.org/en-us/create/create-packages)
- [Winget Manifest](https://github.com/microsoft/winget-pkgs)
- [Code Signing Guide](https://docs.microsoft.com/en-us/windows-hardware/drivers/dashboard/code-signing-cert-manage)
