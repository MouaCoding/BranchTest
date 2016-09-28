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
class CPlayerCapabilityTrainNormal : public CPlayerCapability{
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
        std::string DUnitName;
        CPlayerCapabilityTrainNormal(const std::string &unitname);
        
    public:
        virtual ~CPlayerCapabilityTrainNormal(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityTrainNormal::CRegistrant CPlayerCapabilityTrainNormal::DRegistrant;

CPlayerCapabilityTrainNormal::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityTrainNormal >(new CPlayerCapabilityTrainNormal("Peasant")));   
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityTrainNormal >(new CPlayerCapabilityTrainNormal("Footman")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityTrainNormal >(new CPlayerCapabilityTrainNormal("Archer")));   
}

CPlayerCapabilityTrainNormal::CPlayerCapabilityTrainNormal(const std::string &unitname) : CPlayerCapability(std::string("Build") + unitname, ttNone){
    DUnitName = unitname;
}

bool CPlayerCapabilityTrainNormal::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    auto Iterator = playerdata->AssetTypes()->find(DUnitName);
    
    if(Iterator != playerdata->AssetTypes()->end()){
        auto AssetType = Iterator->second;
        if(AssetType->LumberCost() > playerdata->Lumber()){
            return false;   
        }
        if(AssetType->GoldCost() > playerdata->Gold()){
            return false;   
        }
        if((AssetType->FoodConsumption() + playerdata->FoodConsumption()) > playerdata->FoodProduction()){
            return false;    
        }
        if(!playerdata->AssetRequirementsMet(DUnitName)){
            return false;
        }
    }
    
    return true;
}

bool CPlayerCapabilityTrainNormal::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    return CanInitiate(actor, playerdata);
}

bool CPlayerCapabilityTrainNormal::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    auto Iterator = playerdata->AssetTypes()->find(DUnitName);
    
    if(Iterator != playerdata->AssetTypes()->end()){
        auto AssetType = Iterator->second;
        auto NewAsset = playerdata->CreateAsset(DUnitName);
        SAssetCommand NewCommand;
        CPosition TilePosition;
        TilePosition.SetToTile(actor->Position());
        NewAsset->TilePosition(TilePosition);
        NewAsset->HitPoints(1);
        
        NewCommand.DAction = aaCapability;
        NewCommand.DCapability = AssetCapabilityType();
        NewCommand.DAssetTarget = NewAsset;
        NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, NewAsset, AssetType->LumberCost(), AssetType->GoldCost(), CPlayerAsset::UpdateFrequency() * AssetType->BuildTime());
        actor->PushCommand(NewCommand);
        actor->ResetStep();
    }
    return false;
}

CPlayerCapabilityTrainNormal::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, int lumber, int gold, int steps) :
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


int CPlayerCapabilityTrainNormal::CActivatedCapability::PercentComplete(int max){
    return DCurrentStep * max / DTotalSteps;
}

bool CPlayerCapabilityTrainNormal::CActivatedCapability::IncrementStep(){
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
        
        TempEvent.DType = etReady;
        TempEvent.DAsset = DTarget;
        DPlayerData->AddGameEvent(TempEvent);
        
        DTarget->PopCommand();
        DActor->PopCommand();
        DTarget->TilePosition(DPlayerData->PlayerMap()->FindAssetPlacement(DTarget, DActor, CPosition(DPlayerData->PlayerMap()->Width()-1, DPlayerData->PlayerMap()->Height()-1)));
        return true;    
    }
    return false;
}

void CPlayerCapabilityTrainNormal::CActivatedCapability::Cancel(){
    DPlayerData->IncrementLumber(DLumber);
    DPlayerData->IncrementGold(DGold);
    DPlayerData->DeleteAsset(DTarget);
    DActor->PopCommand();
}


