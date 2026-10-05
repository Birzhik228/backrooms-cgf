CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CPPFLAGS += -Iinclude -I.
SOURCES = backrooms.cpp world.cpp common/audio.cpp common/music.cpp common/Shader.cpp common/ui.cpp common/textures.cpp common/glad.c
ifeq ($(OS),Windows_NT)
TARGET = Backrooms.exe
LDLIBS = -lglfw3 -lopengl32 -lgdi32 -lwinmm
AUDIO_LIBS = -lwinmm
else
TARGET = Backrooms
LDLIBS = -lglfw -lGL -ldl -lpthread
endif
all: $(TARGET)
$(TARGET): $(SOURCES) world.h common/math.h common/Shader.h common/ui.h common/textures.h common/audio.h common/music.h common/gameplay.h common/movement.h common/player_state.h common/settings.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SOURCES) -o $@ $(LDLIBS)
test: world.cpp tests/world_tests.cpp tests/streaming_tests.cpp tests/movement_tests.cpp tests/audio_tests.cpp common/audio.cpp common/audio.h common/gameplay.h common/movement.h common/player_state.h common/settings.h world.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/world_tests.cpp -o world_tests
	./world_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/streaming_tests.cpp -o streaming_tests
	./streaming_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) tests/movement_tests.cpp -o movement_tests
	./movement_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) common/audio.cpp tests/audio_tests.cpp -o audio_tests $(AUDIO_LIBS)
	./audio_tests
ifeq ($(OS),Windows_NT)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) common/music.cpp tests/music_tests.cpp -o music_tests $(AUDIO_LIBS)
	./music_tests
endif
.PHONY: all test
