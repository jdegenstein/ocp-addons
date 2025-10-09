# OCP Addons

## Usage of ocp_addons

- **Serializer**

  ```python
  from ocp_addons.serializer import *
  ```

  Available functions:

  - `serialize_shape()`
  - `deserialize_shape()`
  - `serialize_location()`
  - `deserialize_location()`

- **Optimized Tessellator**

  Will be auto-detected by [ocp_vscode](https://github.com/bernhard-42/vscode_ocp_cad_viewer.git)

  - Disable optimized teasellator: `disable_native_tessellator()`
  - Ensable optimized teasellator: `enable_native_tessellator()`

  Else use `from ocp_addons.tessellator import tessellate`

## Building ocp-addons

### Clone the repository

```bash
git clone https://github.com/jdegenstein/ocp-addons.git
cd ocp-addons
```

### Create the development environment

```bash
micromamba env create -f environment.yml
micromamba activate build-ocp-addons
```

### Build ocp-addons

- Linux (x86_64): `make wheel-linux`
- MacOS (Apple Silicon): `make wheel-macos`
- Windows (Intel): `make wheel-windows`

### Test the library

```bash
cd ..
python test.py
```
