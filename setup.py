import os
import platform
import site
import shutil
import sys
import toml
from pathlib import Path

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

version = toml.load("pyproject.toml")["project"]["version"]
description = toml.load("pyproject.toml")["project"]["description"]
print(sys.argv)
occ_libs = [
    "TKG3d",
    "TKTopAlgo",
    "TKMesh",
    "TKBRep",
    "TKGeomAlgo",
    "TKGeomBase",
    "TKG2d",
    "TKMath",
    "TKShHealing",
    "TKernel",
]

occt_sdk = Path(os.environ["CONDA_PREFIX"]) / "include" / "opencascade"
print("OCCT includes:", str(occt_sdk))

site_pkgs = Path(site.getsitepackages()[0])
print("Site-packages:", str(site_pkgs))

extra_compile_args = []
extra_link_args = []

if platform.system() == "Linux":
    
    def get_libs(ocp_path):
        local_libs = Path("libs")
        libs_exists = Path.exists(local_libs)

        print(f"Copying libs in {ocp_path}")

        Path.mkdir(local_libs, exist_ok=True)

        libs = []
        for lib in occ_libs:
            target = f"lib{lib}.so"
            so_lib = next(Path.glob(ocp_path, f"lib{lib}*.so.*"))
            lib_name = so_lib.name[3:].split("-")[0]
            if not libs_exists:
                print("  - ", so_lib, "==>", target)
                shutil.copy(so_lib, f"./libs/lib{lib_name}.so")
            libs.append(lib_name)

        return libs

    ocp_path = site_pkgs / "cadquery_ocp.libs"
    print("ocp_path", str(ocp_path))
    include_dirs = [str(occt_sdk)]
    library_dirs = ["./libs"]
    extra_compile_args.extend([
        "-O3", 
        "-Wno-deprecated-declarations", 
        "-D_GLIBCXX_USE_CXX11_ABI=0"]
    )

elif platform.system() == "Darwin":

    def get_libs(ocp_path):
        print(f"Finding libs in {ocp_path}")
        libs = []
        for lib in occ_libs:
            dylib = next(Path.glob(ocp_path, f"lib{lib}*.dylib"))
            lib_name = dylib.name[3:].replace(".dylib", "")
            libs.append(lib_name)
        return libs

    ocp_path = site_pkgs / "OCP" / ".dylibs"
    print("ocp_path", str(ocp_path))
    include_dirs = [str(occt_sdk)]
    library_dirs = [str(ocp_path)]

    extra_compile_args.extend([
        "-O3",
        "-Wno-deprecated-declarations",
        "-mmacosx-version-min=11.1",
    ])
    extra_link_args.extend(
        [
            "-Wl,-headerpad_max_install_names",
            "-mmacosx-version-min=11.1",
        ],
    )

elif platform.system() == "Windows":
    include_dirs = [str(occt_sdk / "inc")]
    library_dirs = [str(occt_sdk / "win64/vc14/lib")]

else:
    raise RuntimeError(f"Platform {platform.system()} is not supported")

local_occ_libs = get_libs(ocp_path)

ext_modules = [
    Pybind11Extension(
        "ocp_addons",
        [
            "src/modules.cpp",
            "src/tessellator/tessellator.cpp",
            "src/tessellator/utils.cpp",
            "src/serializer/main.cpp",
        ],
        define_macros=[
            ("VERSION_INFO", version),
            ("DESCRIPTION", description),
        ],
        include_dirs=include_dirs,
        library_dirs=library_dirs,
        libraries=local_occ_libs,
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
        cxx_std=17,
    ),
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)
