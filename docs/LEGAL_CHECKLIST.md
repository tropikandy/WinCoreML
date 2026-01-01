# Pre-Launch Legal & IP Checklist

**Status**: ⚠️ Action required before public launch

Critical legal and intellectual property items to address before making CoreMLWin publicly available.

---

## ✅ Already Done

- [x] **MIT License file** exists (`LICENSE`)
- [x] Copyright holder: "CoreMLWin Contributors" (2025)
- [x] Permissive license (MIT) allows commercial use

---

## 🚨 Action Required Before Launch

### 1. **Clarify Copyright Ownership** ⭐ CRITICAL

**Current status**: LICENSE says "CoreMLWin Contributors" (generic)

**Decision needed**: Who legally owns this code?

**Option A: You own it personally (RECOMMENDED for solo project)**
```
Copyright (c) 2025 [Your Legal Name]
```

**Pros:**
- ✅ Simple, clear ownership
- ✅ You control future licensing decisions
- ✅ Easy to dual-license later (Open Core model)
- ✅ Can assign to LLC/company if you incorporate later

**Cons:**
- ⚠️ Your personal liability (mitigated by MIT "AS IS" clause)

---

**Option B: Create an LLC/company first**
```
Copyright (c) 2025 CoreMLWin, LLC
```

**Pros:**
- ✅ Limited liability protection
- ✅ Professional appearance
- ✅ Easier to raise funding later

**Cons:**
- ❌ Cost: $100-500 to form LLC
- ❌ Ongoing compliance (annual reports, taxes)
- ❌ Delays launch by 1-2 weeks

---

**Option C: Keep generic "CoreMLWin Contributors"**
```
Copyright (c) 2025 CoreMLWin Contributors
```

**Pros:**
- ✅ No action needed
- ✅ Works for multi-contributor projects

**Cons:**
- ⚠️ Unclear who can enforce the license
- ⚠️ Harder to dual-license later (need all contributors' permission)
- ⚠️ May confuse potential buyers/investors

---

**RECOMMENDATION**: Use **Option A** (your name) for now. You can always:
1. Transfer copyright to an LLC later
2. Add "and contributors" for future contributions
3. Use a Contributor License Agreement (CLA) to maintain control

**Action:**
```bash
# Update LICENSE file
sed -i 's/CoreMLWin Contributors/[Your Legal Name]/' LICENSE
```

---

### 2. **AI-Generated Code Disclosure** ⚠️ IMPORTANT

**Reality check**: Much of this codebase was AI-assisted (Claude/Anthropic).

**Legal considerations:**

**Copyright status of AI-generated code (as of 2025):**
- **US Copyright Office**: AI-generated works have NO copyright protection
- **BUT**: Human-authored portions ARE copyrightable
- **Hybrid works** (human + AI): Copyrightable to extent of human contribution

**This codebase:**
- Architecture/design: Human-directed
- Implementation: AI-generated with human review/editing
- **Likely status**: Copyrightable as "collaborative work"

**Required disclosures:**

**Option A: Minimal (legally safe)**
Add to README.md:
```markdown
## AI Assistance Acknowledgment

Portions of this codebase were developed with assistance from AI language models.
All code has been reviewed, tested, and validated by human developers.
```

**Option B: Transparent (community-friendly)**
Add to README.md + new file `AI_DISCLOSURE.md`:
```markdown
## Development Methodology

CoreMLWin was developed using AI-assisted programming:
- Architecture and design: Human-directed
- Code generation: AI-assisted (Claude by Anthropic)
- Review, testing, security: Human-validated
- All code is provided under MIT License regardless of origin
```

**Option C: Full disclosure (academic/research contexts)**
Document which files/functions were AI-generated vs. human-written.

**RECOMMENDATION**: **Option B** (transparent). The AI assistance:
- Doesn't diminish the value (design decisions are yours)
- Doesn't affect licensing (MIT covers it)
- Shows modern development practices
- Builds trust with community

**Action**: Add AI disclosure section to README.md

---

### 3. **Contributor License Agreement (CLA)** 📝

**Why you need this:**
If you plan to accept external contributions AND want to dual-license later (Open Core model), you need rights to relicense contributors' code.

**Two approaches:**

**Approach A: Developer Certificate of Origin (DCO) - Simpler**
Contributors sign off commits:
```bash
git commit -s -m "Add feature X"
# Adds: Signed-off-by: Jane Doe <jane@example.com>
```

**Pros:**
- ✅ Simple, widely accepted (Linux kernel uses this)
- ✅ No paperwork

**Cons:**
- ❌ Doesn't grant relicensing rights
- ❌ Can't dual-license without re-contacting everyone

---

**Approach B: Contributor License Agreement (CLA) - Stronger**
Contributors grant you perpetual rights to their contributions.

**Template CLA** (see `CLA.md` below)

**Pros:**
- ✅ Allows future dual-licensing
- ✅ Clear IP ownership
- ✅ Professional

**Cons:**
- ❌ Friction for contributors (extra step)
- ❌ Requires CLA management (CLA Assistant bot)

---

**RECOMMENDATION**:
- **Now**: Nothing (you're the only contributor)
- **Before accepting first external PR**: Add CONTRIBUTING.md with DCO requirement
- **Before dual-licensing**: Upgrade to CLA (grandfather in DCO contributors)

---

### 4. **Trademark Considerations** ™️

**The name "CoreMLWin":**

**Potential issues:**
- "CoreML" is Apple's trademark
- Adding "Win" (Windows) doesn't automatically make it safe

**Risk assessment:**
- **Low risk**: Descriptive use ("CoreML for Windows")
- **Medium risk**: Name similarity could cause confusion
- **Likelihood Apple sues**: Very low (different platforms, not competing)
- **BUT**: Apple could send cease & desist

**Options:**

**Option A: Keep "CoreMLWin" (RECOMMENDED)**
**Rationale:**
- Clearly describes what it does
- Not confusingly similar (different platform)
- Community already uses "CoreML" to describe this category
- Parallel: "PyTorch" (Facebook) → "PyTorch Lightning" (third-party) ✅

**Mitigation:**
- Don't claim affiliation with Apple
- Add disclaimer: "Not affiliated with Apple Inc."
- Use descriptive tagline: "Apple CoreML-like experience for Windows"

---

**Option B: Rebrand to avoid any risk**
**Alternative names:**
- **WinML** - Wait, Microsoft used this (deprecated)
- **DirectML Runtime** - Too generic
- **TensorWin** - Available
- **ONNXWin** - Technically accurate
- **AccelRT** (Accelerated Runtime)

**Only rebrand if:**
- You get a cease & desist (unlikely)
- You want to be extra cautious

---

**Trademark registration:**
- **Don't register trademark yet** (costs $350-500, not worth it pre-revenue)
- **Do**: Use ™ symbol (common law trademark, free)
  - "CoreMLWin™" in marketing materials
- **Later** (after $100K revenue): Register with USPTO

---

### 5. **Third-Party License Compliance** ✅

**Dependencies you're using:**

| Dependency | License | Compatible with MIT? | Attribution Required? |
|------------|---------|---------------------|---------------------|
| **ONNX Runtime** | MIT | ✅ Yes | ✅ Yes (include license) |
| **DirectML** | Part of Windows | ✅ Yes (Microsoft) | ❌ No (system library) |
| **Protocol Buffers** | BSD 3-Clause | ✅ Yes | ✅ Yes (include license) |
| **Python** | PSF License | ✅ Yes | ❌ No (runtime) |

**Action required:**

Create `THIRD_PARTY_LICENSES.md`:
```markdown
# Third-Party Licenses

## ONNX Runtime (MIT License)
Copyright (c) Microsoft Corporation

[Full ONNX Runtime license text...]

## Protocol Buffers (BSD 3-Clause)
Copyright (c) Google LLC

[Full Protobuf license text...]
```

**Why this matters:**
- Legal requirement to include upstream licenses
- Shows good open source citizenship
- Protects you from infringement claims

---

### 6. **Update Copyright Headers** (Optional but Professional)

**Current**: No copyright headers in source files

**Add to all `.cpp`, `.h`, `.py` files:**

```cpp
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 [Your Name]
```

**Why:**
- Makes copyright clear in every file
- Required for some corporate users (compliance scanners)
- Professional appearance

**How:**
```bash
# Automated header addition (be careful!)
find runtime -name "*.cpp" -o -name "*.h" | while read file; do
  echo "// SPDX-License-Identifier: MIT" > temp
  echo "// Copyright (c) 2025 [Your Name]" >> temp
  echo "" >> temp
  cat "$file" >> temp
  mv temp "$file"
done
```

**RECOMMENDATION**: Add headers after clarifying copyright ownership (step 1)

---

### 7. **Export Control Compliance** 🌍

**Encryption considerations:**

**CoreMLWin uses:**
- SHA-256 hashing (CryptoAPI)
- No encryption for confidentiality
- No cryptographic key exchange

**Export classification:**
- **Likely**: EAR99 (not controlled)
- **No BIS license required** for open source software

**Action**: None required (SHA-256 is freely exportable)

---

### 8. **Privacy Policy** (If collecting telemetry)

**Current**: CoreMLWin doesn't collect data (local-only)

**If you add telemetry later:**
- Must disclose data collection
- GDPR compliance (EU users)
- CCPA compliance (California users)

**RECOMMENDATION**: Stay telemetry-free for v0.1 (simplicity)

---

### 9. **Terms of Service** (For commercial licensing)

**Current**: Not needed (100% open source)

**When to add:**
- If offering SaaS version (CoreMLWin Cloud)
- If selling commercial licenses
- If providing paid support

**Template**: Use Standard Commercial SaaS ToS (get lawyer to review)

---

## 📋 Pre-Launch Checklist

### Critical (Do before GitHub public launch):

- [ ] **Update LICENSE copyright** to your legal name
- [ ] **Add AI disclosure** to README.md
- [ ] **Create THIRD_PARTY_LICENSES.md** with ONNX Runtime + Protobuf licenses
- [ ] **Add disclaimer**: "Not affiliated with Apple Inc." to README
- [ ] **Review README** for any claims you can't substantiate

### Important (Do within first month):

- [ ] **Add copyright headers** to source files
- [ ] **Create CONTRIBUTING.md** with DCO requirement
- [ ] **Trademark usage**: Use "CoreMLWin™" in marketing
- [ ] **Get lawyer review** if planning commercial licensing ($500-1,000 for IP attorney consult)

### Nice-to-have (Do before first $1 revenue):

- [ ] **Contributor License Agreement** (CLA.md)
- [ ] **Code of Conduct** (use Contributor Covenant template)
- [ ] **Security Policy** (SECURITY.md with responsible disclosure)
- [ ] **Patent pledge** (if applicable)

---

## 🔍 Copyright Ownership - Deep Dive

### What happens if you use your personal name?

**Scenario 1: You stay solo**
- You own 100% of copyright
- You control all licensing decisions
- No problems

**Scenario 2: You incorporate later**
- File copyright assignment from yourself to company
- Standard legal procedure, costs $0-500
- Example: "John Doe assigns all CoreMLWin copyright to CoreMLWin, LLC"

**Scenario 3: You get contributors**
- **Option A**: Add "and contributors" to copyright
  - `Copyright (c) 2025 [Your Name] and contributors`
- **Option B**: Require CLA (they assign copyright to you)
  - You maintain 100% control

**Scenario 4: You want to dual-license (Open Core)**
- **If you own copyright**: Easy, just add commercial license
- **If shared with contributors**: Need permission from EVERY contributor
  - Example: MongoDB had to get 1,000+ contributors to sign new license 😱

**RECOMMENDATION**: Own copyright yourself + use CLA for future contributors

---

### What happens if you keep "CoreMLWin Contributors"?

**Problem**: "CoreMLWin Contributors" isn't a legal entity.

**Who can enforce the license?**
- Each contributor individually
- No single point of control

**Who can dual-license?**
- Need unanimous consent of ALL contributors
- Nearly impossible with >10 contributors

**Real-world example:**
- **Firefox**: Owned by Mozilla Foundation (clear ownership) ✅
- **Linux**: "Linus Torvalds and contributors" (can't relicense) ❌

**If you want Open Core model → Must have clear copyright ownership**

---

## 🎯 Recommended Actions (Prioritized)

### **Do TODAY (30 minutes):**

1. **Decide on copyright holder**
   - Use your legal name (recommended)
   - Update `LICENSE` file

2. **Add AI disclosure to README**
   ```markdown
   ## Development
   CoreMLWin was developed using AI-assisted programming techniques.
   All code has been reviewed, tested, and validated for production use.
   ```

3. **Add Apple disclaimer**
   ```markdown
   CoreMLWin is not affiliated with, endorsed by, or sponsored by Apple Inc.
   CoreML is a trademark of Apple Inc.
   ```

### **Do THIS WEEK (2-3 hours):**

4. **Create THIRD_PARTY_LICENSES.md**
   - Copy ONNX Runtime MIT license
   - Copy Protocol Buffers BSD license

5. **Create CONTRIBUTING.md**
   - DCO requirement for now
   - Placeholder for future CLA

6. **Review all marketing claims**
   - Ensure benchmarks are accurate
   - No false comparisons
   - Appropriate disclaimers

### **Do BEFORE COMMERCIAL LAUNCH (when revenue starts):**

7. **Consult IP attorney** ($500-1,000 one-time)
   - Review licensing strategy
   - Trademark clearance
   - CLA template review

8. **Form LLC/company** (if doing Open Core)
   - Limited liability protection
   - Cleaner for commercial licensing
   - Cost: $100-500 + $50-300/year

9. **Register trademark** (optional, $350-500)
   - Only if brand is valuable
   - Only after revenue validation

---

## 📄 Template: CONTRIBUTING.md

```markdown
# Contributing to CoreMLWin

Thank you for your interest in contributing!

## Developer Certificate of Origin

By contributing to CoreMLWin, you agree to the Developer Certificate of Origin (DCO):

> By making a contribution to this project, I certify that:
> (a) I created the contribution entirely or have sufficient rights to submit it under MIT License
> (b) I grant CoreMLWin perpetual, worldwide, non-exclusive, royalty-free license

## How to Contribute

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Sign your commits: `git commit -s -m "Your message"`
6. Submit a pull request

## Code of Conduct

Be respectful and professional. We follow the Contributor Covenant.
```

---

## 🛡️ Liability Protection

**Q: Can I be sued for bugs in CoreMLWin?**

**A: MIT License protects you** (but not 100%):

**MIT "AS IS" clause:**
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND

**What this means:**
- Users accept all risk
- No guarantee of fitness for purpose
- No liability for damages

**Exceptions where you COULD be liable:**
- **Intentional harm**: You knowingly ship malware
- **Negligence**: You know about critical security bug and ignore it
- **Misrepresentation**: You claim it's "HIPAA compliant" when it's not

**Protection strategies:**

1. **Disclaimers in README:**
```markdown
## Disclaimer
CoreMLWin is provided "as is" without warranty. Use in production at your own risk.
Not suitable for safety-critical applications (medical devices, aviation, etc.).
```

2. **Form an LLC** (later):
- Separates personal assets from business liability
- Cost: $100-500 to form, $50-300/year to maintain

3. **Get liability insurance** (if offering paid support):
- E&O (Errors & Omissions) insurance
- Cost: $500-2,000/year for small business
- Only needed if you have revenue

**For now (open source, no revenue):**
- MIT License is sufficient
- Add disclaimers to README
- Don't make guarantees you can't keep

---

## ⚖️ Final Recommendation

**Minimum viable legal setup (DO BEFORE PUBLIC LAUNCH):**

1. ✅ Update `LICENSE` with your legal name
2. ✅ Add `THIRD_PARTY_LICENSES.md`
3. ✅ Add AI disclosure to `README.md`
4. ✅ Add Apple disclaimer to `README.md`
5. ✅ Create `CONTRIBUTING.md` with DCO

**Total time**: 1-2 hours
**Total cost**: $0

**This gives you:**
- Clear copyright ownership
- Ability to dual-license later
- Compliance with upstream licenses
- Protection from trademark issues
- Transparent AI disclosure

**Everything else (LLC, trademark, CLA, attorney) can wait until:**
- You have users/contributors
- You have revenue
- You need to dual-license

---

**You're in great shape!** MIT license is solid, you just need to clarify ownership and add attribution. The legal heavy lifting can come later when there's money involved.

Ready to make these changes? I can update the files for you.
