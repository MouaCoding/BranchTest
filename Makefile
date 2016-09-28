AR=ar
AS=as
CC=gcc
CPP=cpp
CXX=g++
LD=ld
OBJCOPY = objcopy
OBJDUMP = objdump
STRIP = strip

INC_DIR = ./include
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

DEBUG_MODE=TRUE

PKGS = gtk+-2.0 sndfile fluidsynth

ifdef DEBUG_MODE
DEFINES  += -DDEBUG
CFLAGS   += -g -ggdb -D_GLIBCXX_DEBUG
#LDFLAGS  += -Wl,-no_pie
else 
CFLAGS   += -O3
endif

INCLUDE  += -I $(INC_DIR)
CFLAGS   +=  -Wall `pkg-config --cflags $(PKGS)`
LDFLAGS  +=`pkg-config --libs $(PKGS)` -lpng -lportaudio 
#LDFLAGS += -lgdk_imlib
CPPFLAGS += -std=c++11 
GAME_NAME = thegame

GAME_OBJS = $(OBJ_DIR)/main.o                   \
    $(OBJ_DIR)/AIPlayer.o                       \
    $(OBJ_DIR)/AssetDecoratedMap.o              \
    $(OBJ_DIR)/AssetRenderer.o                  \
    $(OBJ_DIR)/BasicCapabilities.o              \
    $(OBJ_DIR)/Bevel.o                          \
    $(OBJ_DIR)/BuildCapabilities.o              \
    $(OBJ_DIR)/BuildingUpgradeCapabilities.o    \
    $(OBJ_DIR)/ButtonRenderer.o                 \
    $(OBJ_DIR)/CursorSet.o                      \
    $(OBJ_DIR)/Debug.o                          \
    $(OBJ_DIR)/EditRenderer.o                   \
    $(OBJ_DIR)/FileDataSource.o                 \
    $(OBJ_DIR)/FogRenderer.o                    \
    $(OBJ_DIR)/FontTileset.o                    \
    $(OBJ_DIR)/GameModel.o                      \
    $(OBJ_DIR)/GraphicLoader.o                  \
    $(OBJ_DIR)/GraphicMulticolorTileset.o       \
    $(OBJ_DIR)/GraphicRecolorMap.o              \
    $(OBJ_DIR)/GraphicTileset.o                 \
    $(OBJ_DIR)/LineDataSource.o                 \
    $(OBJ_DIR)/ListViewRenderer.o               \
    $(OBJ_DIR)/MapRenderer.o                    \
    $(OBJ_DIR)/MiniMapRenderer.o                \
    $(OBJ_DIR)/Path.o                           \
    $(OBJ_DIR)/PeriodicTimeout.o                \
    $(OBJ_DIR)/PixelType.o                      \
    $(OBJ_DIR)/PlayerAsset.o                    \
    $(OBJ_DIR)/Position.o                       \
    $(OBJ_DIR)/ResourceRenderer.o               \
    $(OBJ_DIR)/RouterMap.o                      \
    $(OBJ_DIR)/SoundClip.o                      \
    $(OBJ_DIR)/SoundEventRenderer.o             \
    $(OBJ_DIR)/SoundLibraryMixer.o              \
    $(OBJ_DIR)/TerrainMap.o                     \
    $(OBJ_DIR)/TextFormatter.o                  \
    $(OBJ_DIR)/Tokenizer.o                      \
    $(OBJ_DIR)/TrainCapabilities.o              \
    $(OBJ_DIR)/UnitActionRenderer.o             \
    $(OBJ_DIR)/UnitDescriptionRenderer.o        \
    $(OBJ_DIR)/UnitUpgradeCapabilities.o        \
    $(OBJ_DIR)/ViewportRenderer.o               \
    $(OBJ_DIR)/VisibilityMap.o

all: directories $(BIN_DIR)/$(GAME_NAME)

$(BIN_DIR)/$(GAME_NAME): $(GAME_OBJS)
	$(CXX) $(GAME_OBJS) -o $(BIN_DIR)/$(GAME_NAME) $(CFLAGS) $(CPPFLAGS) $(DEFINES) $(LDFLAGS) 
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@
	
.PHONY: directories
directories:
	mkdir -p $(OBJ_DIR)
	
clean::
	-rm $(GAME_OBJS) $(INC_DIR)/*.*~ $(SRC_DIR)/*.*~
	
.PHONY: clean