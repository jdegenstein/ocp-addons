.PHONY: wheel clean

PYTHONHOME    := $(shell python -c "import sys; print(sys.base_exec_prefix)")
VENVROOT      := $(shell python -c "import sys; print(sys.prefix)")
PYTHONPATH    := $(shell python -c "import site; print(site.getsitepackages()[0])")
PYTHONINCLUDE := $(shell python -c "import sysconfig; print(sysconfig.get_path('include'))")
PYTHONVERSION := $(shell python -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')")
VERSION       := $(shell python -c "import toml; print(toml.load('pyproject.toml')['project']['version'])")

CXX      := clang++

TARGET   := tessellator_test
SRC      := main.cpp src/tessellator/tessellator.cpp src/tessellator/utils.cpp

# Release
# CXXFLAGS := -Wno-deprecated-declarations -std=c++17 -g -O3
# Debug
CXXFLAGS := -Wno-deprecated-declarations -std=c++17 -g -O0 -DDEBUG
CXXFLAGS += -I./occt/include/opencascade
CXXFLAGS += -I./src/tessellator
CXXFLAGS += -I$(PYTHONPATH)/pybind11/include
CXXFLAGS += -I$(PYTHONINCLUDE)

LDFLAGS  := -L./occt/lib
LDFLAGS  += -L$(PYTHONHOME)/lib

LIBS     := -lTKG3d -lTKTopAlgo -lTKMesh -lTKBRep -lTKGeomAlgo -lTKGeomBase -lTKG2d -lTKMath -lTKShHealing -lTKernel
LIBS     += -lpthread -lpython$(PYTHONVERSION)

compile: $(TARGET)

$(TARGET): $(SRC)
	@echo "PYTHONHOME $(PYTHONHOME)"
	@echo "PYTHONPATH $(PYTHONPATH)"
	@echo "PYTHONINCLUDE $(PYTHONINCLUDE)"
	@echo "PYTHONVERSION $(PYTHONVERSION)"
	@echo ""
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

run: compile
	@PYTHONHOME=$(PYTHONHOME):$(VENVROOT) DYLD_LIBRARY_PATH=./occt/lib:/opt/homebrew/lib/ ./$(TARGET)

wheel-macos: clean
	CXX=clang++ python -m build -n -w
	python -m wheel unpack dist/*.whl
	python fix_libs.py
	otool -L ocp_addons-$(VERSION)/ocp_addon*.so
	python -m wheel pack ocp_addons-$(VERSION)
	mkdir -p wheelhouse
	mv ocp_addons-$(VERSION)*.whl wheelhouse

clean:
	rm -fr $(TARGET) $(TARGET).dSYM ocp_addons.egg-info build dist wheelhouse ocp_addons-$(VERSION)
