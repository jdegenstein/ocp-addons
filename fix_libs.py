import os
import platform
import re
import site
import shutil
import subprocess
import sys
import inspect

from pathlib import Path


def execute(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, check=True)
    print(result.stderr)
    return result.stdout


if platform.system() == "Linux":
    so_file = next(Path.glob(Path.cwd(), "ocp_addons.cpython-*.so"))
    so_libs = Path(site.getsitepackages()[0]) / "cadquery_ocp.libs"
    if not so_libs.exists():
        so_libs = Path(site.getsitepackages()[0]) / "cadquery_ocp_novtk.libs"

    # Change the runpath to point to both OCP packages dynamic libs folders
    execute(f"patchelf --set-rpath '$ORIGIN/../cadquery_ocp.libs:$ORIGIN/../cadquery_ocp_novtk.libs' {so_file}")
    dump = execute(f"readelf -d {so_file}").split("\n")

    libs = [
        re.search(r"\[([^\]]+)\]", line).group(1) for line in dump if "libTK" in line
    ]
    for lib in libs:
        ocp_lib = next(Path.glob(so_libs, f"{lib.split('.')[0]}*")).name

        print(" -", lib, "==>", ocp_lib)

        # Change the neede libs to have the same version numbers as in the OCP package
        execute(f"patchelf --replace-needed {lib} {ocp_lib} {so_file}")

elif platform.system() == "Darwin":
    so_file = next(Path.glob(Path.cwd(), "ocp_addons.cpython-*.so"))

    # Patch libc++.1.dylib since rpath is not always found
    execute(
        f"install_name_tool -change @rpath/libc++.1.dylib /usr/lib/libc++.1.dylib {so_file}"
    )

    if sys.version_info >= (3, 12):
        # Delete CONDA rpath
        execute(
            f"install_name_tool -delete_rpath {os.environ['CONDA_PREFIX']}/lib {so_file}"
        )

    # Add vtk rpath
    execute(
        f"install_name_tool -add_rpath @loader_path/../vtkmodules/.dylibs/ {so_file}"
    )

    dump = execute(f"otool -L {so_file}").split("\n")

    prefix = "@rpath" if sys.version_info >= (3, 12) else "/DLC/OCP/.dylibs"

    rpaths = [
        re.search(rf"({prefix}/lib[\w\.]+\.dylib)", line).group(1)
        for line in dump
        if prefix in line
    ]

    dylibs = Path(site.getsitepackages()[0]) / "OCP" / ".dylibs"
    for rpath in rpaths:
        lib = re.search(r"(lib\w+)\.\d+", rpath).group(1)
        ocp_lib = next(Path.glob(dylibs, f"{lib}*.dylib")).name

        print(" -", rpath, "==>", ocp_lib)

        # Patch the actual library of the OCP wheel in the ocp_addons lib
        execute(
            f"install_name_tool -change {rpath} @loader_path/../OCP/.dylibs/{ocp_lib} {so_file}"
        )
elif platform.system() == "Windows":
    so_file = next(Path.glob(Path.cwd(), "ocp_addons.cp*.pyd"))

# Build package tree

target = Path("ocp_addons")
Path.mkdir(target, exist_ok=True)

shutil.move(so_file, target)

with open(target / "__init__.py", "w") as fd:
    if platform.system() == "Windows":
        fd.write("""
def _ocpmodules():
    import os

    for libs_name in ("cadquery_ocp.libs", "cadquery_ocp_novtk.libs"):
        libs_dir = os.path.abspath(
            os.path.join(os.path.dirname(__file__), os.pardir, libs_name)
        )
        if os.path.exists(libs_dir):
            os.add_dll_directory(libs_dir)
            break
_ocpmodules()
del _ocpmodules

# Import OCP first to ensure type registrations are in place for pybind11
import OCP.TopoDS
import OCP.TopLoc

from ocp_addons.ocp_addons import *""")

    else:
        fd.write("""from ocp_addons.ocp_addons import *""")

for module in sys.argv[1:]:
    Path.mkdir(target / module, exist_ok=True)
    p2 = Path(target / module)
    with open(p2 / "__init__.py", "w") as f:
        f.write(f"from ..ocp_addons.{module} import *\n")
