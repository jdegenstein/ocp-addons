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

- Linux

    ```bash
    sudo apt-get update
    sudo apt-get install freetype* libfreetype6-dev libgl1-mesa-glx
    mamba install -c conda-forge gxx_linux-64=12
    ```

## Build ocp-addons

### Build the pybind11 wheel

```bash
python -m build -n
```

### Relocate the libraries in the wheel

- Linux

    ```bash
    auditwheel repair --plat manylinux_2_35_x86_64 dist/ocp_addons-*.whl
    ```

## Test the library

```bash
cd ..
python test.py
```



