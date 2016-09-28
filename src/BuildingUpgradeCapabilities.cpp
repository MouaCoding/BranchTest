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
class CPlayerCapabilityBuildingUpgrade : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                std::shared_ptr< CPlayerAssetType > DOriginalType;
                std::shared_ptr< CPlayerAssetType > DUpgradeType;
                int DCurrentStep;
                int DTotalSteps;
                int DLumber;
                int DGold;
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, std::shared_ptr< CPlayerAssetType > origtype, std::shared_ptr< CPlayerAssetType > upgradetype, int lumber, int gold, int steps);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        std::string DBuildingName;
        CPlayerCapabilityBuildingUpgrade(const std::string &buildingname);
        
    public:
        virtual ~CPlayerCapabilityBuildingUpgrade(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityBuildingUpgrade::CRegistrant CPlayerCapabilityBuildingUpgrade::DRegistrant;

CPlayerCapabilityBuildingUpgrade::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildingUpgrade >(new CPlayerCapabilityBuildingUpgrade("Keep")));   
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildingUpgrade >(new CPlayerCapabilityBuildingUpgrade("Castle")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildingUpgrade >(new CPlayerCapabilityBuildingUpgrade("GuardTower")));   
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildingUpgrade >(new CPlayerCapabilityBuildingUpgrade("CannonTower")));
}

CPlayerCapabilityBuildingUpgrade::CPlayerCapabilityBuildingUpgrade(const std::string &buildingname) : CPlayerCapability(std::string("Build") + buildingname, ttNone){
    DBuildingName = buildingname;
}

bool CPlayerCapabilityBuildingUpgrade::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    auto Iterator = playerdata->AssetTypes()->find(DBuildingName);
    
    if(Iterator != playerdata->AssetTypes()->end()){
        auto AssetType = Iterator->second;
        if(AssetType->LumberCost() > playerdata->Lumber()){
            return false;   
        }
        if(AssetType->GoldCost() > playerdata->Gold()){
            return false;   
        }
        if(!playerdata->AssetRequirementsMet(DBuildingName)){
            return false;
        }
    }
    
    return true;
}

bool CPlayerCapabilityBuildingUpgrade::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    return CanInitiate(actor, playerdata);
}

bool CPlayerCapabilityBuildingUpgrade::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    auto Iterator = playerdata->AssetTypes()->find(DBuildingName);
    
    if(Iterator != playerdata->AssetTypes()->end()){
        SAssetCommand NewCommand;
        auto AssetType = Iterator->second;
        
        actor->ClearCommand();
        NewCommand.DAction = aaCapability;
        NewCommand.DCapability = AssetCapabilityType();
        NewCommand.DAssetTarget = target;
        NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target, actor->AssetType(), AssetType, AssetType->LumberCost(), AssetType->GoldCost(), CPlayerAsset::UpdateFrequency() * AssetType->BuildTime());
        actor->PushCommand(NewCommand);
        
        return true;
    }
    return false;
}

CPlayerCapabilityBuildingUpgrade::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, std::shared_ptr< CPlayerAssetType > origtype, std::shared_ptr< CPlayerAssetType > upgradetype, int lumber, int gold, int steps) :
CActivatedPlayerCapability(actor, playerdata, target){
    SAssetCommand AssetCommand;
    
    DOriginalType = origtype;
    DUpgradeType = upgradetype;
    DCurrentStep = 0;
    DTotalSteps = steps;
    DLumber = lumber;
    DGold = gold;
    DPlayerData->DecrementLumber(DLumber);
    DPlayerData->DecrementGold(DGold);
}


int CPlayerCapabilityBuildingUpgrade::CActivatedCapability::PercentComplete(int max){
    return DCurrentStep * max / DTotalSteps;
}

bool CPlayerCapabilityBuildingUpgrade::CActivatedCapability::IncrementStep(){
    int AddHitPoints = ((DUpgradeType->HitPoints() - DOriginalType->HitPoints()) * (DCurrentStep + 1) / DTotalSteps) - ((DUpgradeType->HitPoints() - DOriginalType->HitPoints()) * DCurrentStep / DTotalSteps);
    
    if(0 == DCurrentStep){
        SAssetCommand AssetCommand = DActor->CurrentCommand();
        AssetCommand.DAction = aaConstruct;
        DActor->PopCommand();
        DActor->PushCommand(AssetCommand);
        DActor->ChangeType(DUpgradeType);  
        DActor->ResetStep();
    }
    
    DActor->IncrementHitPoints(AddHitPoints);
    if(DActor->HitPoints() > DActor->MaxHitPoints()){
        DActor->HitPoints(DActor->MaxHitPoints());
    }
    DCurrentStep++;
    DActor->IncrementStep();
    if(DCurrentStep >= DTotalSteps){
        SGameEvent TempEvent;
        
        TempEvent.DType = etWorkComplete;
        TempEvent.DAsset = DActor;
        DPlayerData->AddGameEvent(TempEvent);
        
        DActor->PopCommand();
        if(DActor->Range()){
            SAssetCommand Command;
            Command.DAction = aaStandGround;
            DActor->PushCommand(Command);   
        }
        return true;    
    }
    return false;
}

void CPlayerCapabilityBuildingUpgrade::CActivatedCapability::Cancel(){
    DPlayerData->IncrementLumber(DLumber);
    DPlayerData->IncrementGold(DGold);
    DActor->ChangeType(DOriginalType);
    DActor->PopCommand();
}


