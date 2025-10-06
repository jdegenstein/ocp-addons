import os
import platform
from pathlib import Path

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

__version__ = "0.1.3"
description = "Addon packages for OCP"

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

here = Path(__file__).resolve().parent

occt_sdk = Path(os.environ.get("OCCT_SDK", here / "occt"))

extra_compile_args = []
extra_link_args = []

if platform.system() == "Linux":
    include_dirs = [str(occt_sdk / "include/opencascade")]
    library_dirs = [str(occt_sdk / "lib")]
    extra_compile_args.extend(["-O3"])

elif platform.system() == "Darwin":
    include_dirs = [str(occt_sdk / "include/opencascade")]
    library_dirs = [str(occt_sdk / "lib")]

    extra_compile_args.extend([
        "-O3",
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

# Do not print outside ensure "python setup.py --version" works correctly
if os.environ.get("DEBUG_SETUP_PY") is not None:
    print("setup.py: include_dirs", include_dirs)
    print("setup.py: library_dirs", library_dirs)
    print("setup.py: extra_compile_args", extra_compile_args)
    print("setup.py: extra_link_args", extra_link_args)

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
        include_dirs=include_dirs,
        library_dirs=library_dirs,
        libraries=occ_libs,
        extra_compile_args=extra_compile_args,
        extra_link_args=extra_link_args,
        cxx_std=17,
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
    python_requires=">=3.11",
)
