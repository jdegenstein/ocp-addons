# Building ocp-addons

## Clone the repository

```bash
git clone https://github.com/jdegenstein/ocp-addons.git
cd ocp-addons
```

## Create the development environment

### The Python environment

```bash
mamba env create -f env.yml
mamba activate ocp-addons
```

### OS specific configurations

- Linux (x86_64)

    ```bash
    sudo apt-get update
    sudo apt-get install freetype* libfreetype6-dev libgl1-mesa-glx
    mamba install -c conda-forge gxx_linux-64=12
    pip install auditwheel patchelf
    ```

- Linux (aarch64)

    ```bash
    sudo apt-get update
    sudo apt-get install freetype* libfreetype6-dev libgl1-mesa-glx
    mamba install -c conda-forge gxx_linux-aarch64=12
    pip install auditwheel patchelf
    ```

- MacOS (Apple Silicon)

    ```bash
    brew install automake
    pip install delocate
    ```

- Windows (Intel)

    ```bash
    mamba install vs2019_win-64
    pip install delvewheel
    ```


## Build ocp-addons

### Build the pybind11 wheel

```bash
python -m build -n
```

### Relocate the libraries in the wheel

- Linux (x86_64)

    ```bash
    auditwheel repair --plat manylinux_2_35_x86_64 dist/ocp_addons-*.whl
    ```
    
- Linux (aarch64)

    ```bash
    auditwheel repair --plat manylinux_2_35_aarch64 dist/ocp_addons-*.whl
    ```

- MacOS (Apple Silicon)

    ```bash
    DYLD_LIBRARY_PATH=$CONDA_PREFIX/lib python -m delocate.cmd.delocate_wheel \
        --wheel-dir=wheelhouse dist/ocp_addons-*_arm64.whl
    ```

- Windows (Intel)

    ```bash
    python -m delvewheel repair --wheel-dir=wheelhouse dist\ocp_addons-0.1.0-cp311-cp311-win_amd64.whl
    ```
    
## Test the library

```bash
cd ..
python test.py
```



