from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
import os, sys
import os.path
import platform

__version__ = "0.1.0"
description = "Addon packages for OCP"

if platform.system() == "Linux":  # TODO: revisit for consistent GH actions behavior
    raw_machine = platform.machine()
    if raw_machine in ("AMD64", "x86_64"):
        archprefix = "x86_64"
    elif raw_machine in ("aarch64"):
        archprefix = raw_machine
    else:
        print(f"unknown machine type: {raw_machine}")
    os.environ["CXX"] = archprefix + "-conda-linux-gnu-g++"

ext_modules = [
    Pybind11Extension(
        "ocp_addons",
        [
            "src/modules.cpp",
            "src/tessellator/tessellator.cpp",
            "src/serializer/main.cpp",
        ],
        define_macros=[
            ("VERSION_INFO", __version__),
            ("DESCRIPTION", description),
        ],
        include_dirs=[
            os.path.join(sys.prefix, "include", "opencascade"),
            os.path.join(sys.prefix, "Library", "include", "opencascade"),
        ],
        library_dirs=[
            os.path.join(sys.prefix, "Library", "lib"),
        ],
        libraries=[
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
        ],
        cxx_std=17,
        extra_compile_args=["-g"],
    ),
]

setup(
    name="ocp_addons",
    version=__version__,
    author="J Degenstein, Matthias J, Bernhard Walter",
    author_email="",
    url="https://github.com/jdegenstein/ocp-addons.git",
    description=description,
    long_description="",
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.9",
)
