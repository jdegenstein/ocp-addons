.PHONY:  clean clean-windows wheel-linux wheel-macos wheel-windows

wheel-macos: clean
	CXX=clang++ python -m build -n -w
	python -m wheel unpack dist/*.whl
	python fix_libs.py
	otool -L ocp_addons-$(VERSION)/ocp_addon*.so
	python -m wheel pack ocp_addons-$(VERSION)
	mkdir -p wheelhouse
	mv ocp_addons-$(VERSION)*.whl wheelhouse

wheel-linux: clean
	pip install patchelf
	CXX=g++-9 python -m build -n -w
	python -m wheel unpack dist/*.whl
	python fix_libs.py
	python -m wheel pack ocp_addons-$(VERSION)
	mkdir -p wheelhouse
	mv ocp_addons-$(VERSION)*.whl wheelhouse

wheel-windows: SHELL:=cmd.exe
wheel-windows: .SHELLFLAGS:=/C
wheel-windows: clean-windows
	@for /f "usebackq delims=" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -property installationPath`) do call "%%i\..\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && ^\
	set CXX=cl.exe && ^\
	python -m build -n -w
	mkdir wheelhouse
	copy dist\ocp_addons-$(VERSION)*.whl wheelhouse

clean:
	rm -fr ocp_addons.egg-info build dist wheelhouse libs ocp_addons-$(VERSION)

clean-windows:
	rmdir /S /Q ocp_addons.egg-info build dist wheelhouse libs ocp_addons-$(VERSION)  2> NUL || exit 0
