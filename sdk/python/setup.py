"""
CoreML-on-Windows Python SDK Setup
"""

from setuptools import setup, find_packages
from pathlib import Path

# Read README
readme_file = Path(__file__).parent / "README.md"
long_description = ""
if readme_file.exists():
    long_description = readme_file.read_text(encoding="utf-8")

setup(
    name="coreml-win",
    version="0.1.0",
    author="Universal ML Runtime Contributors",
    author_email="",
    description="Python SDK for Universal ML Runtime - Run PyTorch, TensorFlow, CoreML, ONNX on Windows",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/tropikandy/WinCoreML",
    packages=find_packages(),
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Operating System :: Microsoft :: Windows",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
    ],
    python_requires=">=3.10",
    install_requires=[
        "protobuf>=4.21.0",
        "numpy>=1.21.0",
        "pywin32>=305; platform_system=='Windows'",
    ],
    extras_require={
        "dev": [
            "pytest>=7.0.0",
            "pytest-cov>=4.0.0",
            "black>=23.0.0",
            "mypy>=1.0.0",
            "ruff>=0.1.0",
        ],
        "converter": [
            "coremltools>=7.0",
            "onnx>=1.14.0",
        ],
    },
    entry_points={
        "console_scripts": [
            "coremlwin=coreml_win.cli:main",
        ],
    },
    include_package_data=True,
    zip_safe=False,
)
