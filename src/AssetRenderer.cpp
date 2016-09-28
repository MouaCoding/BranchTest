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
#include "AssetRenderer.h"
#include "PixelType.h"
#include "Debug.h"

#define TARGET_FREQUENCY        10
//std::vector< std::vector< int > > DBuildIndices;
//std::vector< std::vector< int > > DWalkIndices;
int CAssetRenderer::DAnimationDownsample = 1;

CAssetRenderer::CAssetRenderer(std::vector< std::shared_ptr< CGraphicMulticolorTileset > > tilesets, std::shared_ptr< CGraphicTileset > markertileset, std::shared_ptr< CGraphicTileset > corpsetileset, std::vector< std::shared_ptr< CGraphicTileset > > firetileset, std::shared_ptr< CGraphicTileset > buildingdeath, std::shared_ptr< CGraphicTileset > arrowtileset, std::shared_ptr< CPlayerData > player, std::shared_ptr< CAssetDecoratedMap > map){
    int TypeIndex = 0, MarkerIndex = 0;
    DTilesets = tilesets;
    DMarkerTileset = markertileset;
    DFireTilesets = firetileset;
    DBuildingDeathTileset = buildingdeath;
    DCorpseTileset = corpsetileset;
    DArrowTileset = arrowtileset;
    DPlayerData = player;
    DPlayerMap = map;
    
    DPixelIndices.resize(pcMax + 3);
    DPixelIndices[pcNone] = DTilesets[0]->FindPixel("none");
    DPixelIndices[pcBlue] = DTilesets[0]->FindPixel("blue");
    DPixelIndices[pcRed] = DTilesets[0]->FindPixel("red");
    DPixelIndices[pcGreen] = DTilesets[0]->FindPixel("green");
    DPixelIndices[pcPurple] = DTilesets[0]->FindPixel("purple");
    DPixelIndices[pcOrange] = DTilesets[0]->FindPixel("orange");
    DPixelIndices[pcYellow] = DTilesets[0]->FindPixel("yellow");
    DPixelIndices[pcBlack] = DTilesets[0]->FindPixel("black");
    DPixelIndices[pcWhite] = DTilesets[0]->FindPixel("white");
    DPixelIndices[pcMax] = DTilesets[0]->FindPixel("self");
    DPixelIndices[pcMax + 1] = DTilesets[0]->FindPixel("enemy");
    DPixelIndices[pcMax + 2] = DTilesets[0]->FindPixel("building");
    
    while(true){
        int Index = DMarkerTileset->FindTile(std::string("marker-") + std::to_string(MarkerIndex));
        if(0 > Index){
            break;   
        }
        DMarkerIndices.push_back(Index);
        MarkerIndex++;
    }
    DPlaceGoodIndex = DMarkerTileset->FindTile("place-good");
    DPlaceBadIndex = DMarkerTileset->FindTile("place-bad");
        
    {
        std::string LastDirectionName = "decay-nw";
        for(auto &DirectionName : {"decay-n","decay-ne","decay-e","decay-se","decay-s","decay-sw","decay-w","decay-nw"}){
            int StepIndex = 0, TileIndex;
            while(true){
                TileIndex = DCorpseTileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
                if(0 <= TileIndex){
                    DCorpseIndices.push_back(TileIndex);
                }
                else{
                    TileIndex = DCorpseTileset->FindTile(std::string(LastDirectionName) + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DCorpseIndices.push_back(TileIndex);
                    }
                    else{
                        break;   
                    }
                }
                StepIndex++;
            }
            LastDirectionName = DirectionName;
        }   
    }
    
    for(auto &DirectionName : {"attack-n","attack-ne","attack-e","attack-se","attack-s","attack-sw","attack-w","attack-nw"}){
        int StepIndex = 0, TileIndex;
        while(true){
            TileIndex = DArrowTileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
            if(0 <= TileIndex){
                DArrowIndices.push_back(TileIndex);
            }
            else{
                break;   
            }
            StepIndex++;
        }
    }  
    
    
    DConstructIndices.resize(DTilesets.size());
    DBuildIndices.resize(DTilesets.size());
    DWalkIndices.resize(DTilesets.size());
    DNoneIndices.resize(DTilesets.size());
    DCarryGoldIndices.resize(DTilesets.size());
    DCarryLumberIndices.resize(DTilesets.size());
    DAttackIndices.resize(DTilesets.size());
    DDeathIndices.resize(DTilesets.size());
    DPlaceIndices.resize(DTilesets.size());
    for(auto &Tileset : DTilesets){
        if(Tileset){
            PrintDebug(DEBUG_LOW,"Checking Walk on %d\n",TypeIndex);
            for(auto &DirectionName : {"walk-n","walk-ne","walk-e","walk-se","walk-s","walk-sw","walk-w","walk-nw"}){
                int StepIndex = 0, TileIndex;
                while(true){
                    TileIndex = Tileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DWalkIndices[TypeIndex].push_back(TileIndex);
                    }
                    else{
                        break;   
                    }
                    StepIndex++;
                }
            }
            PrintDebug(DEBUG_LOW,"Checking Construct on %d\n",TypeIndex);
            {
                int StepIndex = 0, TileIndex;
                while(true){
                    TileIndex = Tileset->FindTile(std::string("construct-") + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DConstructIndices[TypeIndex].push_back(TileIndex);
                    }
                    else{
                        if(!StepIndex){
                            DConstructIndices[TypeIndex].push_back(-1);
                        }
                        break;   
                    }
                    StepIndex++;
                }
            }
            PrintDebug(DEBUG_LOW,"Checking Gold on %d\n",TypeIndex);
            for(auto &DirectionName : {"gold-n","gold-ne","gold-e","gold-se","gold-s","gold-sw","gold-w","gold-nw"}){
                int StepIndex = 0, TileIndex;
                while(true){
                    TileIndex = Tileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DCarryGoldIndices[TypeIndex].push_back(TileIndex);
                    }
                    else{
                        break;   
                    }
                    StepIndex++;
                }
            }
            PrintDebug(DEBUG_LOW,"Checking Lumber on %d\n",TypeIndex);
            for(auto &DirectionName : {"lumber-n","lumber-ne","lumber-e","lumber-se","lumber-s","lumber-sw","lumber-w","lumber-nw"}){
                int StepIndex = 0, TileIndex;
                while(true){
                    TileIndex = Tileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DCarryLumberIndices[TypeIndex].push_back(TileIndex);
                    }
                    else{
                        break;   
                    }
                    StepIndex++;
                }
            }
            PrintDebug(DEBUG_LOW,"Checking Attack on %d\n",TypeIndex);
            for(auto &DirectionName : {"attack-n","attack-ne","attack-e","attack-se","attack-s","attack-sw","attack-w","attack-nw"}){
                int StepIndex = 0, TileIndex;
                while(true){
                    TileIndex = Tileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DAttackIndices[TypeIndex].push_back(TileIndex);
                    }
                    else{
                        break;   
                    }
                    StepIndex++;
                }
            }
            if(0 == DAttackIndices[TypeIndex].size()){
                int TileIndex;
                for(int Index = 0; Index < dMax; Index++){
                    if(0 <= (TileIndex = Tileset->FindTile("active"))){
                        DAttackIndices[TypeIndex].push_back(TileIndex);
                    }
                    else if(0 <= (TileIndex = Tileset->FindTile("inactive"))){
                        DAttackIndices[TypeIndex].push_back(TileIndex);
                    }   
                }
            }
            PrintDebug(DEBUG_LOW,"Checking Death on %d\n",TypeIndex);
            std::string LastDirectionName = "death-nw";
            for(auto &DirectionName : {"death-n","death-ne","death-e","death-se","death-s","death-sw","death-w","death-nw"}){
                int StepIndex = 0, TileIndex;
                while(true){
                    TileIndex = Tileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DDeathIndices[TypeIndex].push_back(TileIndex);
                    }
                    else{
                        TileIndex = Tileset->FindTile(std::string(LastDirectionName) + std::to_string(StepIndex));
                        if(0 <= TileIndex){
                            DDeathIndices[TypeIndex].push_back(TileIndex);
                        }
                        else{
                            break;   
                        }
                    }
                    StepIndex++;
                }
                LastDirectionName = DirectionName;
            }
            if(DDeathIndices[TypeIndex].size()){
                
            }
            PrintDebug(DEBUG_LOW,"Checking None on %d\n",TypeIndex);
            for(auto &DirectionName : {"none-n","none-ne","none-e","none-se","none-s","none-sw","none-w","none-nw"}){
                int TileIndex = Tileset->FindTile(std::string(DirectionName));
                if(0 <= TileIndex){
                    DNoneIndices[TypeIndex].push_back(TileIndex);
                }
                else if(DWalkIndices[TypeIndex].size()){
                    DNoneIndices[TypeIndex].push_back(DWalkIndices[TypeIndex][DNoneIndices[TypeIndex].size() * (DWalkIndices[TypeIndex].size() / dMax)]);
                }
                else if(0 <= (TileIndex = Tileset->FindTile("inactive"))){
                    DNoneIndices[TypeIndex].push_back(TileIndex);
                }
            }
            PrintDebug(DEBUG_LOW,"Checking Build on %d\n",TypeIndex);
            for(auto &DirectionName : {"build-n","build-ne","build-e","build-se","build-s","build-sw","build-w","build-nw"}){
                int StepIndex = 0, TileIndex;
                while(true){
                    TileIndex = Tileset->FindTile(std::string(DirectionName) + std::to_string(StepIndex));
                    if(0 <= TileIndex){
                        DBuildIndices[TypeIndex].push_back(TileIndex);
                    }
                    else{
                        if(!StepIndex){
                            if(0 <= (TileIndex = Tileset->FindTile("active"))){
                                DBuildIndices[TypeIndex].push_back(TileIndex);
                            }
                            else if(0 <= (TileIndex = Tileset->FindTile("inactive"))){
                                DBuildIndices[TypeIndex].push_back(TileIndex);
                            }
                        }
                        break;
                    }
                    StepIndex++;
                }
            }
            PrintDebug(DEBUG_LOW,"Checking Place on %d\n",TypeIndex);
            {
                DPlaceIndices[TypeIndex].push_back( Tileset->FindTile("place") );
            }
            
            PrintDebug(DEBUG_LOW,"Done checking type %d\n",TypeIndex);
        }
        TypeIndex++;
    }
}

int CAssetRenderer::UpdateFrequency(int freq){
    if(TARGET_FREQUENCY >= freq){
        DAnimationDownsample = 1;
        return TARGET_FREQUENCY;   
    }
    DAnimationDownsample = freq / TARGET_FREQUENCY;
    return freq;
}

typedef struct{
    EAssetType DType;
    int DX;
    int DY;
    int DBottomY;
    int DTileIndex;
    int DColorIndex;
    guint32 DPixelColor;
} SAssetRenderData, *SAssetRenderDataRef;

bool CompareRenderData(const SAssetRenderData &first, const SAssetRenderData &second){
    if(first.DBottomY < second.DBottomY){
        return true;   
    }
    if(first.DBottomY > second.DBottomY){
        return false;        
    }
    
    return first.DX <= second.DX;
}

void CAssetRenderer::DrawAssets(GdkDrawable *drawable, GdkDrawable *typedrawable, const SRectangle &rect){
    GdkGC *TempGC = gdk_gc_new(drawable);
    GdkGC *TempTypeGC = gdk_gc_new(typedrawable);
    int ScreenRightX = rect.DXPosition + rect.DWidth - 1;
    int ScreenBottomY = rect.DYPosition + rect.DHeight - 1;
    std::list< SAssetRenderData > FinalRenderList;
    
    for(auto &AssetIterator : DPlayerMap->Assets()){
        SAssetRenderData TempRenderData;
        TempRenderData.DType = AssetIterator->Type();
        if(atNone == TempRenderData.DType){
            continue;   
        }
        if((0 <= TempRenderData.DType)&&(TempRenderData.DType < DTilesets.size())){
            CPixelType PixelType(*AssetIterator); 
            int RightX;
            TempRenderData.DX = AssetIterator->PositionX() + (AssetIterator->Size() - 1) * CPosition::HalfTileWidth() - DTilesets[TempRenderData.DType]->TileHalfWidth();
            TempRenderData.DY = AssetIterator->PositionY() + (AssetIterator->Size() - 1) * CPosition::HalfTileHeight() - DTilesets[TempRenderData.DType]->TileHalfHeight();
            TempRenderData.DPixelColor = PixelType.ToPixelColor();
            //printf("%d @ (%d, %d)\n",TempRenderData.DType, TempRenderData.DX , TempRenderData.DY);
            RightX = TempRenderData.DX + DTilesets[TempRenderData.DType]->TileWidth() - 1;
            TempRenderData.DBottomY = TempRenderData.DY + DTilesets[TempRenderData.DType]->TileHeight() - 1;
            bool OnScreen = true;
            if((RightX < rect.DXPosition)||(TempRenderData.DX > ScreenRightX)){
                OnScreen = false;
            }
            else if((TempRenderData.DBottomY < rect.DYPosition)||(TempRenderData.DY > ScreenBottomY)){
                OnScreen = false;   
            }
            TempRenderData.DX -= rect.DXPosition;
            TempRenderData.DY -= rect.DYPosition;
            TempRenderData.DColorIndex = AssetIterator->Color() ? AssetIterator->Color() - 1 : AssetIterator->Color();
            TempRenderData.DTileIndex = -1;
            if(OnScreen){
                int ActionSteps, CurrentStep, TileIndex;
                switch(AssetIterator->Action()){
                    case aaBuild:           ActionSteps = DBuildIndices[TempRenderData.DType].size();
                                            ActionSteps /= dMax;
                                            if(ActionSteps){
                                                TileIndex = AssetIterator->Direction() * ActionSteps + ((AssetIterator->Step() / DAnimationDownsample)% ActionSteps);
                                                TempRenderData.DTileIndex = DBuildIndices[TempRenderData.DType][TileIndex];
                                            }
                                            break;
                    case aaConstruct:       ActionSteps = DConstructIndices[TempRenderData.DType].size();
                                            if(ActionSteps){
                                                int TotalSteps = AssetIterator->BuildTime() * CPlayerAsset::UpdateFrequency();
                                                int CurrentStep = AssetIterator->Step() * ActionSteps / TotalSteps;
                                                if(CurrentStep == DConstructIndices[TempRenderData.DType].size()){
                                                    CurrentStep--;   
                                                }
                                                TempRenderData.DTileIndex = DConstructIndices[TempRenderData.DType][CurrentStep];
                                            }
                                            break;
                    case aaWalk:            if(AssetIterator->Lumber()){
                                                ActionSteps = DCarryLumberIndices[TempRenderData.DType].size();
                                                ActionSteps /= dMax;
                                                TileIndex = AssetIterator->Direction() * ActionSteps + ((AssetIterator->Step() / DAnimationDownsample)% ActionSteps);
                                                TempRenderData.DTileIndex = DCarryLumberIndices[TempRenderData.DType][TileIndex];
                                            }
                                            else if(AssetIterator->Gold()){
                                                ActionSteps = DCarryGoldIndices[TempRenderData.DType].size();
                                                ActionSteps /= dMax;
                                                TileIndex = AssetIterator->Direction() * ActionSteps + ((AssetIterator->Step() / DAnimationDownsample)% ActionSteps);
                                                TempRenderData.DTileIndex = DCarryGoldIndices[TempRenderData.DType][TileIndex];  
                                            }
                                            else{
                                                ActionSteps = DWalkIndices[TempRenderData.DType].size();
                                                ActionSteps /= dMax;
                                                TileIndex = AssetIterator->Direction() * ActionSteps + ((AssetIterator->Step() / DAnimationDownsample)% ActionSteps);
                                                TempRenderData.DTileIndex = DWalkIndices[TempRenderData.DType][TileIndex];
                                            }   
                                            break;
                    case aaAttack:          CurrentStep = AssetIterator->Step() % (AssetIterator->AttackSteps() + AssetIterator->ReloadSteps());
                                            if(CurrentStep < AssetIterator->AttackSteps()){
                                                ActionSteps = DAttackIndices[TempRenderData.DType].size();
                                                ActionSteps /= dMax;
                                                TileIndex = AssetIterator->Direction() * ActionSteps + (CurrentStep * ActionSteps / AssetIterator->AttackSteps());
                                                TempRenderData.DTileIndex = DAttackIndices[TempRenderData.DType][TileIndex];
                                            }
                                            else{
                                                TempRenderData.DTileIndex = DNoneIndices[TempRenderData.DType][AssetIterator->Direction()];   
                                            }
                                            break;
                    case aaRepair:
                    case aaHarvestLumber:   ActionSteps = DAttackIndices[TempRenderData.DType].size();
                                            ActionSteps /= dMax;
                                            TileIndex = AssetIterator->Direction() * ActionSteps + ((AssetIterator->Step() / DAnimationDownsample)% ActionSteps);
                                            TempRenderData.DTileIndex = DAttackIndices[TempRenderData.DType][TileIndex];
                                            break;
                    case aaMineGold:        break;
                    case aaStandGround:
                    case aaNone:            TempRenderData.DTileIndex = DNoneIndices[TempRenderData.DType][AssetIterator->Direction()];
                                            if(AssetIterator->Speed()){
                                                if(AssetIterator->Lumber()){
                                                    ActionSteps = DCarryLumberIndices[TempRenderData.DType].size();
                                                    ActionSteps /= dMax;
                                                    TempRenderData.DTileIndex = DCarryLumberIndices[TempRenderData.DType][AssetIterator->Direction() * ActionSteps];
                                                }
                                                else if(AssetIterator->Gold()){
                                                    ActionSteps = DCarryGoldIndices[TempRenderData.DType].size();
                                                    ActionSteps /= dMax;
                                                    TempRenderData.DTileIndex = DCarryGoldIndices[TempRenderData.DType][AssetIterator->Direction() * ActionSteps];
                                                }
                                            }
                                            break;
                    case aaCapability:      if(AssetIterator->Speed()){
                                                if((actPatrol == AssetIterator->CurrentCommand().DCapability)||(actStandGround == AssetIterator->CurrentCommand().DCapability)){
                                                    TempRenderData.DTileIndex = DNoneIndices[TempRenderData.DType][AssetIterator->Direction()];
                                                }
                                            }
                                            else{
                                                // Buildings
                                                TempRenderData.DTileIndex = DNoneIndices[TempRenderData.DType][AssetIterator->Direction()];
                                            }
                                            break;
                    case aaDeath:           ActionSteps = DDeathIndices[TempRenderData.DType].size();
                                            if(AssetIterator->Speed()){
                                                ActionSteps /= dMax;
                                                if(ActionSteps){
                                                    CurrentStep = AssetIterator->Step() / DAnimationDownsample;
                                                    if(CurrentStep >= ActionSteps){
                                                        CurrentStep = ActionSteps - 1;
                                                    }
                                                    TempRenderData.DTileIndex = DDeathIndices[TempRenderData.DType][AssetIterator->Direction() * ActionSteps + CurrentStep];
                                                }
                                            }
                                            else{
                                                if(AssetIterator->Step() < DBuildingDeathTileset->TileCount()){
                                                    TempRenderData.DTileIndex = DTilesets[TempRenderData.DType]->TileCount() + AssetIterator->Step();
                                                    TempRenderData.DX += DTilesets[TempRenderData.DType]->TileHalfWidth() - DBuildingDeathTileset->TileHalfWidth();
                                                    TempRenderData.DY += DTilesets[TempRenderData.DType]->TileHalfHeight() - DBuildingDeathTileset->TileHalfHeight();
                                                }
                                            }
                    default:                break;
                }
                if(0 <= TempRenderData.DTileIndex){
                    FinalRenderList.push_back(TempRenderData);
                }
            }
        }
    }
    FinalRenderList.sort(CompareRenderData);
    for(auto &RenderIterator : FinalRenderList){
        if(RenderIterator.DTileIndex < DTilesets[RenderIterator.DType]->TileCount()){
            DTilesets[RenderIterator.DType]->DrawTile(drawable, TempGC, RenderIterator.DX, RenderIterator.DY, RenderIterator.DTileIndex, RenderIterator.DColorIndex);
            DTilesets[RenderIterator.DType]->DrawClipped(typedrawable, TempTypeGC, RenderIterator.DX, RenderIterator.DY, RenderIterator.DTileIndex, RenderIterator.DPixelColor);
        }
        else{
            DBuildingDeathTileset->DrawTile(drawable, TempGC, RenderIterator.DX, RenderIterator.DY, RenderIterator.DTileIndex);
        }
    }
    g_object_unref(TempTypeGC);
    g_object_unref(TempGC);
}

void CAssetRenderer::DrawSelections(GdkDrawable *drawable, const SRectangle &rect, const std::list< std::weak_ptr< CPlayerAsset > > &selectionlist, const SRectangle &selectrect, bool highlightbuilding){
    GdkGC *TempGC = gdk_gc_new(drawable);
    GdkColor RectangleColor(DTilesets[0]->PixelColor(DPixelIndices[pcMax]));
    int ScreenRightX = rect.DXPosition + rect.DWidth - 1;
    int ScreenBottomY = rect.DYPosition + rect.DHeight - 1;
    int SelectionX, SelectionY;
    
    if(highlightbuilding){
        RectangleColor = DTilesets[0]->PixelColor(DPixelIndices[pcMax + 2]);
        gdk_gc_set_rgb_fg_color(TempGC, &RectangleColor);
        
        for(auto &AssetIterator : DPlayerMap->Assets()){
            SAssetRenderData TempRenderData;
            TempRenderData.DType = AssetIterator->Type();
            if(atNone == TempRenderData.DType){
                continue;   
            }
            if((0 <= TempRenderData.DType)&&(TempRenderData.DType < DTilesets.size())){
                if(0 == AssetIterator->Speed()){
                    int RightX;
                    int Offset = atGoldMine == TempRenderData.DType ? 1 : 0;
                    
                    TempRenderData.DX = AssetIterator->PositionX() + (AssetIterator->Size() - 1) * CPosition::HalfTileWidth() - DTilesets[TempRenderData.DType]->TileHalfWidth();
                    TempRenderData.DY = AssetIterator->PositionY() + (AssetIterator->Size() - 1) * CPosition::HalfTileHeight() - DTilesets[TempRenderData.DType]->TileHalfHeight();
                    TempRenderData.DX -= Offset * CPosition::TileWidth();
                    TempRenderData.DY -= Offset * CPosition::TileHeight();
                    //printf("%d @ (%d, %d)\n",TempRenderData.DType, TempRenderData.DX , TempRenderData.DY);
                    RightX = TempRenderData.DX + DTilesets[TempRenderData.DType]->TileWidth() + (2 * Offset * CPosition::TileWidth()) - 1;
                    TempRenderData.DBottomY = TempRenderData.DY + DTilesets[TempRenderData.DType]->TileHeight() + (2 * Offset * CPosition::TileHeight()) - 1;
                    bool OnScreen = true;
                    if((RightX < rect.DXPosition)||(TempRenderData.DX > ScreenRightX)){
                        OnScreen = false;
                    }
                    else if((TempRenderData.DBottomY < rect.DYPosition)||(TempRenderData.DY > ScreenBottomY)){
                        OnScreen = false;   
                    }
                    TempRenderData.DX -= rect.DXPosition;
                    TempRenderData.DY -= rect.DYPosition;
                    if(OnScreen){
                        gdk_draw_rectangle(drawable, TempGC, FALSE, TempRenderData.DX, TempRenderData.DY, DTilesets[TempRenderData.DType]->TileWidth() + (2 * Offset * CPosition::TileWidth()), DTilesets[TempRenderData.DType]->TileHeight() + (2 * Offset * CPosition::TileHeight()));   
                    }
                }
            }
        }
        
        
        RectangleColor = DTilesets[0]->PixelColor(DPixelIndices[pcMax]);
    }
    
    
    gdk_gc_set_rgb_fg_color(TempGC, &RectangleColor);
    
    if(selectrect.DWidth && selectrect.DHeight){
        SelectionX = selectrect.DXPosition - rect.DXPosition;
        SelectionY = selectrect.DYPosition - rect.DYPosition;
        
        gdk_draw_rectangle(drawable, TempGC, FALSE, SelectionX, SelectionY, selectrect.DWidth, selectrect.DHeight);
    }
    
    if(selectionlist.size()){
        if(auto Asset = selectionlist.front().lock()){
            if(pcNone == Asset->Color()){
                RectangleColor = DTilesets[0]->PixelColor(DPixelIndices[pcNone]);
            }
            else if(DPlayerData->Color() != Asset->Color()){
                RectangleColor = DTilesets[0]->PixelColor(DPixelIndices[pcMax+1]);
            }
            gdk_gc_set_rgb_fg_color(TempGC, &RectangleColor);
        }
    }
    
    for(auto &AssetIterator : selectionlist){
        if(auto LockedAsset = AssetIterator.lock()){
            SAssetRenderData TempRenderData;
            TempRenderData.DType = LockedAsset->Type();
            if(atNone == TempRenderData.DType){
                if(aaDecay == LockedAsset->Action()){
                    int RightX;
                    bool OnScreen = true;
                    
                    TempRenderData.DX = LockedAsset->PositionX() - DCorpseTileset->TileWidth()/2;
                    TempRenderData.DY = LockedAsset->PositionY() - DCorpseTileset->TileHeight()/2;
                    RightX = TempRenderData.DX + DCorpseTileset->TileWidth();
                    TempRenderData.DBottomY = TempRenderData.DY + DCorpseTileset->TileHeight();
                    
                    if((RightX < rect.DXPosition)||(TempRenderData.DX > ScreenRightX)){
                        OnScreen = false;
                    }
                    else if((TempRenderData.DBottomY < rect.DYPosition)||(TempRenderData.DY > ScreenBottomY)){
                        OnScreen = false;
                    }
                    TempRenderData.DX -= rect.DXPosition;
                    TempRenderData.DY -= rect.DYPosition;
                    if(OnScreen){
                        int  ActionSteps = DCorpseIndices.size();
                        ActionSteps /= dMax;
                        if(ActionSteps){
                            int CurrentStep = LockedAsset->Step() / (DAnimationDownsample * TARGET_FREQUENCY);
                            if(CurrentStep >= ActionSteps){
                                CurrentStep = ActionSteps - 1;
                            }
                            TempRenderData.DTileIndex = DCorpseIndices[LockedAsset->Direction() * ActionSteps + CurrentStep];
                        }

                        DCorpseTileset->DrawTile(drawable, TempGC, TempRenderData.DX, TempRenderData.DY, TempRenderData.DTileIndex);
                    }
                }
                else if(aaAttack != LockedAsset->Action()){
                    int RightX;
                    bool OnScreen = true;
                    
                    TempRenderData.DX = LockedAsset->PositionX() - DMarkerTileset->TileWidth()/2;
                    TempRenderData.DY = LockedAsset->PositionY() - DMarkerTileset->TileHeight()/2;
                    RightX = TempRenderData.DX + DMarkerTileset->TileWidth();
                    TempRenderData.DBottomY = TempRenderData.DY + DMarkerTileset->TileHeight();
                    
                    if((RightX < rect.DXPosition)||(TempRenderData.DX > ScreenRightX)){
                        OnScreen = false;
                    }
                    else if((TempRenderData.DBottomY < rect.DYPosition)||(TempRenderData.DY > ScreenBottomY)){
                        OnScreen = false;
                    }
                    TempRenderData.DX -= rect.DXPosition;
                    TempRenderData.DY -= rect.DYPosition;
                    if(OnScreen){
                        int MarkerIndex = LockedAsset->Step() / DAnimationDownsample;
                        if(MarkerIndex < DMarkerIndices.size()){
                            DMarkerTileset->DrawTile(drawable, TempGC, TempRenderData.DX, TempRenderData.DY, DMarkerIndices[MarkerIndex]);
                        }
                    }
                }
            }
            else if((0 <= TempRenderData.DType)&&(TempRenderData.DType < DTilesets.size())){
                int RightX, RectWidth, RectHeight;
                bool OnScreen = true;
                
                TempRenderData.DX = LockedAsset->PositionX() - CPosition::HalfTileWidth();
                TempRenderData.DY = LockedAsset->PositionY() - CPosition::HalfTileHeight();
                RectWidth = CPosition::TileWidth() * LockedAsset->Size();
                RectHeight = CPosition::TileHeight() * LockedAsset->Size();
                RightX = TempRenderData.DX + RectWidth;
                TempRenderData.DBottomY = TempRenderData.DY + RectHeight;
                if((RightX < rect.DXPosition)||(TempRenderData.DX > ScreenRightX)){
                    OnScreen = false;
                }
                else if((TempRenderData.DBottomY < rect.DYPosition)||(TempRenderData.DY > ScreenBottomY)){
                    OnScreen = false;
                }
                else if((aaMineGold == LockedAsset->Action())||(aaConveyLumber == LockedAsset->Action())||(aaConveyGold == LockedAsset->Action())){
                    OnScreen = false;
                }
                TempRenderData.DX -= rect.DXPosition;
                TempRenderData.DY -= rect.DYPosition;
                if(OnScreen){
                    gdk_draw_rectangle(drawable, TempGC, FALSE, TempRenderData.DX, TempRenderData.DY, RectWidth, RectHeight);
                }
            }
        }
    }
    g_object_unref(TempGC);
}

void CAssetRenderer::DrawOverlays(GdkDrawable *drawable, const SRectangle &rect){
    GdkGC *TempGC = gdk_gc_new(drawable);
    int ScreenRightX = rect.DXPosition + rect.DWidth - 1;
    int ScreenBottomY = rect.DYPosition + rect.DHeight - 1;
    
    for(auto &AssetIterator : DPlayerMap->Assets()){
        SAssetRenderData TempRenderData;
        TempRenderData.DType = AssetIterator->Type();
        if(atNone == TempRenderData.DType){
            if(aaAttack == AssetIterator->Action()){
                int RightX;
                bool OnScreen = true;
                
                TempRenderData.DX = AssetIterator->PositionX() - DArrowTileset->TileWidth()/2;
                TempRenderData.DY = AssetIterator->PositionY() - DArrowTileset->TileHeight()/2;
                RightX = TempRenderData.DX + DArrowTileset->TileWidth();
                TempRenderData.DBottomY = TempRenderData.DY + DArrowTileset->TileHeight();
                
                if((RightX < rect.DXPosition)||(TempRenderData.DX > ScreenRightX)){
                    OnScreen = false;
                }
                else if((TempRenderData.DBottomY < rect.DYPosition)||(TempRenderData.DY > ScreenBottomY)){
                    OnScreen = false;
                }
                TempRenderData.DX -= rect.DXPosition;
                TempRenderData.DY -= rect.DYPosition;
                if(OnScreen){
                    int ActionSteps = DArrowIndices.size();
                    ActionSteps /= dMax;
                    
                    DArrowTileset->DrawTile(drawable, TempGC, TempRenderData.DX, TempRenderData.DY, DArrowIndices[AssetIterator->Direction() * ActionSteps + ((DPlayerData->GameCycle() - AssetIterator->CreationCycle()) % ActionSteps )]);
                }
            }
        }
        else if(0 == AssetIterator->Speed()){
            EAssetAction CurrentAction = AssetIterator->Action();
            
            if(aaDeath != CurrentAction){
                int HitRange = AssetIterator->HitPoints() * DFireTilesets.size() * 2 / AssetIterator->MaxHitPoints();
                
                if(aaConstruct == CurrentAction){
                    auto Command = AssetIterator->CurrentCommand();

                    if(Command.DAssetTarget){
                        Command = Command.DAssetTarget->CurrentCommand();
                        if(Command.DActivatedCapability){
                            int Divisor = Command.DActivatedCapability->PercentComplete(AssetIterator->MaxHitPoints());
                            Divisor = Divisor ? Divisor : 1;
                            HitRange = AssetIterator->HitPoints() * DFireTilesets.size() * 2 / Divisor;
                        }
                    }
                    else if(Command.DActivatedCapability){
                        int Divisor = Command.DActivatedCapability->PercentComplete(AssetIterator->MaxHitPoints());
                        Divisor = Divisor ? Divisor : 1;
                        HitRange = AssetIterator->HitPoints() * DFireTilesets.size() * 2 / Divisor;
                    }
                }
                
                if(HitRange < DFireTilesets.size()){
                    int TilesetIndex = DFireTilesets.size() - 1 - HitRange;
                    int RightX;
                    
                    TempRenderData.DTileIndex = (DPlayerData->GameCycle() - AssetIterator->CreationCycle()) % DFireTilesets[TilesetIndex]->TileCount();
                    TempRenderData.DX = AssetIterator->PositionX() + (AssetIterator->Size() - 1) * CPosition::HalfTileWidth() - DFireTilesets[TilesetIndex]->TileHalfWidth();
                    TempRenderData.DY = AssetIterator->PositionY() + (AssetIterator->Size() - 1) * CPosition::HalfTileHeight() - DFireTilesets[TilesetIndex]->TileHeight();

                    RightX = TempRenderData.DX + DFireTilesets[TilesetIndex]->TileWidth() - 1;
                    TempRenderData.DBottomY = TempRenderData.DY + DFireTilesets[TilesetIndex]->TileHeight() - 1;
                    bool OnScreen = true;
                    if((RightX < rect.DXPosition)||(TempRenderData.DX > ScreenRightX)){
                        OnScreen = false;
                    }
                    else if((TempRenderData.DBottomY < rect.DYPosition)||(TempRenderData.DY > ScreenBottomY)){
                        OnScreen = false;   
                    }
                    TempRenderData.DX -= rect.DXPosition;
                    TempRenderData.DY -= rect.DYPosition;
                    if(OnScreen){
                        DFireTilesets[TilesetIndex]->DrawTile(drawable, TempGC, TempRenderData.DX, TempRenderData.DY, TempRenderData.DTileIndex);   
                    }
                }
            }
        }
    }
    g_object_unref(TempGC);
}

void CAssetRenderer::DrawPlacement(GdkDrawable *drawable, const SRectangle &rect, const CPosition &pos, EAssetType type, std::shared_ptr< CPlayerAsset > builder){
    GdkGC *TempGC = gdk_gc_new(drawable);
    int ScreenRightX = rect.DXPosition + rect.DWidth - 1;
    int ScreenBottomY = rect.DYPosition + rect.DHeight - 1;

    if(atNone != type){
        CPosition TempPosition, TempTilePosition;
        int PlacementRightX, PlacementBottomY;
        bool OnScreen = true;
        auto AssetType = CPlayerAssetType::FindDefaultFromType(type);
        std::vector< std::vector< int > > PlacementTiles;
        int XOff, YOff;
        
        TempTilePosition.SetToTile(pos);
        TempPosition.SetFromTile(TempTilePosition);
        
        TempPosition.IncrementX( (AssetType->Size() - 1) * CPosition::HalfTileWidth() - DTilesets[type]->TileHalfWidth() );
        TempPosition.IncrementY( (AssetType->Size() - 1) * CPosition::HalfTileHeight() - DTilesets[type]->TileHalfHeight() );
        PlacementRightX = TempPosition.X() + DTilesets[type]->TileWidth();
        PlacementBottomY = TempPosition.Y() + DTilesets[type]->TileHeight();
        
        TempTilePosition.SetToTile(TempPosition);
        XOff = 0;
        YOff = 0;
        PlacementTiles.resize(AssetType->Size());
        for(auto &Row : PlacementTiles){
            Row.resize(AssetType->Size());
            for(auto &Cell : Row){
                auto TileType = DPlayerMap->TileType(TempTilePosition.X() + XOff, TempTilePosition.Y() + YOff);
                if((CTerrainMap::ttGrass == TileType)||(CTerrainMap::ttDirt == TileType)||(CTerrainMap::ttStump == TileType)||(CTerrainMap::ttRubble == TileType)){
                    Cell = 1;    
                }
                else{
                    Cell = 0;
                }
                XOff++;
            }
            XOff = 0;
            YOff++;
        }
        XOff = TempTilePosition.X() + AssetType->Size();
        YOff = TempTilePosition.Y() + AssetType->Size();
        for(auto PlayerAsset : DPlayerMap->Assets()){
            int MinX, MaxX, MinY, MaxY;
            int Offset = atGoldMine == PlayerAsset->Type() ? 1 : 0;
            
            if(builder == PlayerAsset){
                continue;   
            }
            if(XOff <= PlayerAsset->TilePositionX() - Offset){
                continue;   
            }
            if(TempTilePosition.X() >= (PlayerAsset->TilePositionX() + PlayerAsset->Size() + Offset)){
                continue;   
            }
            if(YOff <= PlayerAsset->TilePositionY() - Offset){
                continue;   
            }
            if(TempTilePosition.Y() >= (PlayerAsset->TilePositionY() + PlayerAsset->Size() + Offset)){
                continue;   
            }
            MinX = MAX(TempTilePosition.X(), PlayerAsset->TilePositionX() - Offset);
            MaxX = MIN(XOff, PlayerAsset->TilePositionX() + PlayerAsset->Size() + Offset);
            MinY = MAX(TempTilePosition.Y(), PlayerAsset->TilePositionY() - Offset);
            MaxY = MIN(YOff, PlayerAsset->TilePositionY() + PlayerAsset->Size() + Offset);
            for(int Y = MinY; Y < MaxY; Y++){
                for(int X = MinX; X < MaxX; X++){
                    PlacementTiles[Y - TempTilePosition.Y()][X - TempTilePosition.X()] = 0;
                }
            }
        }
        
        if(PlacementRightX <= rect.DXPosition){
            OnScreen = false;
        }
        else if(PlacementBottomY <= rect.DYPosition){
            OnScreen = false;
        }
        else if(TempPosition.X() >= ScreenRightX){
            OnScreen = false;
        }
        else if(TempPosition.Y() >= ScreenBottomY){
            OnScreen = false;
        }
        if(OnScreen){
            int XPos, YPos;
            TempPosition.X(TempPosition.X() - rect.DXPosition);
            TempPosition.Y(TempPosition.Y() - rect.DYPosition);
            DTilesets[type]->DrawTile(drawable, TempGC, TempPosition.X(), TempPosition.Y(), DPlaceIndices[type][0], DPlayerData->Color() - 1);
            XPos = TempPosition.X();
            YPos = TempPosition.Y();
            for(auto &Row : PlacementTiles){
                for(auto &Cell : Row){
                    DMarkerTileset->DrawTile(drawable, TempGC, XPos, YPos, Cell ? DPlaceGoodIndex : DPlaceBadIndex);
                    XPos += DMarkerTileset->TileWidth();
                }
                YPos += DMarkerTileset->TileHeight();
                XPos = TempPosition.X();
            }
            
        }
    }

    g_object_unref(TempGC);
}

void CAssetRenderer::DrawMiniAssets(GdkDrawable *drawable){
    GdkGC *TempGC = gdk_gc_new(drawable);
    if(nullptr != DPlayerData){
        for(auto &AssetIterator : DPlayerMap->Assets()){
            EPlayerColor AssetColor = AssetIterator->Color();
            if(AssetColor == DPlayerData->Color()){
                AssetColor = pcMax;
            }
            DTilesets[0]->DrawPixel(drawable, TempGC, AssetIterator->TilePositionX(), AssetIterator->TilePositionY(), AssetIterator->Size(), DPixelIndices[AssetColor]);
        }
    }
    else{
        for(auto &AssetIterator : DPlayerMap->AssetInitializationList()){
            EPlayerColor AssetColor = AssetIterator.DColor;

            DTilesets[0]->DrawPixel(drawable, TempGC, AssetIterator.DTilePosition.X(), AssetIterator.DTilePosition.Y(), CPlayerAssetType::FindDefaultFromName(AssetIterator.DType)->Size(), DPixelIndices[AssetColor]);
        }
    }
    g_object_unref(TempGC);
}

