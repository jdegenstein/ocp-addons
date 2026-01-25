.PHONY:  clean clean-windows wheel-linux wheel-macos wheel-windows

VERSION := $(shell python -c "import toml; print(toml.load('pyproject.toml')['project']['version'])")
PKG_NAME := $(shell python -c "import toml; print(toml.load('pyproject.toml')['project']['name'])")
MODULES := tessellator serializer

ifeq ($(OS),Windows_NT)
	ifdef GITHUB_ACTIONS
	    VSWHERE := C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe
	    VS_PATH := $(shell "$(VSWHERE)" -latest -property installationPath)
	else
	    VSWHERE := C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe
	    VS_PATH := $(shell "$(VSWHERE)" -latest -property installationPath)\..\BuildTools
	endif
	
	VCVARSALL := $(VS_PATH)\VC\Auxiliary\Build\vcvarsall.bat
endif


wheel-macos: clean
	MACOSX_DEPLOYMENT_TARGET=11.0 CXX=clang++ python -m build -n -w

	python -m wheel unpack dist/*.whl
	cd $(PKG_NAME)-$(VERSION) && python ../fix_libs.py $(PKG_NAME) $(MODULES) && cd ..
	python -m wheel pack $(PKG_NAME)-$(VERSION)

	mkdir -p wheelhouse
	mv $(PKG_NAME)-$(VERSION)*.whl wheelhouse

wheel-linux: clean
	pip install patchelf
	CXX=g++-9 python -m build -n -w

	python -m wheel unpack dist/*.whl
	cd $(PKG_NAME)-$(VERSION) && python ../fix_libs.py $(PKG_NAME) $(MODULES) && cd ..
	python -m wheel pack $(PKG_NAME)-$(VERSION)

	mkdir -p wheelhouse
	mv $(PKG_NAME)-$(VERSION)*.whl wheelhouse

wheel-windows: SHELL:=cmd.exe
wheel-windows: .SHELLFLAGS:=/C
wheel-windows: clean-windows
	echo "VCVARSALL: $(VCVARSALL)"
	call "$(VCVARSALL)" x64 -vcvars_ver=14.29 && ^\
	set CXX=cl.exe && ^\
	python -m build -n -w

	for %%i in (dist\*.whl) do python -m wheel unpack %%i
	cd $(PKG_NAME)-$(VERSION) && python ..\fix_libs.py $(PKG_NAME) $(MODULES) && cd ..
	python -m wheel pack $(PKG_NAME)-$(VERSION)
	
	mkdir wheelhouse
	copy $(PKG_NAME)-$(VERSION)*.whl wheelhouse

clean:
	rm -fr ocp_addons.egg-info build dist libs $(PKG_NAME)-$(VERSION) test

clean-wheelhouse:
	rm -fr wheelhouse

clean-windows:
	rmdir /S /Q ocp_addons.egg-info build dist libs $(PKG_NAME)-$(VERSION) test  2> NUL || exit 0

clean-wheelhouse-windows:
	rmdir /S /Q wheelhouse  2> NUL || exit 0
