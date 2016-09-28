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

// Upgrade capability
class CPlayerCapabilityUnitUpgrade : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                std::shared_ptr< CPlayerAssetType > DUpgradingType;
                std::string DUpgradeName;
                int DCurrentStep;
                int DTotalSteps;
                int DLumber;
                int DGold;
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, std::shared_ptr< CPlayerAssetType > upgradingtype, const std::string &upgradename, int lumber, int gold, int steps);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        std::string DUpgradeName;
        CPlayerCapabilityUnitUpgrade(const std::string &upgradename);
        
    public:
        virtual ~CPlayerCapabilityUnitUpgrade(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityUnitUpgrade::CRegistrant CPlayerCapabilityUnitUpgrade::DRegistrant;

CPlayerCapabilityUnitUpgrade::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("WeaponUpgrade2")));   
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("WeaponUpgrade3")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("ArmorUpgrade2")));   
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("ArmorUpgrade3")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("ArrowUpgrade2")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("ArrowUpgrade3")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("Longbow")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("RangerScouting")));
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityUnitUpgrade >(new CPlayerCapabilityUnitUpgrade("Marksmanship")));
}

CPlayerCapabilityUnitUpgrade::CPlayerCapabilityUnitUpgrade(const std::string &upgradename) : CPlayerCapability(upgradename, ttNone){
    DUpgradeName = upgradename;
}

bool CPlayerCapabilityUnitUpgrade::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    auto Upgrade = CPlayerUpgrade::FindUpgradeFromName(DUpgradeName);
    
    if(Upgrade){

        if(Upgrade->LumberCost() > playerdata->Lumber()){
            return false;   
        }
        if(Upgrade->GoldCost() > playerdata->Gold()){
            return false;   
        }
        /*
        if(!playerdata->AssetRequirementsMet(DUpgradeName)){
            return false;
        }
        */
    }
    
    return true;
}

bool CPlayerCapabilityUnitUpgrade::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    return CanInitiate(actor, playerdata);
}

bool CPlayerCapabilityUnitUpgrade::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    auto Upgrade = CPlayerUpgrade::FindUpgradeFromName(DUpgradeName);
    
    if(Upgrade){
        SAssetCommand NewCommand;
        
        actor->ClearCommand();
        NewCommand.DAction = aaCapability;
        NewCommand.DCapability = AssetCapabilityType();
        NewCommand.DAssetTarget = target;
        NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target, actor->AssetType(), DUpgradeName, Upgrade->LumberCost(), Upgrade->GoldCost(), CPlayerAsset::UpdateFrequency() * Upgrade->ResearchTime());
        actor->PushCommand(NewCommand);
        
        return true;
    }
    return false;
}

CPlayerCapabilityUnitUpgrade::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, std::shared_ptr< CPlayerAssetType > upgradingtype, const std::string &upgradename, int lumber, int gold, int steps) :
CActivatedPlayerCapability(actor, playerdata, target){
    SAssetCommand AssetCommand;
    
    DUpgradingType = upgradingtype;
    DUpgradeName = upgradename;
    DCurrentStep = 0;
    DTotalSteps = steps;
    DLumber = lumber;
    DGold = gold;
    DPlayerData->DecrementLumber(DLumber);
    DPlayerData->DecrementGold(DGold);
    DUpgradingType->RemoveCapability(CPlayerCapability::NameToType(DUpgradeName));
}


int CPlayerCapabilityUnitUpgrade::CActivatedCapability::PercentComplete(int max){
    return DCurrentStep * max / DTotalSteps;
}

bool CPlayerCapabilityUnitUpgrade::CActivatedCapability::IncrementStep(){
    DCurrentStep++;
    DActor->IncrementStep();
    if(DCurrentStep >= DTotalSteps){
        DPlayerData->AddUpgrade(DUpgradeName);
        DActor->PopCommand();
        if(DUpgradeName.rfind("2") == (DUpgradeName.length()-1)){
            DUpgradingType->AddCapability(CPlayerCapability::NameToType(DUpgradeName.substr(0, DUpgradeName.length()-1) + "3"));
        }
        
        return true;    
    }
    return false;
}

void CPlayerCapabilityUnitUpgrade::CActivatedCapability::Cancel(){
    DPlayerData->IncrementLumber(DLumber);
    DPlayerData->IncrementGold(DGold);
    DUpgradingType->AddCapability(CPlayerCapability::NameToType(DUpgradeName));
    DActor->PopCommand();
}


class CPlayerCapabilityBuildRanger : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                std::shared_ptr< CPlayerAssetType > DUpgradingType;
                std::string DUnitName;
                int DCurrentStep;
                int DTotalSteps;
                int DLumber;
                int DGold;
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, std::shared_ptr< CPlayerAssetType > upgradingtype, const std::string &unitname, int lumber, int gold, int steps);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        std::string DUnitName;
        CPlayerCapabilityBuildRanger(const std::string &unitname);
        
    public:
        virtual ~CPlayerCapabilityBuildRanger(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityBuildRanger::CRegistrant CPlayerCapabilityBuildRanger::DRegistrant;

CPlayerCapabilityBuildRanger::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityBuildRanger >(new CPlayerCapabilityBuildRanger("Ranger")));   
}

CPlayerCapabilityBuildRanger::CPlayerCapabilityBuildRanger(const std::string &unitname) : CPlayerCapability(std::string("Build") + unitname, ttNone){
    DUnitName = unitname;
}

bool CPlayerCapabilityBuildRanger::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    
    if(atLumberMill == actor->Type()){
        auto Upgrade = CPlayerUpgrade::FindUpgradeFromName(std::string("Build") + DUnitName);
        
        if(Upgrade){
    
            if(Upgrade->LumberCost() > playerdata->Lumber()){
                return false;   
            }
            if(Upgrade->GoldCost() > playerdata->Gold()){
                return false;   
            }
            if(!playerdata->AssetRequirementsMet(DUnitName)){
                return false;
            }
        }
    }
    else if(atBarracks == actor->Type()){
        auto AssetIterator = playerdata->AssetTypes()->find(DUnitName);
        
        if(AssetIterator != playerdata->AssetTypes()->end()){
            auto AssetType = AssetIterator->second;
            if(AssetType->LumberCost() > playerdata->Lumber()){
                return false;   
            }
            if(AssetType->GoldCost() > playerdata->Gold()){
                return false;   
            }
            if((AssetType->FoodConsumption() + playerdata->FoodConsumption()) > playerdata->FoodProduction()){
                return false;    
            }
        }
    }
    
    return true;
}

bool CPlayerCapabilityBuildRanger::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    return CanInitiate(actor, playerdata);
}

bool CPlayerCapabilityBuildRanger::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    if(atLumberMill == actor->Type()){
        auto Upgrade = CPlayerUpgrade::FindUpgradeFromName(std::string("Build") + DUnitName);
        
        if(Upgrade){
            SAssetCommand NewCommand;
            
            actor->ClearCommand();
            NewCommand.DAction = aaCapability;
            NewCommand.DCapability = AssetCapabilityType();
            NewCommand.DAssetTarget = target;
            NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target, actor->AssetType(), DUnitName, Upgrade->LumberCost(), Upgrade->GoldCost(), CPlayerAsset::UpdateFrequency() * Upgrade->ResearchTime());
            actor->PushCommand(NewCommand);
            
            return true;
        }
    }
    else if(atBarracks == actor->Type()){
        auto AssetIterator = playerdata->AssetTypes()->find(DUnitName);
        
        if(AssetIterator != playerdata->AssetTypes()->end()){
            auto AssetType = AssetIterator->second;
            auto NewAsset = playerdata->CreateAsset(DUnitName);
            SAssetCommand NewCommand;
            CPosition TilePosition;
            TilePosition.SetToTile(actor->Position());
            NewAsset->TilePosition(TilePosition);
            NewAsset->HitPoints(1);
            
            NewCommand.DAction = aaCapability;
            NewCommand.DCapability = AssetCapabilityType();
            NewCommand.DAssetTarget = NewAsset;
            NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, NewAsset, actor->AssetType(), DUnitName, AssetType->LumberCost(), AssetType->GoldCost(), CPlayerAsset::UpdateFrequency() * AssetType->BuildTime());
            actor->PushCommand(NewCommand);
        }
    }
    return false;
}

CPlayerCapabilityBuildRanger::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target, std::shared_ptr< CPlayerAssetType > upgradingtype, const std::string &unitname, int lumber, int gold, int steps) :
CActivatedPlayerCapability(actor, playerdata, target){
    DUnitName = unitname;
    DCurrentStep = 0;
    DTotalSteps = steps;
    DLumber = lumber;
    DGold = gold;
    DPlayerData->DecrementLumber(DLumber);
    DPlayerData->DecrementGold(DGold);
    if(atLumberMill == actor->Type()){
        DUpgradingType = upgradingtype;
        DUpgradingType->RemoveCapability(CPlayerCapability::NameToType(std::string("Build") + DUnitName));
    }
    else if(atBarracks == actor->Type()){
        SAssetCommand AssetCommand;

        AssetCommand.DAction = aaConstruct;
        AssetCommand.DAssetTarget = DActor;
        DTarget->PushCommand(AssetCommand);
    }
}


int CPlayerCapabilityBuildRanger::CActivatedCapability::PercentComplete(int max){
    return DCurrentStep * max / DTotalSteps;
}

bool CPlayerCapabilityBuildRanger::CActivatedCapability::IncrementStep(){
    if(atBarracks == DActor->Type()){
        int AddHitPoints = (DTarget->MaxHitPoints() * (DCurrentStep + 1) / DTotalSteps) - (DTarget->MaxHitPoints() * DCurrentStep / DTotalSteps);
        
        DTarget->IncrementHitPoints(AddHitPoints);
        if(DTarget->HitPoints() > DTarget->MaxHitPoints()){
            DTarget->HitPoints(DTarget->MaxHitPoints());
        }   
    }
    
    DCurrentStep++;
    DActor->IncrementStep();
    if(DCurrentStep >= DTotalSteps){
        SGameEvent TempEvent;
        
        if(atLumberMill == DActor->Type()){
            auto BarracksIterator = DPlayerData->AssetTypes()->find("Barracks");
            auto RangerIterator = DPlayerData->AssetTypes()->find("Ranger");
            auto LumberMillIterator = DPlayerData->AssetTypes()->find("LumberMill");
            
            TempEvent.DType = etWorkComplete;
            TempEvent.DAsset = DActor;
            
            BarracksIterator->second->AddCapability(actBuildRanger);
            BarracksIterator->second->RemoveCapability(actBuildArcher);
            LumberMillIterator->second->AddCapability(actLongbow);
            LumberMillIterator->second->AddCapability(actRangerScouting);
            LumberMillIterator->second->AddCapability(actMarksmanship);
            // Upgrade all Archers
            for(auto WeakAsset : DPlayerData->Assets()){
                if(auto Asset = WeakAsset.lock()){
                    if(atArcher == Asset->Type()){
                        int HitPointIncrement = RangerIterator->second->HitPoints() - Asset->MaxHitPoints();
                        
                        Asset->ChangeType(RangerIterator->second);
                        Asset->IncrementHitPoints(HitPointIncrement);
                    }   
                }
            }
        }
        else if(atBarracks == DActor->Type()){
            TempEvent.DType = etReady;
            TempEvent.DAsset = DTarget;
        
            DTarget->PopCommand();
            DTarget->TilePosition(DPlayerData->PlayerMap()->FindAssetPlacement(DTarget, DActor, CPosition(DPlayerData->PlayerMap()->Width()-1, DPlayerData->PlayerMap()->Height()-1)));
        }
        DPlayerData->AddGameEvent(TempEvent);
        DActor->PopCommand();
        return true;    
    }
    return false;
}

void CPlayerCapabilityBuildRanger::CActivatedCapability::Cancel(){
    DPlayerData->IncrementLumber(DLumber);
    DPlayerData->IncrementGold(DGold);
    if(atLumberMill == DActor->Type()){
        DUpgradingType->AddCapability(CPlayerCapability::NameToType(std::string("Build") + DUnitName));
    }
    else if(atBarracks == DActor->Type()){
        DPlayerData->DeleteAsset(DTarget);
    }
    DActor->PopCommand();
}


