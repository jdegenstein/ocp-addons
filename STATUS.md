# OCP Addons Build Status and Approach

## Build Process Overview

### Architecture

The `ocp-addons` package provides native C++ extensions (tessellator, serializer) that link against OpenCASCADE (OCCT) libraries. The build process creates Python wheels that are compatible with the `cadquery_ocp` and `cadquery_ocp_novtk` packages.

### Key Challenge: Library Name Mangling

The `cadquery_ocp` and `cadquery_ocp_novtk` wheels bundle OCCT shared libraries with **mangled names** (hashes appended) for wheel repair compatibility:

- `cadquery_ocp_novtk`: `libTKBRep-071e31b6.so.7.9.3`
- `cadquery_ocp` (vtk): `libTKBRep-571707e3.so.7.9.3`

**The hashes are different between the two packages**, meaning a wheel built against one cannot be used with the other.

### Solution: Build Two Wheel Variants

We build **two separate wheels** for each platform/Python version:

1. **novtk variant**: Built with `cadquery_ocp_novtk`, for use with `cadquery_ocp_novtk`
2. **vtk variant**: Built with `cadquery_ocp`, for use with `cadquery_ocp`

## Build Environment

### Dependencies (environment.yml)

- **Headers**: `occt=7.9.3` from conda-forge (provides C++ headers for compilation)
- **Libraries**: `cadquery_ocp_novtk` or `cadquery_ocp` from PyPI (provides shared libraries for linking)
- **Build tools**: `pybind11`, `setuptools`, `wheel`, `toml`

### Build Steps

1. **setup.py**:
   - Finds OCCT headers from `$CONDA_PREFIX/include/opencascade`
   - Finds OCCT libraries from `cadquery_ocp.libs` or `cadquery_ocp_novtk.libs`
   - Copies/generates import libraries for linking
   - Compiles C++ extensions with `-O3` optimization

2. **fix_libs.py** (post-build patching):
   - **Linux**: Uses `patchelf` to set rpath to both `.libs` directories
   - **macOS**: Uses `install_name_tool` to patch library references
   - **Windows**: Generates `__init__.py` that adds DLL directory to search path
   - Patches library references to match the mangled names in the OCP package

## CI/CD Workflow

### Build Matrix

| Platform | Python Versions | OCP Variants |
|----------|-----------------|--------------|
| ubuntu-22.04 | 3.10-3.14 | novtk, vtk* |
| ubuntu-22.04-arm | 3.10-3.14 | novtk, vtk* |
| macos-15 (ARM) | 3.10-3.14 | novtk, vtk* |
| macos-15-intel | 3.10-3.14 | novtk, vtk* |
| windows-2022 | 3.10-3.14 | novtk, vtk* |

*vtk variant excluded for Python 3.14 (no `cadquery_ocp` available)

### Caching

Wheels are cached by version, OS, Python version, and OCP variant:
```
ocp-addons-{version}-{variant}-wheel-{os}-cp{python}
```

### Testing

Tests run with `uv` for fast package installation:
- Each wheel variant is tested with its matching OCP package
- Tests use pure OCP code (no build123d/cadquery dependency)

### Release

On tag push, all wheel variants are uploaded to GitHub Release.

## Changes Applied (2026-01-24)

### 1. Dual Wheel Variants
- Added `ocp-variant: ["novtk", "vtk"]` to build matrix
- Updated artifact names to include variant
- Build environment installs matching OCP package

### 2. Library Compatibility
- **setup.py**: Checks both `.libs` directories for OCCT libraries
- **fix_libs.py**: Sets rpath to search both directories on Linux:
  ```
  $ORIGIN/../cadquery_ocp.libs:$ORIGIN/../cadquery_ocp_novtk.libs
  ```

### 3. Test Improvements
- Switched from micromamba to `uv` for faster test environment setup
- Removed build123d/cadquery dependency - tests use pure OCP
- Added `--no-project` flag to `uv run` to avoid local pyproject.toml conflicts

### 4. Serializer Enhancement
- Added `tangents` and `normals` parameters to `serialize_shape()`
- Uses `BinTools_FormatVersion_CURRENT` for serialization

### 5. Dependencies
- Added `numpy` as runtime dependency in pyproject.toml
- Updated `cadquery_ocp_proxy` version constraint to `7.9.3.*`

### 6. Workflow Alignment with ocp-build-system
- Uses runner mapping pattern for ARM builds
- Simplified release job with `merge-multiple: true`
- Added ARM Linux support (`ubuntu-22.04-arm`)

## File Structure

```
.github/
  actions/
    build-wheel/action.yml   # Build action with ocp-variant support
    test-wheel/action.yml    # Test action using uv
  workflows/
    build-wheels.yml         # Main CI/CD workflow
src/
  serializer/main.cpp        # Serialization with tangents/normals params
  tessellator/               # Tessellation module
setup.py                     # Build configuration
fix_libs.py                  # Post-build library patching
environment.yml              # Conda build environment
pyproject.toml               # Package metadata
tests/test_all.py            # Pure OCP tests
```
