# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
import os
import sys
import json
import subprocess
import platform
import wheel.bdist_wheel
import shutil
import glob


def repair_wheel_linux(lib_path, wheel_filename, out_path, machine):

    platform = f"manylinux_2_35_{machine}"

    args = [
        "env",
        f"LD_LIBRARY_PATH={lib_path}",
        sys.executable,
        "-m",
        "auditwheel",
        "--verbose",
        "repair",
        f"--plat={platform}",
        f"--wheel-dir={out_path}",
        wheel_filename,
    ]
    subprocess.check_call(args)


def repair_wheel_macos(lib_path, wheel_filename, out_path, machine):
    args = [
        "env",
        f"DYLD_LIBRARY_PATH={lib_path}",
        sys.executable,
        "-m",
        "delocate.cmd.delocate_listdeps",
        wheel_filename,
    ]
    subprocess.check_call(args)

    # Overwrites the wheel in-place by default
    args = [
        "env",
        f"DYLD_LIBRARY_PATH={lib_path}",
        sys.executable,
        "-m",
        "delocate.cmd.delocate_wheel",
        f"--wheel-dir={out_path}",
        wheel_filename,
    ]
    subprocess.check_call(args)


def repair_wheel_windows(lib_path, wheel_filename, out_path, machine):
    args = [sys.executable, "-m", "delvewheel", "show", wheel_filename]
    subprocess.check_call(args)
    args = [
        sys.executable,
        "-m",
        "delvewheel",
        "repair",
        f"--wheel-dir={out_path}",
        wheel_filename,
    ]
    subprocess.check_call(args)


def consistent_machine():
    raw_machine = platform.machine()

    if raw_machine in ("AMD64", "x86_64"):
        return "x86_64"
    elif raw_machine in ("aarch64", "arm64"):
        return raw_machine
    else:
        print(f"unknown machine type {raw_machine}")


class bdist_wheel_repaired(wheel.bdist_wheel.bdist_wheel):
    def run(self):
        super().run()
        system = platform.system()
        machine = consistent_machine()
        os.environ["CXX"] = f"{machine}-conda-linux-gnu-g++"
        ##
        # conda = "conda.bat" if platform.system() == "Windows" else "conda"#UNCOMMENT
        conda = "micromamba"  # DELETEME
        args = [conda, "info", "--json"]
        info = json.loads(subprocess.check_output(args))
        # conda_prefix = info["active_prefix"] or info["conda_prefix"] #UNCOMMENT
        conda_prefix = info["env location"]  # DELETEME
        lib_path = os.path.join(conda_prefix, "lib")
        dist_files = self.distribution.dist_files
        [(_, _, bad_wheel_filename)] = dist_files
        out_path = os.path.join(self.dist_dir, "repaired")
        ##
        if system == "Linux":
            repair_wheel_linux(lib_path, bad_wheel_filename, out_path, machine)
        elif system == "Darwin":
            repair_wheel_macos(lib_path, bad_wheel_filename, out_path, machine)
        elif system == "Windows":
            repair_wheel_windows(lib_path, bad_wheel_filename, out_path, machine)
        else:
            print(f"unsupported system type: {system}")

        # Exactly one whl is expected in the dist dir, so delete the
        # bad wheel and move the repaired wheel in.
        [repaired_whl_filename] = glob.glob(os.path.join(out_path, "*.whl"))
        os.unlink(bad_wheel_filename)
        new_whl = os.path.join(self.dist_dir, os.path.basename(repaired_whl_filename))
        shutil.move(repaired_whl_filename, new_whl)
        os.rmdir(out_path)
        dist_files[0] = dist_files[0][:-1] + (new_whl,)


# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

__version__ = "0.1.0"
description = "Addon packages for OCP"

ext_modules = [
    Pybind11Extension(
        "ocp_addons",
        ["src/serializer/serialize.cpp", "src/tessellator/tessellate.cpp"],
        define_macros=[
            ("VERSION_INFO", __version__),
            ("DESCRIPTION", description),
        ],
        include_dirs=[os.path.join(sys.prefix, "include/opencascade")],
        libraries=["TKBRep", "TKG3d", "TKTopAlgo", "TKMesh"],
        extra_compile_args=["-O3"],
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
    cmdclass={"bdist_wheel": bdist_wheel_repaired, "build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.9",
)
