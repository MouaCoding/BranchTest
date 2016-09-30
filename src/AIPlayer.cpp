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

/*! \mainpage AI Main Page

    \tableofcontents

    \section intro_sec How the AI Works

    The game AI is contained in two files: \ref AIPlayer.h and \ref AIPlayer.cpp. 

    The class \ref CAIPlayer is used to make all the decisions for the AI. The AI Player may only decide which actions to take and when. It cannot do anything that the Human player can't do. This keeps the game balanced and fair. 
    
    The function in \ref CAIPlayer that decides what actions the AI should take is \ref CAIPlayer::CalculateCommand. One of the first things that this function does is determine whether it is allowed to perform an action yet. To manage difficulty, the AI is only allowed to perform an action every so often. To determine when the AI is allowed to perform an action, \ref CAIPlayer has a member variable DCycle, which is an int that is set to zero in the constructor (\ref CAIPlayer::CAIPlayer) and is incremented whenever \ref CAIPlayer::CalculateCommand is called. The AI is only allowed to perform an action if DCycle is currently a multiple of another member variable: DDownSample. By adjusting the value of DDownSample you change the APM (Actions Per Minute) of the AI and therefore you adjust the difficulty. 

    \subsection gdm_sec General Decision Making
    If the AI is allowed to take an action, it first checks whether it has any gold left at its gold mine; if not it does a \ref CAIPlayer::SearchMap to try to find another gold mine. 
    
    If it still has gold in the gold mine, the AI next checks whether it has a Town Hall (this is the building that produces peasants, which are used to gather resources and build buildings). Even though you start by default with a Town Hall, if it gets destroyed you must build another one. So if the AI doesn't have a Town Hall it will \ref CAIPlayer::BuildTownHall next to a gold mine.

    If the AI has both gold in the mine, a Town Hall and fewer than 5 peasants, the AI calls \ref CAIPlayer::ActivatePeasants which makes peasants gather resources.

    If the AI has at least 5 peasants and doesn't have enough visibility of the map, it will send a unit to explore.

    If the AI has enough map visibility it will start getting ready for battle.

    \subsection bdm_sec Battle Decision Making

    To get ready for battle, the AI will first check whether it has enough food to produce more units. Producing a unit costs a certain amount of food, and you get food by building farms. If the AI doesn't have enough food, it will build (\ref CAIPlayer::BuildBuilding) another farm next to another farm if one exists.

    If the AI has enough food it activates the peasants, so they get back to gathering resources after building.

    When the peasants are gathering again, the AI will check if it has a Barracks. The Barracks is the building used to produce melee units. If there is no Barracks, the AI will build one next to a Farm. After the Barracks is built, the AI will train Footmen until it has five. 

    Next it will build a Lumber Mill, which is used to produce ranged units. If the AI doesn't already have one, it will build one next to the Barracks. Once that is done, it will train five Archers.

    Once its army is ready, the AI will try to \ref CAIPlayer::FindEnemies. Then it will \ref CAIPlayer::ActivateFighters and if it still has five of each offensive unit type it will \ref CAIPlayer::AttackEnemies. 

    \subsection c_sec How to Improve the AI

    The AI is currently static. We need to create different difficulty levels by making the following changes: 

    -Increase or decrease AI APM.

    -Change how many of each type of building the AI should build. (Currently it only builds one Barracks and one Lumber Mill, but if you want to produce more units faster, you need to build more buildings. Also once it runs out of gold and mines from another gold mine, it needs to build another Town Hall close to that mine so the peasants can gather gold faster).

    -Change how many of each type of unit the AI produces. 

    -Give the AI access to the buildings, units and upgrades the player has.(Currently the AI only has access to four buildings and three units, whereas the player has access to all of them)

    -Make a way for the AI to use upgrades.

    -Make the AI spend more of its money. (Holding on to resources is bad, because it isn't being used to produce anything, and doing anything takes time. So, the AI should keep producing units or building buildings if it can afford to).
*/


/** \file AIPlayer.cpp
 * A brief file description.
 * A more h file description.
 */



#include "AIPlayer.h"
#include "Debug.h"

// Constructor for CAIPlayer
/*! 
   \brief Constructor for CAIPlayer

   \param DPlayerData - Holds data about the AI player (gold, lumber, etc.).

   \param DCycle - Clock that increments whenever the AI tries to make a decision.

   \param DDownSample - Determines how often the AI can perform an action.

   \section constrhow_sec How It Works

   First the constructor sets \b DPlayerData equal to \b playerdata which is 
   passed in as an argument. Typically these are the default values for both
   AI and the human players. For example starting gold and lumber is set to 0.

   Next \b DCycle is set to 0 and \b DDownSample is set to \b downsample which is 
   passed in as an argument. \b DCycle is incremented whenever \ref CAIPlayer::CalculateCommand is called. If \b DCycle is a multiple of \b DDownSample, then the AI is allowed to perform an action. 
 */
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


// Decides what actions the AI should take.
/*! 
    \section cchow_sec How It Works

   Explanation of how it works on \ref index.
 */
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

