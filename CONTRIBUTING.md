# Contributing to CoreMLWin

Thank you for your interest in contributing to CoreMLWin! We welcome contributions from the community.

---

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Contribution Process](#contribution-process)
- [Developer Certificate of Origin](#developer-certificate-of-origin)
- [Coding Guidelines](#coding-guidelines)
- [Testing Requirements](#testing-requirements)

---

## 📜 Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inspiring community for all. Please be respectful and professional in all interactions.

### Our Standards

**Positive behavior includes:**
- Using welcoming and inclusive language
- Being respectful of differing viewpoints
- Gracefully accepting constructive criticism
- Focusing on what is best for the community

**Unacceptable behavior includes:**
- Harassment, trolling, or insulting/derogatory comments
- Public or private harassment
- Publishing others' private information without permission
- Other conduct which could reasonably be considered inappropriate

### Enforcement

Instances of abusive behavior may be reported by contacting the project maintainers. All complaints will be reviewed and investigated.

---

## 🤝 How Can I Contribute?

### Reporting Bugs

**Before submitting a bug report:**
1. Check the [GitHub Issues](https://github.com/user/WinCoreML/issues) for existing reports
2. Ensure you're using the latest version
3. Collect relevant information (OS version, GPU model, error logs)

**Bug report should include:**
- Clear, descriptive title
- Steps to reproduce
- Expected vs. actual behavior
- System information (Windows version, GPU, CoreMLWin version)
- Error messages or logs

**Template:**
```markdown
**Environment:**
- OS: Windows 11 22H2
- GPU: NVIDIA RTX 3070
- CoreMLWin version: 0.1.0

**Steps to reproduce:**
1. Install CoreMLWin
2. Load model.onnx
3. Run inference

**Expected:** Model loads successfully
**Actual:** Error: "Failed to load model"

**Logs:**
[Paste error logs here]
```

### Suggesting Enhancements

We welcome feature requests! Please:
1. Check if the feature is already requested
2. Explain the use case (why you need it)
3. Provide examples if possible

### Code Contributions

We accept pull requests for:
- Bug fixes
- Performance improvements
- New features (discuss first in an issue)
- Documentation improvements
- Test coverage improvements

---

## 💻 Development Setup

### Prerequisites

- Windows 10 version 1903 (build 18362) or newer
- Visual Studio 2019/2022 with C++ tools
- CMake 3.15+
- Python 3.8+
- Git

### Setup Steps

```powershell
# 1. Fork and clone
git clone https://github.com/YOUR_USERNAME/WinCoreML.git
cd WinCoreML

# 2. Download ONNX Runtime
$version = "1.16.3"
Invoke-WebRequest -Uri "https://github.com/microsoft/onnxruntime/releases/download/v$version/onnxruntime-win-x64-gpu-$version.zip" -OutFile "onnxruntime.zip"
Expand-Archive -Path "onnxruntime.zip" -DestinationPath "."
$env:ONNXRUNTIME_DIR = (Get-ChildItem -Directory -Filter "onnxruntime-win-*").FullName

# 3. Build
cmake -B build -DCMAKE_BUILD_TYPE=Release -DONNXRUNTIME_DIR=$env:ONNXRUNTIME_DIR
cmake --build build --config Release --parallel

# 4. Install Python SDK in development mode
cd sdk/python
pip install -e .
cd ../..

# 5. Run tests
pytest tests/test_integration.py -v
```

---

## 🔄 Contribution Process

### 1. Create an Issue (for significant changes)

For bug fixes or new features, create an issue first to discuss the approach.

### 2. Fork and Create a Branch

```bash
# Fork on GitHub, then:
git clone https://github.com/YOUR_USERNAME/WinCoreML.git
cd WinCoreML
git checkout -b feature/your-feature-name
```

**Branch naming conventions:**
- `feature/add-rest-api` - New features
- `fix/memory-leak-in-executor` - Bug fixes
- `docs/improve-installation-guide` - Documentation
- `perf/optimize-tensor-copy` - Performance improvements

### 3. Make Your Changes

- Follow the [Coding Guidelines](#coding-guidelines)
- Add tests for new functionality
- Update documentation as needed
- Keep commits focused and atomic

### 4. Test Your Changes

```powershell
# Run all tests
pytest tests/ -v

# Run C++ security tests
.\tests\test_security_utils.exe

# Test on different hardware if possible
# (Intel GPU, AMD GPU, NVIDIA GPU)
```

### 5. Commit with Sign-Off

**IMPORTANT**: All commits must include a sign-off (Developer Certificate of Origin).

```bash
git commit -s -m "Add feature X"

# The -s flag adds:
# Signed-off-by: Your Name <your.email@example.com>
```

**Good commit messages:**
```
fix: Resolve memory leak in ONNX executor cleanup

- Added proper session cleanup in destructor
- Verified with Valgrind, no leaks detected
- Fixes #123

Signed-off-by: Jane Doe <jane@example.com>
```

**Commit message format:**
- **Type**: `feat`, `fix`, `docs`, `perf`, `test`, `refactor`, `chore`
- **Subject**: Imperative mood, max 50 characters
- **Body**: Explain what and why (not how)
- **Footer**: Reference issues ("Fixes #123")

### 6. Push and Create Pull Request

```bash
git push origin feature/your-feature-name
```

Then create a PR on GitHub:

**PR Description Template:**
```markdown
## Description
Brief description of changes

## Motivation
Why is this change needed?

## Testing
How was this tested?

## Checklist
- [ ] Code follows project style guidelines
- [ ] Tests added/updated
- [ ] Documentation updated
- [ ] All tests passing
- [ ] Commits signed off (DCO)
```

### 7. Code Review

Maintainers will review your PR. Be prepared to:
- Answer questions
- Make requested changes
- Rebase if needed

### 8. Merge

Once approved, a maintainer will merge your PR. Congratulations! 🎉

---

## ✍️ Developer Certificate of Origin

**By contributing to CoreMLWin, you agree to the Developer Certificate of Origin (DCO):**

```
Developer Certificate of Origin
Version 1.1

Copyright (C) 2004, 2006 The Linux Foundation and its contributors.

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

**What this means:**
- You created the code OR have the right to contribute it
- You're granting CoreMLWin a license to use your contribution
- Your contribution will be under the MIT License

**How to sign off:**
```bash
# Every commit must be signed off
git commit -s -m "Your commit message"

# If you forgot to sign off, amend the last commit:
git commit --amend -s --no-edit
```

---

## 📝 Coding Guidelines

### C++ Style

**Follow existing code style:**
- Use 4 spaces for indentation (no tabs)
- Class names: `PascalCase`
- Function names: `PascalCase`
- Variable names: `snake_case`
- Constants: `UPPER_SNAKE_CASE`
- File names: `snake_case.cpp`

**Example:**
```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 CoreMLWin Contributors

#include "model_registry.h"

namespace coremlwin {

class ModelRegistry {
public:
    CmwErrorCode RegisterModel(const std::string& model_path);

private:
    std::unordered_map<std::string, ModelInfo> models_;
    static constexpr size_t MAX_MODELS = 1000;
};

} // namespace coremlwin
```

**Best practices:**
- Use RAII for resource management
- Prefer `std::unique_ptr` over raw pointers
- Use `const` liberally
- Check return values
- Add comments for non-obvious code

### Python Style

**Follow PEP 8:**
- 4 spaces for indentation
- Function/variable names: `snake_case`
- Class names: `PascalCase`
- Constants: `UPPER_SNAKE_CASE`
- Max line length: 100 characters

**Example:**
```python
"""
SPDX-License-Identifier: MIT
Copyright (c) 2025 CoreMLWin Contributors

Model management client for CoreMLWin.
"""

class CoreMLWinClient:
    """Client for CoreMLWin runtime service."""

    def register_model(self, model_path: str) -> str:
        """Register a model with the runtime.

        Args:
            model_path: Path to ONNX model file

        Returns:
            Model ID for future inference requests

        Raises:
            CoreMLWinError: If model registration fails
        """
        # Implementation
```

**Best practices:**
- Use type hints
- Write docstrings (Google style)
- Use `pathlib` for file paths
- Handle exceptions appropriately

### Documentation

- Update README.md if changing user-facing features
- Add docstrings to all public functions/classes
- Update CHANGELOG.md for significant changes
- Keep code comments up to date

---

## 🧪 Testing Requirements

### For Bug Fixes

- Add a test that reproduces the bug
- Verify the test fails before the fix
- Verify the test passes after the fix

### For New Features

- Add unit tests for new functions/classes
- Add integration tests for user-facing features
- Aim for >80% code coverage on new code

### Test Structure

**C++ Tests:**
```cpp
// tests/test_model_registry.cpp
TEST(ModelRegistry, RegisterValidModel) {
    ModelRegistry registry;
    auto result = registry.RegisterModel("valid_model.onnx");
    ASSERT_EQ(result, CMW_SUCCESS);
}
```

**Python Tests:**
```python
# tests/test_client.py
def test_register_model_success():
    """Test successful model registration."""
    client = CoreMLWinClient()
    model_id = client.register_model("test_model.onnx")
    assert isinstance(model_id, str)
    assert len(model_id) > 0
```

### Running Tests

```powershell
# Python tests
pytest tests/ -v --cov=coremlwin_client

# C++ tests (when available)
.\build\bin\Release\coremlwin_tests.exe

# Integration tests
pytest tests/test_integration.py -v
```

---

## 🔒 Security

### Reporting Security Vulnerabilities

**DO NOT** open a public GitHub issue for security vulnerabilities.

Instead:
1. Email security concerns to: [SECURITY_EMAIL_HERE]
2. Use subject line: "SECURITY: [brief description]"
3. We'll respond within 48 hours

See [SECURITY.md](SECURITY.md) for our security policy.

### Security Guidelines

- Never commit secrets (API keys, passwords)
- Validate all user input
- Use parameterized queries (if adding database)
- Follow principle of least privilege
- Review [docs/RED_TEAM_AUDIT.md](docs/RED_TEAM_AUDIT.md)

---

## 📚 Additional Resources

- [Architecture Overview](docs/ARCHITECTURE.md)
- [Security Audit Report](docs/RED_TEAM_AUDIT.md)
- [Distribution Guide](docs/DISTRIBUTION_GUIDE.md)
- [Business Model](docs/BUSINESS_MODEL.md)

---

## 🎉 Recognition

Contributors will be:
- Listed in CONTRIBUTORS.md
- Credited in release notes
- Eligible for swag (when available)

---

## ❓ Questions?

- **General questions**: [GitHub Discussions](https://github.com/user/WinCoreML/discussions)
- **Bug reports**: [GitHub Issues](https://github.com/user/WinCoreML/issues)
- **Security**: [SECURITY_EMAIL_HERE]

---

**Thank you for contributing to CoreMLWin!** 🚀

Every contribution, big or small, makes a difference.
