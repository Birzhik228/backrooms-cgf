CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CPPFLAGS += -Iinclude -I.
SOURCES = backrooms.cpp world.cpp common/threat.cpp common/threat_model.cpp common/audio.cpp common/music.cpp common/Shader.cpp common/ui.cpp common/textures.cpp common/glad.c
ifeq ($(OS),Windows_NT)
TARGET = Backrooms.exe
LDLIBS = -lglfw3 -lopengl32 -lgdi32 -lwinmm
AUDIO_LIBS = -lwinmm
else
TARGET = Backrooms
LDLIBS = -lglfw -lGL -ldl -lpthread
endif
all: $(TARGET)
$(TARGET): $(SOURCES) world.h common/level_menu.h common/atmosphere.h common/parking_geometry.h common/darkness.h common/fun_geometry.h common/partygoer.h common/partygoer_model.h common/math.h common/render_limits.h common/Shader.h common/ui.h common/graphics.h common/controls_guide.h common/textures.h common/audio.h common/music.h common/gameplay.h common/movement.h common/player_state.h common/settings.h common/threat.h common/threat_model.h common/encounter.h common/jumpscare.h common/mouse.h
$(TARGET): common/ending.h common/ending_city.h common/ending_sun.h common/ending_renderer.h common/ending_ui.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(SOURCES) -o $@ $(LDLIBS)
test: world.cpp tests/world_tests.cpp tests/streaming_tests.cpp tests/movement_tests.cpp tests/threat_tests.cpp tests/threat_model_tests.cpp tests/encounter_tests.cpp tests/audio_tests.cpp common/audio.cpp common/audio.h common/gameplay.h common/movement.h common/player_state.h common/settings.h common/threat.cpp common/threat.h common/threat_model.cpp common/threat_model.h common/encounter.h common/jumpscare.h common/mouse.h world.h
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) tests/ending_tests.cpp -o ending_tests
	./ending_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) common/audio.cpp tests/ending_audio_tests.cpp -o ending_audio_tests $(AUDIO_LIBS)
	./ending_audio_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/atmosphere_tests.cpp -o atmosphere_tests
	./atmosphere_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/world_tests.cpp -o world_tests
	./world_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/parking_tests.cpp -o parking_tests
	./parking_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/fun_tests.cpp -o fun_tests
	./fun_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/partygoer_tests.cpp -o partygoer_tests
	./partygoer_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp common/threat.cpp tests/progression_tests.cpp -o progression_tests
	./progression_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp tests/streaming_tests.cpp -o streaming_tests
	./streaming_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) tests/movement_tests.cpp -o movement_tests
	./movement_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) world.cpp common/threat.cpp tests/threat_tests.cpp -o threat_tests
	./threat_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) common/threat_model.cpp tests/threat_model_tests.cpp -o threat_model_tests
	./threat_model_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) tests/encounter_tests.cpp -o encounter_tests
	./encounter_tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) common/audio.cpp tests/audio_tests.cpp -o audio_tests $(AUDIO_LIBS)
	./audio_tests
ifeq ($(OS),Windows_NT)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) common/music.cpp tests/music_tests.cpp -o music_tests $(AUDIO_LIBS)
	./music_tests
endif
.PHONY: all test
