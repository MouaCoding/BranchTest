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
#include "SoundEventRenderer.h"
#include <unordered_map>

CSoundEventRenderer::CSoundEventRenderer(std::shared_ptr< CSoundLibraryMixer > mixer, std::shared_ptr< CPlayerData > player){
    std::string Names[atMax];
    
    Names[atNone] = "basic";
    Names[atPeasant] = "peasant";
    Names[atFootman] = "footman";
    Names[atArcher] = "archer";
    Names[atRanger] = "archer";
    Names[atGoldMine] = "gold-mine";
    Names[atTownHall] = "town-hall";
    Names[atKeep] = "keep";
    Names[atCastle] = "castle";
    Names[atFarm] = "farm";
    Names[atBarracks] = "barracks";
    Names[atLumberMill] = "lumber-mill";
    Names[atBlacksmith] = "blacksmith";
    Names[atScoutTower] = "scout-tower";
    Names[atGuardTower] = "guard-tower";
    Names[atCannonTower] = "cannon-tower";
    
    DSoundMixer = mixer;
    DPlayer = player;
    DVolume = 1.0;
    
    DDelayedSelectionIndices.resize(atMax);
    for(auto &Index : DDelayedSelectionIndices){
        Index = -1;    
    }
    DDelayedAcknowledgeIndices.resize(atMax);
    for(auto &Index : DDelayedAcknowledgeIndices){
        Index = -1;    
    }
    
    DWorkCompleteIndices.resize(atMax);
    DSelectionIndices.resize(atMax);
    DAcknowledgeIndices.resize(atMax);
    DReadyIndices.resize(atMax);
    DDeathIndices.resize(atMax);
    DAttackedIndices.resize(atMax);
    DMissleFireIndices.resize(atMax);
    DMissleHitIndices.resize(atMax);
    DHarvestIndices.resize(atMax);
    DMeleeHitIndices.resize(atMax);
    
    DPlaceIndex = DSoundMixer->FindClip("place");
    DTickIndex = DSoundMixer->FindClip("tick");
    DTockIndex = DSoundMixer->FindClip("tock");
    {
        int ClipIndex = DSoundMixer->FindClip("construct");
        if(0 <= ClipIndex){
            DConstructIndices.push_back(ClipIndex);
        }
        else{
            int SoundIndex = 1;
            while(true){
                ClipIndex = DSoundMixer->FindClip("construct" + std::to_string(SoundIndex));
                if(0 > ClipIndex){
                    break;   
                }
                DConstructIndices.push_back(ClipIndex);
                SoundIndex++;
            } 
        }
    }
    
    for(int TypeIndex = 0; TypeIndex < atMax; TypeIndex++){
        auto AssetType = DPlayer->AssetTypes()->find(CPlayerAssetType::TypeToName((EAssetType)TypeIndex))->second;
        int ClipIndex;
        ClipIndex = DSoundMixer->FindClip(Names[TypeIndex] + "-work-completed");
        if(0 <= ClipIndex){
            DWorkCompleteIndices[TypeIndex].push_back(ClipIndex);
        }
        else if(DWorkCompleteIndices[atNone].size()){
            DWorkCompleteIndices[TypeIndex].push_back(DWorkCompleteIndices[atNone].front());
        }
        ClipIndex = DSoundMixer->FindClip(Names[TypeIndex] + "-selected");
        if(0 <= ClipIndex){
            DSelectionIndices[TypeIndex].push_back(ClipIndex);
        }
        else{
            int SoundIndex = 1;
            while(true){
                ClipIndex = DSoundMixer->FindClip(Names[TypeIndex] + "-selected" + std::to_string(SoundIndex));
                if(0 > ClipIndex){
                    break;   
                }
                DSelectionIndices[TypeIndex].push_back(ClipIndex);
                SoundIndex++;
            }
            if(!DSelectionIndices[TypeIndex].size() && AssetType->Speed() && DSelectionIndices[atNone].size()){
                DSelectionIndices[TypeIndex] = DSelectionIndices[atNone];
            }
        }
        ClipIndex = DSoundMixer->FindClip(Names[TypeIndex] + "-acknowledge");
        if(0 <= ClipIndex){
            DAcknowledgeIndices[TypeIndex].push_back(ClipIndex);
        }
        else{
            int SoundIndex = 1;
            while(true){
                ClipIndex = DSoundMixer->FindClip(Names[TypeIndex] + "-acknowledge" + std::to_string(SoundIndex));
                if(0 > ClipIndex){
                    break;   
                }
                DAcknowledgeIndices[TypeIndex].push_back(ClipIndex);
                SoundIndex++;
            }
            if(!DAcknowledgeIndices[TypeIndex].size() && DAcknowledgeIndices[atNone].size()){
                DAcknowledgeIndices[TypeIndex] = DAcknowledgeIndices[atNone];
            }
        }
        ClipIndex = DSoundMixer->FindClip(Names[TypeIndex] + "-ready");
        if(0 <= ClipIndex){
            DReadyIndices[TypeIndex].push_back(ClipIndex);
        }
        else if(atFootman == TypeIndex){
            ClipIndex = DSoundMixer->FindClip(Names[atNone] + "-ready");
            if(0 <= ClipIndex){
                DReadyIndices[TypeIndex].push_back(ClipIndex);
            }
        }
        std::string UnitBuildingName = AssetType->Speed() ? "unit" : "building";
        ClipIndex = DSoundMixer->FindClip(UnitBuildingName + "-death");
        if(0 <= ClipIndex){
            DDeathIndices[TypeIndex].push_back(ClipIndex);
        }
        else{
            int SoundIndex = 1;
            while(true){
                ClipIndex = DSoundMixer->FindClip(UnitBuildingName + "-death" + std::to_string(SoundIndex));
                if(0 > ClipIndex){
                    break;   
                }
                DDeathIndices[TypeIndex].push_back(ClipIndex);
                SoundIndex++;
            }
            if(!DDeathIndices[TypeIndex].size() && DDeathIndices[atNone].size()){
                DDeathIndices[TypeIndex] = DDeathIndices[atNone];
            }
        }
        
        ClipIndex = DSoundMixer->FindClip(UnitBuildingName + "-help");
        if(0 <= ClipIndex){
            DAttackedIndices[TypeIndex].push_back(ClipIndex);
        }
        else{
            int SoundIndex = 1;
            while(true){
                ClipIndex = DSoundMixer->FindClip(UnitBuildingName + "-help" + std::to_string(SoundIndex));
                if(0 > ClipIndex){
                    break;   
                }
                DAttackedIndices[TypeIndex].push_back(ClipIndex);
                SoundIndex++;
            }
            if(!DAttackedIndices[TypeIndex].size() && DAttackedIndices[atNone].size()){
                DAttackedIndices[TypeIndex] = DAttackedIndices[atNone];
            }
        }
        if((atArcher == TypeIndex)||(atRanger == TypeIndex)||(atGuardTower == TypeIndex)){
            DMissleFireIndices[TypeIndex].push_back(DSoundMixer->FindClip("bowfire"));
            DMissleHitIndices[TypeIndex].push_back(DSoundMixer->FindClip("bowhit"));   
        }
        else if(atCannonTower == TypeIndex){
            DMissleFireIndices[TypeIndex].push_back(DSoundMixer->FindClip("cannonfire"));
            DMissleHitIndices[TypeIndex].push_back(DSoundMixer->FindClip("cannonhit"));   
        }
        ClipIndex = DSoundMixer->FindClip("harvest");
        if(0 <= ClipIndex){
            DAttackedIndices[TypeIndex].push_back(ClipIndex);
        }
        else{
            int SoundIndex = 1;
            while(true){
                ClipIndex = DSoundMixer->FindClip("harvest" + std::to_string(SoundIndex));
                if(0 > ClipIndex){
                    break;   
                }
                DHarvestIndices[TypeIndex].push_back(ClipIndex);
                SoundIndex++;
            }
            if(!DHarvestIndices[TypeIndex].size() && DHarvestIndices[atNone].size()){
                DHarvestIndices[TypeIndex] = DHarvestIndices[atNone];
            }
        }
        if(1 == AssetType->Range()){
            ClipIndex = DSoundMixer->FindClip("melee-hit");
            if(0 <= ClipIndex){
                DMeleeHitIndices[TypeIndex].push_back(ClipIndex);
            }
            else{
                int SoundIndex = 1;
                while(true){
                    ClipIndex = DSoundMixer->FindClip("melee-hit" + std::to_string(SoundIndex));
                    if(0 > ClipIndex){
                        break;   
                    }
                    DMeleeHitIndices[TypeIndex].push_back(ClipIndex);
                    SoundIndex++;
                }
                if(!DMeleeHitIndices[TypeIndex].size() && DMeleeHitIndices[atNone].size()){
                    DMeleeHitIndices[TypeIndex] = DMeleeHitIndices[atNone];
                }
            }
        }
    }
    
}

float CSoundEventRenderer::RightBias(const SRectangle &viewportrect, const CPosition &position){
    int LeftX, RightX, CenterX;
    
    LeftX = viewportrect.DXPosition;
    RightX = viewportrect.DXPosition + viewportrect.DWidth - 1;
    CenterX = (LeftX + RightX) / 2;
    
    if(position.X() <= LeftX){
        return -1.0;   
    }
    if(position.X() >= RightX){
        return 1.0;
    }
    if(LeftX == CenterX){
        return 0.0;   
    }
    if(position.X() < CenterX){
        return 1.0 - ((float)(position.X() - LeftX)) / (CenterX - LeftX);
    }
    return ((float)(position.X() - CenterX)) / (RightX - CenterX);
}

bool CSoundEventRenderer::OnScreen(const SRectangle &viewportrect, const CPosition &position){
    int LeftX, RightX, TopY, BottomY;
    
    LeftX = viewportrect.DXPosition - CPosition::TileWidth();
    if(position.X() < LeftX){
        return false;    
    }
    RightX = viewportrect.DXPosition + viewportrect.DWidth + CPosition::TileWidth()- 1;
    if(position.X() > RightX){
        return false;    
    }
    TopY = viewportrect.DYPosition - CPosition::TileHeight();
    if(position.Y() < TopY){
        return false;    
    }
    BottomY = viewportrect.DYPosition + viewportrect.DHeight + CPosition::TileHeight()- 1;
    if(position.Y() > BottomY){
        return false;    
    }
    return true;
}

void CSoundEventRenderer::RenderEvents(const SRectangle &viewportrect){
    int MainRandomNumber = DRandomNumberGenerator.Random();
    std::vector< SGameEvent > AllEvents = DDelayedEvents;
    std::vector< int > Selections, Acknowledges;
    
    DDelayedEvents.clear();
    Selections.resize(atMax);
    Acknowledges.resize(atMax);
    
    AllEvents.insert(AllEvents.end(), DPlayer->GameEvents().begin(), DPlayer->GameEvents().end());
    for(auto Event : AllEvents){
        if(etSelection == Event.DType){
            if(Event.DAsset){
                if((pcNone == Event.DAsset->Color())||(DPlayer->Color() == Event.DAsset->Color())){
                    if(aaConstruct == Event.DAsset->Action()){
                        if(DConstructIndices.size()){
                            int RandomClip = MainRandomNumber % DConstructIndices.size();
                            
                            DSoundMixer->PlayClip(DConstructIndices[RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                        }
                    }
                    else if(DSelectionIndices[Event.DAsset->Type()].size()){
                        if(1 > Selections[Event.DAsset->Type()]){
                            int RandomClip = DRandomNumberGenerator.Random() % DSelectionIndices[Event.DAsset->Type()].size();
                            if(0 > DDelayedSelectionIndices[Event.DAsset->Type()]){
                                DDelayedSelectionIndices[Event.DAsset->Type()] = RandomClip;
                            }
                            else{
                                RandomClip = DDelayedSelectionIndices[Event.DAsset->Type()];    
                            }
                            DSoundMixer->PlayClip(DSelectionIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                            Selections[Event.DAsset->Type()]++;
                        }
                        else if(0 == (DRandomNumberGenerator.Random() & 0x3)){
                            DDelayedEvents.push_back(Event);
                        }
                    }
                }
            }
        }
        else if(etAcknowledge == Event.DType){
            if(Event.DAsset){
                if((pcNone == Event.DAsset->Color())||(DPlayer->Color() == Event.DAsset->Color())){
                    if(DAcknowledgeIndices[Event.DAsset->Type()].size()){
                        if(1 > Acknowledges[Event.DAsset->Type()]){
                            int RandomClip = MainRandomNumber % DAcknowledgeIndices[Event.DAsset->Type()].size();
                            
                            if(0 > DDelayedAcknowledgeIndices[Event.DAsset->Type()]){
                                DDelayedAcknowledgeIndices[Event.DAsset->Type()] = RandomClip;
                            }
                            else{
                                RandomClip = DDelayedAcknowledgeIndices[Event.DAsset->Type()];    
                            }
                            DSoundMixer->PlayClip(DAcknowledgeIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                            Acknowledges[Event.DAsset->Type()]++;
                        }
                        else if(0 == (DRandomNumberGenerator.Random() & 0x3)){
                            DDelayedEvents.push_back(Event);   
                        }
                    }
                }
            }
        }
        else if(etWorkComplete == Event.DType){
            if(Event.DAsset){
                if(DPlayer->Color() == Event.DAsset->Color()){
                    if(DWorkCompleteIndices[Event.DAsset->Type()].size()){
                        int RandomClip = DRandomNumberGenerator.Random() % DWorkCompleteIndices[Event.DAsset->Type()].size();
                        
                        DSoundMixer->PlayClip(DWorkCompleteIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                    }
                }
            }
        }
        else if(etReady == Event.DType){
            if(Event.DAsset){
                if((pcNone == Event.DAsset->Color())||(DPlayer->Color() == Event.DAsset->Color())){
                    if(DReadyIndices[Event.DAsset->Type()].size()){
                        int RandomClip = DRandomNumberGenerator.Random() % DReadyIndices[Event.DAsset->Type()].size();
                        
                        DSoundMixer->PlayClip(DReadyIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                    }
                }
            }
        }
        else if(etDeath == Event.DType){
            if(Event.DAsset){
                if(OnScreen(viewportrect, Event.DAsset->Position())){
                    if(DDeathIndices[Event.DAsset->Type()].size()){
                        int RandomClip = DRandomNumberGenerator.Random() % DDeathIndices[Event.DAsset->Type()].size();
                        
                        DSoundMixer->PlayClip(DDeathIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                    }
                }
            }
        }
        else if(etAttacked == Event.DType){
            if(Event.DAsset){
                if(!OnScreen(viewportrect, Event.DAsset->Position())){
                    if(DAttackedIndices[Event.DAsset->Type()].size()){
                        int RandomClip = DRandomNumberGenerator.Random() % DAttackedIndices[Event.DAsset->Type()].size();
                        
                        DSoundMixer->PlayClip(DAttackedIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                    }
                }
            }
        }
        else if(etMissleFire == Event.DType){
            if(Event.DAsset){
                if(OnScreen(viewportrect, Event.DAsset->Position())){
                    if(DMissleFireIndices[Event.DAsset->Type()].size()){
                        int RandomClip = DRandomNumberGenerator.Random() % DMissleFireIndices[Event.DAsset->Type()].size();
                        
                        DSoundMixer->PlayClip(DMissleFireIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                    }
                }
            }
        }
        else if(etMissleHit == Event.DType){
            if(Event.DAsset){
                if(OnScreen(viewportrect, Event.DAsset->Position())){
                    auto CreationCommand = Event.DAsset->NextCommand(); // Find out type of missle
                    
                    if((aaConstruct == CreationCommand.DAction) && CreationCommand.DAssetTarget){
                        if(DMissleHitIndices[CreationCommand.DAssetTarget->Type()].size()){
                            int RandomClip = DRandomNumberGenerator.Random() % DMissleHitIndices[CreationCommand.DAssetTarget->Type()].size();
                            
                            DSoundMixer->PlayClip(DMissleHitIndices[CreationCommand.DAssetTarget->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                        }
                    }
                }
            }
        }
        else if(etHarvest == Event.DType){
            if(Event.DAsset){
                if(OnScreen(viewportrect, Event.DAsset->Position()) && (Event.DAsset->AttackSteps()-1 == (Event.DAsset->Step() % Event.DAsset->AttackSteps()))){
                    if(DHarvestIndices[Event.DAsset->Type()].size()){
                        int RandomClip = DRandomNumberGenerator.Random() % DHarvestIndices[Event.DAsset->Type()].size();
                        
                        DSoundMixer->PlayClip(DHarvestIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                    }
                }
            }
        }
        else if(etMeleeHit == Event.DType){
            if(Event.DAsset){
                if(OnScreen(viewportrect, Event.DAsset->Position())){
                    if(DMeleeHitIndices[Event.DAsset->Type()].size()){
                        int RandomClip = DRandomNumberGenerator.Random() % DMeleeHitIndices[Event.DAsset->Type()].size();
                        
                        DSoundMixer->PlayClip(DMeleeHitIndices[Event.DAsset->Type()][RandomClip], DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                    }
                }
            }
        }
        else if(etPlaceAction == Event.DType){
            if(Event.DAsset){
                if(0 <= DPlaceIndex){
                    DSoundMixer->PlayClip(DPlaceIndex, DVolume, RightBias(viewportrect, Event.DAsset->Position()));
                }
            }
               
        }
        else if(etButtonTick == Event.DType){
            if(0 <= DTickIndex){
                DSoundMixer->PlayClip(DTickIndex, DVolume, 0.0);
            }
        }
    }
    
    for(int Index = 0; Index < atMax; Index++){
        if(0 == Selections[Index]){
            DDelayedSelectionIndices[Index] = -1;
        }
        if(0 == Acknowledges[Index]){
            DDelayedAcknowledgeIndices[Index] = -1;
        }
    }

}

