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
#include "AIPlayer.h"
#include "Debug.h"

CAIPlayer::CAIPlayer(std::shared_ptr< CPlayerData > playerdata, int downsample){
    DPlayerData = playerdata;
    DCycle = 0;
    DDownSample = downsample;
}

bool CAIPlayer::SearchMap(SPlayerCommandRequest &command){
    auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > MovableAsset;
    
    for(auto WeakAsset : IdleAssets){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->Speed()){
                MovableAsset = Asset;
                break;
            }
        }
    }
    if(MovableAsset){
        CPosition UnknownPosition = DPlayerData->PlayerMap()->FindNearestReachableTileType(MovableAsset->TilePosition(), CTerrainMap::ttNone);
        
        if(0 <= UnknownPosition.X()){
            //printf("Unkown Position (%d, %d)\n", UnknownPosition.X(), UnknownPosition.Y());
            command.DAction = actMove;
            command.DActors.push_back(MovableAsset);
            command.DTargetLocation.SetFromTile(UnknownPosition);
            return true;
        }
    }
    return false;
}

bool CAIPlayer::FindEnemies(SPlayerCommandRequest &command){
    std::shared_ptr< CPlayerAsset > TownHallAsset;
    
    for(auto WeakAsset : DPlayerData->Assets()){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(actBuildPeasant)){
                TownHallAsset = Asset;
                break;
            }
        }
    }
    if(DPlayerData->FindNearestEnemy(TownHallAsset->Position(), -1).expired()){
        return SearchMap(command);
    }
    return false;    
}


bool CAIPlayer::AttackEnemies(SPlayerCommandRequest &command){
    CPosition AverageLocation(0,0);
    
    for(auto WeakAsset : DPlayerData->Assets()){
        if(auto Asset = WeakAsset.lock()){
            if((atFootman == Asset->Type())||(atArcher == Asset->Type())||(atRanger == Asset->Type())){
                if(!Asset->HasAction(aaAttack)){
                    command.DActors.push_back(Asset);
                    AverageLocation.IncrementX(Asset->PositionX());
                    AverageLocation.IncrementY(Asset->PositionY());
                }
            }
        }
    }
    if(command.DActors.size()){
        AverageLocation.X(AverageLocation.X() / command.DActors.size());
        AverageLocation.Y(AverageLocation.Y() / command.DActors.size());
        
        auto TargetEnemy = DPlayerData->FindNearestEnemy(AverageLocation, -1).lock();
        if(!TargetEnemy){
            command.DActors.clear();
            return SearchMap(command);
        }
        command.DAction = actAttack;
        command.DTargetLocation = TargetEnemy->Position();
        command.DTargetColor = TargetEnemy->Color();
        command.DTargetType = TargetEnemy->Type(); 
        return true;
    }
    return false;    
}

bool CAIPlayer::BuildTownHall(SPlayerCommandRequest &command){
    // Build Town Hall
    auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > BuilderAsset;
    
    for(auto WeakAsset : IdleAssets){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(actBuildTownHall)){
                BuilderAsset = Asset;
                break;
            }
        }
    }
    if(BuilderAsset){
        auto GoldMineAsset = DPlayerData->FindNearestAsset(BuilderAsset->Position(), atGoldMine);
        CPosition Placement = DPlayerData->FindBestAssetPlacement(GoldMineAsset->TilePosition(), BuilderAsset, atTownHall, 1);
        if(0 <= Placement.X()){
            command.DAction = actBuildTownHall;
            command.DActors.push_back(BuilderAsset);
            command.DTargetLocation.SetFromTile(Placement);
            return true;
        }
        else{
            return SearchMap(command);  
        }
    }
    return false;
}

bool CAIPlayer::BuildBuilding(SPlayerCommandRequest &command, EAssetType buildingtype, EAssetType neartype){
    std::shared_ptr< CPlayerAsset > BuilderAsset;
    std::shared_ptr< CPlayerAsset > TownHallAsset;
    std::shared_ptr< CPlayerAsset > NearAsset;
    EAssetCapabilityType BuildAction;
    bool AssetIsIdle = false;
    
    switch(buildingtype){
        case atBarracks:    BuildAction = actBuildBarracks;
                            break;
        case atLumberMill:  BuildAction = actBuildLumberMill;
                            break;
        case atBlacksmith:  BuildAction = actBuildBlacksmith;
                            break;
        default:            BuildAction = actBuildFarm;
                            break;
    }

    for(auto WeakAsset : DPlayerData->Assets()){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(BuildAction) && Asset->Interruptible()){
                if(!BuilderAsset || (!AssetIsIdle && (aaNone == Asset->Action()))){
                    BuilderAsset = Asset;
                    AssetIsIdle = aaNone == Asset->Action();
                }
            }
            if(Asset->HasCapability(actBuildPeasant)){
                TownHallAsset = Asset;
            }
            if(Asset->HasActiveCapability(BuildAction)){
                return false;    
            }
            if((neartype == Asset->Type())&&(aaConstruct != Asset->Action())){
                NearAsset = Asset;
            }
            if(buildingtype == Asset->Type()){
                if(aaConstruct == Asset->Action()){
                    return false;   
                }
            }
        }
    }
    if((buildingtype != neartype) && !NearAsset){
        return false;    
    }
    if(BuilderAsset){
        auto PlayerCapability = CPlayerCapability::FindCapability(BuildAction); 
        CPosition SourcePosition = TownHallAsset->TilePosition();
        CPosition MapCenter(DPlayerData->PlayerMap()->Width()/2, DPlayerData->PlayerMap()->Height()/2);
        
        
        if(NearAsset){
            SourcePosition = NearAsset->TilePosition();
        }
        if(MapCenter.X() < SourcePosition.X()){
            SourcePosition.DecrementX(TownHallAsset->Size()/2);   
        }
        else if(MapCenter.X() > SourcePosition.X()){
            SourcePosition.IncrementX(TownHallAsset->Size()/2);
        }
        if(MapCenter.Y() < SourcePosition.Y()){
            SourcePosition.DecrementY(TownHallAsset->Size()/2);   
        }
        else if(MapCenter.Y() > SourcePosition.Y()){
            SourcePosition.IncrementY(TownHallAsset->Size()/2);
        }

        CPosition Placement = DPlayerData->FindBestAssetPlacement(SourcePosition, BuilderAsset, buildingtype, 1);
        if(0 > Placement.X()){
            return SearchMap(command);
        }
        if(PlayerCapability){
            if(PlayerCapability->CanInitiate(BuilderAsset, DPlayerData)){
                if(0 <= Placement.X()){
                    command.DAction = BuildAction;
                    command.DActors.push_back(BuilderAsset);
                    command.DTargetLocation.SetFromTile(Placement);
                    return true;
                }
            }
        }
    }

    return false;
}

bool CAIPlayer::ActivatePeasants(SPlayerCommandRequest &command, bool trainmore){
    // Mine and build peasants
    //auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > MiningAsset;
    std::shared_ptr< CPlayerAsset > InterruptibleAsset;
    std::shared_ptr< CPlayerAsset > TownHallAsset;
    int GoldMiners = 0;
    int LumberHarvesters = 0;
    bool SwitchToGold = false;
    bool SwitchToLumber = false;
    
    for(auto WeakAsset : DPlayerData->Assets()){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(actMine)){
                if(!MiningAsset && (aaNone == Asset->Action())){
                    MiningAsset = Asset;
                }
                
                if(Asset->HasAction(aaMineGold)){
                    GoldMiners++; 
                    if(Asset->Interruptible() && (aaNone != Asset->Action())){
                        InterruptibleAsset = Asset;
                    }
                }
                else if(Asset->HasAction(aaHarvestLumber)){
                    LumberHarvesters++;
                    if(Asset->Interruptible() && (aaNone != Asset->Action())){
                        InterruptibleAsset = Asset;
                    }
                }
            }
            if(Asset->HasCapability(actBuildPeasant) && (aaNone == Asset->Action())){
                TownHallAsset = Asset;
            }
        }
    }
    if((2 <= GoldMiners)&&(0 == LumberHarvesters)){
        SwitchToLumber = true; 
    }
    else if((2 <= LumberHarvesters)&&(0 == GoldMiners)){
        SwitchToGold = true; 
    }
    if(MiningAsset || (InterruptibleAsset && (SwitchToLumber || SwitchToGold))){
        if(MiningAsset && (MiningAsset->Lumber() || MiningAsset->Gold())){
            command.DAction = actConvey;
            command.DTargetColor = TownHallAsset->Color();
            command.DActors.push_back(MiningAsset);
            command.DTargetType = TownHallAsset->Type();
            command.DTargetLocation = TownHallAsset->Position();
        }
        else{
            if(!MiningAsset){
                MiningAsset = InterruptibleAsset;   
            }
            auto GoldMineAsset = DPlayerData->FindNearestAsset(MiningAsset->Position(), atGoldMine);
            if(GoldMiners && ((DPlayerData->Gold() > DPlayerData->Lumber() * 3) || SwitchToLumber)){
                CPosition LumberLocation = DPlayerData->PlayerMap()->FindNearestReachableTileType(MiningAsset->TilePosition(), CTerrainMap::ttTree);
                if(0 <= LumberLocation.X()){
                    command.DAction = actMine;
                    command.DActors.push_back(MiningAsset);
                    command.DTargetLocation.SetFromTile(LumberLocation);
                }
                else{
                    return SearchMap(command);
                }
            }
            else{
                command.DAction = actMine;
                command.DActors.push_back(MiningAsset);
                command.DTargetType = atGoldMine;
                command.DTargetLocation = GoldMineAsset->Position();
            }
        }
        return true;
    }
    else if(TownHallAsset && trainmore){
        auto PlayerCapability = CPlayerCapability::FindCapability(actBuildPeasant); 
        
        if(PlayerCapability){
            if(PlayerCapability->CanApply(TownHallAsset, DPlayerData, TownHallAsset)){
                command.DAction = actBuildPeasant;
                command.DActors.push_back(TownHallAsset);       
                command.DTargetLocation = TownHallAsset->Position();
                return true;
            }
        }
    }
    return false;
}

bool CAIPlayer::ActivateFighters(SPlayerCommandRequest &command){
    auto IdleAssets = DPlayerData->IdleAssets();
    
    for(auto WeakAsset : IdleAssets){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->Speed() && (atPeasant != Asset->Type())){
                if(!Asset->HasAction(aaStandGround) && !Asset->HasActiveCapability(actStandGround)){
                    command.DActors.push_back(Asset);
                }
            }
        }
    }
    if(command.DActors.size()){
        command.DAction = actStandGround;
        return true;
    }
    return false;
}

bool CAIPlayer::TrainFootman(SPlayerCommandRequest &command){
    auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > TrainingAsset;
    
    for(auto WeakAsset : IdleAssets){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(actBuildFootman)){
                TrainingAsset = Asset;
                break;
            }
        }
    }
    if(TrainingAsset){
        auto PlayerCapability = CPlayerCapability::FindCapability(actBuildFootman); 
        
        if(PlayerCapability){
            if(PlayerCapability->CanApply(TrainingAsset, DPlayerData, TrainingAsset)){
                command.DAction = actBuildFootman;
                command.DActors.push_back(TrainingAsset);       
                command.DTargetLocation = TrainingAsset->Position();
                return true;
            }
        }
    }
    return false;
}

bool CAIPlayer::TrainArcher(SPlayerCommandRequest &command){
    auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > TrainingAsset;
    EAssetCapabilityType BuildType = actBuildArcher;
    for(auto WeakAsset : IdleAssets){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(actBuildArcher)){
                TrainingAsset = Asset;
                BuildType = actBuildArcher;
                break;
            }
            if(Asset->HasCapability(actBuildRanger)){
                TrainingAsset = Asset;
                BuildType = actBuildRanger;
                break;
            }
            
        }
    }
    if(TrainingAsset){
        auto PlayerCapability = CPlayerCapability::FindCapability(BuildType); 
        if(PlayerCapability){
            if(PlayerCapability->CanApply(TrainingAsset, DPlayerData, TrainingAsset)){
                command.DAction = BuildType;
                command.DActors.push_back(TrainingAsset);       
                command.DTargetLocation = TrainingAsset->Position();
                return true;
            }
        }
    }
    return false;
}


void CAIPlayer::CalculateCommand(SPlayerCommandRequest &command){
    command.DAction = actNone;
    command.DActors.clear();    
    command.DTargetColor = pcNone;
    command.DTargetType = atNone;   
    if((DCycle % DDownSample) == 0){
        // Do decision   

        if(0 == DPlayerData->FoundAssetCount(atGoldMine)){
            // Search for gold mine
            SearchMap(command);
        }
        else if((0 == DPlayerData->PlayerAssetCount(atTownHall))&&(0 == DPlayerData->PlayerAssetCount(atKeep))&&(0 == DPlayerData->PlayerAssetCount(atCastle))){
            BuildTownHall(command);
        }
        else if(5 > DPlayerData->PlayerAssetCount(atPeasant)){
            ActivatePeasants(command, true);
        }
        else if(12 > DPlayerData->VisibilityMap()->SeenPercent(100)){
            SearchMap(command);
        }
        else{
            bool CompletedAction = false;
            int BarracksCount = 0;
            int FootmanCount = DPlayerData->PlayerAssetCount(atFootman);
            int ArcherCount = DPlayerData->PlayerAssetCount(atArcher)+DPlayerData->PlayerAssetCount(atRanger);
            
            if(!CompletedAction && (DPlayerData->FoodConsumption() >= DPlayerData->FoodProduction())){
                CompletedAction = BuildBuilding(command, atFarm, atFarm);
            }
            if(!CompletedAction){
                CompletedAction = ActivatePeasants(command, false);
            }
            if(!CompletedAction && (0 == (BarracksCount = DPlayerData->PlayerAssetCount(atBarracks)))){
                CompletedAction = BuildBuilding(command, atBarracks, atFarm);
            }
            if(!CompletedAction && (5 > FootmanCount)){
                CompletedAction = TrainFootman(command);
            }
            if(!CompletedAction && (0 == DPlayerData->PlayerAssetCount(atLumberMill))){
                CompletedAction = BuildBuilding(command, atLumberMill, atBarracks);
            }
            if(!CompletedAction &&  (5 > ArcherCount)){
                CompletedAction = TrainArcher(command);
            }
            if(!CompletedAction && DPlayerData->PlayerAssetCount(atFootman)){
                CompletedAction = FindEnemies(command);
            }
            if(!CompletedAction){
                CompletedAction = ActivateFighters(command);
            }
            if(!CompletedAction && ((5 <= FootmanCount) && (5 <= ArcherCount))){
                CompletedAction = AttackEnemies(command);
            }
        }
    }
    DCycle++;
}

