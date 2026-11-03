CMAKE ?= cmake
CTEST ?= ctest
BUILD_DIR ?= build/make
CONFIG ?= Release
JOBS ?= 2
GRAPHICS ?= OFF
CMAKE_ARGS ?=

# Sources, dependencies, compiler flags, assets and test lists live in CMake.
.PHONY: all configure test graphics-test clean
all: configure
	$(CMAKE) --build "$(BUILD_DIR)" --config $(CONFIG) --parallel $(JOBS) --target backrooms_deploy
configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=$(CONFIG) -DBUILD_TESTING=ON -DBACKROOMS_GRAPHICS_TESTS=$(GRAPHICS) $(CMAKE_ARGS)
test: configure
	$(CMAKE) --build "$(BUILD_DIR)" --config $(CONFIG) --parallel $(JOBS) --target native_tests
	$(CTEST) --test-dir "$(BUILD_DIR)" -C $(CONFIG) -L "^native$$" --output-on-failure
graphics-test:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -DCMAKE_BUILD_TYPE=$(CONFIG) -DBUILD_TESTING=ON -DBACKROOMS_GRAPHICS_TESTS=ON $(CMAKE_ARGS)
	$(CMAKE) --build "$(BUILD_DIR)" --config $(CONFIG) --parallel $(JOBS) --target graphics_tests_build
	$(CTEST) --test-dir "$(BUILD_DIR)" -C $(CONFIG) -L "^graphics$$" --output-on-failure
clean:
	$(CMAKE) --build "$(BUILD_DIR)" --target clean
