/*
    Copyright (c) 2015, Christopher Nitta
    All rights reserved.

    All source material (source code, images, sounds, etc.) have been provided to
    University of California, Davis students of course ECS 160 for educational
    purposes. It may not be distributed beyond those enrolled in the course without
    prior permission from the copyright holder.

    All sound files, sound fonts, midi files, and images that have been included 
    that were extracted from original Warcraft II by Blizzard Entertainment 
    were found freely available via internet sources and have been labeld as 
    abandonware. They have been included in this distribution for educational 
    purposes only and this copyright notice does not attempt to claim any 
    ownership of this material.
*/
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <sys/time.h>
#include <list>
#include <sstream>
#include <dirent.h>
#include "AssetDecoratedMap.h"
#include "AssetRenderer.h"
#include "Bevel.h"
#include "CursorSet.h"
#include "Debug.h"
#include "GraphicTileset.h"
#include "GraphicMulticolorTileset.h"
#include "FontTileset.h"
#include "SoundLibraryMixer.h"
#include "RandomNumberGenerator.h"
#include "FileDataSource.h"
#include "MapRenderer.h"
#include "PlayerAsset.h"
#include "RouterMap.h"
#include "FogRenderer.h"
#include "Path.h"
#include "PeriodicTimeout.h"
#include "PixelType.h"
#include "ViewportRenderer.h"
#include "MiniMapRenderer.h"
#include "ResourceRenderer.h"
#include "UnitDescriptionRenderer.h"
#include "UnitActionRenderer.h"
#include "GameModel.h"
#include "PlayerCommand.h"
#include "AIPlayer.h"
#include "SoundEventRenderer.h"
#include "ButtonRenderer.h"
#include "ListViewRenderer.h"
#include "EditRenderer.h"

#define INITIAL_MAP_WIDTH       800
#define INITIAL_MAP_HEIGHT      600
#define TIMEOUT_INTERVAL        50
#define RANDOM_NUMBER_MAX       1000000
#define MINI_MAP_MIN_WIDTH      128
#define MINI_MAP_MIN_HEIGHT     128
#define PAN_SPEED_MAX           0x100
#define PAN_SPEED_SHIFT         1

#ifndef MIN
    #define MIN(a, b)               ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
    #define MAX(a, b)               ((a) > (b) ? (a) : (b))
#endif

typedef struct{
    int DXIndex;
    int DYIndex;
} SMapLocation, *SMapLocationRef;

template <typename T>
inline bool WeakPtrEquals(const std::weak_ptr<T>& t, const std::weak_ptr<T>& u){
    return !t.expired() && t.lock() == u.lock();
}

template <typename T>
inline bool WeakPtrCompare(const std::weak_ptr<T>& t, const std::weak_ptr<T>& u){
    return !t.expired() && t.lock() <= u.lock();
}

typedef void (*TButtonCallbackFunction)(void *calldata);
typedef bool (*TEditTextValidationCallbackFunction)(const std::string &text);

class CApplicationData{
    
    protected:
        typedef enum{
            gmMainMenu,
            gmOptionsMenu,
            gmSoundOptions,
            gmNetworkOptions,
            gmMultiPlayerOptionsMenu,
            gmMapSelect,
            gmHostSelect,
            gmPlayerAISelect,
            gmBattle,
            gmGameOver
        } EGameMode, *EGameModeRef;
        
        typedef enum{
            ctPointer = 0,
            ctInspect,
            ctArrowN,
            ctArrowE,
            ctArrowS,
            ctArrowW,
            ctTargetOff,
            ctTargetOn,
            ctMax
        } ECursorType, *ECursorTypeRef;
        
        typedef enum{
            uictNone = 0,
            uictViewport,
            uictViewportBevelN,
            uictViewportBevelE,
            uictViewportBevelS,
            uictViewportBevelW,
            uictMiniMap,
            uictUserDescription,
            uictUserAction,
            uictMenuButton
        } EUIComponentType, *EUIComponentTypeRef;
        
        typedef enum{
            gstSinglePlayer,
            gstMultiPlayerHost,
            gstMultiPlayerClient
        } EGameSessionType, *EGameSessionTypeRef;
        
        typedef enum{
            ptNone = 0,
            ptHuman,
            ptAIEasy,
            ptAIMedium,
            ptAIHard
        } EPlayerType, *EPlayerTypeRef;
        
        bool DDeleted;
        EGameSessionType DGameSessionType;
        float DSoundVolume;
        float DMusicVolume;
        std::string DUsername;
        std::string DRemoteHostname;
        int DMultiplayerPort;
        std::shared_ptr< CPeriodicTimeout > DPeriodicTimeout;
        GtkWidget *DMainWindow;
        GtkWidget *DDrawingArea;
        GdkCursor *DBlankCursor;
        GdkGC     *DDrawingContext;
        GdkPixmap *DDoubleBufferPixmap;
        GdkPixmap *DWorkingBufferPixmap;
        GdkPixbuf *DWorkingPixbuf;
        GdkPixmap *DMiniMapPixmap;
        GdkPixmap *DViewportPixmap;
        GdkPixmap *DViewportTypePixmap;
        GdkPixmap *DUnitDescriptionPixmap;
        GdkPixmap *DUnitActionPixmap;
        GdkPixmap *DResourcePixmap;
        GdkPixmap *DMapSelectListViewPixmap;
        GdkColor DMiniMapViewportColor;
        int DBorderWidth;
        int DPanningSpeed;
        int DViewportXOffset, DViewportYOffset;
        int DMiniMapXOffset, DMiniMapYOffset;
        int DUnitDescriptionXOffset, DUnitDescriptionYOffset;
        int DUnitActionXOffset, DUnitActionYOffset;
        int DMenuButtonXOffset, DMenuButtonYOffset;
        int DMapSelectListViewXOffset, DMapSelectListViewYOffset;
        std::string DMainMenuTitle;
        std::vector< std::string > DMainMenuButtonsText;
        std::vector< TButtonCallbackFunction > DMainMenuButtonsFunctions;
        std::string DOptionsMenuTitle;
        std::vector< std::string > DOptionsMenuButtonsText;
        std::vector< TButtonCallbackFunction > DOptionsMenuButtonsFunctions;
        std::string DMultiPlayerOptionsMenuTitle;
        std::vector< std::string > DMultiPlayerOptionsMenuButtonsText;
        std::vector< TButtonCallbackFunction > DMultiPlayerOptionsMenuButtonsFunctions;
        std::string DCurrentPageTitle;
        std::vector< std::string > DCurrentPageButtonsText;
        std::vector< TButtonCallbackFunction > DCurrentPageButtonsFunctions;
        std::vector< SRectangle > DCurrentPageButtonLocations;
        std::vector< SRectangle > DColorButtonLocations;
        std::vector< SRectangle > DPlayerTypeButtonLocations;
        EPlayerColor DPlayerColorRequestingChange;
        EPlayerColor DPlayerColorChangeRequest;
        EPlayerColor DPlayerColorRequesTypeChange;
        bool DCurrentPageButtonHovered;
        int DSelectedMapOffset;
        int DSelectedMapIndex;
        std::shared_ptr< CAssetDecoratedMap > DSelectedMap;
        int DOptionsEditSelected;
        int DOptionsEditSelectedCharacter;
        std::vector< SRectangle > DOptionsEditLocations;
        std::vector< std::string > DOptionsEditTitles;
        std::vector< std::string > DOptionsEditText;
        std::vector< TEditTextValidationCallbackFunction > DOptionsEditValidationFunctions;
        
        std::shared_ptr< CMapRenderer > DMapRenderer;
        std::shared_ptr< CCursorSet > DCursorSet;
        int DCursorIndices[ctMax];
        ECursorType DCursorType;
        std::shared_ptr< CSoundLibraryMixer > DSoundLibraryMixer;
        std::shared_ptr< CSoundEventRenderer > DSoundEventRenderer;
        std::shared_ptr< CFontTileset > DFonts[CUnitDescriptionRenderer::fsMax];
        std::shared_ptr< CGraphicTileset > DSplashTileset;
        std::shared_ptr< CGraphicTileset > DMarkerTileset;
        std::shared_ptr< CGraphicTileset > DButtonColorTileset;
        std::shared_ptr< CGraphicTileset > DBackgroundTileset;
        std::shared_ptr< CGraphicTileset > DMiniBevelTileset;
        std::shared_ptr< CGraphicTileset > DInnerBevelTileset;
        std::shared_ptr< CGraphicTileset > DOuterBevelTileset;
        std::shared_ptr< CGraphicTileset > DListViewIconTileset;
        std::shared_ptr< CBevel > DMiniBevel;
        std::shared_ptr< CBevel > DInnerBevel;
        std::shared_ptr< CBevel > DOuterBevel;
        std::shared_ptr< CGraphicTileset > DTerrainTileset;
        std::shared_ptr< CGraphicTileset > DFogTileset;
        std::shared_ptr< CGraphicRecolorMap > DRecolorMap;
        std::shared_ptr< CGraphicMulticolorTileset > DIconTileset;
        std::shared_ptr< CGraphicTileset > DMiniIconTileset;
        std::vector< std::shared_ptr< CGraphicMulticolorTileset > > DAssetTilesets;
        std::vector< std::shared_ptr< CGraphicTileset > > DFireTilesets;
        std::shared_ptr< CGraphicTileset > DBuildingDeathTileset;
        std::shared_ptr< CGraphicTileset > DCorpseTileset;
        std::shared_ptr< CGraphicTileset > DArrowTileset;
        std::shared_ptr< CAssetRenderer > DAssetRenderer;
        std::shared_ptr< CFogRenderer > DFogRenderer;
        std::shared_ptr< CViewportRenderer > DViewportRenderer;
        std::shared_ptr< CMiniMapRenderer > DMiniMapRenderer;
        std::shared_ptr< CUnitDescriptionRenderer > DUnitDescriptionRenderer;
        std::shared_ptr< CUnitActionRenderer > DUnitActionRenderer;
        std::shared_ptr< CResourceRenderer > DResourceRenderer;
        std::shared_ptr< CButtonRenderer > DMenuButtonRenderer;
        std::shared_ptr< CButtonRenderer > DCurrentPageButtonRenderer;
        std::shared_ptr< CListViewRenderer > DMapSelectListViewRenderer;
        std::shared_ptr< CEditRenderer > DOptionsEditRenderer;
        
        // Model
        EPlayerColor DPlayerColor;
        std::shared_ptr< CGameModel > DGameModel;
        SPlayerCommandRequest DPlayerCommands[pcMax];
        std::shared_ptr< CAIPlayer > DAIPlayers[pcMax];
        EPlayerType DLoadingPlayerTypes[pcMax];
        EPlayerColor DLoadingPlayerColors[pcMax];
        std::string DPlayerNames[pcMax];
        
        EGameMode DGameMode;
        EGameMode DNextGameMode;
        
        std::unordered_map< guint, EAssetCapabilityType > DUnitHotKeyMap;
        std::unordered_map< guint, EAssetCapabilityType > DBuildHotKeyMap;
        std::unordered_map< guint, EAssetCapabilityType > DTrainHotKeyMap;
        std::list< std::weak_ptr< CPlayerAsset > > DSelectedPlayerAssets[pcMax];
        EAssetCapabilityType DCurrentAssetCapability[pcMax];
        std::vector< guint > DPressedKeys[pcMax];
        std::vector< guint > DReleasedKeys[pcMax];
        int DCurrentX[pcMax];
        int DCurrentY[pcMax];
        CPosition DMouseDown[pcMax];
        int DLeftClick[pcMax];
        int DRightClick[pcMax];
        bool DLeftDown[pcMax];
        bool DRightDown[pcMax];
        CButtonRenderer::EButtonState DMenuButtonState;
        
        
        static gboolean TimeoutCallback(gpointer data);
        static gboolean MainWindowDeleteEventCallback(GtkWidget *widget, GdkEvent *event, gpointer data);
        static void MainWindowDestroyCallback(GtkWidget *widget, gpointer data);
        static gboolean MainWindowKeyPressEventCallback(GtkWidget *widget, GdkEventKey *event, gpointer data);
        static gboolean MainWindowKeyReleaseEventCallback(GtkWidget *widget, GdkEventKey *event, gpointer data);
        static gboolean MainWindowConfigureEventCallback(GtkWidget *widget, GdkEvent *event, gpointer data);
        static gboolean DrawingAreaExposeCallback(GtkWidget *widget, GdkEventExpose *event, gpointer data);
        static gboolean DrawingAreaButtonPressEventCallback(GtkWidget *widget, GdkEventButton *event, gpointer data);
        static gboolean DrawingAreaButtonReleaseEventCallback(GtkWidget *widget, GdkEventButton *event, gpointer data);
        static gboolean DrawingAreaMotionNotifyEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer data);
        
        
        gboolean Timeout();
        gboolean MainWindowDeleteEvent(GtkWidget *widget, GdkEvent *event);
        void MainWindowDestroy(GtkWidget *widget);
        gboolean MainWindowKeyPressEvent(GtkWidget *widget, GdkEventKey *event);
        gboolean MainWindowKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event);
        gboolean MainWindowConfigureEvent(GtkWidget *widget, GdkEvent *event);
        gboolean DrawingAreaExpose(GtkWidget *widget, GdkEventExpose *event);
        gboolean DrawingAreaButtonPressEvent(GtkWidget *widget, GdkEventButton *event);
        gboolean DrawingAreaButtonReleaseEvent(GtkWidget *widget, GdkEventButton *event);
        gboolean DrawingAreaMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *event);
        
        EUIComponentType FindUIComponentType(const CPosition &pos);
        CPosition ScreenToViewport(const CPosition &pos);
        CPosition ScreenToMiniMap(const CPosition &pos);
        CPosition ScreenToDetailedMap(const CPosition &pos);
        CPosition ScreenToUnitDescription(const CPosition &pos);
        CPosition ScreenToUnitAction(const CPosition &pos);
        CPosition ViewportToDetailedMap(const CPosition &pos);
        CPosition MiniMapToDetailedMap(const CPosition &pos);
        
        // Input
        void ProcessInputButtonMenuMode();
        void ProcessInputMapSelectMode();
        void ProcessInputPlayerAISelectMode();
        void ProcessInputEditOptionsMode();
        void ProcessInputGameMode();

        // Calculate
        void CalculateButtonMenuMode();
        void CalculateMapSelectMode();
        void CalculatePlayerAISelectMode();
        void CalculateEditOptionsMode();
        void CalculateGameMode();
        
        // Output
        void RenderMenuTitle(const std::string &title, gint &titlebottomy, gint &pagewidth, gint &pageheight);
        void RenderButtonMenuMode();
        void RenderMapSelectMode();
        void RenderPlayerAISelectMode();
        void RenderEditOptionsMode();
        void RenderGameMode();
        
        void ChangeMode(EGameMode nextmode);
        void SwitchButtonMenuData();
       
        static void SinglePlayerGameButtonCallback(void *calldata);
        static void MultiPlayerGameButtonCallback(void *calldata);
        static void OptionsButtonCallback(void *calldata);
        static void ExitGameButtonCallback(void *calldata);
        static void MainMenuButtonCallback(void *calldata);
        static void SoundOptionsButtonCallback(void *calldata);
        static void SoundOptionsUpdateButtonCallback(void *calldata);
        static void NetworkOptionsButtonCallback(void *calldata);
        static void HostMultiPlayerButtonCallback(void *calldata);
        static void JoinMultiPlayerButtonCallback(void *calldata);
        static void SelectMapButtonCallback(void *calldata);
        static void PlayGameButtonCallback(void *calldata);
        static bool ValidSoundLevelCallback(const std::string &text);
        static bool ValidHostnameCallback(const std::string &text);
        static bool ValidPortNumberCallback(const std::string &text);
        
        void LoadGameMap(int index);
        void ResetPlayerColors();
        void ResetMap();
        void ResizeCanvases();
 
    public:
        CApplicationData();
        ~CApplicationData();
        
        int Init(int argc, char *argv[]);
        
};

CApplicationData::CApplicationData(){
    DGameMode = DNextGameMode = gmMainMenu;
    DPlayerColor = pcBlue;
    DMiniMapViewportColor = {0xFFFFFF, 0xFFFF, 0xFFFF, 0xFFFF};
    DDeleted = false;
    DMainWindow = nullptr;
    DDrawingArea = nullptr;
    DBlankCursor = nullptr;
    DDrawingContext = nullptr;
    DDoubleBufferPixmap = nullptr;
    DWorkingBufferPixmap = nullptr;
    DViewportPixmap = nullptr;
    DViewportTypePixmap = nullptr;
    DWorkingPixbuf = nullptr;
    DMiniMapPixmap = nullptr;
    DUnitDescriptionPixmap = nullptr;
    DUnitActionPixmap = nullptr;
    DResourcePixmap = nullptr;
    DMapSelectListViewPixmap = nullptr;
    DCursorType = ctPointer;
    
    DMapSelectListViewXOffset = 0;
    DMapSelectListViewYOffset = 0;
    DSelectedMapOffset = 0;
    DSelectedMapIndex = 0;
    DSoundVolume = 1.0;
    DMusicVolume = 0.5;
    DUsername = "user";
    DRemoteHostname = "localhost";
    DMultiplayerPort = 55107; // Ascii WC = 0x5743 or'd with 0x8000
    DBorderWidth = 32;
    DPanningSpeed = 0;
    for(int Index = 0; Index < pcMax; Index++){
       DCurrentX[Index] = 0;
       DCurrentY[Index] = 0;
       DMouseDown[Index] = CPosition(-1, -1);
       DLeftClick[Index] = 0;
       DRightClick[Index] = 0;
       DLeftDown[Index] = false;
       DRightDown[Index] = false;
       DPlayerCommands[Index].DAction = actNone;
       DLoadingPlayerColors[Index] = (EPlayerColor)Index;
    }
    
    DMenuButtonState = CButtonRenderer::bsNone;
    DUnitHotKeyMap[GDK_KEY_A] = actAttack;
    DUnitHotKeyMap[GDK_KEY_a] = actAttack;  
    DUnitHotKeyMap[GDK_KEY_B] = actBuildSimple;
    DUnitHotKeyMap[GDK_KEY_b] = actBuildSimple;
    DUnitHotKeyMap[GDK_KEY_G] = actConvey;
    DUnitHotKeyMap[GDK_KEY_g] = actConvey; 
    DUnitHotKeyMap[GDK_KEY_M] = actMove;
    DUnitHotKeyMap[GDK_KEY_m] = actMove;
    DUnitHotKeyMap[GDK_KEY_P] = actPatrol;
    DUnitHotKeyMap[GDK_KEY_p] = actPatrol;
    DUnitHotKeyMap[GDK_KEY_R] = actRepair;
    DUnitHotKeyMap[GDK_KEY_r] = actRepair;
    DUnitHotKeyMap[GDK_KEY_T] = actStandGround;
    DUnitHotKeyMap[GDK_KEY_t] = actStandGround; 
    
    DBuildHotKeyMap[GDK_KEY_B] = actBuildBarracks;
    DBuildHotKeyMap[GDK_KEY_b] = actBuildBarracks;
    DBuildHotKeyMap[GDK_KEY_F] = actBuildFarm;
    DBuildHotKeyMap[GDK_KEY_f] = actBuildFarm;
    DBuildHotKeyMap[GDK_KEY_H] = actBuildTownHall;
    DBuildHotKeyMap[GDK_KEY_h] = actBuildTownHall;
    DBuildHotKeyMap[GDK_KEY_L] = actBuildLumberMill;
    DBuildHotKeyMap[GDK_KEY_l] = actBuildLumberMill;
    DBuildHotKeyMap[GDK_KEY_S] = actBuildBlacksmith;
    DBuildHotKeyMap[GDK_KEY_s] = actBuildBlacksmith;
    DBuildHotKeyMap[GDK_KEY_T] = actBuildScoutTower;
    DBuildHotKeyMap[GDK_KEY_t] = actBuildScoutTower;
    
    
    DTrainHotKeyMap[GDK_KEY_A] = actBuildArcher;
    DTrainHotKeyMap[GDK_KEY_a] = actBuildArcher;
    DTrainHotKeyMap[GDK_KEY_F] = actBuildFootman;
    DTrainHotKeyMap[GDK_KEY_f] = actBuildFootman;
    DTrainHotKeyMap[GDK_KEY_P] = actBuildPeasant;
    DTrainHotKeyMap[GDK_KEY_p] = actBuildPeasant;
}

CApplicationData::~CApplicationData(){
    
}


gboolean CApplicationData::TimeoutCallback(gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    if(AppData->DDeleted){
        return FALSE;   
    }
    
    if(AppData->Timeout()){
        g_timeout_add(AppData->DPeriodicTimeout->MiliSecondsUntilDeadline(), TimeoutCallback, data);
    }
    return FALSE;
}


// This is a callback function to signify that the delete windows has been called.
// If you return FALSE in the "delete-event" signal handler, GTK will emit the 
// "destroy" signal. Returning TRUE means you don't want the window to be 
// destroyed. This is useful for popping up 'are you sure you want to quit?'
// type dialogs.
gboolean CApplicationData::MainWindowDeleteEventCallback(GtkWidget *widget, GdkEvent *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->MainWindowDeleteEvent(widget, event);
}

// The destroy window callback
void CApplicationData::MainWindowDestroyCallback(GtkWidget *widget, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    AppData->MainWindowDestroy(widget);
}


gboolean CApplicationData::MainWindowKeyPressEventCallback(GtkWidget *widget, GdkEventKey *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->MainWindowKeyPressEvent(widget, event);
}

gboolean CApplicationData::MainWindowKeyReleaseEventCallback(GtkWidget *widget, GdkEventKey *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->MainWindowKeyReleaseEvent(widget, event);
}

gboolean CApplicationData::MainWindowConfigureEventCallback(GtkWidget *widget, GdkEvent *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->MainWindowConfigureEvent(widget, event);
}

gboolean CApplicationData::DrawingAreaExposeCallback(GtkWidget *widget, GdkEventExpose *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->DrawingAreaExpose(widget, event);
}

gboolean CApplicationData::DrawingAreaButtonPressEventCallback(GtkWidget *widget, GdkEventButton *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->DrawingAreaButtonPressEvent(widget, event);
}

gboolean CApplicationData::DrawingAreaButtonReleaseEventCallback(GtkWidget *widget, GdkEventButton *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->DrawingAreaButtonReleaseEvent(widget, event);
}

gboolean CApplicationData::DrawingAreaMotionNotifyEventCallback(GtkWidget *widget, GdkEventMotion *event, gpointer data){
    CApplicationData *AppData = static_cast<CApplicationData *>(data);

    return AppData->DrawingAreaMotionNotifyEvent(widget, event);
}

gboolean CApplicationData::Timeout(){
    
    switch(DGameMode){
        case gmMainMenu:
        case gmOptionsMenu:                 
        case gmMultiPlayerOptionsMenu:      ProcessInputButtonMenuMode();
                                            CalculateButtonMenuMode();
                                            RenderButtonMenuMode();
                                            break;
        case gmSoundOptions:
        case gmNetworkOptions:              ProcessInputEditOptionsMode();
                                            CalculateEditOptionsMode();
                                            RenderEditOptionsMode();
                                            break;
        case gmMapSelect:                   ProcessInputMapSelectMode();
                                            CalculateMapSelectMode();
                                            RenderMapSelectMode();
                                            break;
        case gmPlayerAISelect:              ProcessInputPlayerAISelectMode();
                                            CalculatePlayerAISelectMode();
                                            RenderPlayerAISelectMode();
                                            break;
        case gmBattle:
        default:                            DGameModel->ClearGameEvents();
                                            ProcessInputGameMode();
                                            CalculateGameMode();
                                            RenderGameMode();
                                            break;
    }
    
    for(int Index = 0; Index < pcMax; Index++){
        if(!DLeftDown[Index] || (2 != DLeftClick[Index])){
            DLeftClick[Index]  = 0;    
        }
        DRightClick[Index] = 0;
    }

    DCursorSet->DrawCursor(DWorkingBufferPixmap, DDrawingContext, DCurrentX[DPlayerColor], DCurrentY[DPlayerColor], DCursorIndices[DCursorType]);   
    
    gdk_draw_pixmap(DDoubleBufferPixmap, DDrawingContext, DWorkingBufferPixmap, 0, 0, 0, 0, -1, -1);
    
    gdk_draw_pixmap(DDrawingArea->window, DDrawingContext, DDoubleBufferPixmap, 0, 0, 0, 0, -1, -1);
    if(DNextGameMode != DGameMode){
        SwitchButtonMenuData();
    }
    DGameMode = DNextGameMode;
    
    return TRUE;
}

gboolean CApplicationData::MainWindowDeleteEvent(GtkWidget *widget, GdkEvent *event){
    PrintDebug(DEBUG_LOW, "Delete event occurred\n");   
    DDeleted = true;
    return FALSE;    
}

void CApplicationData::MainWindowDestroy(GtkWidget *widget){
    gtk_main_quit();
}

gboolean CApplicationData::MainWindowKeyPressEvent(GtkWidget *widget, GdkEventKey *event){
    bool Found = false;
    for(auto Key : DPressedKeys[pcNone]){
        if(Key == event->keyval){
            Found = true;
            break;
        }
    }
    if(!Found){
        DPressedKeys[pcNone].push_back(event->keyval);
        DPressedKeys[DPlayerColor].push_back(event->keyval);
    }
    return TRUE;
}

gboolean CApplicationData::MainWindowKeyReleaseEvent(GtkWidget *widget, GdkEventKey *event){
    bool Found = false;
    int Index = 0;
    for(auto Key : DPressedKeys[pcNone]){
        if(Key == event->keyval){
            Found = true;
            break;
        }
        Index++;
    }
    if(Found){
        DPressedKeys[pcNone].erase(DPressedKeys[pcNone].begin() + Index);
        DPressedKeys[DPlayerColor].erase(DPressedKeys[DPlayerColor].begin() + Index);
    }
    Found = false;
    for(auto Key : DReleasedKeys[pcNone]){
        if(Key == event->keyval){
            Found = true;
            break;
        }
    }
    if(!Found){
        DReleasedKeys[pcNone].push_back(event->keyval);
        DReleasedKeys[DPlayerColor].push_back(event->keyval);
    }
    return TRUE;
}

gboolean CApplicationData::MainWindowConfigureEvent(GtkWidget *widget, GdkEvent *event){
    ResizeCanvases();
    return TRUE;
}

gboolean CApplicationData::DrawingAreaExpose(GtkWidget *widget, GdkEventExpose *event){
    gdk_draw_pixmap(widget->window, widget->style->fg_gc[gtk_widget_get_state (widget)], DDoubleBufferPixmap, event->area.x, event->area.y, event->area.x, event->area.y, event->area.width, event->area.height);
    return FALSE;
}

gboolean CApplicationData::DrawingAreaButtonPressEvent(GtkWidget *widget, GdkEventButton *event){
    if(1 == event->button){
        if(GDK_CONTROL_MASK & event->state){
            DRightClick[pcNone] = DRightClick[DPlayerColor] = GDK_2BUTTON_PRESS == event->type ? 2 : 1;
            DRightDown[pcNone] = DRightDown[DPlayerColor] = true;
        }
        else{
            DLeftClick[pcNone] = DLeftClick[DPlayerColor] = GDK_2BUTTON_PRESS == event->type ? 2 : 1;
            DLeftDown[pcNone] = DLeftDown[DPlayerColor] = true;
        }
    }
    if(3 == event->button){
        DRightClick[pcNone] = DRightClick[DPlayerColor] = GDK_2BUTTON_PRESS == event->type ? 2 : 1;
        DRightDown[pcNone] = DRightDown[DPlayerColor] = true;
    }
    return TRUE;
}

gboolean CApplicationData::DrawingAreaButtonReleaseEvent(GtkWidget *widget, GdkEventButton *event){
    if(1 == event->button){
        if(GDK_CONTROL_MASK & event->state){
            DRightClick[pcNone] = DRightClick[DPlayerColor] = GDK_2BUTTON_PRESS == event->type ? 2 : 1;
            DRightDown[pcNone] = DRightDown[DPlayerColor] = false;
        }
        else{
            if(0 == DLeftClick[DPlayerColor]){
                DLeftClick[pcNone] = DLeftClick[DPlayerColor] = GDK_2BUTTON_PRESS == event->type ? 2 : 1;
            }
            DLeftDown[pcNone] = DLeftDown[DPlayerColor] = false;
        }
    }
    if(3 == event->button){
        DRightClick[pcNone] = DRightClick[DPlayerColor] = GDK_2BUTTON_PRESS == event->type ? 2 : 1;
        DRightDown[pcNone] = DRightDown[DPlayerColor] = false;
    }
    return TRUE;
}

gboolean CApplicationData::DrawingAreaMotionNotifyEvent(GtkWidget *widget, GdkEventMotion *event){
    GtkAllocation DrawingAreaAllocation;
    int EventX, EventY;
    EventX = event->x;
    EventY = event->y;

    gtk_widget_get_allocation(DDrawingArea, &DrawingAreaAllocation);
    
    if(EventX > DrawingAreaAllocation.width){
        EventX = DrawingAreaAllocation.width - 1;
    }
    if(0 > EventX){
        EventX = 0;   
    }
    if(EventY > DrawingAreaAllocation.height){
        EventY = DrawingAreaAllocation.height - 1;
    }
    if(0 > EventY){
        EventY = 0;   
    }

    DCurrentX[pcNone] = DCurrentX[DPlayerColor] = EventX;
    DCurrentY[pcNone] = DCurrentY[DPlayerColor] = EventY;
    return TRUE;
}

CApplicationData::EUIComponentType CApplicationData::FindUIComponentType(const CPosition &pos){
    gint ViewWidth, ViewHeight;
    gint MiniWidth, MiniHeight;
    gint DescrWidth, DescrHeight;
    gint ActWidth, ActHeight;
    
    gdk_pixmap_get_size(DViewportPixmap, &ViewWidth, &ViewHeight); 
    if((DViewportXOffset > pos.X())||(DViewportYOffset > pos.Y())||(DViewportXOffset + ViewWidth <= pos.X())||(DViewportYOffset + ViewHeight <= pos.Y())){
        if((DViewportXOffset - DInnerBevel->Width() <= pos.X())&&(DViewportXOffset > pos.X())){
            if((DViewportYOffset <= pos.Y())&&(pos.Y() < DViewportYOffset + ViewHeight)){
                return uictViewportBevelW;
            }
        }
        else if((DViewportXOffset + ViewWidth <= pos.X())&&(DViewportXOffset + ViewWidth + DInnerBevel->Width() > pos.X())){
            if((DViewportYOffset <= pos.Y())&&(pos.Y() < DViewportYOffset + ViewHeight)){
                return uictViewportBevelE;
            }
        }
        else if((DViewportXOffset <= pos.X())&&(pos.X() < DViewportXOffset + ViewWidth)){
            if((DViewportYOffset - DInnerBevel->Width() <= pos.Y())&&(DViewportYOffset > pos.Y())){
                return uictViewportBevelN;
            }
            else if((DViewportYOffset + ViewHeight <= pos.Y())&&(DViewportYOffset + ViewHeight + DInnerBevel->Width() > pos.Y())){
                return uictViewportBevelS;
            }
        }
    }
    else{
        return uictViewport;
    }
    gdk_pixmap_get_size(DMiniMapPixmap, &MiniWidth, &MiniHeight); 
    if((DMiniMapXOffset <= pos.X())&&(DMiniMapXOffset + MiniWidth > pos.X())&&(DMiniMapYOffset <= pos.Y())&&(DMiniMapYOffset + MiniHeight > pos.Y())){
        return uictMiniMap;  
    }
    gdk_pixmap_get_size(DUnitDescriptionPixmap, &DescrWidth, &DescrHeight); 
    if((DUnitDescriptionXOffset <= pos.X())&&(DUnitDescriptionXOffset + DescrWidth > pos.X())&&(DUnitDescriptionYOffset <= pos.Y())&&(DUnitDescriptionYOffset + DescrHeight > pos.Y())){
        return uictUserDescription;
    }
    gdk_pixmap_get_size(DUnitActionPixmap, &ActWidth, &ActHeight); 
    if((DUnitActionXOffset <= pos.X())&&(DUnitActionXOffset + ActWidth > pos.X())&&(DUnitActionYOffset <= pos.Y())&&(DUnitActionYOffset + ActHeight > pos.Y())){
        return uictUserAction;
    }
    
    if((DMenuButtonXOffset <= pos.X())&&(DMenuButtonXOffset + DMenuButtonRenderer->Width() > pos.X())&&(DMenuButtonYOffset <= pos.Y())&&(DMenuButtonYOffset + DMenuButtonRenderer->Height() > pos.Y())){
        return uictMenuButton;
    }
    return uictNone;
}

CPosition CApplicationData::ScreenToViewport(const CPosition &pos){
    return CPosition(pos.X() - DViewportXOffset, pos.Y() - DViewportYOffset);
}

CPosition CApplicationData::ScreenToMiniMap(const CPosition &pos){
    return CPosition(pos.X() - DMiniMapXOffset, pos.Y() - DMiniMapYOffset);
}

CPosition CApplicationData::ScreenToDetailedMap(const CPosition &pos){
    return ViewportToDetailedMap(ScreenToViewport(pos));
}

CPosition CApplicationData::ScreenToUnitDescription(const CPosition &pos){
    return CPosition(pos.X() - DUnitDescriptionXOffset, pos.Y() - DUnitDescriptionYOffset);
}

CPosition CApplicationData::ScreenToUnitAction(const CPosition &pos){
    return CPosition(pos.X() - DUnitActionXOffset, pos.Y() - DUnitActionYOffset);
}

CPosition CApplicationData::ViewportToDetailedMap(const CPosition &pos){
    return DViewportRenderer->DetailedPosition(pos);
}

CPosition CApplicationData::MiniMapToDetailedMap(const CPosition &pos){
    int X = pos.X() * DGameModel->Map()->Width() / DMiniMapRenderer->VisibleWidth();
    int Y = pos.Y() * DGameModel->Map()->Height() / DMiniMapRenderer->VisibleHeight();
    
    if(0 > X){
        X = 0;    
    }
    if(DGameModel->Map()->Width() <= X){
        X = DGameModel->Map()->Width() - 1;   
    }
    if(0 > Y){
        Y = 0;    
    }
    if(DGameModel->Map()->Height() <= Y){
        Y = DGameModel->Map()->Height() - 1;   
    }
    CPosition Temp;
    Temp.SetXFromTile(X);
    Temp.SetYFromTile(Y);
    return Temp;
}

void CApplicationData::ProcessInputGameMode(){
    bool Panning = false;
    bool ShiftPressed = false;
    EDirection PanningDirection = dMax;
    
    for(auto Key : DPressedKeys[DPlayerColor]){
        if(GDK_KEY_Up == Key){
            PanningDirection = dNorth;
            Panning = true;
        }
        else if(GDK_KEY_Down == Key){
            PanningDirection = dSouth;
            Panning = true;
        }
        else if(GDK_KEY_Left == Key){
            PanningDirection = dWest;
            Panning = true;
        }
        else if(GDK_KEY_Right == Key){
            PanningDirection = dEast;
            Panning = true;
        }
        else if((GDK_KEY_Shift_L == Key)||(GDK_KEY_Shift_R == Key)){
            ShiftPressed = true;   
        }
        
    }
    
    for(auto Key : DReleasedKeys[DPlayerColor]){
        // Handle releases
        if(DSelectedPlayerAssets[DPlayerColor].size()){
            bool CanMove = true;
            for(auto &Asset : DSelectedPlayerAssets[DPlayerColor]){
                if(auto LockedAsset = Asset.lock()){
                    if(0 == LockedAsset->Speed()){
                        CanMove = false;
                        break;
                    }
                }
            }
            if(GDK_KEY_Escape == Key){
                DCurrentAssetCapability[DPlayerColor] = actNone;   
            }
            if(actBuildSimple == DCurrentAssetCapability[DPlayerColor]){
                auto KeyLookup = DBuildHotKeyMap.find(Key);
                // check build
                if(KeyLookup != DBuildHotKeyMap.end()){
                    auto PlayerCapability = CPlayerCapability::FindCapability(KeyLookup->second); 
            
                    if(PlayerCapability){
                        auto ActorTarget = DSelectedPlayerAssets[DPlayerColor].front().lock();
                        
                        if(PlayerCapability->CanInitiate(ActorTarget, DGameModel->Player(DPlayerColor))){
                            SGameEvent TempEvent;
                            TempEvent.DType = etButtonTick;
                            DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
                            
                            DCurrentAssetCapability[DPlayerColor] = KeyLookup->second;
                        }
                    }
                }
            }
            else if(CanMove){
                auto KeyLookup = DUnitHotKeyMap.find(Key);
                
                if(KeyLookup != DUnitHotKeyMap.end()){
                    bool HasCapability = true;
                    for(auto &Asset : DSelectedPlayerAssets[DPlayerColor]){
                        if(auto LockedAsset = Asset.lock()){   
                            if(!LockedAsset->HasCapability(KeyLookup->second)){
                                HasCapability = false;
                                break;
                            }
                        }
                    }
                    if(HasCapability){
                        auto PlayerCapability = CPlayerCapability::FindCapability(KeyLookup->second); 
                        SGameEvent TempEvent;
                        TempEvent.DType = etButtonTick;
                        DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
            
                        if(PlayerCapability){
                            if((CPlayerCapability::ttNone == PlayerCapability->TargetType())||(CPlayerCapability::ttPlayer == PlayerCapability->TargetType())){
                                auto ActorTarget = DSelectedPlayerAssets[DPlayerColor].front().lock();
                                
                                if(PlayerCapability->CanApply(ActorTarget, DGameModel->Player(DPlayerColor), ActorTarget)){
                                    
                                    DPlayerCommands[DPlayerColor].DAction = KeyLookup->second;
                                    DPlayerCommands[DPlayerColor].DActors = DSelectedPlayerAssets[DPlayerColor];
                                    DPlayerCommands[DPlayerColor].DTargetColor = pcNone;
                                    DPlayerCommands[DPlayerColor].DTargetType = atNone;
                                    DPlayerCommands[DPlayerColor].DTargetLocation = ActorTarget->TilePosition();
                                    DCurrentAssetCapability[DPlayerColor] = actNone;
                                }
                            }
                            else{ 
                                DCurrentAssetCapability[DPlayerColor] = KeyLookup->second;
                            }
                        }
                        else{
                            DCurrentAssetCapability[DPlayerColor] = KeyLookup->second;
                        }
                    }
                }
            }
            else{
                auto KeyLookup = DTrainHotKeyMap.find(Key);
                
                if(KeyLookup != DTrainHotKeyMap.end()){
                    bool HasCapability = true;
                    for(auto &Asset : DSelectedPlayerAssets[DPlayerColor]){
                        if(auto LockedAsset = Asset.lock()){   
                            if(!LockedAsset->HasCapability(KeyLookup->second)){
                                HasCapability = false;
                                break;
                            }
                        }
                    }
                    if(HasCapability){
                        auto PlayerCapability = CPlayerCapability::FindCapability(KeyLookup->second); 
                        SGameEvent TempEvent;
                        TempEvent.DType = etButtonTick;
                        DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
                        
                        if(PlayerCapability){
                            if((CPlayerCapability::ttNone == PlayerCapability->TargetType())||(CPlayerCapability::ttPlayer == PlayerCapability->TargetType())){
                                auto ActorTarget = DSelectedPlayerAssets[DPlayerColor].front().lock();
                                
                                if(PlayerCapability->CanApply(ActorTarget, DGameModel->Player(DPlayerColor), ActorTarget)){
                                    
                                    DPlayerCommands[DPlayerColor].DAction = KeyLookup->second;
                                    DPlayerCommands[DPlayerColor].DActors = DSelectedPlayerAssets[DPlayerColor];
                                    DPlayerCommands[DPlayerColor].DTargetColor = pcNone;
                                    DPlayerCommands[DPlayerColor].DTargetType = atNone;
                                    DPlayerCommands[DPlayerColor].DTargetLocation = ActorTarget->TilePosition();
                                    DCurrentAssetCapability[DPlayerColor] = actNone;
                                }
                            }
                            else{ 
                                DCurrentAssetCapability[DPlayerColor] = KeyLookup->second;
                            }
                        }
                        else{
                            DCurrentAssetCapability[DPlayerColor] = KeyLookup->second;
                        }
                    }
                }
            }
        }
    }
    DReleasedKeys[pcNone].clear();
    DReleasedKeys[DPlayerColor].clear();
    
    DMenuButtonState = CButtonRenderer::bsNone; 
    EUIComponentType ComponentType = FindUIComponentType(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor]));
    if(uictViewport == ComponentType){
        CPosition TempPosition(ScreenToDetailedMap(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));
        CPixelType PixelType = CPixelType::GetPixelType(DViewportTypePixmap, ScreenToViewport(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));
        
        if(DRightClick[DPlayerColor] && !DRightDown[DPlayerColor] && DSelectedPlayerAssets[DPlayerColor].size()){
            bool CanMove = true;
            for(auto &Asset : DSelectedPlayerAssets[DPlayerColor]){
                if(auto LockedAsset = Asset.lock()){
                    if(0 == LockedAsset->Speed()){
                        CanMove = false;
                        break;
                    }
                }
            }
            if(CanMove){
                if(pcNone != PixelType.Color()){
                    // Command is either walk/deliver, repair, or attack
                    
                    DPlayerCommands[DPlayerColor].DAction = actMove;
                    DPlayerCommands[DPlayerColor].DTargetColor = PixelType.Color();
                    DPlayerCommands[DPlayerColor].DTargetType = PixelType.AssetType();
                    DPlayerCommands[DPlayerColor].DActors = DSelectedPlayerAssets[DPlayerColor];
                    DPlayerCommands[DPlayerColor].DTargetLocation = TempPosition;
                    if(PixelType.Color() == DPlayerColor){
                        bool HaveLumber = false;
                        bool HaveGold = false;
                        
                        for(auto &Asset : DSelectedPlayerAssets[DPlayerColor]){
                            if(auto LockedAsset = Asset.lock()){
                                if(LockedAsset->Lumber()){
                                    HaveLumber = true;     
                                }
                                if(LockedAsset->Gold()){
                                    HaveGold = true;
                                }
                            }
                        }
                        if(HaveGold){
                            if((atTownHall == DPlayerCommands[DPlayerColor].DTargetType)||(atKeep == DPlayerCommands[DPlayerColor].DTargetType)||(atCastle == DPlayerCommands[DPlayerColor].DTargetType)){
                                DPlayerCommands[DPlayerColor].DAction = actConvey;
                            }
                        }
                        else if(HaveLumber){
                            if((atTownHall == DPlayerCommands[DPlayerColor].DTargetType)||(atKeep == DPlayerCommands[DPlayerColor].DTargetType)||(atCastle == DPlayerCommands[DPlayerColor].DTargetType)||(atLumberMill == DPlayerCommands[DPlayerColor].DTargetType)){
                                DPlayerCommands[DPlayerColor].DAction = actConvey;
                            }   
                        }
                        else{
                            auto TargetAsset = DGameModel->Player(DPlayerColor)->SelectAsset(TempPosition, PixelType.AssetType()).lock();
                            if((0 == TargetAsset->Speed())&&(TargetAsset->MaxHitPoints() > TargetAsset->HitPoints())){
                                DPlayerCommands[DPlayerColor].DAction = actRepair;
                            }
                        }
                    }
                    else{
                        DPlayerCommands[DPlayerColor].DAction = actAttack;     
                    }
                    DCurrentAssetCapability[DPlayerColor] = actNone;
                }
                else{                
                    // Command is either walk, mine, harvest
                    CPosition TempPosition(ScreenToDetailedMap(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));
                    bool CanHarvest = true;
                    
                    DPlayerCommands[DPlayerColor].DAction = actMove;
                    DPlayerCommands[DPlayerColor].DTargetColor = pcNone;
                    DPlayerCommands[DPlayerColor].DTargetType = atNone;
                    DPlayerCommands[DPlayerColor].DActors = DSelectedPlayerAssets[DPlayerColor];
                    DPlayerCommands[DPlayerColor].DTargetLocation = TempPosition;
                    
                    for(auto &Asset : DSelectedPlayerAssets[DPlayerColor]){
                        if(auto LockedAsset = Asset.lock()){
                            if(!LockedAsset->HasCapability(actMine)){
                                CanHarvest = false;
                                break;
                            }
                        }
                    }
                    if(CanHarvest){
                        if(CPixelType::attTree == PixelType.Type()){
                            CPosition TempTilePosition;
                            
                            DPlayerCommands[DPlayerColor].DAction = actMine;
                            TempTilePosition.SetToTile(DPlayerCommands[DPlayerColor].DTargetLocation);
                            if(CTerrainMap::ttTree != DGameModel->Player(DPlayerColor)->PlayerMap()->TileType(TempTilePosition)){
                                // Could be tree pixel, but tops of next row
                                TempTilePosition.IncrementY(1);
                                if(CTerrainMap::ttTree == DGameModel->Player(DPlayerColor)->PlayerMap()->TileType(TempTilePosition)){
                                    DPlayerCommands[DPlayerColor].DTargetLocation.SetFromTile(TempTilePosition);
                                }
                            }
                        }
                        else if(CPixelType::attGoldMine == PixelType.Type()){
                            DPlayerCommands[DPlayerColor].DAction = actMine;
                            DPlayerCommands[DPlayerColor].DTargetType = atGoldMine;
                        }
                    }
                    DCurrentAssetCapability[DPlayerColor] = actNone;
                }
            }
        }
        else if(DLeftClick[DPlayerColor]){
            if((actNone == DCurrentAssetCapability[DPlayerColor])||(actBuildSimple == DCurrentAssetCapability[DPlayerColor])){
                if(DLeftDown[DPlayerColor]){
                    DMouseDown[DPlayerColor] = TempPosition;
                }
                else{
                    SRectangle TempRectangle; 
                    EPlayerColor SearchColor = DPlayerColor;
                    std::list< std::shared_ptr< CPlayerAsset > > PreviousSelections;
                    
                    for(auto WeakAsset : DSelectedPlayerAssets[DPlayerColor]){
                        if(auto LockedAsset = WeakAsset.lock()){
                            PreviousSelections.push_back(LockedAsset);   
                        }
                    }
                    
                    TempRectangle.DXPosition = MIN(DMouseDown[DPlayerColor].X(), TempPosition.X());
                    TempRectangle.DYPosition = MIN(DMouseDown[DPlayerColor].Y(), TempPosition.Y());
                    TempRectangle.DWidth = MAX(DMouseDown[DPlayerColor].X(), TempPosition.X()) - TempRectangle.DXPosition;
                    TempRectangle.DHeight = MAX(DMouseDown[DPlayerColor].Y(), TempPosition.Y()) - TempRectangle.DYPosition;
                    
                    if((TempRectangle.DWidth < CPosition::TileWidth())||(TempRectangle.DHeight < CPosition::TileHeight())||(2 == DLeftClick[DPlayerColor])){
                        TempRectangle.DXPosition = TempPosition.X();
                        TempRectangle.DYPosition = TempPosition.Y();
                        TempRectangle.DWidth = 0;
                        TempRectangle.DHeight = 0;
                        SearchColor = PixelType.Color();
                    }
                    if(SearchColor != DPlayerColor){
                        DSelectedPlayerAssets[DPlayerColor].clear();    
                    }
                    if(ShiftPressed){
                        if(!DSelectedPlayerAssets[DPlayerColor].empty()){
                            if(auto TempAsset = DSelectedPlayerAssets[DPlayerColor].front().lock()){
                                if(TempAsset->Color() != DPlayerColor){
                                    DSelectedPlayerAssets[DPlayerColor].clear();
                                }
                            }
                        }
                        DSelectedPlayerAssets[DPlayerColor].splice(DSelectedPlayerAssets[DPlayerColor].end(), DGameModel->Player(SearchColor)->SelectAssets(TempRectangle, PixelType.AssetType(), 2 == DLeftClick[DPlayerColor]));
                        DSelectedPlayerAssets[DPlayerColor].sort(WeakPtrCompare<CPlayerAsset>);
                        DSelectedPlayerAssets[DPlayerColor].unique(WeakPtrEquals<CPlayerAsset>);
                    }
                    else{
                        PreviousSelections.clear();
                        DSelectedPlayerAssets[DPlayerColor] = DGameModel->Player(SearchColor)->SelectAssets(TempRectangle, PixelType.AssetType(), 2 == DLeftClick[DPlayerColor]);
                    }
                    for(auto WeakAsset : DSelectedPlayerAssets[DPlayerColor]){
                        if(auto LockedAsset = WeakAsset.lock()){
                            bool FoundPrevious = false;
                            for(auto PrevAsset : PreviousSelections){
                                if(PrevAsset == LockedAsset){
                                    FoundPrevious = true;
                                    break;
                                }
                            }
                            if(!FoundPrevious){
                                SGameEvent TempEvent;
                                TempEvent.DType = etSelection;
                                TempEvent.DAsset = LockedAsset;
                                DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
                            }
                        }
                    }
                    
                
                    DMouseDown[DPlayerColor] = CPosition(-1,-1);
                }
                DCurrentAssetCapability[DPlayerColor] = actNone;
            }
            else{
                auto PlayerCapability = CPlayerCapability::FindCapability(DCurrentAssetCapability[DPlayerColor]); 
                
                if(PlayerCapability && !DLeftDown[DPlayerColor]){
                    if(((CPlayerCapability::ttAsset == PlayerCapability->TargetType())||(CPlayerCapability::ttTerrainOrAsset == PlayerCapability->TargetType()))&&(atNone != PixelType.AssetType())){
                        auto NewTarget = DGameModel->Player(PixelType.Color())->SelectAsset(TempPosition, PixelType.AssetType()).lock();
                        
                        if(PlayerCapability->CanApply(DSelectedPlayerAssets[DPlayerColor].front().lock(), DGameModel->Player(DPlayerColor), NewTarget)){
                            //PlayerCapability->ApplyCapability(DSelectedPlayerAssets[DPlayerColor].front().lock(), DGameModel->Player(DPlayerColor), NewTarget);
                            SGameEvent TempEvent;
                            TempEvent.DType = etPlaceAction;
                            TempEvent.DAsset = NewTarget;
                            DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
                            
                            DPlayerCommands[DPlayerColor].DAction = DCurrentAssetCapability[DPlayerColor];
                            DPlayerCommands[DPlayerColor].DActors = DSelectedPlayerAssets[DPlayerColor];
                            DPlayerCommands[DPlayerColor].DTargetColor = PixelType.Color();
                            DPlayerCommands[DPlayerColor].DTargetType = PixelType.AssetType();
                            DPlayerCommands[DPlayerColor].DTargetLocation = TempPosition;
                            DCurrentAssetCapability[DPlayerColor] = actNone;
                        }
                    }
                    else if(((CPlayerCapability::ttTerrain == PlayerCapability->TargetType())||(CPlayerCapability::ttTerrainOrAsset == PlayerCapability->TargetType()))&&((atNone == PixelType.AssetType())&&(pcNone == PixelType.Color()))){
                        auto NewTarget = DGameModel->Player(DPlayerColor)->CreateMarker(TempPosition, false);
                        
                        if(PlayerCapability->CanApply(DSelectedPlayerAssets[DPlayerColor].front().lock(), DGameModel->Player(DPlayerColor), NewTarget)){
                            //PlayerCapability->ApplyCapability(DSelectedPlayerAssets[DPlayerColor].front().lock(), DGameModel->Player(DPlayerColor), NewTarget);
                            SGameEvent TempEvent;
                            TempEvent.DType = etPlaceAction;
                            TempEvent.DAsset = NewTarget;
                            DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
                            
                            DPlayerCommands[DPlayerColor].DAction = DCurrentAssetCapability[DPlayerColor];
                            DPlayerCommands[DPlayerColor].DActors = DSelectedPlayerAssets[DPlayerColor];
                            DPlayerCommands[DPlayerColor].DTargetColor = pcNone;
                            DPlayerCommands[DPlayerColor].DTargetType = atNone;
                            DPlayerCommands[DPlayerColor].DTargetLocation = TempPosition;
                            DCurrentAssetCapability[DPlayerColor] = actNone;
                        }
                    }
                    else{
                        
                    }

                }
            }
        }
    }
    else if(uictViewportBevelN == ComponentType){
        PanningDirection = dNorth;
        Panning = true;
    }
    else if(uictViewportBevelE == ComponentType){
        PanningDirection = dEast;
        Panning = true;
    }
    else if(uictViewportBevelS == ComponentType){
        PanningDirection = dSouth;
        Panning = true;
    }
    else if(uictViewportBevelW == ComponentType){
        PanningDirection = dWest;
        Panning = true;
    }
    else if(uictMiniMap == ComponentType){
        if(DLeftClick[DPlayerColor] && !DLeftDown[DPlayerColor]){
            CPosition TempPosition(ScreenToMiniMap(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));
            TempPosition = MiniMapToDetailedMap(TempPosition);

            DViewportRenderer->CenterViewport(TempPosition);
        }
    }
    else if(uictUserDescription == ComponentType){
        if(DLeftClick[DPlayerColor] && !DLeftDown[DPlayerColor]){
            int IconPressed = DUnitDescriptionRenderer->Selection(ScreenToUnitDescription(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));   
            
            if(1 == DSelectedPlayerAssets[DPlayerColor].size()){
                if(0 == IconPressed){
                    if(auto Asset = DSelectedPlayerAssets[DPlayerColor].front().lock()){
                        DViewportRenderer->CenterViewport(Asset->Position());      
                    }
                }
            }
            else if(0 <= IconPressed){
                while(IconPressed){
                    IconPressed--;
                    DSelectedPlayerAssets[DPlayerColor].pop_front();
                }
                while(1 < DSelectedPlayerAssets[DPlayerColor].size()){
                    DSelectedPlayerAssets[DPlayerColor].pop_back();
                }
                SGameEvent TempEvent;
                TempEvent.DType = etSelection;
                TempEvent.DAsset = DSelectedPlayerAssets[DPlayerColor].front().lock();
                DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
            }
        }
    }
    else if(uictUserAction == ComponentType){
        if(DLeftClick[DPlayerColor] && !DLeftDown[DPlayerColor]){
            EAssetCapabilityType CapabilityType = DUnitActionRenderer->Selection(ScreenToUnitAction(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));   
            auto PlayerCapability = CPlayerCapability::FindCapability(CapabilityType); 

            if(actNone != CapabilityType){            
                SGameEvent TempEvent;
                TempEvent.DType = etButtonTick;
                DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
            }
            if(PlayerCapability){
                if((CPlayerCapability::ttNone == PlayerCapability->TargetType())||(CPlayerCapability::ttPlayer == PlayerCapability->TargetType())){
                    auto ActorTarget = DSelectedPlayerAssets[DPlayerColor].front().lock();
                    
                    if(PlayerCapability->CanApply(ActorTarget, DGameModel->Player(DPlayerColor), ActorTarget)){
                        
                        DPlayerCommands[DPlayerColor].DAction = CapabilityType;
                        DPlayerCommands[DPlayerColor].DActors = DSelectedPlayerAssets[DPlayerColor];
                        DPlayerCommands[DPlayerColor].DTargetColor = pcNone;
                        DPlayerCommands[DPlayerColor].DTargetType = atNone;
                        DPlayerCommands[DPlayerColor].DTargetLocation = ActorTarget->Position();
                        DCurrentAssetCapability[DPlayerColor] = actNone;
                    }
                }
                else{ 
                    DCurrentAssetCapability[DPlayerColor] = CapabilityType;
                }
            }
            else{
                DCurrentAssetCapability[DPlayerColor] = CapabilityType;
            }
        }
    }
    else if(uictMenuButton == ComponentType){
        DMenuButtonState = DLeftDown[DPlayerColor] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
    }
    if(!Panning){
        DPanningSpeed = 0;   
    }
    else{
        if(dNorth == PanningDirection){
            DViewportRenderer->PanNorth(DPanningSpeed>>PAN_SPEED_SHIFT);    
        }
        else if(dEast == PanningDirection){
            DViewportRenderer->PanEast(DPanningSpeed>>PAN_SPEED_SHIFT);    
        }
        else if(dSouth == PanningDirection){
            DViewportRenderer->PanSouth(DPanningSpeed>>PAN_SPEED_SHIFT);    
        }
        else if(dWest == PanningDirection){
            DViewportRenderer->PanWest(DPanningSpeed>>PAN_SPEED_SHIFT);    
        }
        if(DPanningSpeed){
            DPanningSpeed++;
            if(PAN_SPEED_MAX < DPanningSpeed){
                DPanningSpeed = PAN_SPEED_MAX;
            }
        }
        else{
            DPanningSpeed = 1<<PAN_SPEED_SHIFT;
        }
    }
    
}

void CApplicationData::RenderGameMode(){
    SRectangle TempRectangle({0,0,0,0});
    gint BufferWidth, BufferHeight;
    gint ViewWidth, ViewHeight;
    gint MiniMapWidth, MiniMapHeight;
    gint DescriptionWidth, DescriptionHeight;
    gint ActionWidth, ActionHeight;
    gint ResourceWidth, ResourceHeight;
    std::list< std::weak_ptr< CPlayerAsset > > SelectedAndMarkerAssets = DSelectedPlayerAssets[DPlayerColor];

    gdk_pixmap_get_size(DWorkingBufferPixmap, &BufferWidth, &BufferHeight); 
    gdk_pixmap_get_size(DViewportPixmap, &ViewWidth, &ViewHeight); 
    gdk_pixmap_get_size(DMiniMapPixmap, &MiniMapWidth, &MiniMapHeight); 
    gdk_pixmap_get_size(DUnitDescriptionPixmap, &DescriptionWidth, &DescriptionHeight); 
    gdk_pixmap_get_size(DUnitActionPixmap, &ActionWidth, &ActionHeight); 
    gdk_pixmap_get_size(DResourcePixmap, &ResourceWidth, &ResourceHeight); 
    
    if(DLeftDown[DPlayerColor] && 0 < DMouseDown[DPlayerColor].X()){
        CPosition TempPosition(ScreenToDetailedMap(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));
        TempRectangle.DXPosition = MIN(DMouseDown[DPlayerColor].X(), TempPosition.X());
        TempRectangle.DYPosition = MIN(DMouseDown[DPlayerColor].Y(), TempPosition.Y());
        TempRectangle.DWidth = MAX(DMouseDown[DPlayerColor].X(), TempPosition.X()) - TempRectangle.DXPosition;
        TempRectangle.DHeight = MAX(DMouseDown[DPlayerColor].Y(), TempPosition.Y()) - TempRectangle.DYPosition;
    }
    else{
        CPosition TempPosition(ScreenToDetailedMap(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor])));
        TempRectangle.DXPosition = TempPosition.X();
        TempRectangle.DYPosition = TempPosition.Y();
    }
    DBackgroundTileset->DrawTileRectangle(DWorkingBufferPixmap, 0, 0, BufferWidth, BufferHeight, 0);
    DInnerBevel->DrawBevel(DWorkingBufferPixmap, DViewportXOffset, DViewportYOffset, ViewWidth, ViewHeight);
    DInnerBevel->DrawBevel(DWorkingBufferPixmap, DMiniMapXOffset, DMiniMapYOffset, MiniMapWidth, MiniMapHeight);
    
    gdk_draw_pixmap(DResourcePixmap, DDrawingContext, DWorkingBufferPixmap, DViewportXOffset, 0, 0, 0, ResourceWidth, ResourceHeight);
    DResourceRenderer->DrawResources(DResourcePixmap, DDrawingContext);
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DResourcePixmap, 0, 0, DViewportXOffset, 0, -1, -1);
    
    DOuterBevel->DrawBevel(DWorkingBufferPixmap, DUnitDescriptionXOffset, DUnitDescriptionYOffset, DescriptionWidth, DescriptionHeight);
    gdk_draw_pixmap(DUnitDescriptionPixmap, DDrawingContext, DWorkingBufferPixmap, DUnitDescriptionXOffset, DUnitDescriptionYOffset, 0, 0, DescriptionWidth, DescriptionHeight);
    DUnitDescriptionRenderer->DrawUnitDescription(DUnitDescriptionPixmap, DSelectedPlayerAssets[DPlayerColor]);
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DUnitDescriptionPixmap, 0, 0, DUnitDescriptionXOffset, DUnitDescriptionYOffset, -1, -1);
    
    DOuterBevel->DrawBevel(DWorkingBufferPixmap, DUnitActionXOffset, DUnitActionYOffset, ActionWidth, ActionHeight);
    gdk_draw_pixmap(DUnitActionPixmap, DDrawingContext, DWorkingBufferPixmap, DUnitActionXOffset, DUnitActionYOffset, 0, 0, ActionWidth, ActionHeight);
    DUnitActionRenderer->DrawUnitAction(DUnitActionPixmap, DSelectedPlayerAssets[DPlayerColor], DCurrentAssetCapability[DPlayerColor]);
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DUnitActionPixmap, 0, 0, DUnitActionXOffset, DUnitActionYOffset, -1, -1);
    
    for(auto Asset : DGameModel->Player(DPlayerColor)->PlayerMap()->Assets()){
        if(atNone == Asset->Type()){
            SelectedAndMarkerAssets.push_back(Asset);      
        }
    }
    DViewportRenderer->DrawViewport(DViewportPixmap, DViewportTypePixmap, SelectedAndMarkerAssets, TempRectangle, DCurrentAssetCapability[DPlayerColor]);
    DMiniMapRenderer->DrawMiniMap(DMiniMapPixmap);
    
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DMiniMapPixmap, 0, 0, DMiniMapXOffset, DMiniMapYOffset, -1, -1);
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DViewportPixmap, 0, 0, DViewportXOffset, DViewportYOffset, -1, -1);

    
    DMenuButtonRenderer->DrawButton(DWorkingBufferPixmap, DMenuButtonXOffset, DMenuButtonYOffset, DMenuButtonState);
    
    switch(FindUIComponentType(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor]))){
        case uictViewport:          {
                                        CPosition ViewportCursorLocation = ScreenToViewport(CPosition(DCurrentX[DPlayerColor], DCurrentY[DPlayerColor]));
                                        CPixelType PixelType = CPixelType::GetPixelType(DViewportTypePixmap, ViewportCursorLocation.X(), ViewportCursorLocation.Y());
                                        DCursorType = ctPointer;
                                        if(actNone == DCurrentAssetCapability[DPlayerColor]){
                                            if(PixelType.Color() == DPlayerColor){
                                                DCursorType = ctInspect;
                                            }
                                        }
                                        else{
                                            auto PlayerCapability = CPlayerCapability::FindCapability(DCurrentAssetCapability[DPlayerColor]); 
                
                                            if(PlayerCapability){
                                                bool CanApply = false;

                                                if(atNone == PixelType.AssetType()){
                                                    if((CPlayerCapability::ttTerrain == PlayerCapability->TargetType())||(CPlayerCapability::ttTerrainOrAsset == PlayerCapability->TargetType())){
                                                        auto NewTarget = DGameModel->Player(DPlayerColor)->CreateMarker(ViewportToDetailedMap(ViewportCursorLocation), false);

                                                        CanApply = PlayerCapability->CanApply(DSelectedPlayerAssets[DPlayerColor].front().lock(), DGameModel->Player(DPlayerColor), NewTarget);  
                                                    }
                                                }
                                                else{
                                                    if((CPlayerCapability::ttAsset == PlayerCapability->TargetType())||(CPlayerCapability::ttTerrainOrAsset == PlayerCapability->TargetType())){
                                                        auto NewTarget = DGameModel->Player(PixelType.Color())->SelectAsset(ViewportToDetailedMap(ViewportCursorLocation), PixelType.AssetType()).lock();

                                                        CanApply = PlayerCapability->CanApply(DSelectedPlayerAssets[DPlayerColor].front().lock(), DGameModel->Player(DPlayerColor), NewTarget);     
                                                    }
                                                }

                                                DCursorType = CanApply ? ctTargetOn : ctTargetOff;
                                            }
                                        }
                                    
                                    }
                                    break;
        case uictViewportBevelN:    DCursorType = ctArrowN;
                                    break;
        case uictViewportBevelE:    DCursorType = ctArrowE;
                                    break;
        case uictViewportBevelS:    DCursorType = ctArrowS;
                                    break;
        case uictViewportBevelW:    DCursorType = ctArrowW;
                                    break;
        default:                    DCursorType = ctPointer;
                                    break;
    }
    SRectangle ViewportRectangle({DViewportRenderer->ViewportX(),DViewportRenderer->ViewportY(),DViewportRenderer->LastViewportWidth(),DViewportRenderer->LastViewportHeight()});
    
    DSoundEventRenderer->RenderEvents(ViewportRectangle);
}

void CApplicationData::CalculateGameMode(){
    for(int Index = 1; Index < pcMax; Index++){
        if(DGameModel->Player((EPlayerColor)Index)->IsAlive() && DGameModel->Player((EPlayerColor)Index)->IsAI()){
            DAIPlayers[Index]->CalculateCommand(DPlayerCommands[Index]); 
        }
    }

    
    for(int Index = 1; Index < pcMax; Index++){
        if(actNone != DPlayerCommands[Index].DAction){
            auto PlayerCapability = CPlayerCapability::FindCapability(DPlayerCommands[Index].DAction); 
            if(PlayerCapability){
                std::shared_ptr< CPlayerAsset > NewTarget;

                if((CPlayerCapability::ttNone != PlayerCapability->TargetType())&&(CPlayerCapability::ttPlayer != PlayerCapability->TargetType())){
                    if(atNone == DPlayerCommands[Index].DTargetType){
                        NewTarget = DGameModel->Player((EPlayerColor)Index)->CreateMarker(DPlayerCommands[Index].DTargetLocation, true);
                    }
                    else{
                        NewTarget = DGameModel->Player(DPlayerCommands[Index].DTargetColor)->SelectAsset(DPlayerCommands[Index].DTargetLocation, DPlayerCommands[Index].DTargetType).lock();
                    }
                }
                
                for(auto &WeakActor : DPlayerCommands[Index].DActors){
                    if(auto Actor = WeakActor.lock()){
                        if(PlayerCapability->CanApply(Actor, DGameModel->Player((EPlayerColor)Index), NewTarget) && (Actor->Interruptible() || (actCancel == DPlayerCommands[Index].DAction))){
                            PlayerCapability->ApplyCapability(Actor, DGameModel->Player((EPlayerColor)Index), NewTarget);
                        }
                    }
                }
            }
            DPlayerCommands[Index].DAction = actNone;
        }
    }
    DGameModel->Timestep();
    auto WeakAsset = DSelectedPlayerAssets[DPlayerColor].begin();
    while(WeakAsset != DSelectedPlayerAssets[DPlayerColor].end()){
        if(auto Asset = WeakAsset->lock()){
            if(DGameModel->ValidAsset(Asset) && Asset->Alive()){
                if(Asset->Speed() && (aaCapability == Asset->Action())){
                    auto Command = Asset->CurrentCommand();
                    
                    if(Command.DAssetTarget && (aaConstruct == Command.DAssetTarget->Action())){
                        SGameEvent TempEvent;
                        
                        DSelectedPlayerAssets[DPlayerColor].clear();
                        DSelectedPlayerAssets[DPlayerColor].push_back(Command.DAssetTarget);
                        
                        TempEvent.DType = etSelection;
                        TempEvent.DAsset = Command.DAssetTarget;
                        DGameModel->Player(DPlayerColor)->AddGameEvent(TempEvent);
                                
                        break;
                    }
                }
                WeakAsset++;
            }
            else{
                WeakAsset = DSelectedPlayerAssets[DPlayerColor].erase(WeakAsset);   
            }
        }
        else{
            WeakAsset = DSelectedPlayerAssets[DPlayerColor].erase(WeakAsset);   
        }
    }
}

void CApplicationData::ProcessInputButtonMenuMode(){
    if(DLeftClick[pcNone] && !DLeftDown[pcNone]){
        for(int Index = 0; Index < DCurrentPageButtonLocations.size(); Index++){
            if((DCurrentPageButtonLocations[Index].DXPosition <= DCurrentX[pcNone])&&((DCurrentPageButtonLocations[Index].DXPosition + DCurrentPageButtonLocations[Index].DWidth > DCurrentX[pcNone]))){
                if((DCurrentPageButtonLocations[Index].DYPosition <= DCurrentY[pcNone])&&((DCurrentPageButtonLocations[Index].DYPosition + DCurrentPageButtonLocations[Index].DHeight > DCurrentY[pcNone]))){
                    DCurrentPageButtonsFunctions[Index](this);
                }
            }
        }
    }   
}

void CApplicationData::CalculateButtonMenuMode(){

}

void CApplicationData::RenderMenuTitle(const std::string &title, gint &titlebottomy, gint &pagewidth, gint &pageheight){
    gint TitleWidth;
    int TextColor, ShadowColor;

    gdk_pixmap_get_size(DWorkingBufferPixmap, &pagewidth, &pageheight); 

    DBackgroundTileset->DrawTileRectangle(DWorkingBufferPixmap, 0, 0, pagewidth, pageheight, 0);
    DOuterBevel->DrawBevel(DWorkingBufferPixmap, DOuterBevel->Width(), DOuterBevel->Width(), pagewidth - DOuterBevel->Width() * 2, pageheight - DOuterBevel->Width() * 2);
    
    DFonts[CUnitDescriptionRenderer::fsGiant]->MeasureText(title, TitleWidth, titlebottomy);
    TextColor = DFonts[CUnitDescriptionRenderer::fsGiant]->FindPixel("white");
    ShadowColor = DFonts[CUnitDescriptionRenderer::fsGiant]->FindPixel("black");
    DFonts[CUnitDescriptionRenderer::fsGiant]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, pagewidth/2 - TitleWidth/2, DOuterBevel->Width(), TextColor, ShadowColor, 1, title);
    
}

void CApplicationData::RenderButtonMenuMode(){
    gint BufferWidth, BufferHeight;
    gint TitleHeight;
    int ButtonLeft, ButtonSkip, ButtonTop;
    int ButtonIndex;
    bool ButtonXAlign = false, ButtonHovered = false;

    RenderMenuTitle(DCurrentPageTitle, TitleHeight, BufferWidth, BufferHeight); 
    DCurrentPageButtonRenderer->Text(DCurrentPageButtonsText.front(), true);   
    for(auto Text : DCurrentPageButtonsText){
        DCurrentPageButtonRenderer->Text(Text);
    }
    DCurrentPageButtonRenderer->Height(DCurrentPageButtonRenderer->Height() * 3 / 2);
    DCurrentPageButtonRenderer->Width(DCurrentPageButtonRenderer->Width() * 5 / 4);
    ButtonSkip = DCurrentPageButtonRenderer->Height() * 3 / 2;
    ButtonLeft = BufferWidth/2 - DCurrentPageButtonRenderer->Width()/2;
    ButtonTop = (BufferHeight - TitleHeight)/2 - (ButtonSkip * (DCurrentPageButtonsText.size() - 1) + DCurrentPageButtonRenderer->Height())/2;
    ButtonTop += DOuterBevel->Width();
    if((ButtonLeft <= DCurrentX[pcNone])&&((ButtonLeft + DCurrentPageButtonRenderer->Width() > DCurrentX[pcNone]))){
        ButtonXAlign = true;       
    }
    ButtonIndex = 0;
    for(auto Text : DCurrentPageButtonsText){
        if(Text.length()){
            CButtonRenderer::EButtonState ButtonState = CButtonRenderer::bsNone;
            if(ButtonXAlign){
                if((ButtonTop <= DCurrentY[pcNone])&&((ButtonTop + DCurrentPageButtonRenderer->Height() > DCurrentY[pcNone]))){
                    ButtonState = DLeftDown[pcNone] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
                    ButtonHovered = true;
                }
            }
            DCurrentPageButtonRenderer->Text(Text);
            DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, ButtonLeft, ButtonTop, ButtonState);
            DCurrentPageButtonLocations[ButtonIndex] = SRectangle({ButtonLeft, ButtonTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()});
        }
        else{
            DCurrentPageButtonLocations[ButtonIndex] = SRectangle({0, 0, 0, 0});
        }
        ButtonIndex++;
        ButtonTop += ButtonSkip;
    }
    if(!DCurrentPageButtonHovered && ButtonHovered){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("tick"), DSoundVolume, 0.0);
    }
    if(DNextGameMode != DGameMode){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("place"), DSoundVolume, 0.0);
    }
    DCurrentPageButtonHovered = ButtonHovered;
}

void CApplicationData::ProcessInputEditOptionsMode(){
    if(DLeftClick[pcNone] && !DLeftDown[pcNone]){
        bool ClickedEdit = false;
        for(int Index = 0; Index < DCurrentPageButtonLocations.size(); Index++){
            if((DCurrentPageButtonLocations[Index].DXPosition <= DCurrentX[pcNone])&&((DCurrentPageButtonLocations[Index].DXPosition + DCurrentPageButtonLocations[Index].DWidth > DCurrentX[pcNone]))){
                if((DCurrentPageButtonLocations[Index].DYPosition <= DCurrentY[pcNone])&&((DCurrentPageButtonLocations[Index].DYPosition + DCurrentPageButtonLocations[Index].DHeight > DCurrentY[pcNone]))){
                    DCurrentPageButtonsFunctions[Index](this);
                }
            }
        }
        for(int Index = 0; Index < DOptionsEditLocations.size(); Index++){
            if((DOptionsEditLocations[Index].DXPosition <= DCurrentX[pcNone])&&((DOptionsEditLocations[Index].DXPosition + DOptionsEditLocations[Index].DWidth > DCurrentX[pcNone]))){
                if((DOptionsEditLocations[Index].DYPosition <= DCurrentY[pcNone])&&((DOptionsEditLocations[Index].DYPosition + DOptionsEditLocations[Index].DHeight > DCurrentY[pcNone]))){
                    if(Index != DOptionsEditSelected){
                        DOptionsEditSelected = Index;
                        DOptionsEditSelectedCharacter = DOptionsEditText[Index].size();
                        ClickedEdit = true;
                    }
                }
            } 
        }
        if(!ClickedEdit){
            DOptionsEditSelected = -1;     
        }
    }
    for(auto Key : DReleasedKeys[pcNone]){
        if(GDK_KEY_Escape == Key){
            DOptionsEditSelected = -1;
        }
        else if(0 <= DOptionsEditSelected){
            if((GDK_KEY_Delete == Key)||(GDK_KEY_BackSpace == Key)){
                if(DOptionsEditSelectedCharacter){
                    DOptionsEditText[DOptionsEditSelected] = DOptionsEditText[DOptionsEditSelected].substr(0,DOptionsEditSelectedCharacter-1) + DOptionsEditText[DOptionsEditSelected].substr(DOptionsEditSelectedCharacter,DOptionsEditText[DOptionsEditSelected].length()-DOptionsEditSelectedCharacter);
                    DOptionsEditSelectedCharacter--;
                }
                else if(DOptionsEditText[DOptionsEditSelected].length()){
                    DOptionsEditText[DOptionsEditSelected] = DOptionsEditText[DOptionsEditSelected].substr(1);
                }
            }
            else if(GDK_KEY_Left == Key){
                if(DOptionsEditSelectedCharacter){
                   DOptionsEditSelectedCharacter--; 
                }
            }
            else if(GDK_KEY_Right == Key){
                if(DOptionsEditSelectedCharacter < DOptionsEditText[DOptionsEditSelected].length()){
                   DOptionsEditSelectedCharacter++; 
                }
            }
            else if(((GDK_KEY_0 <= Key)&&(GDK_KEY_9 >= Key))||((GDK_KEY_A <= Key)&&(GDK_KEY_Z >= Key))||((GDK_KEY_a <= Key)&&(GDK_KEY_z >= Key))||(GDK_KEY_period == Key)){
                DOptionsEditText[DOptionsEditSelected] = DOptionsEditText[DOptionsEditSelected].substr(0,DOptionsEditSelectedCharacter) + std::string(1, (char)Key) + DOptionsEditText[DOptionsEditSelected].substr(DOptionsEditSelectedCharacter,DOptionsEditText[DOptionsEditSelected].length()-DOptionsEditSelectedCharacter);
                DOptionsEditSelectedCharacter++;
            }
        }
    }
    DReleasedKeys[pcNone].clear();
    DReleasedKeys[DPlayerColor].clear();
}                   

void CApplicationData::CalculateEditOptionsMode(){

}

void CApplicationData::RenderEditOptionsMode(){
    gint BufferWidth, BufferHeight, BufferCenter;
    gint TitleHeight, OptionSkip, OptionTop, TextOffsetY;
    int ButtonLeft, ButtonSkip, ButtonTop;
    bool ButtonXAlign = false, ButtonHovered = false;
    bool FirstButton = true, AllInputsValid = true;
    int GoldColor, WhiteColor, ShadowColor;
    
    RenderMenuTitle(DCurrentPageTitle, TitleHeight, BufferWidth, BufferHeight); 
    
    GoldColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("gold");
    WhiteColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("white");
    ShadowColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("black");
    
    DCurrentPageButtonLocations.clear();
    for(auto Text : DCurrentPageButtonsText){
        DCurrentPageButtonRenderer->Text(Text, FirstButton);
        FirstButton = false;
    }
    for(int Index = 0; Index < DOptionsEditText.size(); Index++){
        if(!DOptionsEditValidationFunctions[Index](DOptionsEditText[Index])){
            AllInputsValid = false;
            break;
        }
    }
    
    DCurrentPageButtonRenderer->Width(DCurrentPageButtonRenderer->Width() * 3 / 2);
    DCurrentPageButtonRenderer->Height(DCurrentPageButtonRenderer->Height() * 3 / 2);                            
    ButtonLeft = BufferWidth - DBorderWidth - DCurrentPageButtonRenderer->Width();
    ButtonSkip = DCurrentPageButtonRenderer->Height() * 3 / 2;
    ButtonTop = BufferHeight - DBorderWidth - (DCurrentPageButtonsText.size() * ButtonSkip - DCurrentPageButtonRenderer->Height()/2);
    
    if((ButtonLeft <= DCurrentX[pcNone])&&((ButtonLeft + DCurrentPageButtonRenderer->Width() > DCurrentX[pcNone]))){
        ButtonXAlign = true;       
    }
    FirstButton = true;
    for(auto Text : DCurrentPageButtonsText){
        CButtonRenderer::EButtonState ButtonState = CButtonRenderer::bsNone;
        
        DCurrentPageButtonRenderer->Text(Text);
        if(ButtonXAlign){
            if((ButtonTop <= DCurrentY[pcNone])&&((ButtonTop + DCurrentPageButtonRenderer->Height() > DCurrentY[pcNone]))){
                ButtonState = DLeftDown[pcNone] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
                ButtonHovered = true;
            }
        }
        if(FirstButton && !AllInputsValid){
            if(CButtonRenderer::bsNone != ButtonState){
                ButtonHovered = false;   
            }
            ButtonState = CButtonRenderer::bsInactive;
        }    
        FirstButton = false;
        DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, ButtonLeft, ButtonTop, ButtonState);
        if(CButtonRenderer::bsInactive != ButtonState){
            DCurrentPageButtonLocations.push_back( SRectangle({ButtonLeft, ButtonTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()}));
        }
        else{
            DCurrentPageButtonLocations.push_back( SRectangle({0, 0, 0, 0}));
        }
        ButtonTop += ButtonSkip;
    }
    
    DOptionsEditLocations.clear();
    BufferCenter = BufferWidth/2;
    OptionSkip = DOptionsEditRenderer->Height() * 3 / 2;
    OptionTop = (BufferHeight + TitleHeight) / 2 - (OptionSkip * DOptionsEditTitles.size()) / 2;
    for(int Index = 0; Index < DOptionsEditTitles.size(); Index++){
        std::string TempString;
        gint TextWidth, TextHeight;
        TempString = DOptionsEditTitles[Index];
        
        DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
        TextOffsetY = DOptionsEditRenderer->Height()/2 - TextHeight/2;
        DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, BufferCenter - TextWidth, OptionTop + TextOffsetY, WhiteColor, ShadowColor, 1, TempString);
        
        DOptionsEditRenderer->Text(DOptionsEditText[Index], DOptionsEditValidationFunctions[Index](DOptionsEditText[Index]));
        DOptionsEditRenderer->DrawEdit(DWorkingBufferPixmap, BufferCenter, OptionTop, Index == DOptionsEditSelected ? DOptionsEditSelectedCharacter : -1);
        DOptionsEditLocations.push_back(SRectangle({BufferCenter, OptionTop, DOptionsEditRenderer->Width(), DOptionsEditRenderer->Height()}));
        OptionTop += OptionSkip;
    }
    
    if(!DCurrentPageButtonHovered && ButtonHovered){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("tick"), DSoundVolume, 0.0);
    }
    if(DNextGameMode != DGameMode){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("place"), DSoundVolume, 0.0);
    }
    DCurrentPageButtonHovered = ButtonHovered;
}

void CApplicationData::ProcessInputMapSelectMode(){
    if(DLeftClick[pcNone] && !DLeftDown[pcNone]){
        int ItemSelected = DMapSelectListViewRenderer->ItemAt(DCurrentX[pcNone] - DMapSelectListViewXOffset, DCurrentY[pcNone] - DMapSelectListViewYOffset);
        
        if(CListViewRenderer::lvoUpArrow == ItemSelected){
            if(DSelectedMapOffset){
                DSelectedMapOffset--;    
            }
        }
        else if(CListViewRenderer::lvoDownArrow == ItemSelected){
            DSelectedMapOffset++;
        }
        else if(CListViewRenderer::lvoNone != ItemSelected){
            if(DSelectedMapIndex != ItemSelected){
                DSelectedMapIndex = ItemSelected;
                
                *DSelectedMap = *CAssetDecoratedMap::GetMap(DSelectedMapIndex);
            }
        }
        else{
            for(int Index = 0; Index < DCurrentPageButtonLocations.size(); Index++){
                if((DCurrentPageButtonLocations[Index].DXPosition <= DCurrentX[pcNone])&&((DCurrentPageButtonLocations[Index].DXPosition + DCurrentPageButtonLocations[Index].DWidth > DCurrentX[pcNone]))){
                    if((DCurrentPageButtonLocations[Index].DYPosition <= DCurrentY[pcNone])&&((DCurrentPageButtonLocations[Index].DYPosition + DCurrentPageButtonLocations[Index].DHeight > DCurrentY[pcNone]))){
                        DCurrentPageButtonsFunctions[Index](this);
                    }
                }
            }
        }
    }
}

void CApplicationData::CalculateMapSelectMode(){

}

void CApplicationData::RenderMapSelectMode(){
    gint BufferWidth, BufferHeight;
    gint MiniMapWidth, MiniMapHeight, MiniMapCenter, MiniMapLeft;
    gint ListViewWidth = 0, ListViewHeight = 0;
    gint TitleHeight;
    gint TextWidth, TextHeight, TextTop;
    int TextColor, ShadowColor;
    std::string PageTitle = "Select Map", TempString;
    CButtonRenderer::EButtonState ButtonState = CButtonRenderer::bsNone;
    bool ButtonXAlign = false, ButtonHovered = false;
    
    RenderMenuTitle(PageTitle, TitleHeight, BufferWidth, BufferHeight); 

    gdk_pixmap_get_size(DMiniMapPixmap, &MiniMapWidth, &MiniMapHeight); 
    if(nullptr != DMapSelectListViewPixmap){
        gdk_pixmap_get_size(DMapSelectListViewPixmap, &ListViewWidth, &ListViewHeight); 
    }

    if((ListViewHeight != (BufferHeight - TitleHeight - DInnerBevel->Width() - DBorderWidth))||(ListViewWidth != (BufferWidth - DViewportXOffset - DBorderWidth - DInnerBevel->Width() * 2))){
        if(nullptr != DMapSelectListViewPixmap){
            g_object_unref(DMapSelectListViewPixmap);
        }
        ListViewHeight = BufferHeight - TitleHeight - DInnerBevel->Width() - DBorderWidth;
        ListViewWidth = BufferWidth - DViewportXOffset - DBorderWidth - DInnerBevel->Width() * 2;
        DMapSelectListViewPixmap = gdk_pixmap_new(DDrawingArea->window, ListViewWidth, ListViewHeight, -1);
    }
    
    DMapSelectListViewXOffset = DBorderWidth;
    DMapSelectListViewYOffset = TitleHeight + DInnerBevel->Width();
    
    gdk_draw_pixmap(DMapSelectListViewPixmap, DDrawingContext, DWorkingBufferPixmap, DMapSelectListViewXOffset, DMapSelectListViewYOffset, 0, 0, ListViewWidth, ListViewHeight);
    
    std::vector< std::string > MapNames;       
    while(CAssetDecoratedMap::GetMap(MapNames.size())){
        MapNames.push_back(CAssetDecoratedMap::GetMap(MapNames.size())->MapName());
    }

    
    DMapSelectListViewRenderer->DrawListView(DMapSelectListViewPixmap, DSelectedMapIndex, DSelectedMapOffset, MapNames);
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DMapSelectListViewPixmap, 0, 0, DMapSelectListViewXOffset, DMapSelectListViewYOffset, ListViewWidth, ListViewHeight);
    DInnerBevel->DrawBevel(DWorkingBufferPixmap, DMapSelectListViewXOffset, DMapSelectListViewYOffset, ListViewWidth, ListViewHeight);
    
    DMiniMapRenderer->DrawMiniMap(DMiniMapPixmap);
    MiniMapLeft = DMapSelectListViewXOffset + ListViewWidth + DInnerBevel->Width() * 4;
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DMiniMapPixmap, 0, 0, MiniMapLeft, DMapSelectListViewYOffset, -1, -1);
    DInnerBevel->DrawBevel(DWorkingBufferPixmap, MiniMapLeft, DMapSelectListViewYOffset, MiniMapWidth, MiniMapHeight);
    
    TextTop = DMapSelectListViewYOffset + MiniMapHeight + DInnerBevel->Width() * 2;
    MiniMapCenter = MiniMapLeft + MiniMapWidth / 2;
    TextColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("white");
    ShadowColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("black");
    
    TempString = std::to_string(DSelectedMap->PlayerCount()) + " Players";
    DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
    DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, MiniMapCenter - TextWidth/2, TextTop, TextColor, ShadowColor, 1, TempString);
    TextTop += TextHeight;
    TempString = std::to_string(DSelectedMap->Width()) + " x " + std::to_string(DSelectedMap->Height());
    DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
    DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, MiniMapCenter - TextWidth/2, TextTop, TextColor, ShadowColor, 1, TempString);
    TextTop += TextHeight * 3 / 2;
    DCurrentPageButtonLocations.resize(2);
    
    if((MiniMapLeft <= DCurrentX[pcNone])&&(MiniMapLeft + MiniMapWidth > DCurrentX[pcNone])){
        ButtonXAlign = true;
    }
    
    DCurrentPageButtonsFunctions.clear();
    
    TempString = "Select";
    DCurrentPageButtonRenderer->Text(TempString, true);   
    DCurrentPageButtonRenderer->Height(DCurrentPageButtonRenderer->Height() * 3 / 2);
    DCurrentPageButtonRenderer->Width(MiniMapWidth);
    
    TextTop = BufferHeight - DBorderWidth - (DCurrentPageButtonRenderer->Height() * 9 / 4);
    if(ButtonXAlign){
        if((TextTop <= DCurrentY[pcNone])&&((TextTop + DCurrentPageButtonRenderer->Height() > DCurrentY[pcNone]))){
            ButtonState = DLeftDown[pcNone] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
            ButtonHovered = true;
        }
    }
    DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, MiniMapLeft, TextTop, ButtonState);
    DCurrentPageButtonLocations[0] = SRectangle({MiniMapLeft, TextTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()});
    DCurrentPageButtonsFunctions.push_back(SelectMapButtonCallback);
    
    TextTop = BufferHeight - DBorderWidth - DCurrentPageButtonRenderer->Height();
    ButtonState = CButtonRenderer::bsNone;
    if(ButtonXAlign){
        if((TextTop <= DCurrentY[pcNone])&&((TextTop + DCurrentPageButtonRenderer->Height() > DCurrentY[pcNone]))){
            ButtonState = DLeftDown[pcNone] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
            ButtonHovered = true;
        }
    }
    TempString = "Cancel";
    DCurrentPageButtonRenderer->Text(TempString);   
    DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, MiniMapLeft, TextTop, ButtonState);
    DCurrentPageButtonLocations[1] = SRectangle({MiniMapLeft, TextTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()});
    DCurrentPageButtonsFunctions.push_back(MainMenuButtonCallback);
    
    if(!DCurrentPageButtonHovered && ButtonHovered){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("tick"), DSoundVolume, 0.0);
    }
    if(DNextGameMode != DGameMode){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("place"), DSoundVolume, 0.0);
    }
    DCurrentPageButtonHovered = ButtonHovered;

}

void CApplicationData::ProcessInputPlayerAISelectMode(){
    DPlayerColorRequestingChange = pcNone;
    DPlayerColorChangeRequest = pcNone;
    DPlayerColorRequesTypeChange = pcNone;
    if(DLeftClick[pcNone] && !DLeftDown[pcNone]){
        for(int Index = 0; Index < DColorButtonLocations.size(); Index++){
            if((DColorButtonLocations[Index].DXPosition <= DCurrentX[pcNone])&&((DColorButtonLocations[Index].DXPosition + DColorButtonLocations[Index].DWidth > DCurrentX[pcNone]))){
                if((DColorButtonLocations[Index].DYPosition <= DCurrentY[pcNone])&&((DColorButtonLocations[Index].DYPosition + DColorButtonLocations[Index].DHeight > DCurrentY[pcNone]))){
                    int PlayerSelecting = 1 + (Index / (pcMax - 1));
                    int ColorSelecting = 1 + (Index % (pcMax - 1));
                    if((PlayerSelecting == DPlayerColor)||(gstMultiPlayerClient != DGameSessionType)){
                        if((PlayerSelecting == DPlayerColor)||(ptHuman != DLoadingPlayerTypes[PlayerSelecting])){
                            DPlayerColorRequestingChange = (EPlayerColor)PlayerSelecting;
                            DPlayerColorChangeRequest = (EPlayerColor)ColorSelecting;
                        }
                    }
                }
            } 
        }
        for(int Index = 0; Index < DCurrentPageButtonLocations.size(); Index++){
            if((DCurrentPageButtonLocations[Index].DXPosition <= DCurrentX[pcNone])&&((DCurrentPageButtonLocations[Index].DXPosition + DCurrentPageButtonLocations[Index].DWidth > DCurrentX[pcNone]))){
                if((DCurrentPageButtonLocations[Index].DYPosition <= DCurrentY[pcNone])&&((DCurrentPageButtonLocations[Index].DYPosition + DCurrentPageButtonLocations[Index].DHeight > DCurrentY[pcNone]))){
                    DCurrentPageButtonsFunctions[Index](this);
                }
            }
        }
        for(int Index = 0; Index < DPlayerTypeButtonLocations.size(); Index++){
            if((DPlayerTypeButtonLocations[Index].DXPosition <= DCurrentX[pcNone])&&((DPlayerTypeButtonLocations[Index].DXPosition + DPlayerTypeButtonLocations[Index].DWidth > DCurrentX[pcNone]))){
                if((DPlayerTypeButtonLocations[Index].DYPosition <= DCurrentY[pcNone])&&((DPlayerTypeButtonLocations[Index].DYPosition + DPlayerTypeButtonLocations[Index].DHeight > DCurrentY[pcNone]))){
                    DPlayerColorRequesTypeChange = (EPlayerColor)(Index + 2);
                    break;
                }
            }
            
        }
    }
}

void CApplicationData::CalculatePlayerAISelectMode(){
    if(pcNone != DPlayerColorRequestingChange){
        EPlayerColor NewColorInUse = pcNone;
        for(int Index = 1; Index < pcMax; Index++){
            if(Index != DPlayerColorRequestingChange){
                if(ptNone != DLoadingPlayerTypes[Index]){
                    if(DLoadingPlayerColors[Index] == DPlayerColorChangeRequest){
                        NewColorInUse = (EPlayerColor)Index;
                        break;
                    }
                }
            }
        }
        if(pcNone != NewColorInUse){
            DLoadingPlayerColors[NewColorInUse] = DLoadingPlayerColors[DPlayerColorRequestingChange];
        }
        DLoadingPlayerColors[DPlayerColorRequestingChange] = DPlayerColorChangeRequest;
                                    
        *DSelectedMap = *CAssetDecoratedMap::DuplicateMap(DSelectedMapIndex,DLoadingPlayerColors);
    }
    if(pcNone != DPlayerColorRequesTypeChange){
        if(gstSinglePlayer == DGameSessionType){
            switch(DLoadingPlayerTypes[DPlayerColorRequesTypeChange]){
                case ptAIEasy:      DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptAIMedium;
                                    break;
                case ptAIMedium:    DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptAIHard;
                                    break;
                default:            DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptAIEasy;
                                    break;
            }
        }
        else if(gstMultiPlayerHost == DGameSessionType){
            switch(DLoadingPlayerTypes[DPlayerColorRequesTypeChange]){
                case ptHuman:       DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptAIEasy;
                                    break;
                case ptAIEasy:      DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptAIMedium;
                                    break;
                case ptAIMedium:    DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptAIHard;
                                    break;
                case ptAIHard:      DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptNone;
                                    break;
                default:            DLoadingPlayerTypes[DPlayerColorRequesTypeChange] = ptHuman;
                                    break;
            }
        }
    }
}

void CApplicationData::RenderPlayerAISelectMode(){
    gint BufferWidth, BufferHeight;
    gint TitleHeight;
    gint TextWidth, TextHeight, MaxTextWidth;
    gint ColumnWidth, RowHeight;
    gint MiniMapWidth, MiniMapHeight, MiniMapCenter, MiniMapLeft;
    gint TextTop, ButtonLeft, ButtonTop, AIButtonLeft, ColorButtonHeight;
    int GoldColor, WhiteColor, ShadowColor;
    std::string TempString;
    CButtonRenderer::EButtonState ButtonState = CButtonRenderer::bsNone;
    bool ButtonXAlign = false, ButtonHovered = false;
    
    RenderMenuTitle("Select Colors/Difficulty", TitleHeight, BufferWidth, BufferHeight); 
    
    GoldColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("gold");
    WhiteColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("white");
    ShadowColor = DFonts[CUnitDescriptionRenderer::fsLarge]->FindPixel("black");
    
    gdk_pixmap_get_size(DMiniMapPixmap, &MiniMapWidth, &MiniMapHeight); 
    
    DMiniMapRenderer->DrawMiniMap(DMiniMapPixmap);
    MiniMapLeft = BufferWidth - MiniMapWidth - DBorderWidth;
    gdk_draw_pixmap(DWorkingBufferPixmap, DDrawingContext, DMiniMapPixmap, 0, 0, MiniMapLeft, TitleHeight + DInnerBevel->Width(), -1, -1);
    DInnerBevel->DrawBevel(DWorkingBufferPixmap, MiniMapLeft, TitleHeight + DInnerBevel->Width(), MiniMapWidth, MiniMapHeight);
    
    TextTop = TitleHeight + MiniMapHeight + DInnerBevel->Width() * 3;
    MiniMapCenter = MiniMapLeft + MiniMapWidth / 2;
    
    TempString = std::to_string(DSelectedMap->PlayerCount()) + " Players";
    DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
    DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, MiniMapCenter - TextWidth/2, TextTop, WhiteColor, ShadowColor, 1, TempString);
    TextTop += TextHeight;
    TempString = std::to_string(DSelectedMap->Width()) + " x " + std::to_string(DSelectedMap->Height());
    DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
    DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, MiniMapCenter - TextWidth/2, TextTop, WhiteColor, ShadowColor, 1, TempString);

    
    TextTop = TitleHeight;
    TempString = "Player";
    DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
    DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, DBorderWidth, TextTop, WhiteColor, ShadowColor, 1, TempString);
    TextTop += TextHeight;
    
    DCurrentPageButtonRenderer->Text("AI Easy", true);
    ColorButtonHeight = DCurrentPageButtonRenderer->Height();
    RowHeight = DCurrentPageButtonRenderer->Height() + DInnerBevel->Width() * 2;
    if(RowHeight < TextHeight){
        RowHeight = TextHeight;
    }
    DCurrentPageButtonRenderer->Text("X", true);
    DCurrentPageButtonRenderer->Height(ColorButtonHeight);
    ColumnWidth = DCurrentPageButtonRenderer->Width() + DInnerBevel->Width() * 2;
    MaxTextWidth = 0;
    for(int Index = 1; Index <= DSelectedMap->PlayerCount(); Index++){
        if(Index == DPlayerColor){
            DPlayerNames[Index] = TempString = std::to_string(Index) + ". You";   
        }
        else if(ptHuman != DLoadingPlayerTypes[Index]){
            DPlayerNames[Index] = TempString = std::to_string(Index) + ". Player " + std::to_string(Index);   
        }
        DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
        if(MaxTextWidth < TextWidth){
            MaxTextWidth = TextWidth;   
        }
    }
    TempString = "Color";
    DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
    DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, DBorderWidth + MaxTextWidth + (ColumnWidth * (pcMax + 1))/2 - TextWidth/2, TitleHeight, WhiteColor, ShadowColor, 1, TempString);
    DColorButtonLocations.clear();
    for(int Index = 1; Index <= DSelectedMap->PlayerCount(); Index++){
        TempString = DPlayerNames[Index];
        DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
        DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, DBorderWidth, TextTop, Index == DPlayerColor ? GoldColor : WhiteColor, ShadowColor, 1, TempString);
        for(int ColorIndex = 1; ColorIndex < pcMax; ColorIndex++){
            int ButtonLeft = DBorderWidth + MaxTextWidth + ColorIndex * ColumnWidth;
            DCurrentPageButtonRenderer->Text(DLoadingPlayerColors[Index] == ColorIndex ? "X" : "");
            DCurrentPageButtonRenderer->ButtonColor((EPlayerColor)ColorIndex);
            DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, ButtonLeft, TextTop, CButtonRenderer::bsNone);
            DColorButtonLocations.push_back(SRectangle({ButtonLeft, TextTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()}));
            AIButtonLeft = ButtonLeft + ColumnWidth;
        }
        TextTop += RowHeight;
    }
    
    
    DCurrentPageButtonRenderer->ButtonColor(pcNone);
    TempString = "AI Easy";
    DCurrentPageButtonRenderer->Text(TempString);   
    DCurrentPageButtonRenderer->Width(DCurrentPageButtonRenderer->Width() * 3 / 2);
    
    TextTop = TitleHeight;
    TempString = "Difficulty";
    DFonts[CUnitDescriptionRenderer::fsLarge]->MeasureText(TempString, TextWidth, TextHeight);
    DFonts[CUnitDescriptionRenderer::fsLarge]->DrawTextWithShadow(DWorkingBufferPixmap, DDrawingContext, AIButtonLeft + (DCurrentPageButtonRenderer->Width() - TextWidth)/2, TextTop, WhiteColor, ShadowColor, 1, TempString);
    
    ButtonXAlign = false;
    if((AIButtonLeft <= DCurrentX[pcNone])&&(AIButtonLeft + DCurrentPageButtonRenderer->Width() > DCurrentX[pcNone])){
        ButtonXAlign = true;
    }
    TextTop += RowHeight + TextHeight;
    DPlayerTypeButtonLocations.clear();
    for(int Index = 2; Index <= DSelectedMap->PlayerCount(); Index++){
        switch(DLoadingPlayerTypes[Index]){
            case ptHuman:       DCurrentPageButtonRenderer->Text("Human");
                                break;
            case ptAIEasy:      DCurrentPageButtonRenderer->Text("AI Easy");
                                break;
            case ptAIMedium:    DCurrentPageButtonRenderer->Text("AI Medium");
                                break;
            case ptAIHard:      DCurrentPageButtonRenderer->Text("AI Hard");
                                break;
            default:            DCurrentPageButtonRenderer->Text("Closed");
                                break;
        }
        ButtonState = CButtonRenderer::bsNone;
        if(ButtonXAlign){
            if((TextTop <= DCurrentY[pcNone])&&((TextTop + DCurrentPageButtonRenderer->Height() > DCurrentY[pcNone]))){
                ButtonState = DLeftDown[pcNone] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
                ButtonHovered = true;
            }
        }
        DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, AIButtonLeft, TextTop, ButtonState);
        DPlayerTypeButtonLocations.push_back(SRectangle({AIButtonLeft, TextTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()}));
                
        TextTop += RowHeight;
    }
    
    DCurrentPageButtonLocations.clear();
    DCurrentPageButtonsFunctions.clear();
    
    DCurrentPageButtonRenderer->ButtonColor(pcNone);
    TempString = "Play Game";
    DCurrentPageButtonRenderer->Text(TempString, true);   
    DCurrentPageButtonRenderer->Height(DCurrentPageButtonRenderer->Height() * 3 / 2);
    DCurrentPageButtonRenderer->Width(MiniMapWidth);
    ButtonLeft = BufferWidth - DCurrentPageButtonRenderer->Width() - DBorderWidth;
    ButtonTop = BufferHeight - (DCurrentPageButtonRenderer->Height() * 9 / 4) - DBorderWidth;
    ButtonState = CButtonRenderer::bsNone;
    if((ButtonLeft <= DCurrentX[pcNone])&&(ButtonLeft + DCurrentPageButtonRenderer->Width() > DCurrentX[pcNone])){
        ButtonXAlign = true;
    }
    if(ButtonXAlign){
        if((ButtonTop <= DCurrentY[pcNone])&&((ButtonTop + DCurrentPageButtonRenderer->Height() > DCurrentY[pcNone]))){
            ButtonState = DLeftDown[pcNone] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
            ButtonHovered = true;
        }
    }
    DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, ButtonLeft, ButtonTop, ButtonState);
    DCurrentPageButtonLocations.push_back(SRectangle({ButtonLeft, ButtonTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()}));
    DCurrentPageButtonsFunctions.push_back(PlayGameButtonCallback);
    
    ButtonTop = BufferHeight - DCurrentPageButtonRenderer->Height() - DBorderWidth;
    ButtonState = CButtonRenderer::bsNone;
    if(ButtonXAlign){
        if((ButtonTop <= DCurrentY[pcNone])&&((ButtonTop + DCurrentPageButtonRenderer->Height() > DCurrentY[pcNone]))){
            ButtonState = DLeftDown[pcNone] ? CButtonRenderer::bsPressed : CButtonRenderer::bsHover;
            ButtonHovered = true;
        }
    }
    TempString = "Cancel";
    DCurrentPageButtonRenderer->Text(TempString, false);   
    DCurrentPageButtonRenderer->DrawButton(DWorkingBufferPixmap, ButtonLeft, ButtonTop, ButtonState);
    DCurrentPageButtonLocations.push_back(SRectangle({ButtonLeft, ButtonTop, DCurrentPageButtonRenderer->Width(), DCurrentPageButtonRenderer->Height()}));
    DCurrentPageButtonsFunctions.push_back(MainMenuButtonCallback);
    
    if(!DCurrentPageButtonHovered && ButtonHovered){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("tick"), DSoundVolume, 0.0);
    }
    if(DNextGameMode != DGameMode){
        DSoundLibraryMixer->PlayClip(DSoundLibraryMixer->FindClip("place"), DSoundVolume, 0.0);
    }
    DCurrentPageButtonHovered = ButtonHovered;
}

void CApplicationData::ChangeMode(EGameMode nextmode){
    DNextGameMode = nextmode;
}

void CApplicationData::SwitchButtonMenuData(){
    bool Changed = false;
    if(gmMainMenu == DNextGameMode){
        DCurrentPageTitle = DMainMenuTitle;
        DCurrentPageButtonsText = DMainMenuButtonsText;
        DCurrentPageButtonsFunctions = DMainMenuButtonsFunctions;
        Changed = true;
    }
    else if(gmOptionsMenu == DNextGameMode){
        DCurrentPageTitle = DOptionsMenuTitle;
        DCurrentPageButtonsText = DOptionsMenuButtonsText;
        DCurrentPageButtonsFunctions = DOptionsMenuButtonsFunctions;
        Changed = true;
    }
    else if(gmMultiPlayerOptionsMenu == DNextGameMode){
        DCurrentPageTitle = DMultiPlayerOptionsMenuTitle;
        DCurrentPageButtonsText = DMultiPlayerOptionsMenuButtonsText;
        DCurrentPageButtonsFunctions = DMultiPlayerOptionsMenuButtonsFunctions;
        Changed = true;
    }
    if(Changed){
        DCurrentPageButtonLocations.resize(DCurrentPageButtonsText.size());
        for(auto &Location : DCurrentPageButtonLocations){
            Location.DXPosition = 0;
            Location.DYPosition = 0;
            Location.DWidth = 0;
            Location.DHeight = 0;
        }
    }
}

void CApplicationData::SinglePlayerGameButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);

    AppData->ResetPlayerColors();
    AppData->DGameSessionType = gstSinglePlayer;
    AppData->DCurrentPageButtonHovered = false;
    AppData->DSelectedMapOffset = 0;
    AppData->DSelectedMapIndex = 0;                                        
    AppData->DSelectedMap = CAssetDecoratedMap::DuplicateMap(0,AppData->DLoadingPlayerColors);
    AppData->DMapRenderer = std::make_shared< CMapRenderer >(AppData->DTerrainTileset, AppData->DSelectedMap); 
    AppData->DAssetRenderer = std::make_shared< CAssetRenderer >( AppData->DAssetTilesets, AppData->DMarkerTileset, AppData->DCorpseTileset, AppData->DFireTilesets, AppData->DBuildingDeathTileset, AppData->DArrowTileset, nullptr, AppData->DSelectedMap);
    AppData->DMiniMapRenderer = std::make_shared< CMiniMapRenderer >(AppData->DMapRenderer, AppData->DAssetRenderer, nullptr, nullptr, gdk_drawable_get_depth(AppData->DDrawingArea->window) );
    
    AppData->ChangeMode(gmMapSelect);
}
                                                                        
void CApplicationData::MultiPlayerGameButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
    
    AppData->ChangeMode(gmMultiPlayerOptionsMenu);
}

void CApplicationData::OptionsButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
                  
    AppData->ChangeMode(gmOptionsMenu);
}

void CApplicationData::ExitGameButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
    
    AppData->DDeleted = TRUE;
    gtk_main_quit();
}

void CApplicationData::MainMenuButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
    
    AppData->ChangeMode(gmMainMenu);
}

void CApplicationData::SoundOptionsButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
    
    AppData->DCurrentPageTitle = "Sound Options";
    AppData->DCurrentPageButtonsText.clear();
    AppData->DCurrentPageButtonsFunctions.clear();
    AppData->DCurrentPageButtonLocations.clear();
    
    AppData->DCurrentPageButtonsText.push_back("OK");
    AppData->DCurrentPageButtonsFunctions.push_back(SoundOptionsUpdateButtonCallback);
    AppData->DCurrentPageButtonsText.push_back("Cancel");
    AppData->DCurrentPageButtonsFunctions.push_back(OptionsButtonCallback);
    
    AppData->DOptionsEditSelected = -1;
    AppData->DOptionsEditSelectedCharacter = -1;
    AppData->DOptionsEditLocations.clear();
    AppData->DOptionsEditTitles.clear();
    AppData->DOptionsEditText.clear();
    AppData->DOptionsEditValidationFunctions.clear();
    
    AppData->DOptionsEditTitles.push_back("FX Volume:");
    AppData->DOptionsEditText.push_back(std::to_string((int)(AppData->DSoundVolume * 100)));
    AppData->DOptionsEditValidationFunctions.push_back(ValidSoundLevelCallback);
    AppData->DOptionsEditTitles.push_back("Music Volume:");
    AppData->DOptionsEditText.push_back(std::to_string((int)(AppData->DMusicVolume * 100)));
    AppData->DOptionsEditValidationFunctions.push_back(ValidSoundLevelCallback);
    
    AppData->ChangeMode(gmSoundOptions);
}

void CApplicationData::SoundOptionsUpdateButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
    
    for(int Index = 0; Index < AppData->DOptionsEditText.size(); Index++){
        if(!AppData->DOptionsEditValidationFunctions[Index](AppData->DOptionsEditText[Index])){
            return;   
        }
    }
    AppData->DSoundVolume = std::stod(AppData->DOptionsEditText[0]) / 100.0;
    AppData->DMusicVolume = std::stod(AppData->DOptionsEditText[1]) / 100.0;
    AppData->DSoundLibraryMixer->PlaySong(AppData->DSoundLibraryMixer->FindSong("menu"), AppData->DMusicVolume);   
    AppData->ChangeMode(gmOptionsMenu);
}

void CApplicationData::NetworkOptionsButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);

    AppData->DCurrentPageTitle = "Network Options";
    AppData->DCurrentPageButtonsText.clear();
    AppData->DCurrentPageButtonsFunctions.clear();
    AppData->DCurrentPageButtonLocations.clear();
    
    AppData->DCurrentPageButtonsText.push_back("OK");
    AppData->DCurrentPageButtonsFunctions.push_back(MainMenuButtonCallback);
    AppData->DCurrentPageButtonsText.push_back("Cancel");
    AppData->DCurrentPageButtonsFunctions.push_back(OptionsButtonCallback);
    
    AppData->DOptionsEditSelected = -1;
    AppData->DOptionsEditSelectedCharacter = -1;
    AppData->DOptionsEditLocations.clear();
    AppData->DOptionsEditTitles.clear();
    AppData->DOptionsEditText.clear();
    AppData->DOptionsEditValidationFunctions.clear();
    
    
    AppData->DOptionsEditTitles.push_back("User Name:");
    AppData->DOptionsEditText.push_back(AppData->DUsername);
    AppData->DOptionsEditValidationFunctions.push_back(ValidHostnameCallback);
    AppData->DOptionsEditTitles.push_back("Remote Hostname:");
    AppData->DOptionsEditText.push_back(AppData->DRemoteHostname);
    AppData->DOptionsEditValidationFunctions.push_back(ValidHostnameCallback);
    AppData->DOptionsEditTitles.push_back("Port Number:");
    AppData->DOptionsEditText.push_back(std::to_string(AppData->DMultiplayerPort));
    AppData->DOptionsEditValidationFunctions.push_back(ValidPortNumberCallback);

    AppData->ChangeMode(gmNetworkOptions);
}

void CApplicationData::HostMultiPlayerButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
    
    AppData->ResetPlayerColors();
    AppData->DGameSessionType = gstMultiPlayerHost;
    AppData->DCurrentPageButtonHovered = false;
    AppData->DSelectedMapOffset = 0;
    AppData->DSelectedMapIndex = 0;
    AppData->DSelectedMap = CAssetDecoratedMap::DuplicateMap(0,AppData->DLoadingPlayerColors);
    AppData->DMapRenderer = std::make_shared< CMapRenderer >(AppData->DTerrainTileset, AppData->DSelectedMap); 
    AppData->DAssetRenderer = std::make_shared< CAssetRenderer >( AppData->DAssetTilesets, AppData->DMarkerTileset, AppData->DCorpseTileset, AppData->DFireTilesets, AppData->DBuildingDeathTileset, AppData->DArrowTileset, nullptr, AppData->DSelectedMap);
    AppData->DMiniMapRenderer = std::make_shared< CMiniMapRenderer >(AppData->DMapRenderer, AppData->DAssetRenderer, nullptr, nullptr, gdk_drawable_get_depth(AppData->DDrawingArea->window) );
    AppData->ChangeMode(gmMapSelect);
}

void CApplicationData::JoinMultiPlayerButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
 
    AppData->ChangeMode(gmHostSelect);
}

void CApplicationData::SelectMapButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
 
    for(int Index = 0; Index < pcMax; Index++){
        AppData->DLoadingPlayerTypes[Index] = ptNone;
        if(Index){
            if(1 == Index){
                AppData->DLoadingPlayerTypes[Index] = ptHuman;
            }
            else if(Index <= AppData->DSelectedMap->PlayerCount()){
                AppData->DLoadingPlayerTypes[Index] = gstMultiPlayerHost == AppData->DGameSessionType ?  ptHuman : ptAIEasy;
            }
        }
    }
    AppData->DPlayerColor = (EPlayerColor)1;
    AppData->ChangeMode(gmPlayerAISelect);
}

void CApplicationData::PlayGameButtonCallback(void *calldata){
    CApplicationData *AppData = static_cast<CApplicationData *>(calldata);
    
    AppData->DPlayerColor = AppData->DLoadingPlayerColors[AppData->DPlayerColor];
    AppData->LoadGameMap(AppData->DSelectedMapIndex);
    AppData->ChangeMode(gmBattle);
    AppData->DSoundLibraryMixer->PlaySong(AppData->DSoundLibraryMixer->FindSong("game1"), AppData->DMusicVolume);
}

bool CApplicationData::ValidSoundLevelCallback(const std::string &text){
    try{
        int Level = std::stoi(text);
        if((0 <= Level)&&(100 >= Level)){
            return text == std::to_string(Level);
        }
    }
    catch(std::exception &E){
        return false;
    }
    return false;
}

bool CApplicationData::ValidHostnameCallback(const std::string &text){
    int CharSinceDot = 0;

    if(253 < text.length()){
        return false;    
    }
    if(0 == text.length()){
        return false;
    }
    for(auto Char : text){
        if('.' == Char){
            if(0 == CharSinceDot){
                return false;
            }
            CharSinceDot = 0;
        }
        else{
            CharSinceDot++;
            if(63 < CharSinceDot){
                return false;   
            }
            if(('-' != Char)&&(!(('0' <= Char)&&('9' >= Char)))&&(!(('a' <= Char)&&('z' >= Char)))&&(!(('A' <= Char)&&('Z' >= Char)))){
                return false;
            }
        }
    }
    return true;
}

bool CApplicationData::ValidPortNumberCallback(const std::string &text){
    try{
        int Port = std::stoi(text);
        if((1024 < Port)&&(65535 >= Port)){
            return text == std::to_string(Port);
        }
    }
    catch(std::exception &E){
        return false;
    }
    return false;
}
          
void CApplicationData::ResetPlayerColors(){
    for(int Index = 0; Index < pcMax; Index++){
        DLoadingPlayerColors[Index] = (EPlayerColor)Index;
    }
}

void CApplicationData::ResetMap(){
    DNextGameMode = gmBattle;
}

void CApplicationData::LoadGameMap(int index){
    gint DetailedMapWidth, DetailedMapHeight;
    
    DGameModel = std::make_shared< CGameModel >(index, 0x123456789ABCDEFULL, DLoadingPlayerColors);
    
    for(int Index = 1; Index < pcMax; Index++){
        DGameModel->Player(DPlayerColor)->IsAI((ptAIEasy <= DLoadingPlayerTypes[Index])&&(ptAIHard >= DLoadingPlayerTypes[Index]));        
    }
    for(int Index = 1; Index < pcMax; Index++){
        if(DGameModel->Player((EPlayerColor)Index)->IsAI()){
            int Downsample = 1;
            switch(DLoadingPlayerTypes[Index]){
                case ptAIEasy:      Downsample = CPlayerAsset::UpdateFrequency();
                                    break;
                case ptAIMedium:    Downsample = CPlayerAsset::UpdateFrequency() / 2;
                                    break;
                default:            Downsample = CPlayerAsset::UpdateFrequency() / 4;
                                    break;
            }
            DAIPlayers[Index] = std::make_shared< CAIPlayer > (DGameModel->Player((EPlayerColor)Index), Downsample);
        }
        DCurrentAssetCapability[Index] = actNone;
    }

    DetailedMapWidth = DGameModel->Map()->Width() * DTerrainTileset->TileWidth();
    DetailedMapHeight = DGameModel->Map()->Height() * DTerrainTileset->TileHeight();

    ResetMap();

    DMapRenderer = std::make_shared< CMapRenderer >(DTerrainTileset, DGameModel->Player(DPlayerColor)->PlayerMap()); 
    DAssetRenderer = std::make_shared< CAssetRenderer >( DAssetTilesets, DMarkerTileset, DCorpseTileset, DFireTilesets, DBuildingDeathTileset, DArrowTileset, DGameModel->Player(DPlayerColor), DGameModel->Player(DPlayerColor)->PlayerMap() );
    DFogRenderer = std::make_shared< CFogRenderer >(DFogTileset , DGameModel->Player(DPlayerColor)->VisibilityMap());
    DViewportRenderer = std::make_shared< CViewportRenderer>(DMapRenderer, DAssetRenderer, DFogRenderer);
    DMiniMapRenderer = std::make_shared< CMiniMapRenderer >(DMapRenderer, DAssetRenderer, DFogRenderer, DViewportRenderer, gdk_drawable_get_depth(DDrawingArea->window) );
    DUnitDescriptionRenderer = std::make_shared< CUnitDescriptionRenderer >(DMiniBevel, DIconTileset, DFonts, DPlayerColor);
    DUnitActionRenderer = std::make_shared< CUnitActionRenderer >(DMiniBevel, DIconTileset, DPlayerColor, DGameModel->Player(DPlayerColor));
    DResourceRenderer = std::make_shared< CResourceRenderer > (DMiniIconTileset, DFonts[CUnitDescriptionRenderer::fsMedium], DGameModel->Player(DPlayerColor));
    DSoundEventRenderer = std::make_shared< CSoundEventRenderer > (DSoundLibraryMixer, DGameModel->Player(DPlayerColor));
    DMenuButtonRenderer = std::make_shared< CButtonRenderer > (DButtonColorTileset, DInnerBevel, DOuterBevel, DFonts[CUnitDescriptionRenderer::fsMedium]);
    
    DMenuButtonRenderer->Text("Menu");
    DMenuButtonRenderer->ButtonColor(DPlayerColor);
    
    GdkGeometry Geometry; 
    gint LeftPanelWidth = MAX(DUnitDescriptionRenderer->MinimumWidth(), DUnitActionRenderer->MinimumWidth()) + DOuterBevel->Width() * 4;
    LeftPanelWidth = MAX(LeftPanelWidth, MINI_MAP_MIN_WIDTH + DInnerBevel->Width() * 4);
    gint MinUnitDescrHeight;
    
    DMiniMapXOffset = DInnerBevel->Width() * 2;
    DUnitDescriptionXOffset = DOuterBevel->Width() * 2;
    DUnitActionXOffset = DUnitDescriptionXOffset;
    DViewportXOffset = LeftPanelWidth + DInnerBevel->Width();
    
    
    DMiniMapYOffset = DBorderWidth;
    DUnitDescriptionYOffset = DMiniMapYOffset + (LeftPanelWidth - DInnerBevel->Width() * 3) + DOuterBevel->Width() * 2;
    MinUnitDescrHeight = DUnitDescriptionRenderer->MinimumHeight(LeftPanelWidth - DOuterBevel->Width() * 4, 9);
    DUnitActionYOffset = DUnitDescriptionYOffset + MinUnitDescrHeight + DOuterBevel->Width() * 3;
    DViewportYOffset = DBorderWidth;
    
    Geometry.min_width = INITIAL_MAP_WIDTH;
    Geometry.min_height = DUnitDescriptionYOffset + MinUnitDescrHeight + DUnitActionRenderer->MinimumHeight() + DOuterBevel->Width() * 5;
    Geometry.max_width = DViewportXOffset + DetailedMapWidth + DBorderWidth;
    Geometry.max_height = MAX(Geometry.min_height, DetailedMapHeight + DBorderWidth * 2);
    gtk_window_set_geometry_hints(GTK_WINDOW(DMainWindow), DDrawingArea, &Geometry, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
    
    ResizeCanvases();
    
    DMenuButtonRenderer->Width(DViewportXOffset/2);
    DMenuButtonXOffset = DViewportXOffset/2 - DMenuButtonRenderer->Width()/2;
    DMenuButtonYOffset = (DViewportYOffset - DOuterBevel->Width())/2 - DMenuButtonRenderer->Height()/2;
    
    gint CurWidth, CurHeight;
        
    gdk_pixmap_get_size(DViewportPixmap, &CurWidth, &CurHeight); 
    DViewportRenderer->InitViewportDimensions(CurWidth, CurHeight); 
    
    for(auto WeakAsset : DGameModel->Player(DPlayerColor)->Assets()){
        if(auto Asset = WeakAsset.lock()){
            DViewportRenderer->CenterViewport(Asset->Position());
            break;
        }
    }
    
}

void CApplicationData::ResizeCanvases(){
    GtkAllocation DrawingAreaAllocation;
    int ViewportWidth, ViewportHeight;
    int UserDescrWidth, UserDescrHeight;
    // Resize the canvas
    if(!DDrawingContext || !DDrawingArea){
        return;    
    }
    
    gtk_widget_get_allocation(DDrawingArea, &DrawingAreaAllocation);
    PrintDebug(DEBUG_LOW, "Resizing %d x %d\n",DrawingAreaAllocation.width, DrawingAreaAllocation.height);
    ViewportWidth = DrawingAreaAllocation.width - DViewportXOffset - DBorderWidth;
    ViewportHeight = DrawingAreaAllocation.height - DViewportYOffset - DBorderWidth;
    if(nullptr != DDoubleBufferPixmap){
        gint CurWidth, CurHeight;
        
        gdk_pixmap_get_size(DDoubleBufferPixmap, &CurWidth, &CurHeight); 
        if((DrawingAreaAllocation.width != CurWidth)||(DrawingAreaAllocation.height != CurHeight)){
            g_object_unref(DDoubleBufferPixmap);
            DDoubleBufferPixmap = nullptr;
        }
    }
    if(nullptr == DDoubleBufferPixmap){
        DDoubleBufferPixmap = gdk_pixmap_new(DDrawingArea->window, DrawingAreaAllocation.width, DrawingAreaAllocation.height, -1);
    }
    if(nullptr != DWorkingBufferPixmap){
        gint CurWidth, CurHeight;
        
        gdk_pixmap_get_size(DWorkingBufferPixmap, &CurWidth, &CurHeight); 
        if((DrawingAreaAllocation.width != CurWidth)||(DrawingAreaAllocation.height != CurHeight)){
            g_object_unref(DWorkingBufferPixmap);
            DWorkingBufferPixmap = nullptr;
        }
    }
    if(nullptr == DWorkingBufferPixmap){
        DWorkingBufferPixmap = gdk_pixmap_new(DDrawingArea->window, DrawingAreaAllocation.width, DrawingAreaAllocation.height, -1);
    }
    if(nullptr != DWorkingPixbuf){
        g_object_unref(DWorkingPixbuf);
        DWorkingPixbuf = nullptr;
    }
    if(nullptr == DMiniMapPixmap){
        gint Dimension = DViewportXOffset - DInnerBevel->Width() * 5;
        DMiniMapPixmap = gdk_pixmap_new(DDrawingArea->window, Dimension, Dimension, -1);   
    }
    if(nullptr != DMiniMapPixmap){
        GdkColor ColorBlack = {0,0,0,0};
        gint CurWidth, CurHeight;
        
        gdk_pixmap_get_size(DMiniMapPixmap, &CurWidth, &CurHeight);

        gdk_gc_set_rgb_fg_color(DDrawingContext, &ColorBlack);
        gdk_gc_set_rgb_bg_color(DDrawingContext, &ColorBlack);
        gdk_draw_rectangle(DMiniMapPixmap, DDrawingContext, true, 0, 0, CurWidth, CurHeight);
    }
    
    UserDescrWidth = DViewportXOffset - DInnerBevel->Width() - DOuterBevel->Width() * 4;
    UserDescrHeight = DUnitDescriptionRenderer->MinimumHeight(UserDescrWidth, 9);
    
    if(nullptr != DUnitDescriptionPixmap){
        gint CurWidth, CurHeight;
        
        gdk_pixmap_get_size(DUnitDescriptionPixmap, &CurWidth, &CurHeight);
        if((CurWidth != UserDescrWidth)||(CurHeight != UserDescrHeight)){
            g_object_unref(DUnitDescriptionPixmap);
            DUnitDescriptionPixmap = nullptr;
        }
    }
    if(nullptr == DUnitDescriptionPixmap){
        DUnitDescriptionPixmap = gdk_pixmap_new(DDrawingArea->window, UserDescrWidth, UserDescrHeight, -1);
    }
    DUnitActionYOffset = DUnitDescriptionYOffset + UserDescrHeight + DOuterBevel->Width() * 3;
    
    if(nullptr != DUnitActionPixmap){
        gint CurWidth, CurHeight;
        
        gdk_pixmap_get_size(DUnitActionPixmap, &CurWidth, &CurHeight);
        if((CurWidth != DUnitActionRenderer->MinimumWidth())||(CurHeight != DUnitActionRenderer->MinimumHeight())){
            g_object_unref(DUnitActionPixmap);
            DUnitActionPixmap = nullptr;
        }
    }
    if(nullptr == DUnitActionPixmap){
        DUnitActionPixmap = gdk_pixmap_new(DDrawingArea->window, DUnitActionRenderer->MinimumWidth(), DUnitActionRenderer->MinimumHeight(), -1);
    }
    if(nullptr != DResourcePixmap){
        gint CurWidth, CurHeight;
        
        gdk_pixmap_get_size(DResourcePixmap, &CurWidth, &CurHeight); 
        
        if((ViewportWidth != CurWidth)||(DBorderWidth != CurHeight)){
            g_object_unref(DResourcePixmap);
            DResourcePixmap = nullptr;
        }  
    }
    if(nullptr == DResourcePixmap){
        DResourcePixmap = gdk_pixmap_new(DDrawingArea->window, ViewportWidth, DBorderWidth, -1);
    }
    
    if(nullptr != DViewportPixmap){
        gint CurWidth, CurHeight;
        
        gdk_pixmap_get_size(DViewportPixmap, &CurWidth, &CurHeight); 
        
        if((ViewportWidth != CurWidth)||(ViewportHeight != CurHeight)){
            g_object_unref(DViewportPixmap);
            DViewportPixmap = nullptr;
            g_object_unref(DViewportTypePixmap);
            DViewportTypePixmap = nullptr;
        }
    }
    if(nullptr == DViewportPixmap){
        GdkColor ColorBlack = {0,0,0,0};
        
        if(0 > ViewportWidth){
            ViewportWidth = 1;   
        }
        if(0 > ViewportHeight){
            ViewportHeight = 1;   
        }
        DViewportPixmap = gdk_pixmap_new(DDrawingArea->window, ViewportWidth, ViewportHeight, -1);
        gdk_gc_set_rgb_fg_color(DDrawingContext, &ColorBlack);
        gdk_gc_set_rgb_bg_color(DDrawingContext, &ColorBlack);
        gdk_draw_rectangle(DViewportPixmap, DDrawingContext, true, 0, 0, ViewportWidth, ViewportHeight);
        DViewportTypePixmap = gdk_pixmap_new(DDrawingArea->window, ViewportWidth, ViewportHeight, -1);
    }
}

int CApplicationData::Init(int argc, char *argv[]){
    std::shared_ptr< CDataSourceContainer > TempDataContainer(new CDirectoryDataSourceContainer("./data"));
    std::shared_ptr< CDataSourceContainerIterator > FileIterator;
    std::shared_ptr< CDataSource > TempDataSource;
    // This is called in all GTK applications. Arguments are parsed from the 
    // command line and are returned to the application. All GTK+ specific 
    // arguments are removed from the argc/argv list.
    gtk_init(&argc, &argv);
    
    
    // Create a new main window 
    DMainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    // When the window is given the "delete-event" signal (this is given by the 
    // window manager, usually by the "close" option, or on the titlebar), we 
    // ask it to call the delete_event () function as defined above. The data 
    // passed to the callback function is nullptr and is ignored in the callback 
    // function. 
    g_signal_connect(DMainWindow, "delete-event", G_CALLBACK(MainWindowDeleteEventCallback), this);
    
    // Here we connect the "destroy" event to a signal handler. This event 
    // occurs when we call gtk_widget_destroy() on the window, or if we return 
    // FALSE in the "delete-event" callback. 
    g_signal_connect(DMainWindow, "destroy", G_CALLBACK(MainWindowDestroyCallback), this);
    
    g_signal_connect(DMainWindow, "key-press-event", G_CALLBACK(MainWindowKeyPressEventCallback), this);
    g_signal_connect(DMainWindow, "key-release-event", G_CALLBACK(MainWindowKeyReleaseEventCallback), this);
    //g_signal_connect(DMainWindow, "configure-event", G_CALLBACK(MainWindowConfigureEventCallback), this);
    
    
    // Sets the border width of the window. 
    gtk_container_set_border_width(GTK_CONTAINER(DMainWindow), 2);
    
    // Creates a drawing surface
    DDrawingArea = gtk_drawing_area_new();

    // Add drawing surface to main window
    gtk_container_add(GTK_CONTAINER(DMainWindow), DDrawingArea);
    
    
    GdkGeometry Geometry;
    Geometry.min_width = INITIAL_MAP_WIDTH;
    Geometry.min_height = INITIAL_MAP_HEIGHT;
    gtk_window_set_geometry_hints(GTK_WINDOW(DMainWindow), DDrawingArea, &Geometry, GDK_HINT_MIN_SIZE);
    //gtk_widget_set_size_request(DDrawingArea, INITIAL_MAP_WIDTH, INITIAL_MAP_HEIGHT);
    
    g_signal_connect(DDrawingArea, "expose-event", G_CALLBACK(DrawingAreaExposeCallback), this);
    g_signal_connect(DDrawingArea, "button-press-event", G_CALLBACK(DrawingAreaButtonPressEventCallback), this);
    g_signal_connect(DDrawingArea, "button-release-event", G_CALLBACK(DrawingAreaButtonReleaseEventCallback), this);
    g_signal_connect(DDrawingArea, "motion-notify-event", G_CALLBACK(DrawingAreaMotionNotifyEventCallback), this);
    g_signal_connect(DDrawingArea, "configure-event", G_CALLBACK(MainWindowConfigureEventCallback), this);
    
    gtk_widget_set_events(DDrawingArea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
    
    DBlankCursor = gdk_cursor_new(GDK_BLANK_CURSOR);
    // Show all widgets so they are displayed
    TempDataSource = TempDataContainer->DataSource("Splash.dat");
    DSplashTileset = std::make_shared< CGraphicTileset >();
    if(!DSplashTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to splash screen.\n");
        return -1;
    }
    gtk_widget_show(DDrawingArea);           
    gtk_widget_show(DMainWindow);
    
    gdk_window_set_cursor(DDrawingArea->window, DBlankCursor); 
    
    DDrawingContext = DDrawingArea->style->fg_gc[gtk_widget_get_state(DDrawingArea)];
    DDoubleBufferPixmap = gdk_pixmap_new(DDrawingArea->window, INITIAL_MAP_WIDTH, INITIAL_MAP_HEIGHT, -1);
    DSplashTileset->DrawTile(DDoubleBufferPixmap, DDrawingContext, 0, 0, 0);
        
    gdk_draw_pixmap(DDrawingArea->window, DDrawingContext, DDoubleBufferPixmap, 0, 0, 0, 0, -1, -1);
    
    TempDataSource = TempDataContainer->DataSource("Cursors.dat");
    DCursorSet = std::make_shared< CCursorSet >();
    if(!DCursorSet->LoadCursors(TempDataSource)){
        PrintError("Failed to load cursors.\n");
        return -1;
    }
    DCursorIndices[ctPointer] = DCursorSet->FindCursor("pointer");
    DCursorIndices[ctInspect] = DCursorSet->FindCursor("magnifier");
    DCursorIndices[ctArrowN] = DCursorSet->FindCursor("arrow-n");
    DCursorIndices[ctArrowE] = DCursorSet->FindCursor("arrow-e");
    DCursorIndices[ctArrowS] = DCursorSet->FindCursor("arrow-s");
    DCursorIndices[ctArrowW] = DCursorSet->FindCursor("arrow-w");
    DCursorIndices[ctTargetOff] = DCursorSet->FindCursor("target-off");
    DCursorIndices[ctTargetOn] = DCursorSet->FindCursor("target-on");
    
    //gtk_main_iteration_do(FALSE);
    
    TempDataSource = TempDataContainer->DataSource("SoundClips.dat");
    DSoundLibraryMixer = std::make_shared< CSoundLibraryMixer >();
    if(!DSoundLibraryMixer->LoadLibrary(TempDataSource)){
        PrintError("Failed to sound mixer.\n");
        return -1;
    }
    DSoundLibraryMixer->PlaySong(DSoundLibraryMixer->FindSong("load"), DMusicVolume);

    TempDataSource = TempDataContainer->DataSource("ButtonColors.dat");
    DButtonColorTileset = std::make_shared< CGraphicTileset >();
    if(!DButtonColorTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load markers.\n");
        return -1;
    }
    
    TempDataSource = TempDataContainer->DataSource("Marker.dat");
    DMarkerTileset = std::make_shared< CGraphicTileset >();
    if(!DMarkerTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load markers.\n");
        return -1;
    }
    
    TempDataSource = TempDataContainer->DataSource("Texture.dat");
    DBackgroundTileset = std::make_shared< CGraphicTileset >();
    if(!DBackgroundTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load background tileset.\n");
        return -1;
    }    
    
    
    TempDataSource = TempDataContainer->DataSource("FontKingthings10.dat");
    DFonts[CUnitDescriptionRenderer::fsSmall] = std::make_shared< CFontTileset >();
    if(!DFonts[CUnitDescriptionRenderer::fsSmall]->LoadFont(TempDataSource)){
        PrintError("Failed to load font tileset.\n");
        return -1;
    }
    TempDataSource = TempDataContainer->DataSource("FontKingthings12.dat");
    DFonts[CUnitDescriptionRenderer::fsMedium] = std::make_shared< CFontTileset >();
    if(!DFonts[CUnitDescriptionRenderer::fsMedium]->LoadFont(TempDataSource)){
        PrintError("Failed to load font tileset.\n");
        return -1;
    }
    TempDataSource = TempDataContainer->DataSource("FontKingthings16.dat");
    DFonts[CUnitDescriptionRenderer::fsLarge] = std::make_shared< CFontTileset >();
    if(!DFonts[CUnitDescriptionRenderer::fsLarge]->LoadFont(TempDataSource)){
        PrintError("Failed to load font tileset.\n");
        return -1;
    }
    TempDataSource = TempDataContainer->DataSource("FontKingthings24.dat");
    DFonts[CUnitDescriptionRenderer::fsGiant] = std::make_shared< CFontTileset >();
    if(!DFonts[CUnitDescriptionRenderer::fsGiant]->LoadFont(TempDataSource)){
        PrintError("Failed to load font tileset.\n");
        return -1;
    }
    
    
    TempDataSource = TempDataContainer->DataSource("MiniBevel.dat");
    DMiniBevelTileset = std::make_shared< CGraphicTileset >();
    if(!DMiniBevelTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load bevel tileset.\n");
        return -1;
    }        
    DMiniBevel = std::make_shared< CBevel >(DMiniBevelTileset);
    
    TempDataSource = TempDataContainer->DataSource("InnerBevel.dat");
    DInnerBevelTileset = std::make_shared< CGraphicTileset >();
    if(!DInnerBevelTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load bevel tileset.\n");
        return -1;
    }        
    DInnerBevel = std::make_shared< CBevel >(DInnerBevelTileset);
    
    TempDataSource = TempDataContainer->DataSource("OuterBevel.dat");
    DOuterBevelTileset = std::make_shared< CGraphicTileset >();
    if(!DOuterBevelTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load bevel tileset.\n");
        return -1;
    }        
    DOuterBevel = std::make_shared< CBevel >(DOuterBevelTileset);
    
    TempDataSource = TempDataContainer->DataSource("ListViewIcons.dat");
    DListViewIconTileset = std::make_shared< CGraphicTileset >();
    if(!DListViewIconTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load list view tileset.\n");
        return -1;
    }
    
    
    TempDataSource = TempDataContainer->DataSource("Terrain.dat");
    DTerrainTileset = std::make_shared< CGraphicTileset >();
    if(!DTerrainTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load tileset.\n");
        return -1;
    }
    CPosition::SetTileDimensions(DTerrainTileset->TileWidth(), DTerrainTileset->TileHeight());
    
    TempDataSource = TempDataContainer->DataSource("Fog.dat");
    DFogTileset = std::make_shared< CGraphicTileset >();
    if(!DFogTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load tileset.\n");
        return -1;
    }
    
    TempDataSource = TempDataContainer->DataSource("png/Colors.png");
    DRecolorMap = std::make_shared< CGraphicRecolorMap > ();
    if(!DRecolorMap->Load(TempDataSource)){
        PrintError("Failed to load recolor map.\n");
        return -1;
    }
    
    DAssetTilesets.resize(atMax);
    
    
    PrintDebug(DEBUG_LOW, "Loading Icons\n");
    TempDataSource = TempDataContainer->DataSource("Icons.dat");
    DIconTileset = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DIconTileset->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load icons.\n");
        return -1;
    }
    
    PrintDebug(DEBUG_LOW, "Loading Mini Icons\n");
    TempDataSource = TempDataContainer->DataSource("MiniIcons.dat");
    DMiniIconTileset = std::make_shared< CGraphicTileset > ();
    if(!DMiniIconTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load mini icons.\n");
        return -1;
    }
    
    
    PrintDebug(DEBUG_LOW, "Loading Coprse\n");
    TempDataSource = TempDataContainer->DataSource("Corpse.dat");
    DCorpseTileset = std::make_shared< CGraphicTileset > ();
    if(!DCorpseTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load corpse tileset.\n");
        return -1;
    }
    
    PrintDebug(DEBUG_LOW, "Loading FireSmall\n");
    TempDataSource = TempDataContainer->DataSource("FireSmall.dat");
    auto FireTileset = std::make_shared< CGraphicTileset > ();
    if(!FireTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load fire small tileset.\n");
        return -1;
    }
    DFireTilesets.push_back(FireTileset);
    
    PrintDebug(DEBUG_LOW, "Loading FireLarge\n");
    TempDataSource = TempDataContainer->DataSource("FireLarge.dat");
    FireTileset = std::make_shared< CGraphicTileset > ();
    if(!FireTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load fire large tileset.\n");
        return -1;
    }
    DFireTilesets.push_back(FireTileset);
    
    PrintDebug(DEBUG_LOW, "Loading BuildingDeath\n");
    TempDataSource = TempDataContainer->DataSource("BuildingDeath.dat");
    DBuildingDeathTileset = std::make_shared< CGraphicTileset > ();
    if(!DBuildingDeathTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load building death tileset.\n");
        return -1;
    }
    
    PrintDebug(DEBUG_LOW, "Loading Arrow\n");
    TempDataSource = TempDataContainer->DataSource("Arrow.dat");
    DArrowTileset = std::make_shared< CGraphicTileset > ();
    if(!DArrowTileset->LoadTileset(TempDataSource)){
        PrintError("Failed to load arrow tileset.\n");
        return -1;
    }
    
    PrintDebug(DEBUG_LOW, "Loading AssetNone\n");
    TempDataSource = TempDataContainer->DataSource("AssetNone.dat");
    DAssetTilesets[atNone] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atNone]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load character tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Peasant\n");
    TempDataSource = TempDataContainer->DataSource("Peasant.dat");
    DAssetTilesets[atPeasant] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atPeasant]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load peasant tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Footman\n");
    TempDataSource = TempDataContainer->DataSource("Footman.dat");
    DAssetTilesets[atFootman] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atFootman]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load footman tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Archer\n");
    TempDataSource = TempDataContainer->DataSource("Archer.dat");
    DAssetTilesets[atArcher] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atArcher]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load archer tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Ranger\n");
    TempDataSource = TempDataContainer->DataSource("Ranger.dat");
    DAssetTilesets[atRanger] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atRanger]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load archer tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading GoldMine\n");
    TempDataSource = TempDataContainer->DataSource("GoldMine.dat");
    DAssetTilesets[atGoldMine] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atGoldMine]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load gold mine tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading TownHall\n");
    TempDataSource = TempDataContainer->DataSource("TownHall.dat");
    DAssetTilesets[atTownHall] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atTownHall]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load town hall tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Keep\n");
    TempDataSource = TempDataContainer->DataSource("Keep.dat");
    DAssetTilesets[atKeep] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atKeep]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load keep tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Castle\n");
    TempDataSource = TempDataContainer->DataSource("Castle.dat");
    DAssetTilesets[atCastle] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atCastle]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load castle tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Farm\n");
    TempDataSource = TempDataContainer->DataSource("Farm.dat");
    DAssetTilesets[atFarm] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atFarm]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load farm tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Barracks\n");
    TempDataSource = TempDataContainer->DataSource("Barracks.dat");
    DAssetTilesets[atBarracks] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atBarracks]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load barracks tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading Blacksmith\n");
    TempDataSource = TempDataContainer->DataSource("Blacksmith.dat");
    DAssetTilesets[atBlacksmith] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atBlacksmith]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load blacksmith tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading LumberMill\n");
    TempDataSource = TempDataContainer->DataSource("LumberMill.dat");
    DAssetTilesets[atLumberMill] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atLumberMill]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load lumber mill tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading ScoutTower\n");
    TempDataSource = TempDataContainer->DataSource("ScoutTower.dat");
    DAssetTilesets[atScoutTower] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atScoutTower]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load scout tower tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading GuardTower\n");
    TempDataSource = TempDataContainer->DataSource("GuardTower.dat");
    DAssetTilesets[atGuardTower] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atGuardTower]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load guard tower tileset.\n");
        return -1;
    }
    PrintDebug(DEBUG_LOW, "Loading CannonTower\n");
    TempDataSource = TempDataContainer->DataSource("CannonTower.dat");
    DAssetTilesets[atCannonTower] = std::make_shared< CGraphicMulticolorTileset > ();
    if(!DAssetTilesets[atCannonTower]->LoadTileset(DRecolorMap, TempDataSource)){
        PrintError("Failed to load cannon tower tileset.\n");
        return -1;
    }
    
    PrintDebug(DEBUG_LOW, "Assets Loaded\n");
    
    PrintDebug(DEBUG_LOW, "Loading res directory\n");
    std::shared_ptr< CDataSourceContainer > AssetDirectory = TempDataContainer->DataSourceContainer("res");
    if(!CPlayerAssetType::LoadTypes(AssetDirectory)){
        PrintError("Failed to load resources\n");
        return 0;   
    }
    
    PrintDebug(DEBUG_LOW, "Loading upg directory\n");
    std::shared_ptr< CDataSourceContainer > UpgradeDirectory = TempDataContainer->DataSourceContainer("upg");
    if(!CPlayerUpgrade::LoadUpgrades(UpgradeDirectory)){
        PrintError("Failed to load upgrades\n");
        return 0;   
    }

    PrintDebug(DEBUG_LOW, "Loading Maps\n");
    std::shared_ptr< CDataSourceContainer > MapDirectory = TempDataContainer->DataSourceContainer("map");
    if(!CAssetDecoratedMap::LoadMaps(MapDirectory)){
        PrintError("Failed to load maps\n");
        return 0;   
    }
    
    DPeriodicTimeout = std::make_shared< CPeriodicTimeout > (TIMEOUT_INTERVAL);
    CPlayerAsset::UpdateFrequency(DPeriodicTimeout->Frequency());
    CAssetRenderer::UpdateFrequency(DPeriodicTimeout->Frequency());

    
    LoadGameMap(0);
    
    DMainMenuTitle = "The Game";
    DMainMenuButtonsText.push_back("Single Player Game");
    DMainMenuButtonsFunctions.push_back(SinglePlayerGameButtonCallback);
    DMainMenuButtonsText.push_back("Multi Player Game");
    DMainMenuButtonsFunctions.push_back(MultiPlayerGameButtonCallback);
    DMainMenuButtonsText.push_back("Options");
    DMainMenuButtonsFunctions.push_back(OptionsButtonCallback);
    DMainMenuButtonsText.push_back("");
    DMainMenuButtonsFunctions.push_back(nullptr);
    DMainMenuButtonsText.push_back("Exit Game");
    DMainMenuButtonsFunctions.push_back(ExitGameButtonCallback);
    
        
    DOptionsMenuTitle = "Options";
    DOptionsMenuButtonsText.push_back("Sound Options");
    DOptionsMenuButtonsFunctions.push_back(SoundOptionsButtonCallback);
    DOptionsMenuButtonsText.push_back("Network Options");
    DOptionsMenuButtonsFunctions.push_back(NetworkOptionsButtonCallback);
    DOptionsMenuButtonsText.push_back("");
    DOptionsMenuButtonsFunctions.push_back(nullptr);
    DOptionsMenuButtonsText.push_back("Back");
    DOptionsMenuButtonsFunctions.push_back(MainMenuButtonCallback);
    
    DMultiPlayerOptionsMenuTitle = "Multi Player Game Options";
    DMultiPlayerOptionsMenuButtonsText.push_back("Host Multi Player Game");
    DMultiPlayerOptionsMenuButtonsFunctions.push_back(HostMultiPlayerButtonCallback);
    DMultiPlayerOptionsMenuButtonsText.push_back("Join Multi Player Game");
    DMultiPlayerOptionsMenuButtonsFunctions.push_back(JoinMultiPlayerButtonCallback);
    DMultiPlayerOptionsMenuButtonsText.push_back("");
    DMultiPlayerOptionsMenuButtonsFunctions.push_back(nullptr);
    DMultiPlayerOptionsMenuButtonsText.push_back("Back");
    DMultiPlayerOptionsMenuButtonsFunctions.push_back(MainMenuButtonCallback);
    
    DCurrentPageButtonRenderer = std::make_shared< CButtonRenderer > (DButtonColorTileset, DInnerBevel, DOuterBevel, DFonts[CUnitDescriptionRenderer::fsLarge]);
    DMapSelectListViewRenderer = std::make_shared< CListViewRenderer > (DListViewIconTileset, DFonts[CUnitDescriptionRenderer::fsLarge]);
    DOptionsEditRenderer = std::make_shared< CEditRenderer > (DButtonColorTileset, DInnerBevel, DFonts[CUnitDescriptionRenderer::fsLarge]);
    //ResizeCanvases();
    ChangeMode(gmMainMenu);
    SwitchButtonMenuData();
    gdk_draw_pixmap(DDoubleBufferPixmap, DDrawingContext, DWorkingBufferPixmap, 0, 0, 0, 0, -1, -1);
    gdk_draw_pixmap(DDrawingArea->window, DDrawingContext, DDoubleBufferPixmap, 0, 0, 0, 0, -1, -1);
    
    
    g_timeout_add(DPeriodicTimeout->MiliSecondsUntilDeadline(), TimeoutCallback, this);
    // All GTK applications must have a gtk_main(). Control ends here and waits 
    // for an event to occur (like a key press or mouse event). 
    DSoundLibraryMixer->StopSong();
    DSoundLibraryMixer->PlaySong(DSoundLibraryMixer->FindSong("menu"), DMusicVolume);
    gtk_main ();
    return 0;
}


#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_HIGH
#endif

int main(int argc, char *argv[]){
    CApplicationData MainData;
    int ReturnValue;    
    
    OpenDebug("Debug.out", DEBUG_LEVEL);
    
    ReturnValue = MainData.Init(argc, argv);

    return ReturnValue;
}

