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
#include "GameModel.h"
#include "Debug.h"

#ifndef MIN
    #define MIN(a, b)               ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
    #define MAX(a, b)               ((a) > (b) ? (a) : (b))
#endif

int RangeToDistanceSquared(int range){
    range *= CPosition::TileWidth();
    range *= range;
    range += CPosition::TileWidth() * CPosition::TileWidth();
    return range;
}

CPlayerData::CPlayerData(std::shared_ptr< CAssetDecoratedMap > map, EPlayerColor color){
    DIsAI = true;
    DGameCycle = 0;
    DColor = color;
    DActualMap = map;
    DAssetTypes = CPlayerAssetType::DuplicateRegistry(color);
    DPlayerMap = DActualMap->CreateInitializeMap();
    DVisibilityMap = DActualMap->CreateVisibilityMap();
    DGold = 0;
    DLumber = 0;
    
    DUpgrades.resize(actMax);
    for(int Index = 0; Index < DUpgrades.size(); Index++){
        DUpgrades[Index] = false;   
    }
    for(auto &ResourceInit : DActualMap->ResourceInitializationList()){
        if(ResourceInit.DColor == color){
            DGold = ResourceInit.DGold;
            DLumber = ResourceInit.DLumber;
        }
    }
    for(auto &AssetInit : DActualMap->AssetInitializationList()){
        if(AssetInit.DColor == color){
            PrintDebug(DEBUG_LOW, "Init %s %d (%d, %d)\n", AssetInit.DType.c_str(), AssetInit.DColor, AssetInit.DTilePosition.X(), AssetInit.DTilePosition.Y());
            std::shared_ptr< CPlayerAsset > InitAsset = CreateAsset(AssetInit.DType);
            InitAsset->TilePosition(AssetInit.DTilePosition);
            if(atGoldMine == CPlayerAssetType::NameToType(AssetInit.DType)){
                InitAsset->Gold(DGold);
            }
        }
    }
}

int CPlayerData::FoodConsumption() const{
    int TotalConsumption = 0;
    
    for(auto &WeakAsset : DAssets){
        if(auto Asset = WeakAsset.lock()){
            int AssetConsumption = Asset->FoodConsumption();
            if(0 < AssetConsumption){
                TotalConsumption += AssetConsumption;    
            }
        }
    }
    return TotalConsumption;
}

int CPlayerData::FoodProduction() const{
    int TotalProduction = 0;
    
    for(auto &WeakAsset : DAssets){
        if(auto Asset = WeakAsset.lock()){
            int AssetConsumption = Asset->FoodConsumption();
            if((0 > AssetConsumption)&&((aaConstruct != Asset->Action())||(!Asset->CurrentCommand().DAssetTarget))){
                TotalProduction += -AssetConsumption;
            }
        }
    }
    return TotalProduction;
}

std::shared_ptr< CPlayerAsset > CPlayerData::CreateMarker(const CPosition &pos, bool addtomap){
    std::shared_ptr< CPlayerAsset > NewMarker = (*DAssetTypes)["None"]->Construct();
    CPosition TilePosition;
    
    TilePosition.SetToTile(pos);
    NewMarker->TilePosition(TilePosition);
    if(addtomap){
        DPlayerMap->AddAsset(NewMarker);
    }
    
    return NewMarker;
}

std::shared_ptr< CPlayerAsset > CPlayerData::CreateAsset(const std::string &assettypename){
    std::shared_ptr< CPlayerAsset > CreatedAsset = (*DAssetTypes)[assettypename]->Construct();
    
    CreatedAsset->CreationCycle(DGameCycle);
    DAssets.push_back(CreatedAsset);
    DActualMap->AddAsset(CreatedAsset);
    return CreatedAsset;
}

void CPlayerData::DeleteAsset(std::shared_ptr< CPlayerAsset > asset){
    auto Iterator = DAssets.begin();
    while(Iterator != DAssets.end()){
        if(Iterator->lock() == asset){
            DAssets.erase(Iterator);
            break;
        }
        Iterator++;
    }
    DActualMap->RemoveAsset(asset);
}

bool CPlayerData::AssetRequirementsMet(const std::string &assettypename){
    std::vector< int > AssetCount;
    
    AssetCount.resize(atMax);
    
    for(auto WeakAsset : DAssets){
        if(auto Asset = WeakAsset.lock()){
            if(aaConstruct != Asset->Action()){
                AssetCount[Asset->Type()]++;
            }
        }
    }
    for(auto Requirement : (*DAssetTypes)[assettypename]->AssetRequirements()){
        if(0 == AssetCount[Requirement]){
            if((atKeep == Requirement)&&(AssetCount[atCastle])){
                continue;               
            }
            if((atTownHall == Requirement)&&(AssetCount[atKeep]||AssetCount[atCastle])){
                continue;               
            }
            return false;    
        }
    }
    return true;
}

void CPlayerData::UpdateVisibility(){
    std::list< std::shared_ptr< CPlayerAsset > > RemoveList;
    
    DVisibilityMap->Update(DAssets);
    DPlayerMap->UpdateMap(*DVisibilityMap, *DActualMap);
    for(auto &Asset : DPlayerMap->Assets()){
        if((atNone == Asset->Type())&&(aaNone == Asset->Action())){
            Asset->IncrementStep();
            if(CPlayerAsset::UpdateFrequency() < Asset->Step() * 2){
                RemoveList.push_back(Asset);   
            }
        }
    }
    for(auto &Asset : RemoveList){
        DPlayerMap->RemoveAsset(Asset);     
    }
    
}

std::list< std::weak_ptr< CPlayerAsset > > CPlayerData::SelectAssets(const SRectangle &selectarea, EAssetType assettype, bool selectidentical){
    std::list< std::weak_ptr< CPlayerAsset > > ReturnList;
    
    if((!selectarea.DWidth)||(!selectarea.DHeight)){
        std::weak_ptr< CPlayerAsset > BestAsset = SelectAsset(CPosition(selectarea.DXPosition, selectarea.DYPosition), assettype);
        if(auto LockedAsset = BestAsset.lock()){
            ReturnList.push_back(BestAsset);
            if(selectidentical && LockedAsset->Speed()){
                for(auto &WeakAsset : DAssets){
                    if(auto Asset = WeakAsset.lock()){
                        if((LockedAsset != Asset)&&(Asset->Type() == assettype)){
                            ReturnList.push_back(Asset);   
                        }
                    }
                }
            }
        }
    }
    else{
        bool AnyMovable = false;
        for(auto &WeakAsset : DAssets){
            if(auto Asset = WeakAsset.lock()){
                if((selectarea.DXPosition <= Asset->PositionX())&&(Asset->PositionX() < selectarea.DXPosition + selectarea.DWidth)&&(selectarea.DYPosition <= Asset->PositionY())&&(Asset->PositionY() < selectarea.DYPosition + selectarea.DHeight)){
                    if(AnyMovable){
                        if(Asset->Speed()){
                            ReturnList.push_back(Asset);
                        }
                    }
                    else{
                        if(Asset->Speed()){
                            ReturnList.clear();
                            ReturnList.push_back(Asset);
                            AnyMovable = true;
                        }
                        else{
                            if(ReturnList.empty()){
                                ReturnList.push_back(Asset);
                            }
                        }
                    }
                }
            }
        }
    }
    return ReturnList;
}

std::weak_ptr< CPlayerAsset > CPlayerData::SelectAsset(const CPosition &pos, EAssetType assettype){
    std::shared_ptr< CPlayerAsset > BestAsset;
    int BestDistanceSquared = -1;
    
    if(atNone != assettype){
        for(auto &WeakAsset : DAssets){
            if(auto Asset = WeakAsset.lock()){
                if(Asset->Type() == assettype){
                    int CurrentDistance = Asset->Position().DistanceSquared(pos);
                    
                    if((-1 == BestDistanceSquared)||(CurrentDistance < BestDistanceSquared)){
                        BestDistanceSquared = CurrentDistance;
                        BestAsset = Asset;
                    }
                }
            }
        }
    }
    return BestAsset;
}

std::weak_ptr< CPlayerAsset > CPlayerData::FindNearestOwnedAsset(const CPosition &pos, const std::vector< EAssetType > assettypes){
    std::shared_ptr< CPlayerAsset > BestAsset;
    int BestDistanceSquared = -1;

    for(auto &WeakAsset : DAssets){
        if(auto Asset = WeakAsset.lock()){
            for(auto &AssetType : assettypes){
                if((Asset->Type() == AssetType)&&((aaConstruct != Asset->Action())||(atKeep == AssetType)||(atCastle == AssetType))){
                    int CurrentDistance = Asset->Position().DistanceSquared(pos);
                    
                    if((-1 == BestDistanceSquared)||(CurrentDistance < BestDistanceSquared)){
                        BestDistanceSquared = CurrentDistance;
                        BestAsset = Asset;
                    }
                    break;
                }
            }
        }
    }
    return BestAsset;
}

std::shared_ptr< CPlayerAsset > CPlayerData::FindNearestAsset(const CPosition &pos, EAssetType assettype){
    std::shared_ptr< CPlayerAsset > BestAsset;
    int BestDistanceSquared = -1;

    for(auto &Asset : DPlayerMap->Assets()){
        if(Asset->Type() == assettype){
            int CurrentDistance = Asset->Position().DistanceSquared(pos);
            
            if((-1 == BestDistanceSquared)||(CurrentDistance < BestDistanceSquared)){
                BestDistanceSquared = CurrentDistance;
                BestAsset = Asset;
            }
        }
    }
    return BestAsset;
}

std::weak_ptr< CPlayerAsset > CPlayerData::FindNearestEnemy(const CPosition &pos, int range){
    std::shared_ptr< CPlayerAsset > BestAsset;
    int BestDistanceSquared = -1;
    
    // Assume tile width == tile height
    if(0 < range){
        range = RangeToDistanceSquared(range);
    }
    for(auto Asset : DPlayerMap->Assets()){
        if((Asset->Color() != DColor)&&(Asset->Color() != pcNone)&&(Asset->Alive())){
            auto Command = Asset->CurrentCommand();
            if(aaCapability == Command.DAction){
                if((Command.DAssetTarget)&&(aaConstruct == Command.DAssetTarget->Action())){
                    continue;   
                }
            }
            if((aaConveyGold != Command.DAction)&&(aaConveyLumber != Command.DAction)&&(aaMineGold != Command.DAction)){
                int CurrentDistance = Asset->ClosestPosition(pos).DistanceSquared( pos );//Asset->Position().DistanceSquared(pos);
                
                if((0 > range)||(CurrentDistance <= range)){
                    if((-1 == BestDistanceSquared)||(CurrentDistance < BestDistanceSquared)){
                        BestDistanceSquared = CurrentDistance;
                        BestAsset = Asset;
                    }
                }
            }
        }
    }
    return BestAsset;
}

CPosition CPlayerData::FindBestAssetPlacement(const CPosition &pos, std::shared_ptr< CPlayerAsset > builder, EAssetType assettype, int buffer){
    auto AssetType = (*DAssetTypes)[CPlayerAssetType::TypeToName(assettype)];
    int PlacementSize = AssetType->Size() + 2 * buffer;
    int MaxDistance = MAX(DPlayerMap->Width(), DPlayerMap->Height()); 
    
    for(int Distance = 1; Distance < MaxDistance; Distance++){
        CPosition BestPosition;
        int BestDistance = -1;
        int LeftX = pos.X() - Distance;
        int TopY = pos.Y() - Distance;
        int RightX = pos.X() + Distance;
        int BottomY = pos.Y() + Distance;
        bool LeftValid = true;
        bool RightValid = true;
        bool TopValid = true;
        bool BottomValid = true;
        
        if(0 > LeftX){
            LeftValid = false;
            LeftX = 0;
        }
        if(0 > TopY){
            TopValid = false;
            TopY = 0;
        }
        if(DPlayerMap->Width() <= RightX){
            RightValid = false;
            RightX = DPlayerMap->Width() - 1;
        }
        if(DPlayerMap->Height() <= BottomY){
            BottomValid = false;
            BottomY = DPlayerMap->Height() - 1;
        }
        if(TopValid){
           for(int Index = LeftX; Index <= RightX; Index++){
                CPosition TempPosition(Index, TopY);
                if(DPlayerMap->CanPlaceAsset(TempPosition, PlacementSize, builder)){
                    int CurrentDistance = builder->TilePosition().DistanceSquared(TempPosition);
                    if((-1 == BestDistance)||(CurrentDistance < BestDistance)){
                        BestDistance = CurrentDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        if(RightValid){
           for(int Index = TopY; Index <= BottomY; Index++){
                CPosition TempPosition(RightX, Index);
                if(DPlayerMap->CanPlaceAsset(TempPosition, PlacementSize, builder)){
                    int CurrentDistance = builder->TilePosition().DistanceSquared(TempPosition);
                    if((-1 == BestDistance)||(CurrentDistance < BestDistance)){
                        BestDistance = CurrentDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        if(BottomValid){
           for(int Index = LeftX; Index <= RightX; Index++){
                CPosition TempPosition(Index, BottomY);
                if(DPlayerMap->CanPlaceAsset(TempPosition, PlacementSize, builder)){
                    int CurrentDistance = builder->TilePosition().DistanceSquared(TempPosition);
                    if((-1 == BestDistance)||(CurrentDistance < BestDistance)){
                        BestDistance = CurrentDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        if(LeftValid){
           for(int Index = TopY; Index <= BottomY; Index++){
                CPosition TempPosition(LeftX, Index);
                if(DPlayerMap->CanPlaceAsset(TempPosition, PlacementSize, builder)){
                    int CurrentDistance = builder->TilePosition().DistanceSquared(TempPosition);
                    if((-1 == BestDistance)||(CurrentDistance < BestDistance)){
                        BestDistance = CurrentDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        if(-1 != BestDistance){
            return CPosition(BestPosition.X() + buffer, BestPosition.Y() + buffer); 
        }
    }
    
    return CPosition(-1, -1);
}

int CPlayerData::PlayerAssetCount(EAssetType type){
    int Count = 0;
    
    for(auto Asset : DPlayerMap->Assets()){
        if((Asset->Color() == DColor)&&(type == Asset->Type())){
            Count++;
        }
    }
    
    return Count;
}

int CPlayerData::FoundAssetCount(EAssetType type){
    int Count = 0;
    
    for(auto Asset : DPlayerMap->Assets()){
        if(type == Asset->Type()){
            Count++;
        }
    }
    
    return Count;
}

std::list< std::weak_ptr< CPlayerAsset > > CPlayerData::IdleAssets() const{
    std::list< std::weak_ptr< CPlayerAsset > > AssetList;
    
    for(auto WeakAsset : DAssets){
        if(auto Asset = WeakAsset.lock()){
            if((aaNone == Asset->Action())&&(atNone != Asset->Type())){
                AssetList.push_back(Asset);
            }
        }
    }
    
    return AssetList;
}

void CPlayerData::AddUpgrade(const std::string &upgradename){
    auto Upgrade = CPlayerUpgrade::FindUpgradeFromName(upgradename);
    
    if(Upgrade){
        for(auto AssetType : Upgrade->AffectedAssets()){
            std::string AssetName = CPlayerAssetType::TypeToName(AssetType);
            auto AssetIterator = DAssetTypes->find(AssetName);
            
            if(AssetIterator != DAssetTypes->end()){
                AssetIterator->second->AddUpgrade(Upgrade);
            }
            
        }
        DUpgrades[CPlayerCapability::NameToType(upgradename)] = true;
    }
}

CGameModel::CGameModel(int mapindex, uint64_t seed, EPlayerColor newcolors[pcMax]){

    DHarvestTime = 5;
    DHarvestSteps = CPlayerAsset::UpdateFrequency() * DHarvestTime;
    DMineTime = 5;
    DMineSteps = CPlayerAsset::UpdateFrequency() * DMineTime;
    DConveyTime = 1;
    DConveySteps = CPlayerAsset::UpdateFrequency() * DConveyTime;
    DDeathTime = 1;
    DDeathSteps = CPlayerAsset::UpdateFrequency() * DDeathTime;
    DDecayTime = 4;
    DDecaySteps = CPlayerAsset::UpdateFrequency() * DDecayTime;
    DLumberPerHarvest = 100;
    DGoldPerMining = 100;
    
    DRandomNumberGenerator.Seed(seed);
    DActualMap = CAssetDecoratedMap::DuplicateMap(mapindex, newcolors);
    
    for(int PlayerIndex = 0; PlayerIndex < pcMax; PlayerIndex++){
        DPlayers[PlayerIndex] = std::make_shared< CPlayerData > (DActualMap, (EPlayerColor)PlayerIndex);
    }
    DAssetOccupancyMap.resize(DActualMap->Height());
    for(auto &Row : DAssetOccupancyMap){
        Row.resize(DActualMap->Width());
    }
    DDiagonalOccupancyMap.resize(DActualMap->Height());
    for(auto &Row : DDiagonalOccupancyMap){
        Row.resize(DActualMap->Width());
    }
    
    DLumberAvailable.resize(DActualMap->Height());
    for(auto &Row : DLumberAvailable){
        Row.resize(DActualMap->Width());
    }
    for(int Row = 0; Row < DActualMap->Height(); Row++){
        for(int Col = 0; Col < DActualMap->Width(); Col++){
            if(CTerrainMap::ttTree == DActualMap->TileType(Col, Row)){
                DLumberAvailable[Row][Col] = DPlayers[0]->Lumber();      
            }
            else{
                DLumberAvailable[Row][Col] = 0;    
            }
        }
    }
    
}

bool CGameModel::ValidAsset(std::shared_ptr< CPlayerAsset > asset){
    for(auto &Asset : DActualMap->Assets()){
        if(asset == Asset){
            return true;    
        }
    }
    return false;
}

std::shared_ptr< CPlayerData > CGameModel::Player(EPlayerColor color) const{
    if((0 > color)||(pcMax <= color)){
        return nullptr;   
    }
    return DPlayers[color];
}

void CGameModel::Timestep(){
    std::vector< SGameEvent > CurrentEvents;
    SGameEvent TempEvent;
    
    for(auto &Row : DAssetOccupancyMap){
        for(auto &Cell : Row){
            Cell = nullptr;
        }
    }
    for(auto &Row : DDiagonalOccupancyMap){
        for(int Index = 0; Index < Row.size(); Index++){
            Row[Index] = false;
        }
    }
    for(auto &Asset : DActualMap->Assets()){
        if((aaConveyGold != Asset->Action())&&(aaConveyLumber != Asset->Action())&&(aaMineGold != Asset->Action())){
            DAssetOccupancyMap[Asset->TilePositionY()][Asset->TilePositionX()] = Asset;
        }
    }
    for(int PlayerIndex = 1; PlayerIndex < pcMax; PlayerIndex++){
        if(DPlayers[PlayerIndex]->IsAlive()){
            DPlayers[PlayerIndex]->UpdateVisibility();
        }
    }
    auto AllAssets = DActualMap->Assets();
    for(auto &Asset : AllAssets){
        if(aaNone == Asset->Action()){
            Asset->PopCommand(); 
        }
        
        if(aaCapability == Asset->Action()){
            SAssetCommand Command = Asset->CurrentCommand();
            if(Command.DActivatedCapability){
                if(Command.DActivatedCapability->IncrementStep()){
                    // All Done    
                }
            }
            else{
                auto PlayerCapability = CPlayerCapability::FindCapability(Command.DCapability);
                
                Asset->PopCommand();
                if(PlayerCapability->CanApply(Asset, DPlayers[Asset->Color()], Command.DAssetTarget)){
                    PlayerCapability->ApplyCapability(Asset, DPlayers[Asset->Color()], Command.DAssetTarget);
                }
                else{
                    // Can't apply notify problem   
                }
            }
        } 
        else if(aaHarvestLumber == Asset->Action()){
            SAssetCommand Command = Asset->CurrentCommand();
            CPosition TilePosition = Command.DAssetTarget->TilePosition();
            EDirection HarvestDirection = Asset->TilePosition().AdjacentTileDirection(TilePosition);
            
            if(CTerrainMap::ttTree != DActualMap->TileType(TilePosition)){
                HarvestDirection = dMax;
                TilePosition = Asset->TilePosition();
            }
            if(dMax == HarvestDirection){
                if(TilePosition == Asset->TilePosition()){
                    CPosition NewPosition = DPlayers[Asset->Color()]->PlayerMap()->FindNearestReachableTileType(Asset->TilePosition(), CTerrainMap::ttTree);
                    // Find new lumber
                    Asset->PopCommand();
                    if(0 <= NewPosition.X()){
                        NewPosition.SetFromTile(NewPosition);
                        Command.DAssetTarget = DPlayers[Asset->Color()]->CreateMarker(NewPosition, false);
                        Asset->PushCommand(Command);
                        Command.DAction = aaWalk;
                        Asset->PushCommand(Command);
                        Asset->ResetStep();
                    }
                }
                else{
                    SAssetCommand NewCommand = Command;
                    
                    NewCommand.DAction = aaWalk;
                    Asset->PushCommand(NewCommand);
                    Asset->ResetStep();
                }
            }
            else{
                TempEvent.DType = etHarvest;
                TempEvent.DAsset = Asset;
                CurrentEvents.push_back(TempEvent);
                Asset->Direction(HarvestDirection);
                Asset->IncrementStep();
                if(DHarvestSteps <= Asset->Step()){
                    std::weak_ptr< CPlayerAsset > NearestRepository = DPlayers[Asset->Color()]->FindNearestOwnedAsset(Asset->Position(), {atTownHall, atKeep, atCastle, atLumberMill});
                    
                    DLumberAvailable[TilePosition.Y()][TilePosition.X()] -= DLumberPerHarvest;
                    if(0 >= DLumberAvailable[TilePosition.Y()][TilePosition.X()]){
                        DActualMap->ChangeTileType(TilePosition, CTerrainMap::ttStump);  
                    }
                    
                    if(!NearestRepository.expired()){
                        Command.DAction = aaConveyLumber;
                        Command.DAssetTarget = NearestRepository.lock();
                        Asset->PushCommand(Command);
                        Command.DAction = aaWalk;
                        Asset->PushCommand(Command);
                        Asset->Lumber(DLumberPerHarvest);
                        Asset->ResetStep();
                    }
                    else{
                        Asset->PopCommand();
                        Asset->Lumber(DLumberPerHarvest);
                        Asset->ResetStep();
                    }
                }
            }
        }
        else if(aaMineGold == Asset->Action()){
            SAssetCommand Command = Asset->CurrentCommand();
            CPosition ClosestPosition = Command.DAssetTarget->ClosestPosition(Asset->Position());
            CPosition TilePosition;
            EDirection MineDirection;
            
            TilePosition.SetToTile(ClosestPosition);
            MineDirection = Asset->TilePosition().AdjacentTileDirection(TilePosition);
            if((dMax == MineDirection)&&(TilePosition != Asset->TilePosition())){
                SAssetCommand NewCommand = Command;
                
                NewCommand.DAction = aaWalk;
                Asset->PushCommand(NewCommand);
                Asset->ResetStep();
            }
            else{
                if(0 == Asset->Step()){
                    if((Command.DAssetTarget->CommandCount() + 1) * DGoldPerMining <=  Command.DAssetTarget->Gold()){
                        SAssetCommand NewCommand;
                        NewCommand.DAction = aaBuild; // Pretend active
                        NewCommand.DAssetTarget = Asset;
                        
                        Command.DAssetTarget->EnqueueCommand(NewCommand);
                        Asset->IncrementStep();
                        Asset->TilePosition(Command.DAssetTarget->TilePosition());
                    }
                    else{
                        // Look for new mine or give up?   
                        Asset->PopCommand();
                    }
                }
                else{
                    Asset->IncrementStep();
                    if(DMineSteps <= Asset->Step()){
                        std::shared_ptr< CPlayerAsset > OldTarget = Command.DAssetTarget;
                        std::weak_ptr< CPlayerAsset > NearestRepository = DPlayers[Asset->Color()]->FindNearestOwnedAsset(Asset->Position(), {atTownHall, atKeep, atCastle});
                        CPosition NextTarget(DPlayers[Asset->Color()]->PlayerMap()->Width()-1,DPlayers[Asset->Color()]->PlayerMap()->Height()-1);
                        
                        Command.DAssetTarget->DecrementGold(DGoldPerMining);
                        Command.DAssetTarget->PopCommand();
                        if(0 >= Command.DAssetTarget->Gold()){
                            SAssetCommand NewCommand;

                            NewCommand.DAction = aaDeath;
                            Command.DAssetTarget->ClearCommand();
                            Command.DAssetTarget->PushCommand(NewCommand);
                            Command.DAssetTarget->ResetStep();
                        }
                        Asset->Gold(DGoldPerMining);
                        if(!NearestRepository.expired()){
                            Command.DAction = aaConveyGold;
                            Command.DAssetTarget = NearestRepository.lock();
                            Asset->PushCommand(Command);
                            Command.DAction = aaWalk;
                            Asset->PushCommand(Command);
                            Asset->ResetStep();
                            NextTarget = Command.DAssetTarget->TilePosition();
                        }
                        else{
                            Asset->PopCommand();
                        }
                        Asset->TilePosition(DPlayers[Asset->Color()]->PlayerMap()->FindAssetPlacement(Asset, OldTarget, NextTarget));
                    }
                }
            }
        }
        else if(aaStandGround == Asset->Action()){
            SAssetCommand Command = Asset->CurrentCommand();
            auto NewTarget = DPlayers[Asset->Color()]->FindNearestEnemy(Asset->Position(), Asset->EffectiveRange());
            
            if(NewTarget.expired()){
                Command.DAction = aaNone;
            }
            else{
                Command.DAction = aaAttack;
                Command.DAssetTarget = NewTarget.lock();
            }
            Asset->PushCommand(Command);
            Asset->ResetStep();
        }
        else if(aaRepair == Asset->Action()){
            SAssetCommand CurrentCommand = Asset->CurrentCommand();
            if(CurrentCommand.DAssetTarget->Alive()){
                EDirection RepairDirection = Asset->TilePosition().AdjacentTileDirection(CurrentCommand.DAssetTarget->TilePosition(), CurrentCommand.DAssetTarget->Size());
                if(dMax == RepairDirection){
                    SAssetCommand NextCommand = Asset->NextCommand();
                    
                    CurrentCommand.DAction = aaWalk;
                    Asset->PushCommand(CurrentCommand);
                    Asset->ResetStep();
                }
                else{
                    Asset->Direction(RepairDirection);
                    Asset->IncrementStep();
                    // Assume same movement as attack
                    if(Asset->Step() == Asset->AttackSteps()){
                        if(DPlayers[Asset->Color()]->Gold() && DPlayers[Asset->Color()]->Lumber()){
                            int RepairPoints = (CurrentCommand.DAssetTarget->MaxHitPoints() * (Asset->AttackSteps() + Asset->ReloadSteps())) / (CPlayerAsset::UpdateFrequency() * CurrentCommand.DAssetTarget->BuildTime());
                            
                            if(0 == RepairPoints){
                                RepairPoints = 1;   
                            }
                            DPlayers[Asset->Color()]->DecrementGold(1);
                            DPlayers[Asset->Color()]->DecrementLumber(1);
                            CurrentCommand.DAssetTarget->IncrementHitPoints(RepairPoints);
                            if(CurrentCommand.DAssetTarget->HitPoints() == CurrentCommand.DAssetTarget->MaxHitPoints()){
                                TempEvent.DType = etWorkComplete;
                                TempEvent.DAsset = Asset;
                                DPlayers[Asset->Color()]->AddGameEvent(TempEvent);
                                Asset->PopCommand();
                            }
                        }
                        else{
                            // Stop repair
                            Asset->PopCommand();   
                        }
                    }
                    if(Asset->Step() >= (Asset->AttackSteps() + Asset->ReloadSteps())){
                       Asset->ResetStep();
                    }
                }
            }
            else{
                Asset->PopCommand();
            }
        }
        else if(aaAttack == Asset->Action()){
            SAssetCommand CurrentCommand = Asset->CurrentCommand();
            if(atNone == Asset->Type()){
                CPosition ClosestTargetPosition = CurrentCommand.DAssetTarget->ClosestPosition(Asset->Position());
                CPosition DeltaPosition(ClosestTargetPosition.X() - Asset->PositionX(), ClosestTargetPosition.Y() - Asset->PositionY());
                int Movement = CPosition::TileWidth() * 5 / CPlayerAsset::UpdateFrequency();
                int TargetDistance = Asset->Position().Distance(ClosestTargetPosition);
                int Divisor = (TargetDistance + Movement - 1)/ Movement;
                
                if(Divisor){
                    DeltaPosition.X( DeltaPosition.X() / Divisor);    
                    DeltaPosition.Y( DeltaPosition.Y() / Divisor);
                }
                Asset->PositionX(Asset->PositionX() + DeltaPosition.X());
                Asset->PositionY(Asset->PositionY() + DeltaPosition.Y());
                Asset->Direction(Asset->Position().DirectionTo(ClosestTargetPosition));
                if(CPosition::HalfTileWidth() * CPosition::HalfTileHeight() > Asset->Position().DistanceSquared(ClosestTargetPosition)){
                    TempEvent.DType = etMissleHit;
                    TempEvent.DAsset = Asset;
                    CurrentEvents.push_back(TempEvent);
                    
                    if(CurrentCommand.DAssetTarget->Alive()){
                        SAssetCommand TargetCommand = CurrentCommand.DAssetTarget->CurrentCommand();
                        TempEvent.DType = etAttacked;
                        TempEvent.DAsset = CurrentCommand.DAssetTarget;
                        DPlayers[CurrentCommand.DAssetTarget->Color()]->AddGameEvent(TempEvent);
                        if(aaMineGold != TargetCommand.DAction){
                            
                            if((aaConveyGold == TargetCommand.DAction)||(aaConveyLumber == TargetCommand.DAction)){
                                // Damage the target 
                                CurrentCommand.DAssetTarget = TargetCommand.DAssetTarget;
                            }
                            else if((aaCapability == TargetCommand.DAction) && TargetCommand.DAssetTarget){
                                if(CurrentCommand.DAssetTarget->Speed() && (aaConstruct == TargetCommand.DAssetTarget->Action())){
                                    CurrentCommand.DAssetTarget = TargetCommand.DAssetTarget;
                                }
                            }
                            CurrentCommand.DAssetTarget->DecrementHitPoints(Asset->HitPoints());
                            if(!CurrentCommand.DAssetTarget->Alive()){
                                SAssetCommand Command = CurrentCommand.DAssetTarget->CurrentCommand();
                                
                                TempEvent.DType = etDeath;
                                TempEvent.DAsset = CurrentCommand.DAssetTarget;
                                CurrentEvents.push_back(TempEvent);
                                // Remove constructing
                                if((aaCapability == Command.DAction)&&(Command.DAssetTarget)){
                                    if(aaConstruct == Command.DAssetTarget->Action()){
                                        DPlayers[Command.DAssetTarget->Color()]->DeleteAsset(Command.DAssetTarget);
                                    }
                                }
                                else if(aaConstruct == Command.DAction){
                                    if(Command.DAssetTarget){
                                        Command.DAssetTarget->ClearCommand();
                                    }
                                }
                                CurrentCommand.DAssetTarget->Direction((EDirection)((Asset->Direction() + dMax/2) % dMax));
                                Command.DAction = aaDeath;
                                CurrentCommand.DAssetTarget->ClearCommand();
                                CurrentCommand.DAssetTarget->PushCommand(Command);
                                CurrentCommand.DAssetTarget->ResetStep();
                            }
                        }
                    }
                    DPlayers[Asset->Color()]->DeleteAsset(Asset);
                }
            }
            else if(CurrentCommand.DAssetTarget->Alive()){
                if(1 == Asset->EffectiveRange()){
                    EDirection AttackDirection = Asset->TilePosition().AdjacentTileDirection(CurrentCommand.DAssetTarget->TilePosition(), CurrentCommand.DAssetTarget->Size());
                    if(dMax == AttackDirection){
                        SAssetCommand NextCommand = Asset->NextCommand();
                        
                        if(aaStandGround != NextCommand.DAction){
                            CurrentCommand.DAction = aaWalk;
                            Asset->PushCommand(CurrentCommand);
                            Asset->ResetStep();
                        }
                        else{
                            Asset->PopCommand();      
                        }
                    }
                    else{
                        Asset->Direction(AttackDirection);
                        Asset->IncrementStep();
                        if(Asset->Step() == Asset->AttackSteps()){
                            int Damage = Asset->EffectiveBasicDamage() - CurrentCommand.DAssetTarget->EffectiveArmor();
                            Damage = 0 > Damage ? 0 : Damage;
                            Damage += Asset->EffectivePiercingDamage();
                            if(DRandomNumberGenerator.Random() & 0x1){ // 50% chance half damage
                                Damage /= 2;
                            }
                            CurrentCommand.DAssetTarget->DecrementHitPoints(Damage);
                            TempEvent.DType = etMeleeHit;
                            TempEvent.DAsset = Asset;
                            CurrentEvents.push_back(TempEvent);
                            TempEvent.DType = etAttacked;
                            TempEvent.DAsset = CurrentCommand.DAssetTarget;
                            DPlayers[CurrentCommand.DAssetTarget->Color()]->AddGameEvent(TempEvent);
                            if(!CurrentCommand.DAssetTarget->Alive()){
                                SAssetCommand Command = CurrentCommand.DAssetTarget->CurrentCommand();
                                
                                TempEvent.DType = etDeath;
                                TempEvent.DAsset = CurrentCommand.DAssetTarget;
                                CurrentEvents.push_back(TempEvent);
                                // Remove constructing
                                if((aaCapability == Command.DAction)&&(Command.DAssetTarget)){
                                    if(aaConstruct == Command.DAssetTarget->Action()){
                                        DPlayers[Command.DAssetTarget->Color()]->DeleteAsset(Command.DAssetTarget);
                                    }
                                }
                                else if(aaConstruct == Command.DAction){
                                    if(Command.DAssetTarget){
                                        Command.DAssetTarget->ClearCommand();
                                    }
                                }
                                Command.DCapability = actNone;
                                Command.DAssetTarget = nullptr;
                                Command.DActivatedCapability = nullptr;
                                CurrentCommand.DAssetTarget->Direction((EDirection)((AttackDirection + dMax/2) % dMax));
                                Command.DAction = aaDeath;
                                CurrentCommand.DAssetTarget->ClearCommand();
                                CurrentCommand.DAssetTarget->PushCommand(Command);
                                CurrentCommand.DAssetTarget->ResetStep();
                            }
                        }
                        if(Asset->Step() >= (Asset->AttackSteps() + Asset->ReloadSteps())){
                           Asset->ResetStep();
                        }
                    }
                }
                else{ // EffectiveRanged
                    CPosition ClosestTargetPosition = CurrentCommand.DAssetTarget->ClosestPosition(Asset->Position());
                    if(ClosestTargetPosition.DistanceSquared( Asset->Position() ) > RangeToDistanceSquared(Asset->EffectiveRange())){
                        SAssetCommand NextCommand = Asset->NextCommand();
                        
                        if(aaStandGround != NextCommand.DAction){
                            CurrentCommand.DAction = aaWalk;
                            Asset->PushCommand(CurrentCommand);
                            Asset->ResetStep();
                        }
                        else{
                            Asset->PopCommand();
                        }   
                    }
                    else{
                        /*
                        CPosition DeltaPosition(ClosestTargetPosition.X() - Asset->PositionX(), ClosestTargetPosition.Y() - Asset->PositionY());
                        int DivX = DeltaPosition.X() / CPosition::HalfTileWidth();
                        int DivY = DeltaPosition.Y() / CPosition::HalfTileHeight();
                        int Div;
                        EDirection AttackDirection;
                        DivX = 0 > DivX ? -DivX : DivX;
                        DivY = 0 > DivY ? -DivY : DivY;
                        Div = DivX > DivY ? DivX : DivY;

                        if(Div){
                            DeltaPosition.X(DeltaPosition.X() / Div);
                            DeltaPosition.Y(DeltaPosition.Y() / Div);
                        }
                        DeltaPosition.IncrementX(CPosition::HalfTileWidth());
                        DeltaPosition.IncrementY(CPosition::HalfTileHeight());
                        if(0 > DeltaPosition.X()){
                            DeltaPosition.X(0);
                        }
                        if(0 > DeltaPosition.Y()){
                            DeltaPosition.Y(0);
                        }
                        if(CPosition::TileWidth() <= DeltaPosition.X()){
                            DeltaPosition.X(CPosition::TileWidth() - 1);
                        }
                        if(CPosition::TileHeight() <= DeltaPosition.Y()){
                            DeltaPosition.Y(CPosition::TileHeight() - 1);
                        }
                        AttackDirection = DeltaPosition.TileOctant();
                        */
                        EDirection AttackDirection = Asset->Position().DirectionTo(ClosestTargetPosition);
                        Asset->Direction(AttackDirection);
                        Asset->IncrementStep();
                        if(Asset->Step() == Asset->AttackSteps()){
                            SAssetCommand AttackCommand; // Create missle
                            auto ArrowAsset = DPlayers[pcNone]->CreateAsset("None");
                            int Damage = Asset->EffectiveBasicDamage() - CurrentCommand.DAssetTarget->EffectiveArmor();
                            Damage = 0 > Damage ? 0 : Damage;
                            Damage += Asset->EffectivePiercingDamage();
                            if(DRandomNumberGenerator.Random() & 0x1){ // 50% chance half damage
                                Damage /= 2;
                            }
                            
                            TempEvent.DType = etMissleFire;
                            TempEvent.DAsset = Asset;
                            CurrentEvents.push_back(TempEvent);
                            
                            ArrowAsset->HitPoints(Damage);
                            ArrowAsset->Position(Asset->Position());
                            if(ArrowAsset->PositionX() < ClosestTargetPosition.X()){
                                ArrowAsset->PositionX(ArrowAsset->PositionX() + CPosition::HalfTileWidth());
                            }
                            else if(ArrowAsset->PositionX() > ClosestTargetPosition.X()){
                                ArrowAsset->PositionX(ArrowAsset->PositionX() - CPosition::HalfTileWidth());
                            }
                            
                            if(ArrowAsset->PositionY() < ClosestTargetPosition.Y()){
                                ArrowAsset->PositionY(ArrowAsset->PositionY() + CPosition::HalfTileHeight());
                            }
                            else if(ArrowAsset->PositionY() > ClosestTargetPosition.Y()){
                                ArrowAsset->PositionY(ArrowAsset->PositionY() - CPosition::HalfTileHeight());
                            }
                            ArrowAsset->Direction(AttackDirection);
                            AttackCommand.DAction = aaConstruct;
                            AttackCommand.DAssetTarget = Asset;
                            ArrowAsset->PushCommand(AttackCommand);
                            AttackCommand.DAction = aaAttack;
                            AttackCommand.DAssetTarget = CurrentCommand.DAssetTarget;
                            ArrowAsset->PushCommand(AttackCommand);
                        }
                        if(Asset->Step() >= (Asset->AttackSteps() + Asset->ReloadSteps())){
                           Asset->ResetStep();
                        }
                    }
                }
            }
            else{
                SAssetCommand NextCommand = Asset->NextCommand();
                
                Asset->PopCommand();
                if(aaStandGround != NextCommand.DAction){
                    auto NewTarget = DPlayers[Asset->Color()]->FindNearestEnemy(Asset->Position(), Asset->EffectiveSight());

                    if(!NewTarget.expired()){
                        CurrentCommand.DAssetTarget = NewTarget.lock();
                        Asset->PushCommand(CurrentCommand);
                        Asset->ResetStep();
                    }
                }
            }
        }
        else if((aaConveyLumber == Asset->Action())||(aaConveyGold == Asset->Action())){
            Asset->IncrementStep();
            if(DConveySteps <= Asset->Step()){
                SAssetCommand Command = Asset->CurrentCommand();
                CPosition NextTarget(DPlayers[Asset->Color()]->PlayerMap()->Width()-1,DPlayers[Asset->Color()]->PlayerMap()->Height()-1);
                
                DPlayers[Asset->Color()]->IncrementGold(Asset->Gold());
                DPlayers[Asset->Color()]->IncrementLumber(Asset->Lumber());
                Asset->Gold(0);
                Asset->Lumber(0);
                Asset->PopCommand();
                Asset->ResetStep();
                if(aaNone != Asset->Action()){
                    NextTarget = Asset->CurrentCommand().DAssetTarget->TilePosition();
                }
                Asset->TilePosition(DPlayers[Asset->Color()]->PlayerMap()->FindAssetPlacement(Asset, Command.DAssetTarget, NextTarget));
            }
        }
        else if(aaConstruct == Asset->Action()){
            SAssetCommand Command = Asset->CurrentCommand();
            if(Command.DActivatedCapability){
                if(Command.DActivatedCapability->IncrementStep()){
                    // All Done    
                }
            }   
        }
        else if(aaDeath == Asset->Action()){
            Asset->IncrementStep();
            if(Asset->Step() > DDeathSteps){
                if(Asset->Speed()){
                    SAssetCommand DecayCommand;
                    // Create corpse
                    auto CorpseAsset = DPlayers[pcNone]->CreateAsset("None");
                    
                    DecayCommand.DAction = aaDecay;
                    CorpseAsset->Position(Asset->Position());
                    CorpseAsset->Direction(Asset->Direction());
                    CorpseAsset->PushCommand(DecayCommand);
                    
                }
                DPlayers[Asset->Color()]->DeleteAsset(Asset);
            }
        }
        else if(aaDecay == Asset->Action()){
            Asset->IncrementStep();
            if(Asset->Step() > DDecaySteps){
                DPlayers[Asset->Color()]->DeleteAsset(Asset);
            }
        }
        
        if(aaWalk == Asset->Action()){
            if(Asset->TileAligned()){
                SAssetCommand Command = Asset->CurrentCommand();
                SAssetCommand NextCommand = Asset->NextCommand();
                EDirection TravelDirection;
                CPosition MapTarget = Command.DAssetTarget->ClosestPosition(Asset->Position());
                
                if(aaAttack == NextCommand.DAction){
                    // Check to see if can attack now
                    if(NextCommand.DAssetTarget->ClosestPosition(Asset->Position()).DistanceSquared( Asset->Position() ) <= RangeToDistanceSquared(Asset->EffectiveRange())){
                        Asset->PopCommand();
                        Asset->ResetStep();
                        continue;   
                    }
                }
                TravelDirection = DRouterMap.FindRoute(*DPlayers[Asset->Color()]->PlayerMap(), *Asset, MapTarget);
                if(dMax != TravelDirection){
                    Asset->Direction(TravelDirection);
                }
                else{
                    CPosition TilePosition;
                    TilePosition.SetToTile(MapTarget);
                    if((TilePosition == Asset->TilePosition())||(dMax != Asset->TilePosition().AdjacentTileDirection(TilePosition))){
                        Asset->PopCommand();
                        Asset->ResetStep();
                        continue;    
                    }
                    else if(aaHarvestLumber == NextCommand.DAction){
                        CPosition NewPosition = DPlayers[Asset->Color()]->PlayerMap()->FindNearestReachableTileType(Asset->TilePosition(), CTerrainMap::ttTree);
                        // Find new lumber
                        Asset->PopCommand();
                        Asset->PopCommand();
                        if(0 <= NewPosition.X()){
                            NewPosition.SetFromTile(NewPosition);
                            Command.DAction = aaHarvestLumber;
                            Command.DAssetTarget = DPlayers[Asset->Color()]->CreateMarker(NewPosition, false);
                            Asset->PushCommand(Command);
                            Command.DAction = aaWalk;
                            Asset->PushCommand(Command);
                            Asset->ResetStep();
                            continue;
                        }
                    }
                    else{
                        Command.DAction = aaNone;
                        Asset->PushCommand(Command);
                        Asset->ResetStep();
                        continue;    
                    }
                }
            }

            if(!Asset->MoveStep(DAssetOccupancyMap, DDiagonalOccupancyMap)){
                Asset->Direction((EDirection)((Asset->Position().TileOctant() + dMax/2) % dMax));
         
            }
        }
    }
    DGameCycle++;
    for(int PlayerIndex = 0; PlayerIndex < pcMax; PlayerIndex++){
        DPlayers[PlayerIndex]->IncrementCycle();
        DPlayers[PlayerIndex]->AppendGameEvents(CurrentEvents);
    }
}

void CGameModel::ClearGameEvents(){
    for(int PlayerIndex = 0; PlayerIndex < pcMax; PlayerIndex++){
        DPlayers[PlayerIndex]->ClearGameEvents();
    }
}

