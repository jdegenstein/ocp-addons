from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
import os, sys
import os.path
import platform

__version__ = "0.1.0"
description="Addon packages for OCP"

if platform.system() == "Linux":
    os.environ["CXX"] = "x86_64-conda-linux-gnu-g++"

ext_modules = [
    Pybind11Extension(
        "ocp_addons",
        ["src/tessellator/tessellate.cpp"],
        define_macros=[
            ("VERSION_INFO", __version__),
            ("DESCRIPTION", description),
        ],
        include_dirs = [os.path.join(sys.prefix, "include/opencascade")],
        libraries = ["TKG3d", "TKTopAlgo", "TKMesh"],
        cxx_std = 17
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
