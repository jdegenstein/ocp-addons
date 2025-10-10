import os
import platform
import site
import shutil
import subprocess
from pathlib import Path

import toml

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

version = toml.load("pyproject.toml")["project"]["version"]
description = toml.load("pyproject.toml")["project"]["description"]


def tool_path(tool_name):
    """
    Returns the full path to a specified Visual Studio tool executable.

    This function runs the Visual Studio Installer's `vswhere.exe` to locate the installation path
    of Visual Studio Build Tools. It then constructs the path to the requested tool executable
    (e.g., 'cl.exe', 'link.exe') within the MSVC toolchain directory.

    Args:
        tool_name (str): The name of the tool executable to locate (e.g., 'cl.exe').

    Returns:
        pathlib.Path or None: The full path to the tool executable if found, otherwise None.

    Raises:
        subprocess.CalledProcessError: If the `vswhere.exe` command fails.
    """
    result = subprocess.run(
        ["C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe"],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        text=True,
        check=True,
    )
    lines = result.stdout.splitlines()
    for line in lines:
        if "installationPath" in line:
            prefix = Path(line.split(": ")[1].strip()).parent
            return (
                prefix
                / "BuildTools"
                / "VC"
                / "Tools"
                / "MSVC"
                / "14.29.30133"
                / "bin"
                / "Hostx64"
                / "x64"
                / tool_name
            )
    return None


def generate_def_file(dll_path, def_path):
    """
    Extracts exported symbols from a DLL file and writes them into a module-definition (DEF) file.

    Args:
        dll_path (Path or str): Path to the DLL file whose exported symbols are to be extracted.
        def_path (Path or str): Path to the output DEF file to be generated.

    Description:
        This function uses 'dumpbin.exe' to list the exported symbols from the specified DLL file.
        It parses the output to extract symbol names and writes them into a DEF file in the format
        required for linking, including the 'LIBRARY' and 'EXPORTS' sections.

    Raises:
        subprocess.CalledProcessError: If the 'dumpbin.exe' command fails.
        OSError: If writing to the DEF file fails.
    """
    result = subprocess.run(
        [f"{tool_path('dumpbin.exe')}", "/EXPORTS", dll_path],
        stdout=subprocess.PIPE,
        encoding="utf-8",
        text=True,
        check=True,
    )
    #
    lines = result.stdout.splitlines()
    exports = []
    parsing = False
    for line in lines:
        if "ordinal" in line.lower() and "name" in line.lower():
            parsing = True
            continue
        if parsing:
            # End at a blank line or summary
            if line.lstrip().startswith("Summary"):
                break
            if not line.strip():
                continue
            # Extract the name, usually the last column
            parts = line.strip().split()
            if len(parts) >= 4:
                exports.append(parts[3])
            elif len(parts) == 3:
                exports.append(parts[2])
    #
    with open(def_path, "w", encoding="utf-8") as f:
        f.write(f"LIBRARY {dll_path.name}\nEXPORTS\n")
        for symbol in exports:
            f.write(f"    {symbol}\n")


def build_import_lib(def_path, lib_path, machine="x64"):
    """
    Runs Microsoft's lib.exe to generate an import library (.lib) from a module definition (.def) file.

    Args:
        def_path (str): Path to the .def file containing exported symbols.
        lib_path (str): Path where the output .lib file will be created.
        machine (str, optional): Target architecture for the library (e.g., 'x64', 'x86'). Defaults to 'x64'.

    Returns:
        bool: True if the import library was successfully created, False otherwise.

    Raises:
        subprocess.CalledProcessError: If lib.exe fails to execute or returns a non-zero exit code.
    """
    result = subprocess.run(
        [
            f"{tool_path('lib.exe')}",
            f"/def:{def_path}",
            f"/out:{lib_path}",
            f"/machine:{machine}",
        ],
        check=True,
        encoding="utf-8",
    )
    return result.returncode == 0


# The libs needed for ocp-addons
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

site_pkgs = Path([p for p in site.getsitepackages() if "site-packages" in p][0])
print("Site-packages:", str(site_pkgs))

if platform.system() == "Windows":
    occt_sdk = [
        Path(os.environ["CONDA_PREFIX"]) / "Library" / "include" / "opencascade",
        Path(os.environ["CONDA_PREFIX"]) / "Library" / "lib",
    ]
    print("OCCT includes:", str(occt_sdk[0]))
    print("OCCT dlls:", str(occt_sdk[1]))

else:
    occt_sdk = Path(os.environ["CONDA_PREFIX"]) / "include" / "opencascade"
    print("OCCT includes:", str(occt_sdk))

extra_compile_args = []
extra_link_args = []

local_libs = Path("libs")

if platform.system() == "Linux":

    def get_libs(lib_path):
        print(f"Copying libs in {lib_path}")

        libs_exists = Path.exists(local_libs)

        Path.mkdir(local_libs, exist_ok=True)

        libs = []
        for lib in occ_libs:
            target = f"lib{lib}.so"
            so_lib = next(Path.glob(lib_path, f"lib{lib}*.so.*"))
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

    extra_compile_args.extend(
        ["-O3", "-Wno-deprecated-declarations", "-D_GLIBCXX_USE_CXX11_ABI=0"]
    )

elif platform.system() == "Darwin":

    def get_libs(lib_path):
        print(f"Finding libs in {lib_path}")
        libs = []
        for lib in occ_libs:
            dylib = next(Path.glob(lib_path, f"lib{lib}*.dylib"))
            lib_name = dylib.name[3:].replace(".dylib", "")
            libs.append(lib_name)
        return libs

    ocp_path = site_pkgs / "OCP" / ".dylibs"
    print("ocp_path", str(ocp_path))

    include_dirs = [str(occt_sdk)]
    library_dirs = [str(ocp_path)]

    extra_compile_args.extend(
        [
            "-O3",
            "-Wno-deprecated-declarations",
            "-mmacosx-version-min=11.1",
        ]
    )
    extra_link_args.extend(
        [
            "-Wl,-headerpad_max_install_names",
            "-mmacosx-version-min=11.1",
        ],
    )

elif platform.system() == "Windows":

    def get_libs(lib_path):
        libs_exists = Path.exists(local_libs)

        Path.mkdir(local_libs, exist_ok=True)
        libs = []
        for lib in occ_libs:
            # target = f"lib{lib}.so"
            dll = next(Path.glob(lib_path, f"{lib}-*.dll"))
            def_file = local_libs / dll.name.replace("dll", "def")
            lib_file = local_libs / dll.name.replace("dll", "lib")
            if not libs_exists:
                print(f"  - Generating {lib_file} from {dll}")
                generate_def_file(dll, def_file)
                build_import_lib(def_file, lib_file, machine="x64")
            libs.append(dll.name.replace(".dll", ""))
        return libs

    ocp_path = site_pkgs / "cadquery_ocp.libs"
    print("ocp_path", str(ocp_path))

    include_dirs = [str(occt_sdk[0])]
    library_dirs = [str(local_libs)]

else:
    raise RuntimeError(f"Platform {platform.system()} is not supported")

print("include_dirs", include_dirs)
print("library_dirs", library_dirs)

local_occ_libs = get_libs(ocp_path)
print("local_occ_libs", local_occ_libs)

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
