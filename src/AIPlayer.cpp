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
    
    If it still has gold in the gold mine, the AI next checks whether it has a Town Hall (this is the building that produces peasants, which are used to gather resources and construct buildings). Even though you usually start by default with a Town Hall, if it gets destroyed you must build another one. So if the AI doesn't have a Town Hall it will \ref CAIPlayer::BuildTownHall next to a gold mine.

    If the AI has gold in the mine, a Town Hall and fewer than 5 peasants, the AI calls \ref CAIPlayer::ActivatePeasants which makes peasants gather resources and produce more peasants.

    If the AI has at least 5 peasants and doesn't have enough visibility of the map, it will send a unit to explore.

    If the AI has enough map visibility it will start getting ready for battle.

    \subsection bdm_sec Battle Decision Making

    To get ready for battle, the AI will first check whether it has enough food to produce more units. Producing a unit costs a certain amount of food, and you get food by building farms. If the AI doesn't have enough food, it will build (\ref CAIPlayer::BuildBuilding) a farm.

    If the AI has enough food, it will \ref CAIPlayer::ActivatePeasants, so they get back to gathering resources after building.

    When the peasants are gathering again, the AI will check if it has a Barracks. The Barracks is the building used to produce melee units. If there is no Barracks, the AI will build one next to a Farm. After the Barracks is built, the AI will train a Footman if it has fewer than five. 

    Next it will build a Lumber Mill, which is used to produce ranged units. If the AI doesn't already have one, it will build one next to the Barracks. Once that is done, it will train an archer if it has fewer than five.

    If the AI has a Footman, the AI will try to \ref CAIPlayer::FindEnemies. Then it will \ref CAIPlayer::ActivateFighters and if it still has five of each fighter unit it will \ref CAIPlayer::AttackEnemies. 

    \subsection c_sec How to Improve the AI

    We need to create different difficulty levels by making the following changes: 

    -Increase or decrease AI APM.

    -Change how many of each type of building the AI should build. (Currently it only builds one Barracks and one Lumber Mill, but if you want to produce more units faster, you need to build more buildings. Also once it runs out of gold and mines from another gold mine, it needs to build another Town Hall close to that mine so the peasants can gather gold faster).

    -Change how many of each type of unit the AI produces. 

    -Give the AI access to the buildings, units and upgrades the player has.(Currently the AI only has access to four buildings and three units, whereas the player has access to all of them).

    -Make a way for the AI to use upgrades.

    -Make the AI spend more of its money. (Holding on to resources is bad, because it isn't being used to produce anything, and doing anything takes time. So, the AI should keep producing units or building buildings if it can afford to).
*/


/** \file AIPlayer.cpp
    The member functions of \ref CAIPlayer are defined in this file.
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

   It sets all the parameters to their default values.

   For more information about how DCycle and DDownSample work check \ref CAIPlayer::CalculateCommand.
 */
CAIPlayer::CAIPlayer(std::shared_ptr< CPlayerData > playerdata, int downsample){
    // Sets the AI Player's data to default values (e.g. lumber = 0, etc.)
    DPlayerData = playerdata;
    // Sets the current cycle to 0.
    DCycle = 0;
    // Initializes to DDownSample
    DDownSample = downsample;
}

// Function used by the AI to explore the map.
bool CAIPlayer::SearchMap(SPlayerCommandRequest &command){
    auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > MovableAsset;
    
    // This loop searches through all the idle assets of the AI to find one that
    // can move.
    for(auto WeakAsset : IdleAssets){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->Speed()){
                MovableAsset = Asset;
                break;
            }
        }
    }
    // Checks if a unit was found.
    if(MovableAsset){
        // Finds the nearest tile that is still covered by fog of war.
        CPosition UnknownPosition = DPlayerData->PlayerMap()->FindNearestReachableTileType(MovableAsset->TilePosition(), CTerrainMap::ttNone);
        
        // Checks if it has found an unexplored tile.
        if(0 <= UnknownPosition.X()){
            //printf("Unkown Position (%d, %d)\n", UnknownPosition.X(), UnknownPosition.Y());

            // Moves the unit toward that tile.
            command.DAction = actMove;
            command.DActors.push_back(MovableAsset);
            command.DTargetLocation.SetFromTile(UnknownPosition);
            // Successfully explored more of the map.
            return true;
        }
    }
    // Couldn't explore more of the map.
    return false;
}

// Function used by the AI to find enemies.
/*! 
   \brief Function used by the AI to find enemies by searching assets that are visible to it on the map.

   \section fehow_sec How It Works

   It first finds a Town Hall of the AI, then it searches in a range around that building. The range in this case is unlimited. The function calls \ref CPlayerData::FindNearestEnemy, which iterates through all the assets the AI can see on the map and checks whether its player color is different than the AI's. It also checks if that asset it found is alive. 
   If the pointer to the nearest enemy is expired, the function returns the result of \ref SearchMap. It returns false if it can't find anything.
 */
bool CAIPlayer::FindEnemies(SPlayerCommandRequest &command){
    std::shared_ptr< CPlayerAsset > TownHallAsset;
    
    // This loop looks through assets to find a Town Hall.
    for(auto WeakAsset : DPlayerData->Assets()){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(actBuildPeasant)){
                TownHallAsset = Asset;
                break;
            }
        }
    }

    // Checks if the pointer to the enemy has expired (was deleted).
    // It starts searching for an enemy from the Town Hall. 
    // The range is unlimited (-1).
    // FindNearestEnemy searches for an asset on the map that has a different 
    // color than the AI and that is alive.
    if(DPlayerData->FindNearestEnemy(TownHallAsset->Position(), -1).expired()){
        // If the asset pointer is expired it starts exploring the map.
        return SearchMap(command);
    }
    // Returns false if it didn't find an enemy.
    return false;    
}

// Function used by the AI to attack enemies.
/*! 
   \brief Function used by the AI to select fighters and attack the enemy.

   \section aehow_sec How It Works

   The function looks through all the assets of the AI and tries to find fighters that are not currently fighting, and adds them to a list. If the list isn't empty, all those fighters are ordered to attack the nearest enemy. If none are found, the AI starts searching the map.
 */
bool CAIPlayer::AttackEnemies(SPlayerCommandRequest &command){
    CPosition AverageLocation(0,0);
    
    // The functions searches through all of the AI's assets to find fighters.
    for(auto WeakAsset : DPlayerData->Assets()){
        if(auto Asset = WeakAsset.lock()){
            if((atFootman == Asset->Type())||(atArcher == Asset->Type())||(atRanger == Asset->Type())){
                // Checks if the fighter is currently not attacking.
                if(!Asset->HasAction(aaAttack)){
                    // The fighter is added to a list of actors, ready to
                    // receive orders.
                    command.DActors.push_back(Asset);
                    AverageLocation.IncrementX(Asset->PositionX());
                    AverageLocation.IncrementY(Asset->PositionY());
                }
            }
        }
    }
    // Checks if there are any actors on the list.
    if(command.DActors.size()){
        // The average location of the group is calculated, since every unit
        // is on a different tile. 
        AverageLocation.X(AverageLocation.X() / command.DActors.size());
        AverageLocation.Y(AverageLocation.Y() / command.DActors.size());
        // Finds the location of the nearest enemy.
        auto TargetEnemy = DPlayerData->FindNearestEnemy(AverageLocation, -1).lock();
        // If there are no nearby enemies, get back to searching the map.
        if(!TargetEnemy){
            command.DActors.clear();
            return SearchMap(command);
        }
        // Attacks the enemy.
        command.DAction = actAttack;
        command.DTargetLocation = TargetEnemy->Position();
        command.DTargetColor = TargetEnemy->Color();
        command.DTargetType = TargetEnemy->Type(); 
        // Successfully attacked the enemy.
        return true;
    }
    // Failed to attack the enemy.
    return false;    
}


// Function used by the AI to build a Town Hall
/*! 
   \brief Function used by the AI to build a Town Hall. Returns true or false, depending on whether it was able to build or not.

   \section bthhow_sec How It Works

   First the function looks through all the AI's assets to find one that can build things. Once a builder is found, the function looks for the closest gold mine to the builder. Next, it determines the best position to build the Town Hall. Usually that is right next to the mine, so the peasants can mine gold faster. If it finds a good position, it commands that builder to build a Town Hall. If it can't find a placement for the building, it will search the map. If a Town Hall can't be built, the function returns false.
 */
bool CAIPlayer::BuildTownHall(SPlayerCommandRequest &command){
    // Creates a list of the AI's idle assets.
    auto IdleAssets = DPlayerData->IdleAssets();
    // Declares a pointer to an asset.
    std::shared_ptr< CPlayerAsset > BuilderAsset;
    
    // Looks through every idle asset
    for(auto WeakAsset : IdleAssets){
        // Checks whether the asset pointer is valid and assigns it to Asset.
        if(auto Asset = WeakAsset.lock()){
            // Checks whether the asset is able to build a Town Hall.
            if(Asset->HasCapability(actBuildTownHall)){
                // If the asset can build a Town Hall, the BuilderAsset will be 
                // set to point to that asset.
                BuilderAsset = Asset;
                break;
            }
        }
    }

    // Checks if an asset that can build was found (or peasant).
    if(BuilderAsset){
        // Find the gold mine that is nearest to the peasant and assign it to a
        // variable.
        auto GoldMineAsset = DPlayerData->FindNearestAsset(BuilderAsset->Position(), atGoldMine);
        // Finds the best place to build the Town Hall, based on the gold mine's
        // position.
        CPosition Placement = DPlayerData->FindBestAssetPlacement(GoldMineAsset->TilePosition(), BuilderAsset, atTownHall, 1);
        // Checks if a position for the Town Hall was found
        if(0 <= Placement.X()){
            // Assigns up the command to execute (Build Town Hall).
            command.DAction = actBuildTownHall;
            // Adds the actor who is supposed to perform the action to DActors.
            command.DActors.push_back(BuilderAsset);
            // Sets the location where the action should be performed.
            command.DTargetLocation.SetFromTile(Placement);
            // Returns true because the peasant started building.
            return true;
        }
        else{
            // Searches the map to find another position. Possible reason
            // could be that there was no gold mine nearby. Returns the result
            // of that search.
            return SearchMap(command);  
        }
    }
    // Returns false if it couldn't build a Town Hall.
    return false;
}

// Builds a building of type buildingtype next to a building of type neartype.
/*! 
   \brief Function used by the AI to build a building other than the Town Hall. Returns true or false, depending on whether it was able to build or not.

   \section bthhow_sec How It Works

   First he function tries to find a builder, the Town Hall and the building next to which it is supposed to build. The functions has multiple checks for determining whether a building is currently being built. If another building is being built, the function returns false. Otherwise it can proceed. In case no neartype was passed in, the building will be constructed next to the town hall. The building will be built such that the AI expands its base toward the center of the map. This means that for example, if the center of the map is to the North-West of the AI's Town Hall, the building will be constructed to the North-West of the neartype building.
 */
bool CAIPlayer::BuildBuilding(SPlayerCommandRequest &command, EAssetType buildingtype, EAssetType neartype){
    std::shared_ptr< CPlayerAsset > BuilderAsset;
    std::shared_ptr< CPlayerAsset > TownHallAsset;
    std::shared_ptr< CPlayerAsset > NearAsset;
    EAssetCapabilityType BuildAction;
    bool AssetIsIdle = false;
    
    // Assigns an action based on the type of building that must be built.
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

    // Loops through all the AI's assets.
    for(auto WeakAsset : DPlayerData->Assets()){
        // Checks that the asset pointer is valid.
        if(auto Asset = WeakAsset.lock()){
            // Checks if the asset is able to build a specific building, 
            // and whether it can be interrupted from whatever it is doing.
            if(Asset->HasCapability(BuildAction) && Asset->Interruptible()){
                // Checks if no builder was assinged yet, or if there is a 
                // mismatch between the local bool AssetIsIdle and whether
                // the asset really is idle.
                if(!BuilderAsset || (!AssetIsIdle && (aaNone == Asset->Action()))){
                    BuilderAsset = Asset;
                    // Sets AssetIsIdle to true if the asset isn't doing 
                    // anything.
                    AssetIsIdle = aaNone == Asset->Action();
                }
            }
            // Checks if the asset can build peasants (basically looking for a 
            // Town Hall).
            if(Asset->HasCapability(actBuildPeasant)){
                TownHallAsset = Asset;
            }
            // Checks if the asset is currently building.
            if(Asset->HasActiveCapability(BuildAction)){
                // Returns false, most likely because it can better determine 
                // what it needs to do after the building is completed. It might
                // change where this building should be built, or whether it 
                // should be built at all.
                return false;    
            }
            // Checks if this asset is a building of type neartype, and if 
            // that building isn't currently being contructed.
            if((neartype == Asset->Type())&&(aaConstruct != Asset->Action())){
                NearAsset = Asset;
            }
            // Checks if the asset is a building of type buildingtype.
            if(buildingtype == Asset->Type()){
                // Checks if the building is currently being built.
                if(aaConstruct == Asset->Action()){
                    // Returns false to avoid building more buildings than
                    // needed.
                    return false;   
                }
            }
        }
    }

    //Checks if the building the AI is trying to build is not the same as the 
    // building it is supposed to be build near, and  if there is no building
    // of the neartype.
    if((buildingtype != neartype) && !NearAsset){
        // Returns false because there is no building that buildingtype can be
        // built near. 
        return false;    
    }

    // Checks if there is a builder (peasant).
    if(BuilderAsset){
        auto PlayerCapability = CPlayerCapability::FindCapability(BuildAction);
        // Sets the position where to build at the Town Hall, in case no
        // neartype is passed in.
        CPosition SourcePosition = TownHallAsset->TilePosition();
        CPosition MapCenter(DPlayerData->PlayerMap()->Width()/2, DPlayerData->PlayerMap()->Height()/2);
        
        // If a neartype is passed in, it sets the source to that.
        if(NearAsset){
            SourcePosition = NearAsset->TilePosition();
        }

        /*  
            The next four if statements are used to determine how to adjust the
            source position. The adjustment is calculated based on where the 
            center of the map is. The source position is adjusted in such a way 
            that the AI will expand its base toward the center of the map.
        */
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

        // Finds the best location to build, based on the source position.
        CPosition Placement = DPlayerData->FindBestAssetPlacement(SourcePosition, BuilderAsset, buildingtype, 1);
        // Checks if a placement was not found.
        if(0 > Placement.X()){
            // Searches the map for another place where it could build.
            return SearchMap(command);
        }

        // Checks if the AI Player is capable of building.
        if(PlayerCapability){
            // Checks if the AI Player can start building, based on the chosen
            // peasant and the AI's current resources.
            if(PlayerCapability->CanInitiate(BuilderAsset, DPlayerData)){
                // Checks if a placement was found. 
                if(0 <= Placement.X()){
                    // The builder is commanded to build at the chosen position,
                    // and true is returned.
                    command.DAction = BuildAction;
                    command.DActors.push_back(BuilderAsset);
                    command.DTargetLocation.SetFromTile(Placement);
                    return true;
                }
            }
        }
    }

    // Returns false if it was unable to build the building.
    return false;
}

// Function that makes the peasants gather resources or to produce peasants.
/*! 
   \brief Function used by the AI to produce peasants and to make them gather resources.

   \section aphow_sec How It Works

   The function checks if there are any peasants that are idle or if there are any that could be interrupted from what they are doing. Then it has some logic that determines whether the peasants should be gathering wood or gold. For example if the AI currently has too much gold or too many gold miners, those peasants will be reassigned to harvest lumber. If there are no idle peasants or none that need to be switched, but trainmore is true, the function will check if the AI has enough resources to train a peasant, and if it can it will do so.
 */
bool CAIPlayer::ActivatePeasants(SPlayerCommandRequest &command, bool trainmore){
    //auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > MiningAsset;
    std::shared_ptr< CPlayerAsset > InterruptibleAsset;
    std::shared_ptr< CPlayerAsset > TownHallAsset;
    int GoldMiners = 0;
    int LumberHarvesters = 0;
    bool SwitchToGold = false;
    bool SwitchToLumber = false;
    
    // Iterates through all of the AI's assets.
    for(auto WeakAsset : DPlayerData->Assets()){
        // Checks that the asset pointer is valid.
        if(auto Asset = WeakAsset.lock()){
            // Checks if the asset has the ability to mine (is a peasant).
            if(Asset->HasCapability(actMine)){
                // Checks there is no peasant mining and if the current asset
                // is idle.
                if(!MiningAsset && (aaNone == Asset->Action())){
                    MiningAsset = Asset;
                }
                // Checks if the asset is currently mining.
                if(Asset->HasAction(aaMineGold)){
                    GoldMiners++; 
                    // Checks if the asset can be interrupted (stopped from its
                    // current action), and if it is not idle.
                    if(Asset->Interruptible() && (aaNone != Asset->Action())){
                        InterruptibleAsset = Asset;
                    }
                }
                // Checks if the asset is currently harvesting wood.
                else if(Asset->HasAction(aaHarvestLumber)){
                    LumberHarvesters++;
                    if(Asset->Interruptible() && (aaNone != Asset->Action())){
                        InterruptibleAsset = Asset;
                    }
                }
            }
            // Checks if the asset is a Town Hall and whether it is idle.
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
    // Checks if there is an idle peasant, or if a peasant must switch from
    // gathering wood to gathering gold, or vice versa.
    if(MiningAsset || (InterruptibleAsset && (SwitchToLumber || SwitchToGold))){
        // Checks if there is a peasant that just finished gathering a resource.
        if(MiningAsset && (MiningAsset->Lumber() || MiningAsset->Gold())){
            // Makes that peasant take the resources back to Town Hall.
            command.DAction = actConvey;
            command.DTargetColor = TownHallAsset->Color();
            command.DActors.push_back(MiningAsset);
            command.DTargetType = TownHallAsset->Type();
            command.DTargetLocation = TownHallAsset->Position();
        }
        else{
            // Checks if there are no idle peasants.
            if(!MiningAsset){
                // Since the function got here, it means that the AI needs to 
                // gather a different resource, but there are no idle peasants,
                // so one of them must be interrupted.
                MiningAsset = InterruptibleAsset;   
            }
            // Finds the nearest gold mine.
            auto GoldMineAsset = DPlayerData->FindNearestAsset(MiningAsset->Position(), atGoldMine);
            // Checks if there are peasants gathering gold and either the 
            // AI has three times more gold than lumber, or there 
            // are too many peasants mining gold.
            if(GoldMiners && ((DPlayerData->Gold() > DPlayerData->Lumber() * 3) || SwitchToLumber)){
                // Finds the nearest tree tile location
                CPosition LumberLocation = DPlayerData->PlayerMap()->FindNearestReachableTileType(MiningAsset->TilePosition(), CTerrainMap::ttTree);
                // Checks if a tree tile was found.
                if(0 <= LumberLocation.X()){
                    // Makes the peasant start gathering wood.
                    command.DAction = actMine;
                    command.DActors.push_back(MiningAsset);
                    command.DTargetLocation.SetFromTile(LumberLocation);
                }
                else{
                    // If no wood was found, the map must be explored so wood is
                    // found.
                    return SearchMap(command);
                }
            }
            else{
                // If there aren't too many gold miners or if the AI doesn't 
                // have too much gold, the peasants will be given the command
                // to mine gold.
                command.DAction = actMine;
                command.DActors.push_back(MiningAsset);
                command.DTargetType = atGoldMine;
                command.DTargetLocation = GoldMineAsset->Position();
            }
        }
        // Peasants successfully activated.
        return true;
    }
    // If no changes must be made regarding gathering, but more peasants must
    // be produced.
    else if(TownHallAsset && trainmore){
        auto PlayerCapability = CPlayerCapability::FindCapability(actBuildPeasant); 
        
        // Checks if the AI player has the capability to produce peasants.
        if(PlayerCapability){
            // Checks if the AI has enough resources to produce a peasant.
            if(PlayerCapability->CanApply(TownHallAsset, DPlayerData, TownHallAsset)){
                command.DAction = actBuildPeasant;
                command.DActors.push_back(TownHallAsset);       
                command.DTargetLocation = TownHallAsset->Position();
                // Successfully started producing a peasant.
                return true;
            }
        }
    }
    // Was unable to make peasants gather or to produce any peasants.
    return false;
}


// Function used by the AI to select fighters
/*! 
   \brief Selects fighting units that will be able to receive orders as a group.
   Returns true or false depending on whether it was able to select any fighters or not.

   \section afhow_sec How It Works

   The function looks through all the idle assets it has and checks whether they have a speed property and whether they are not a peasant. Then it checks whether the asset is not currently standing ground. If the asset is not standing ground, it is added to a list of actors (assets that can perform actions). The function checks whether that list is empty and if it isn't it will make all the actors on the list stand ground.
 */
bool CAIPlayer::ActivateFighters(SPlayerCommandRequest &command){
    // Variable that holds the idle assets of the player.
    auto IdleAssets = DPlayerData->IdleAssets();
    
    // Searches through all the IdleAssets.
    // ":" is the range operator in c++ 11. It goes through all the elements
    // in a list.
    for(auto WeakAsset : IdleAssets){
        // Checks whether the asset can be assigned to the current asset.
        // lock() checks whether the pointer is valid.
        if(auto Asset = WeakAsset.lock()){
            // Checks whether the current asset has a speed property and that
            // it is not a peasant. If those two things are true, then the unit
            // should be a fighter.
            if(Asset->Speed() && (atPeasant != Asset->Type())){
                // Checks if the asset is currently not standing ground.
                if(!Asset->HasAction(aaStandGround) && !Asset->HasActiveCapability(actStandGround)){
                    // If the unit is not standing ground it is added to a list
                    // of actors (assets that can act) which will perform an
                    // action as a group.
                    command.DActors.push_back(Asset);
                }
            }
        }
    }
    // Checks whether there are any actors ready to receive orders.
    if(command.DActors.size()){
        // Makes the fighters Stand Ground
        command.DAction = actStandGround;
        // returns true because it was able to activate some fighters
        return true;
    }
    // returns false if there are no fighters to activate
    return false;
}

// Function used by the AI to train Footmen
/*! 
   \brief Function used by the AI to train Footmen

   \section tfhow_sec How It Works

   The function first checks whether the AI has an idle asset which can train Footmen. Then it checks whether the AI has the ability to train footmen and the resources necessary to do so. If those conditions are met, the Barracks is given a command to build a Footman.
 */
bool CAIPlayer::TrainFootman(SPlayerCommandRequest &command){
    auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > TrainingAsset;
    
    // Searches through a list of all idle assets for one which has the 
    // capability of training Footmen (Barracks).
    for(auto WeakAsset : IdleAssets){
        if(auto Asset = WeakAsset.lock()){
            if(Asset->HasCapability(actBuildFootman)){
                TrainingAsset = Asset;
                break;
            }
        }
    }

    // Checks if a Barracks was found
    if(TrainingAsset){
        // Finds the AI's ability to train Footmen
        auto PlayerCapability = CPlayerCapability::FindCapability(actBuildFootman); 
        
        // Checks if the AI has the capability of training Footmen.
        if(PlayerCapability){
            // Checks if the AI has enough resources to train a Footman.
            if(PlayerCapability->CanApply(TrainingAsset, DPlayerData, TrainingAsset)){
                // Gives the Barracks the command to train a Footman.
                command.DAction = actBuildFootman;
                command.DActors.push_back(TrainingAsset);       
                command.DTargetLocation = TrainingAsset->Position();
                // Returns true because it was successful.
                return true;
            }
        }
    }
    // Returns false because it was unable to train a Footman.
    return false;
}

// Function used by the AI to train Archers
/*! 
   \brief Function used by the AI to train Archers

   \section tfhow_sec How It Works

   The function first checks whether the AI has an idle asset which can train Archers. Then it checks whether the AI has the ability to train archers and the resources necessary to do so. If those conditions are met, the Barracks is given a command to build a Footman.
 */
bool CAIPlayer::TrainArcher(SPlayerCommandRequest &command){
    auto IdleAssets = DPlayerData->IdleAssets();
    std::shared_ptr< CPlayerAsset > TrainingAsset;
    EAssetCapabilityType BuildType = actBuildArcher;

    // Searches through a list of all idle assets for one which has the 
    // capability of training Archers (Lumber Mill).
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
    // Checks if Lumber Mill was found.
    if(TrainingAsset){
        // Finds the AI's ability to train Archers.
        auto PlayerCapability = CPlayerCapability::FindCapability(BuildType); 
        if(PlayerCapability){
            // Checks if the AI has enough resources to train an Archer.
            if(PlayerCapability->CanApply(TrainingAsset, DPlayerData, TrainingAsset)){
                // Gives the Lumber Mill the command to train an archer
                command.DAction = BuildType;
                command.DActors.push_back(TrainingAsset);       
                command.DTargetLocation = TrainingAsset->Position();
                // Returns true because it was successful.
                return true;
            }
        }
    }
     // Returns false because it was unable to train an Archer.
    return false;
}


// Decides what actions the AI should take.
/*! 
    \brief Decides what actions the AI should take.

    \section cchow_sec How It Works

   Explanation of how it works on \ref index.
 */
void CAIPlayer::CalculateCommand(SPlayerCommandRequest &command){
    command.DAction = actNone;
    command.DActors.clear();    
    command.DTargetColor = pcNone;
    command.DTargetType = atNone;
    // Checks if the AI is allowed to take an action yet. 
    if((DCycle % DDownSample) == 0){

        // Checks if the AI has any gold left in the local gold mine.
        if(0 == DPlayerData->FoundAssetCount(atGoldMine)){
            // Search for gold mine
            SearchMap(command);
        } 
        // Checks if there is a Town Hall
        else if((0 == DPlayerData->PlayerAssetCount(atTownHall))&&(0 == DPlayerData->PlayerAssetCount(atKeep))&&(0 == DPlayerData->PlayerAssetCount(atCastle))){
            BuildTownHall(command);
        } 
        // Checks if the AI has fewer than five peasants
        else if(5 > DPlayerData->PlayerAssetCount(atPeasant)){
            // Makes the peasants gather resources/ produce more peasants.
            ActivatePeasants(command, true);
        }
        // Checks if the AI has enough map visibility.
        else if(12 > DPlayerData->VisibilityMap()->SeenPercent(100)){
            // Starts searching the map to get more visibility.
            SearchMap(command);
        }
        else{
            // CompletedAction is initialized to false, and set to true if the 
            // AI has completed an action. To prevent the AI from completing
            // multiple actions in one cycle, every if statement checks
            // !CompletedAction
            bool CompletedAction = false;
            int BarracksCount = 0;
            int FootmanCount = DPlayerData->PlayerAssetCount(atFootman);
            int ArcherCount = DPlayerData->PlayerAssetCount(atArcher)+DPlayerData->PlayerAssetCount(atRanger);
            // Checks whether the food consumption of units is bigger than
            // the food production
            if(!CompletedAction && (DPlayerData->FoodConsumption() >= DPlayerData->FoodProduction())){
                // Builds a farm to have more food production
                CompletedAction = BuildBuilding(command, atFarm, atFarm);
            }
            if(!CompletedAction){
                // Makes peasants go back to gathering, because they will be
                // idle after building a farm.
                CompletedAction = ActivatePeasants(command, false);
            }
            // Checks whether the AI has a Barracks
            if(!CompletedAction && (0 == (BarracksCount = DPlayerData->PlayerAssetCount(atBarracks)))){
                // Builds a Barracks next to a Farm
                CompletedAction = BuildBuilding(command, atBarracks, atFarm);
            }
            // Checks if there are fewer than five Footmen
            if(!CompletedAction && (5 > FootmanCount)){
                // Trains a footman
                CompletedAction = TrainFootman(command);
            }
            // Checks whether the AI has a Lumber Mill
            if(!CompletedAction && (0 == DPlayerData->PlayerAssetCount(atLumberMill))){
                // Builds a Lumber Mill next to the Barracks
                CompletedAction = BuildBuilding(command, atLumberMill, atBarracks);
            }
            // Checks whether the AI has fewer than five archers
            if(!CompletedAction &&  (5 > ArcherCount)){
                // Trains an Archer
                CompletedAction = TrainArcher(command);
            }
            // It checks whether it has a Footman.
            if(!CompletedAction && DPlayerData->PlayerAssetCount(atFootman)){
                // Sends the Footman to find the enemy.
                CompletedAction = FindEnemies(command);
            }
            if(!CompletedAction){
                // Selects the fighters
                CompletedAction = ActivateFighters(command);
            }
            // Checks if the AI has at least five Footmen and five Archers.
            if(!CompletedAction && ((5 <= FootmanCount) && (5 <= ArcherCount))){
                CompletedAction = AttackEnemies(command);
            }
        }
    }
    DCycle++;
}

