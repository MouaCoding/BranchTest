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

// Build normal buildings capability
class CPlayerCapabilityBuildNormal : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                int DCurrentStep;
                int DTotalSteps;
                int DLumber;
                int DGold;
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, int lumber, int gold, int steps);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        std::string DBuildingName;
        CPlayerCapabilityBuildNormal(const std::string &buildingname);
        
    public:
        virtual ~CPlayerCapabilityBuildNormal(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityBuildNormal::CRegistrant CPlayerCapabilityBuildNormal::DRegistrant;

CPlayerCapabilityBuildNormal::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildNormal >(new CPlayerCapabilityBuildNormal("TownHall")));   
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildNormal >(new CPlayerCapabilityBuildNormal("Farm")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildNormal >(new CPlayerCapabilityBuildNormal("Barracks")));   
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildNormal >(new CPlayerCapabilityBuildNormal("LumberMill")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildNormal >(new CPlayerCapabilityBuildNormal("Blacksmith")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildNormal >(new CPlayerCapabilityBuildNormal("ScoutTower")));
}

CPlayerCapabilityBuildNormal::CPlayerCapabilityBuildNormal(const std::string &buildingname) : CPlayerCapability(std::string("Build") + buildingname, ttTerrainOrAsset){
    DBuildingName = buildingname;
}

bool CPlayerCapabilityBuildNormal::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    auto Iterator = playerdata->AssetTypes()->find(DBuildingName);
    
    if(Iterator != playerdata->AssetTypes()->end()){
        auto AssetType = Iterator->second;
        if(AssetType->LumberCost() > playerdata->Lumber()){
            return false;   
        }
        if(AssetType->GoldCost() > playerdata->Gold()){
            return false;   
        }
    }
    
    return true;
}

bool CPlayerCapabilityBuildNormal::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    auto Iterator = playerdata->AssetTypes()->find(DBuildingName);
    
    if((actor != target)&&(atNone != target->Type())){
        return false;
    }
    if(Iterator != playerdata->AssetTypes()->end()){
        auto AssetType = Iterator->second;

        if(AssetType->LumberCost() > playerdata->Lumber()){
            return false;   
        }
        if(AssetType->GoldCost() > playerdata->Gold()){
            return false;   
        }
        if(!playerdata->PlayerMap()->CanPlaceAsset(target->TilePosition(), AssetType->Size(), actor)){
            return false;
        }
    }

    return true;
}

bool CPlayerCapabilityBuildNormal::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    auto Iterator = playerdata->AssetTypes()->find(DBuildingName);
    
    if(Iterator != playerdata->AssetTypes()->end()){
        SAssetCommand NewCommand;
        
        actor->ClearCommand();
        if(actor->TilePosition() == target->TilePosition()){
            auto AssetType = Iterator->second;
            auto NewAsset = playerdata->CreateAsset(DBuildingName);
            CPosition TilePosition;
            TilePosition.SetToTile(target->Position());
            NewAsset->TilePosition(TilePosition);
            NewAsset->HitPoints(1);

            NewCommand.DAction = aaCapability;
            NewCommand.DCapability = AssetCapabilityType();
            NewCommand.DAssetTarget = NewAsset;
            NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, NewAsset, AssetType->LumberCost(), AssetType->GoldCost(), CPlayerAsset::UpdateFrequency() * AssetType->BuildTime());
            actor->PushCommand(NewCommand);
        }
        else{
            NewCommand.DAction = aaCapability;
            NewCommand.DCapability = AssetCapabilityType();
            NewCommand.DAssetTarget = target;
            actor->PushCommand(NewCommand);
            
            NewCommand.DAction = aaWalk;
            actor->PushCommand(NewCommand);
        }
        return true;
    }
    return false;
}

CPlayerCapabilityBuildNormal::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, int lumber, int gold, int steps) :
CActivatedPlayerCapability(actor, playerdata, target){
    SAssetCommand AssetCommand;
    
    DCurrentStep = 0;
    DTotalSteps = steps;
    DLumber = lumber;
    DGold = gold;
    DPlayerData->DecrementLumber(DLumber);
    DPlayerData->DecrementGold(DGold);
    AssetCommand.DAction = aaConstruct;
    AssetCommand.DAssetTarget = DActor;
    DTarget->PushCommand(AssetCommand);
}


int CPlayerCapabilityBuildNormal::CActivatedCapability::PercentComplete(int max){
    return DCurrentStep * max / DTotalSteps;
}

bool CPlayerCapabilityBuildNormal::CActivatedCapability::IncrementStep(){
    int AddHitPoints = (DTarget->MaxHitPoints() * (DCurrentStep + 1) / DTotalSteps) - (DTarget->MaxHitPoints() * DCurrentStep / DTotalSteps);
    
    DTarget->IncrementHitPoints(AddHitPoints);
    if(DTarget->HitPoints() > DTarget->MaxHitPoints()){
        DTarget->HitPoints(DTarget->MaxHitPoints());
    }
    DCurrentStep++;
    DActor->IncrementStep();
    DTarget->IncrementStep();
    if(DCurrentStep >= DTotalSteps){
        SGameEvent TempEvent;
        
        TempEvent.DType = etWorkComplete;
        TempEvent.DAsset = DActor;
        DPlayerData->AddGameEvent(TempEvent);
        
        DTarget->PopCommand();
        DActor->PopCommand();
        DActor->TilePosition(DPlayerData->PlayerMap()->FindAssetPlacement(DActor, DTarget, CPosition(DPlayerData->PlayerMap()->Width()-1, DPlayerData->PlayerMap()->Height()-1)));
        DActor->ResetStep();
        DTarget->ResetStep();
        
        return true;    
    }
    return false;
}

void CPlayerCapabilityBuildNormal::CActivatedCapability::Cancel(){
    DPlayerData->IncrementLumber(DLumber);
    DPlayerData->IncrementGold(DGold);
    DPlayerData->DeleteAsset(DTarget);
    DActor->PopCommand();
}


