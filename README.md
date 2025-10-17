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

On Linux for OCP 7.8.1 also install g++ 9.x

```
sudo add-apt-repository ppa:ubuntu-toolchain-r/ppa -y
sudo apt update
sudo apt install gcc-9 g++-9
```

### Build ocp-addons

Edit `pyproject.toml` and change dependencies = `["cadquery_ocp==7.8.1.1.post1"]` under `[project]` to the version you want to build `ocp-addons` for

Then

- Linux (x86_64): `make wheel-linux`
- MacOS (Apple Silicon): `make wheel-macos`
- Windows (Intel): `make wheel-windows`

### Test the library

```bash
mkdir test
cd test
uv venv -p 3.12
source .venv/bin/activate
uv pip install ../wheelhouse/ocp_addons-1.0.0.rc1-cp312-cp312-macosx_11_0_arm64.whl
```

This should be installed for cadquery_ocps 7.8.1.1.post1:

```text
Resolved 14 packages in 5ms
Installed 14 packages in 33ms
 + cadquery-ocp==7.8.1.1.post1
 + contourpy==1.3.3
 + cycler==0.12.1
 + fonttools==4.60.1
 + kiwisolver==1.4.9
 + matplotlib==3.10.7
 + numpy==2.3.3
 + ocp-addons==1.0.0.rc1 (from file:///Users/bernhard/Development/CAD/ocp-addons/wheelhouse/ocp_addons-1.0.0.rc1-cp312-cp312-macosx_11_0_arm64.whl)
 + packaging==25.0
 + pillow==11.3.0
 + pyparsing==3.2.5
 + python-dateutil==2.9.0.post0
 + six==1.17.0
 + vtk==9.3.1
```

Now install build123d and ocp_vscode and run the test:

```bash
uv pip install build123d ocp_vscode

cp  -R ../examples ../test.py .  # or similar under Windows
python test.py
```
