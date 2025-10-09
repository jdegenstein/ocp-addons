import os
import re
import site
import subprocess
from pathlib import Path


def execute(cmd):
    return subprocess.run(cmd, shell=True, capture_output=True, text=True).stdout


so_file = next(Path.glob(Path.cwd(), "ocp_addons-*/ocp_addons.cpython-*-darwin.so"))

dylibs = Path(site.getsitepackages()[0]) / "OCP" / ".dylibs"

execute(
    f"install_name_tool -change @rpath/libc++.1.dylib /usr/lib/libc++.1.dylib {so_file}"
)
execute(f"install_name_tool -delete_rpath {os.environ['CONDA_PREFIX']}/lib {so_file}")

so_libs = execute(f"otool -L {so_file}").split("\n")

rpaths = [
    re.search(r"(@rpath/lib[\w\.]+\.dylib)", line).group(1)
    for line in so_libs
    if "@rpath" in line
]

for rpath in rpaths:
    lib = re.search(r"(lib\w+)\.\d+", rpath).group(1)
    ocp_lib = next(Path.glob(dylibs, f"{lib}*.dylib")).name
    print(" -", rpath, "==>", ocp_lib)
    execute(
        f"install_name_tool -change {rpath} @loader_path/OCP/.dylibs/{ocp_lib} {so_file}"
    )
