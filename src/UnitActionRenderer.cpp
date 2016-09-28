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
#include "UnitActionRenderer.h"

CUnitActionRenderer::CUnitActionRenderer(std::shared_ptr< CBevel > bevel, std::shared_ptr< CGraphicTileset > icons, EPlayerColor color, std::shared_ptr< CPlayerData > player){
    DIconTileset = icons;
    DBevel = bevel;
    DPlayerData = player;
    DPlayerColor = color;
    
    DCommandIndices.resize(actMax);
    DFullIconWidth = DIconTileset->TileWidth() + DBevel->Width() * 2;
    DFullIconHeight = DIconTileset->TileHeight() + DBevel->Width() * 2;
    DDisplayedCommands.resize(9);
    for(auto &Commands : DDisplayedCommands){
        Commands = actNone;
    }
    DCommandIndices.resize(actMax);                              
    DCommandIndices[actNone] = -1;
    DCommandIndices[actBuildPeasant] = DIconTileset->FindTile("peasant");
    DCommandIndices[actBuildFootman] = DIconTileset->FindTile("footman");
    DCommandIndices[actBuildArcher] = DIconTileset->FindTile("archer");
    DCommandIndices[actBuildRanger] = DIconTileset->FindTile("ranger");
    DCommandIndices[actBuildFarm] = DIconTileset->FindTile("chicken-farm");
    DCommandIndices[actBuildTownHall] = DIconTileset->FindTile("town-hall");
    DCommandIndices[actBuildBarracks] = DIconTileset->FindTile("human-barracks");
    DCommandIndices[actBuildLumberMill] = DIconTileset->FindTile("human-lumber-mill");
    DCommandIndices[actBuildBlacksmith] = DIconTileset->FindTile("human-blacksmith");
    DCommandIndices[actBuildKeep] = DIconTileset->FindTile("keep");
    DCommandIndices[actBuildCastle] = DIconTileset->FindTile("castle");
    DCommandIndices[actBuildScoutTower] = DIconTileset->FindTile("scout-tower");
    DCommandIndices[actBuildGuardTower] = DIconTileset->FindTile("human-guard-tower");
    DCommandIndices[actBuildCannonTower] = DIconTileset->FindTile("human-cannon-tower");
    DCommandIndices[actMove] = DIconTileset->FindTile("human-move");
    DCommandIndices[actRepair] = DIconTileset->FindTile("repair");
    DCommandIndices[actMine] = DIconTileset->FindTile("mine");
    DCommandIndices[actBuildSimple] = DIconTileset->FindTile("build-simple");
    DCommandIndices[actBuildAdvanced] = DIconTileset->FindTile("build-advanced");
    DCommandIndices[actConvey] = DIconTileset->FindTile("human-convey");
    DCommandIndices[actCancel] = DIconTileset->FindTile("cancel");
    DCommandIndices[actBuildWall] = DIconTileset->FindTile("human-wall");
    DCommandIndices[actAttack] = DIconTileset->FindTile("human-weapon-1");
    DCommandIndices[actStandGround] = DIconTileset->FindTile("human-armor-1");
    DCommandIndices[actPatrol] = DIconTileset->FindTile("human-patrol");
    DCommandIndices[actWeaponUpgrade1] = DIconTileset->FindTile("human-weapon-1");
    DCommandIndices[actWeaponUpgrade2] = DIconTileset->FindTile("human-weapon-2");
    DCommandIndices[actWeaponUpgrade3] = DIconTileset->FindTile("human-weapon-3");
    DCommandIndices[actArrowUpgrade1] = DIconTileset->FindTile("human-arrow-1");
    DCommandIndices[actArrowUpgrade2] = DIconTileset->FindTile("human-arrow-2");
    DCommandIndices[actArrowUpgrade3] = DIconTileset->FindTile("human-arrow-3");
    DCommandIndices[actArmorUpgrade1] = DIconTileset->FindTile("human-armor-1");
    DCommandIndices[actArmorUpgrade2] = DIconTileset->FindTile("human-armor-2");
    DCommandIndices[actArmorUpgrade3] = DIconTileset->FindTile("human-armor-3");
    DCommandIndices[actLongbow] = DIconTileset->FindTile("longbow");
    DCommandIndices[actRangerScouting] = DIconTileset->FindTile("ranger-scouting");
    DCommandIndices[actMarksmanship] = DIconTileset->FindTile("marksmanship");
    
    DDisabledIndex = DIconTileset->FindTile("disabled");
}

CUnitActionRenderer::~CUnitActionRenderer(){
    
}

int CUnitActionRenderer::MinimumWidth() const{
    return DFullIconWidth * 3 + DBevel->Width() * 2;
}

int CUnitActionRenderer::MinimumHeight() const{
    return DFullIconHeight * 3 + DBevel->Width() * 2;
}

EAssetCapabilityType CUnitActionRenderer::Selection(const CPosition &pos) const{
    if(((pos.X() % (DFullIconWidth + DBevel->Width())) < DFullIconWidth)&&((pos.Y() % (DFullIconHeight + DBevel->Width())) < DFullIconHeight)){
        int Index = (pos.X() / (DFullIconWidth + DBevel->Width())) + (pos.Y() / (DFullIconHeight + DBevel->Width())) * 3;
        return DDisplayedCommands[Index];
    }
    return actNone;
}

void CUnitActionRenderer::DrawUnitAction(GdkDrawable *drawable, const std::list< std::weak_ptr< CPlayerAsset > > &selectionlist, EAssetCapabilityType currentaction){
    bool AllSame = true;
    bool IsFirst = true;
    bool Moveable = true;
    bool HasCargo = false;
    EAssetType UnitType;
    
    for(auto &Command : DDisplayedCommands){
        Command = actNone;    
    }
    if(!selectionlist.size()){
        return;    
    }
    for(auto &Iterator : selectionlist){
        if(auto Asset = Iterator.lock()){
            if(DPlayerColor != Asset->Color()){
                return;   
            }
            if(IsFirst){
                UnitType = Asset->Type();     
                IsFirst = false;
                Moveable = 0 < Asset->Speed();
            }
            else if(UnitType != Asset->Type()){
                AllSame = false;   
            }
            if(Asset->Lumber() || Asset->Gold()){
                HasCargo = true;      
            }
        }
    }
    if(actNone == currentaction){
        if(Moveable){

            DDisplayedCommands[0] = HasCargo ? actConvey : actMove;
            DDisplayedCommands[1] = actStandGround;
            DDisplayedCommands[2] = actAttack;
            if(auto Asset = selectionlist.front().lock()){
                if(Asset->HasCapability(actRepair)){
                    DDisplayedCommands[3] = actRepair;
                }
                if(Asset->HasCapability(actPatrol)){
                    DDisplayedCommands[3] = actPatrol;
                }
                if(Asset->HasCapability(actMine)){
                    DDisplayedCommands[4] = actMine;
                }
                if(Asset->HasCapability(actBuildSimple) && (1 == selectionlist.size())){
                    DDisplayedCommands[6] = actBuildSimple;
                }
            }
        }
        else{
            if(auto Asset = selectionlist.front().lock()){
                if((aaConstruct == Asset->Action())||(aaCapability == Asset->Action())){
                    DDisplayedCommands.back() = actCancel;    
                }
                else{
                    int Index = 0;
                    for(auto &Capability : Asset->Capabilities()){
                        DDisplayedCommands[Index] = Capability;
                        Index++;
                        if(DDisplayedCommands.size() <= Index){
                            break;   
                        }
                    }
                }
            }
        }
    }
    else if(actBuildSimple == currentaction){
        if(auto Asset = selectionlist.front().lock()){
            int Index = 0;
            for(auto Capability : {actBuildFarm,
                                    actBuildTownHall,
                                    actBuildBarracks,
                                    actBuildLumberMill,
                                    actBuildBlacksmith,
                                    actBuildKeep,
                                    actBuildCastle,
                                    actBuildScoutTower,
                                    actBuildGuardTower,
                                    actBuildCannonTower }){
                if(Asset->HasCapability(Capability)){
                    DDisplayedCommands[Index] = Capability;
                    Index++;
                    if(DDisplayedCommands.size() <= Index){
                        break;   
                    }
                }
            }
            DDisplayedCommands.back() = actCancel;
        }
    }
    else{
        DDisplayedCommands.back() = actCancel;
    }
    
    gint XOffset = DBevel->Width();
    gint YOffset = DBevel->Width();
    int Index = 0;
    GdkGC *TempGC = gdk_gc_new(drawable);
    for(auto IconType : DDisplayedCommands){
        if(actNone != IconType){
            auto PlayerCapability = CPlayerCapability::FindCapability(IconType);
            DBevel->DrawBevel(drawable, XOffset, YOffset, DIconTileset->TileWidth(), DIconTileset->TileHeight());   
            DIconTileset->DrawTile(drawable, TempGC, XOffset, YOffset, DCommandIndices[IconType]);
            if(PlayerCapability){
                if(!PlayerCapability->CanInitiate(selectionlist.front().lock(), DPlayerData)){
                    DIconTileset->DrawTile(drawable, TempGC, XOffset, YOffset, DDisabledIndex);
                }
            }
        }
        
        XOffset += DFullIconWidth + DBevel->Width();
        Index++;
        if(0 == (Index % 3)){
            XOffset = DBevel->Width();
            YOffset += DFullIconHeight + DBevel->Width();  
        }
    }
    g_object_unref(TempGC);
}

