.PHONY:  clean clean-windows wheel-linux wheel-macos wheel-windows

VERSION := $(shell python -c "import toml; print(toml.load('pyproject.toml')['project']['version'])")
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
	CXX=clang++ python -m build -n -w

	python -m wheel unpack dist/*.whl
	cd ocp_addons-$(VERSION) && python ../fix_libs.py $(MODULES) && cd ..
	python -m wheel pack ocp_addons-$(VERSION)
	
	mkdir -p wheelhouse
	mv ocp_addons-$(VERSION)*.whl wheelhouse

wheel-linux: clean
	pip install patchelf
	CXX=g++-9 python -m build -n -w

	python -m wheel unpack dist/*.whl
	cd ocp_addons-$(VERSION) && python ../fix_libs.py $(MODULES) && cd ..
	python -m wheel pack ocp_addons-$(VERSION)
	
	mkdir -p wheelhouse
	mv ocp_addons-$(VERSION)*.whl wheelhouse

wheel-windows: SHELL:=cmd.exe
wheel-windows: .SHELLFLAGS:=/C
wheel-windows: clean-windows
	echo "VCVARSALL: $(VCVARSALL)"
	call "$(VCVARSALL)" x64 -vcvars_ver=14.29 && ^\
	set CXX=cl.exe && ^\
	python -m build -n -w

	for %%i in (dist\*.whl) do python -m wheel unpack %%i
	cd ocp_addons-$(VERSION) && python ..\fix_libs.py $(MODULES) && cd ..
	python -m wheel pack ocp_addons-$(VERSION)
	
	mkdir wheelhouse
	copy ocp_addons-$(VERSION)*.whl wheelhouse

clean:
	rm -fr ocp_addons.egg-info build dist wheelhouse libs ocp_addons-$(VERSION) test

clean-windows:
	rmdir /S /Q ocp_addons.egg-info build dist wheelhouse libs ocp_addons-$(VERSION) test  2> NUL || exit 0
