# Installation & Deployment Guide

**CoreMLWin Universal ML Runtime** - Easy installation for Windows

---

## 🎯 Installation Methods

We provide **5 easy installation methods** for different users:

| Method | Best For | Ease of Use | Auto-Updates |
|--------|----------|-------------|--------------|
| **Microsoft Store** | End users | ⭐⭐⭐⭐⭐ | ✅ Automatic |
| **Winget** | Developers | ⭐⭐⭐⭐ | ✅ Yes |
| **Chocolatey** | IT admins | ⭐⭐⭐⭐ | ✅ Yes |
| **MSI Installer** | Enterprise | ⭐⭐⭐ | ❌ Manual |
| **Portable ZIP** | No install needed | ⭐⭐⭐ | ❌ Manual |

---

## 🏪 Method 1: Microsoft Store (RECOMMENDED)

**One-click install with automatic updates!**

### Install
1. Open Microsoft Store
2. Search for "CoreMLWin" or "ML Runtime"
3. Click "Get" / "Install"
4. Done! ✅

### Or use command line:
```powershell
winget install CoreMLWin.UniversalMLRuntime
```

### Benefits:
- ✅ Automatic updates
- ✅ Sandboxed installation
- ✅ Automatic ONNX Runtime bundling
- ✅ DirectML included
- ✅ Uninstall from Settings
- ✅ No admin rights needed

---

## 📦 Method 2: Winget (Windows Package Manager)

**For developers and power users**

### Install:
```powershell
winget install CoreMLWin.UniversalMLRuntime
```

### Upgrade:
```powershell
winget upgrade CoreMLWin.UniversalMLRuntime
```

### Uninstall:
```powershell
winget uninstall CoreMLWin.UniversalMLRuntime
```

### Configuration:
```powershell
# Install to custom location
winget install CoreMLWin.UniversalMLRuntime --location "C:\ML\CoreMLWin"

# Silent install
winget install CoreMLWin.UniversalMLRuntime --silent
```

---

## 🍫 Method 3: Chocolatey

**For IT administrators managing multiple machines**

### Install:
```powershell
choco install coremlwin
```

### Upgrade:
```powershell
choco upgrade coremlwin
```

### Uninstall:
```powershell
choco uninstall coremlwin
```

### Mass Deployment:
```powershell
# Deploy to multiple machines
choco install coremlwin -y --limit-output --no-progress

# Pin version
choco pin add -n=coremlwin --version=0.1.0
```

---

## 💾 Method 4: MSI Installer

**Traditional installer for enterprise environments**

### Download:
- [CoreMLWin-0.1.0-x64.msi](https://github.com/user/WinCoreML/releases/latest)

### Install:

**GUI:**
1. Double-click `CoreMLWin-0.1.0-x64.msi`
2. Follow installation wizard
3. Choose installation directory
4. Click "Install"

**Command Line (Silent):**
```powershell
msiexec /i CoreMLWin-0.1.0-x64.msi /quiet /qn
```

**Command Line (Interactive):**
```powershell
msiexec /i CoreMLWin-0.1.0-x64.msi
```

### Uninstall:
```powershell
# Via Control Panel
appwiz.cpl

# Or command line
msiexec /x CoreMLWin-0.1.0-x64.msi /quiet
```

### Enterprise Deployment:
```powershell
# Install to specific location
msiexec /i CoreMLWin-0.1.0-x64.msi INSTALLDIR="C:\Program Files\CoreMLWin" /quiet

# Enable logging
msiexec /i CoreMLWin-0.1.0-x64.msi /l*v install.log

# Network deployment
msiexec /i \\server\share\CoreMLWin-0.1.0-x64.msi /quiet
```

---

## 📂 Method 5: Portable ZIP

**No installation required - extract and run!**

### Download:
- [CoreMLWin-0.1.0-portable-x64.zip](https://github.com/user/WinCoreML/releases/latest)

### Setup:
```powershell
# Extract
Expand-Archive -Path CoreMLWin-0.1.0-portable-x64.zip -DestinationPath C:\CoreMLWin

# Add to PATH (optional)
$env:PATH += ";C:\CoreMLWin\bin"

# Run
C:\CoreMLWin\bin\coremlwin_service.exe
```

### Benefits:
- ✅ No admin rights needed
- ✅ Runs from USB drive
- ✅ Multiple versions side-by-side
- ✅ Easy to remove (just delete folder)

---

## 🔧 Installation Components

All installation methods include:

### Core Components:
- ✅ CoreMLWin Runtime Service (`coremlwin_service.exe`)
- ✅ Python SDK (`pip install coremlwin`)
- ✅ CLI Tools (`coremlwin_cli.py`)
- ✅ Documentation

### Dependencies (Bundled):
- ✅ ONNX Runtime 1.16.3+ (GPU-enabled)
- ✅ DirectML.dll (Windows GPU acceleration)
- ✅ Protobuf runtime
- ✅ Visual C++ Redistributable 2022

### Optional Components:
- 🔲 Sample models (checkbox during install)
- 🔲 Development headers (for C++ developers)
- 🔲 Debug symbols

---

## 🌐 Python SDK Installation

After installing the runtime, install the Python SDK:

```powershell
pip install coremlwin
```

Or from source:
```powershell
cd C:\Program Files\CoreMLWin\sdk\python
pip install -e .
```

---

## 📍 Installation Paths

### Microsoft Store / MSIX:
```
C:\Program Files\WindowsApps\CoreMLWin_0.1.0_x64\
```

### MSI Installer (Default):
```
C:\Program Files\CoreMLWin\
├── bin\
│   ├── coremlwin_service.exe
│   ├── onnxruntime.dll
│   └── DirectML.dll
├── sdk\
│   └── python\
├── tools\
│   └── coremlwin_cli.py
├── docs\
└── models\
```

### Chocolatey:
```
C:\ProgramData\chocolatey\lib\coremlwin\tools\
```

### Winget:
```
C:\Program Files\CoreMLWin\
```

### Portable:
```
[Your chosen directory]\
```

---

## 🔐 System Requirements

### Minimum:
- **OS**: Windows 10 version 1903 (build 18362) or newer
- **RAM**: 4 GB
- **Storage**: 500 MB free space
- **CPU**: x64 processor

### Recommended:
- **OS**: Windows 11
- **RAM**: 8 GB or more
- **Storage**: 2 GB free space
- **GPU**: DirectX 12 capable (NVIDIA, AMD, or Intel)
- **Drivers**: Latest GPU drivers

### For DirectML GPU Acceleration:
- ✅ Windows 10 version 1903+
- ✅ DirectX 12 capable GPU
- ✅ Latest GPU drivers (WDDM 2.4+)

---

## ⚙️ Post-Installation Setup

### 1. Verify Installation:
```powershell
# Check service version
coremlwin_service.exe --version

# Check Python SDK
python -c "import coremlwin; print(coremlwin.__version__)"
```

### 2. Start Service:

**Automatic (Recommended):**
```powershell
# Windows Service (runs at startup)
sc create CoreMLWin binPath="C:\Program Files\CoreMLWin\bin\coremlwin_service.exe" start=auto
sc start CoreMLWin
```

**Manual:**
```powershell
# Run in terminal
coremlwin_service.exe
```

### 3. Test Installation:
```powershell
# Download sample model
cd C:\Program Files\CoreMLWin\tools
python coremlwin_cli.py health

# Should show:
# ✓ RUNNING (v0.1.0)
```

---

## 🔄 Updating

### Microsoft Store:
Updates automatically or click "Update" in Microsoft Store.

### Winget:
```powershell
winget upgrade CoreMLWin.UniversalMLRuntime
```

### Chocolatey:
```powershell
choco upgrade coremlwin
```

### MSI:
1. Download new MSI
2. Run installer (will upgrade in-place)

### Portable:
1. Download new ZIP
2. Extract to new folder
3. Update PATH if needed

---

## 🗑️ Uninstallation

### Microsoft Store:
```
Settings → Apps → CoreMLWin → Uninstall
```

### Winget:
```powershell
winget uninstall CoreMLWin.UniversalMLRuntime
```

### Chocolatey:
```powershell
choco uninstall coremlwin
```

### MSI:
```powershell
# Via Control Panel
appwiz.cpl

# Or command line
msiexec /x CoreMLWin-0.1.0-x64.msi /quiet
```

### Portable:
Just delete the folder.

---

## 🏢 Enterprise Deployment

### Group Policy Deployment:

1. **Create GPO:**
   ```
   Computer Configuration → Policies → Software Settings → Software Installation
   ```

2. **Add MSI:**
   - Right-click → New → Package
   - Browse to network share: `\\server\share\CoreMLWin-0.1.0-x64.msi`
   - Choose "Assigned" for automatic install

3. **Deploy:**
   - Link GPO to OU
   - Run `gpupdate /force` on clients

### SCCM/Intune Deployment:

**SCCM:**
```xml
<Application Name="CoreMLWin">
  <DeploymentType>
    <Installer>msiexec /i CoreMLWin-0.1.0-x64.msi /quiet</Installer>
    <DetectionMethod>File</DetectionMethod>
    <DetectionPath>C:\Program Files\CoreMLWin\bin\coremlwin_service.exe</DetectionPath>
  </DeploymentType>
</Application>
```

**Intune:**
1. Upload MSI to Intune
2. Configure as "Line-of-Business app"
3. Assign to device groups
4. Deploy

### Docker Container:

For testing/development:
```dockerfile
FROM mcr.microsoft.com/windows/servercore:ltsc2022
COPY CoreMLWin-0.1.0-portable-x64.zip C:\
RUN powershell Expand-Archive C:\CoreMLWin-0.1.0-portable-x64.zip C:\CoreMLWin
EXPOSE 8080
CMD ["C:\\CoreMLWin\\bin\\coremlwin_service.exe"]
```

---

## 🐛 Troubleshooting

### Installation Failed:
- **Check admin rights**: Right-click installer → "Run as administrator"
- **Check disk space**: Need 500 MB minimum
- **Check Windows version**: `winver` (must be 1903+)

### Service Won't Start:
```powershell
# Check if port is in use
netstat -ano | findstr :9001

# Check event log
eventvwr.msc → Windows Logs → Application
```

### DirectML Not Available:
```powershell
# Update GPU drivers
# Check DirectX version
dxdiag
```

### Python SDK Import Error:
```powershell
# Reinstall SDK
pip uninstall coremlwin
pip install coremlwin --force-reinstall
```

---

## 📞 Support

- **Documentation**: `C:\Program Files\CoreMLWin\docs\`
- **GitHub Issues**: https://github.com/user/WinCoreML/issues
- **Email**: support@coremlwin.com

---

## 📋 Comparison Matrix

| Feature | Store | Winget | Choco | MSI | Portable |
|---------|-------|--------|-------|-----|----------|
| Auto-update | ✅ | ✅ | ✅ | ❌ | ❌ |
| No admin | ✅ | ❌ | ❌ | ❌ | ✅ |
| Offline install | ❌ | ❌ | ❌ | ✅ | ✅ |
| Enterprise GPO | ❌ | ❌ | ✅ | ✅ | ❌ |
| USB portable | ❌ | ❌ | ❌ | ❌ | ✅ |
| Silent install | ✅ | ✅ | ✅ | ✅ | N/A |

---

## 🎯 Quick Start

**For end users:**
```
Microsoft Store → Search "CoreMLWin" → Install
```

**For developers:**
```powershell
winget install CoreMLWin.UniversalMLRuntime
pip install coremlwin
```

**For IT admins:**
```powershell
choco install coremlwin -y
```

That's it! 🚀
