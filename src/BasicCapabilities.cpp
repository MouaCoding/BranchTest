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

// Normal capabilities
class CPlayerCapabilityMove : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityMove();
        
    public:
        virtual ~CPlayerCapabilityMove(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityMove::CRegistrant CPlayerCapabilityMove::DRegistrant;

CPlayerCapabilityMove::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityMove >(new CPlayerCapabilityMove()));   
}

CPlayerCapabilityMove::CPlayerCapabilityMove() : CPlayerCapability(std::string("Move"), ttTerrainOrAsset){

}

bool CPlayerCapabilityMove::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    return actor->Speed() > 0;
}

bool CPlayerCapabilityMove::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    return actor->Speed() > 0;
}

bool CPlayerCapabilityMove::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    if(actor->TilePosition() != target->TilePosition()){
        SAssetCommand NewCommand;
        
        NewCommand.DAction = aaCapability;
        NewCommand.DCapability = AssetCapabilityType();
        NewCommand.DAssetTarget = target;
        NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
        actor->ClearCommand();
        actor->PushCommand(NewCommand);
        return true;
    }

    return false;
}

CPlayerCapabilityMove::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityMove::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityMove::CActivatedCapability::IncrementStep(){
    SAssetCommand AssetCommand;
    SGameEvent TempEvent;
    
    TempEvent.DType = etAcknowledge;
    TempEvent.DAsset = DActor;
    DPlayerData->AddGameEvent(TempEvent);
        
    AssetCommand.DAction = aaWalk;
    AssetCommand.DAssetTarget = DTarget;
    if(!DActor->TileAligned()){
        DActor->Direction((EDirection)((DActor->Position().TileOctant() + dMax/2) % dMax));
    }
    DActor->ClearCommand();
    DActor->PushCommand(AssetCommand);
    return true;
}

void CPlayerCapabilityMove::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}

class CPlayerCapabilityMineHarvest : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityMineHarvest();
        
    public:
        virtual ~CPlayerCapabilityMineHarvest(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityMineHarvest::CRegistrant CPlayerCapabilityMineHarvest::DRegistrant;

CPlayerCapabilityMineHarvest::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityMineHarvest >(new CPlayerCapabilityMineHarvest()));   
}

CPlayerCapabilityMineHarvest::CPlayerCapabilityMineHarvest() : CPlayerCapability(std::string("Mine"), ttTerrainOrAsset){

}

bool CPlayerCapabilityMineHarvest::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    return actor->HasCapability(actMine);
}

bool CPlayerCapabilityMineHarvest::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    
    if(!actor->HasCapability(actMine)){
        return false;
    }
    if(actor->Lumber()||actor->Gold()){
        return false;
    }
    if(atGoldMine == target->Type()){
        return true;
    }
    if(atNone != target->Type()){
        return false;
    }
    return CTerrainMap::ttTree == playerdata->PlayerMap()->TileType(target->TilePosition());
}

bool CPlayerCapabilityMineHarvest::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    SAssetCommand NewCommand;
    
    NewCommand.DAction = aaCapability;
    NewCommand.DCapability = AssetCapabilityType();
    NewCommand.DAssetTarget = target;
    NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
    actor->ClearCommand();
    actor->PushCommand(NewCommand);

    return true;
}

CPlayerCapabilityMineHarvest::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityMineHarvest::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityMineHarvest::CActivatedCapability::IncrementStep(){
    SAssetCommand AssetCommand;
    SGameEvent TempEvent;
    
    TempEvent.DType = etAcknowledge;
    TempEvent.DAsset = DActor;
    DPlayerData->AddGameEvent(TempEvent);
    
    AssetCommand.DAssetTarget = DTarget;
    if(atGoldMine == DTarget->Type()){
        AssetCommand.DAction = aaMineGold;
    }
    else{
        AssetCommand.DAction = aaHarvestLumber;
    }
    DActor->ClearCommand();
    DActor->PushCommand(AssetCommand);
    AssetCommand.DAction = aaWalk;
    if(!DActor->TileAligned()){
        DActor->Direction((EDirection)((DActor->Position().TileOctant() + dMax/2) % dMax));
    }
    DActor->PushCommand(AssetCommand);
    return true;
}

void CPlayerCapabilityMineHarvest::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}

class CPlayerCapabilityStandGround : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityStandGround();
        
    public:
        virtual ~CPlayerCapabilityStandGround(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityStandGround::CRegistrant CPlayerCapabilityStandGround::DRegistrant;

CPlayerCapabilityStandGround::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityStandGround >(new CPlayerCapabilityStandGround()));   
}

CPlayerCapabilityStandGround::CPlayerCapabilityStandGround() : CPlayerCapability(std::string("StandGround"), ttNone){

}

bool CPlayerCapabilityStandGround::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    return true;
}

bool CPlayerCapabilityStandGround::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    
    return true;
}

bool CPlayerCapabilityStandGround::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    SAssetCommand NewCommand;
    
    NewCommand.DAction = aaCapability;
    NewCommand.DCapability = AssetCapabilityType();
    NewCommand.DAssetTarget = target;
    NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
    actor->ClearCommand();
    actor->PushCommand(NewCommand);

    return true;
}

CPlayerCapabilityStandGround::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityStandGround::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityStandGround::CActivatedCapability::IncrementStep(){
    SAssetCommand AssetCommand;
    SGameEvent TempEvent;
    
    TempEvent.DType = etAcknowledge;
    TempEvent.DAsset = DActor;
    DPlayerData->AddGameEvent(TempEvent);
    
    AssetCommand.DAssetTarget = DPlayerData->CreateMarker(DActor->Position(), false);
    AssetCommand.DAction = aaStandGround;

    DActor->ClearCommand();
    DActor->PushCommand(AssetCommand);
    
    if(!DActor->TileAligned()){
        AssetCommand.DAction = aaWalk;
        DActor->Direction((EDirection)((DActor->Position().TileOctant() + dMax/2) % dMax));
        DActor->PushCommand(AssetCommand);
    }
    
    return true;
}

void CPlayerCapabilityStandGround::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}

class CPlayerCapabilityCancel : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityCancel();
        
    public:
        virtual ~CPlayerCapabilityCancel(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityCancel::CRegistrant CPlayerCapabilityCancel::DRegistrant;

CPlayerCapabilityCancel::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityCancel >(new CPlayerCapabilityCancel()));   
}

CPlayerCapabilityCancel::CPlayerCapabilityCancel() : CPlayerCapability(std::string("Cancel"), ttNone){

}

bool CPlayerCapabilityCancel::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    return true;
}

bool CPlayerCapabilityCancel::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    return true;
}

bool CPlayerCapabilityCancel::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    SAssetCommand NewCommand;
    
    NewCommand.DAction = aaCapability;
    NewCommand.DCapability = AssetCapabilityType();
    NewCommand.DAssetTarget = target;
    NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
    actor->PushCommand(NewCommand);

    return true;
}

CPlayerCapabilityCancel::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityCancel::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityCancel::CActivatedCapability::IncrementStep(){
    DActor->PopCommand();
    
    if(aaNone != DActor->Action()){
        SAssetCommand AssetCommand;
        
        AssetCommand = DActor->CurrentCommand();
        if(aaConstruct == AssetCommand.DAction){
            if(AssetCommand.DAssetTarget){
                AssetCommand.DAssetTarget->CurrentCommand().DActivatedCapability->Cancel();
            }
            else if(AssetCommand.DActivatedCapability){
                AssetCommand.DActivatedCapability->Cancel();
            }
        }
        else if(AssetCommand.DActivatedCapability){
            AssetCommand.DActivatedCapability->Cancel();
        }
    }
    
    return true;
}

void CPlayerCapabilityCancel::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}

class CPlayerCapabilityConvey : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityConvey();
        
    public:
        virtual ~CPlayerCapabilityConvey(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityConvey::CRegistrant CPlayerCapabilityConvey::DRegistrant;

CPlayerCapabilityConvey::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityConvey >(new CPlayerCapabilityConvey()));   
}

CPlayerCapabilityConvey::CPlayerCapabilityConvey() : CPlayerCapability(std::string("Convey"), ttAsset){

}

bool CPlayerCapabilityConvey::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    return actor->Speed() > 0 && (actor->Lumber() || actor->Gold());
}

bool CPlayerCapabilityConvey::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){

    if(actor->Speed() && (actor->Lumber() || actor->Gold())){
        if(aaConstruct == target->Action()){
            return false;
        }
        if((atTownHall == target->Type())||(atKeep == target->Type())||(atCastle == target->Type())){
            return true;
        }
        if(actor->Lumber() && (atLumberMill == target->Type())){
            return true;    
        }
    }
    return false;
}

bool CPlayerCapabilityConvey::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    SAssetCommand NewCommand;
    
    NewCommand.DAction = aaCapability;
    NewCommand.DCapability = AssetCapabilityType();
    NewCommand.DAssetTarget = target;
    NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
    actor->ClearCommand();
    actor->PushCommand(NewCommand);
    return true;
}

CPlayerCapabilityConvey::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityConvey::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityConvey::CActivatedCapability::IncrementStep(){
    std::weak_ptr< CPlayerAsset > NearestRepository;
    SAssetCommand AssetCommand;
    SGameEvent TempEvent;
    
    TempEvent.DType = etAcknowledge;
    TempEvent.DAsset = DActor;
    DPlayerData->AddGameEvent(TempEvent);
    
    DActor->PopCommand();
    if(DActor->Lumber()){
        AssetCommand.DAction = aaConveyLumber;
        AssetCommand.DAssetTarget = DTarget;
        DActor->PushCommand(AssetCommand);
        AssetCommand.DAction = aaWalk;
        DActor->PushCommand(AssetCommand);
        DActor->ResetStep();
    }
    else if(DActor->Gold()){
        AssetCommand.DAction = aaConveyGold;
        AssetCommand.DAssetTarget = DTarget;
        DActor->PushCommand(AssetCommand);
        AssetCommand.DAction = aaWalk;
        DActor->PushCommand(AssetCommand);
        DActor->ResetStep();
    }
    
    return true;
}

void CPlayerCapabilityConvey::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}

class CPlayerCapabilityPatrol : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityPatrol();
        
    public:
        virtual ~CPlayerCapabilityPatrol(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityPatrol::CRegistrant CPlayerCapabilityPatrol::DRegistrant;

CPlayerCapabilityPatrol::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityPatrol >(new CPlayerCapabilityPatrol()));   
}

CPlayerCapabilityPatrol::CPlayerCapabilityPatrol() : CPlayerCapability(std::string("Patrol"), ttTerrain){

}

bool CPlayerCapabilityPatrol::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    return actor->Speed() > 0;
}

bool CPlayerCapabilityPatrol::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    return actor->Speed() > 0;
}

bool CPlayerCapabilityPatrol::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    if(actor->TilePosition() != target->TilePosition()){
        SAssetCommand NewCommand;
        
        NewCommand.DAction = aaCapability;
        NewCommand.DCapability = AssetCapabilityType();
        NewCommand.DAssetTarget = target;
        NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
        actor->ClearCommand();
        actor->PushCommand(NewCommand);
        return true;
    }

    return false;
}

CPlayerCapabilityPatrol::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityPatrol::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityPatrol::CActivatedCapability::IncrementStep(){
    SAssetCommand PatrolCommand, WalkCommand;
    SGameEvent TempEvent;
    
    TempEvent.DType = etAcknowledge;
    TempEvent.DAsset = DActor;
    DPlayerData->AddGameEvent(TempEvent);
    
    PatrolCommand.DAction = aaCapability;
    PatrolCommand.DCapability = actPatrol;
    PatrolCommand.DAssetTarget = DPlayerData->CreateMarker(DActor->Position(), false);
    PatrolCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(DActor, DPlayerData, PatrolCommand.DAssetTarget);
    DActor->ClearCommand();
    DActor->PushCommand(PatrolCommand);
    
    WalkCommand.DAction = aaWalk;
    WalkCommand.DAssetTarget = DTarget;
    if(!DActor->TileAligned()){
        DActor->Direction((EDirection)((DActor->Position().TileOctant() + dMax/2) % dMax));
    }
    DActor->PushCommand(WalkCommand);
    return true;
}

void CPlayerCapabilityPatrol::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}

class CPlayerCapabilityAttack : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityAttack();
        
    public:
        virtual ~CPlayerCapabilityAttack(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityAttack::CRegistrant CPlayerCapabilityAttack::DRegistrant;

CPlayerCapabilityAttack::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityAttack >(new CPlayerCapabilityAttack()));   
}

CPlayerCapabilityAttack::CPlayerCapabilityAttack() : CPlayerCapability(std::string("Attack"), ttAsset){

}

bool CPlayerCapabilityAttack::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    return actor->Speed() > 0;
}

bool CPlayerCapabilityAttack::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    if((actor->Color() == target->Color())||(pcNone == target->Color())){
        return false;   
    }
    return actor->Speed() > 0;
}

bool CPlayerCapabilityAttack::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    if(actor->TilePosition() != target->TilePosition()){
        SAssetCommand NewCommand;
        
        NewCommand.DAction = aaCapability;
        NewCommand.DCapability = AssetCapabilityType();
        NewCommand.DAssetTarget = target;
        NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
        actor->ClearCommand();
        actor->PushCommand(NewCommand);
        return true;
    }

    return false;
}

CPlayerCapabilityAttack::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityAttack::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityAttack::CActivatedCapability::IncrementStep(){
    SAssetCommand AssetCommand;
    SGameEvent TempEvent;
    
    TempEvent.DType = etAcknowledge;
    TempEvent.DAsset = DActor;
    DPlayerData->AddGameEvent(TempEvent);
    
    AssetCommand.DAction = aaAttack;
    AssetCommand.DAssetTarget = DTarget;
    DActor->ClearCommand();
    DActor->PushCommand(AssetCommand);
    
    AssetCommand.DAction = aaWalk;
    if(!DActor->TileAligned()){
        DActor->Direction((EDirection)((DActor->Position().TileOctant() + dMax/2) % dMax));
    }
    DActor->PushCommand(AssetCommand);
    return true;
}

void CPlayerCapabilityAttack::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}

class CPlayerCapabilityRepair : public CPlayerCapability{
    protected:
        class CRegistrant{
            public:  
                CRegistrant();
        };
        static CRegistrant DRegistrant;
        
        class CActivatedCapability : public CActivatedPlayerCapability{
            protected:
                
            public:
                CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
                virtual ~CActivatedCapability(){};
                
                int PercentComplete(int max);
                bool IncrementStep();
                void Cancel();
        };
        CPlayerCapabilityRepair();
        
    public:
        virtual ~CPlayerCapabilityRepair(){};
        
        bool CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata);
        bool CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
        bool ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target);
};

CPlayerCapabilityRepair::CRegistrant CPlayerCapabilityRepair::DRegistrant;

CPlayerCapabilityRepair::CRegistrant::CRegistrant(){
    CPlayerCapability::Register(std::shared_ptr< CPlayerCapabilityRepair >(new CPlayerCapabilityRepair()));   
}

CPlayerCapabilityRepair::CPlayerCapabilityRepair() : CPlayerCapability(std::string("Repair"), ttAsset){

}

bool CPlayerCapabilityRepair::CanInitiate(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata){
    
    return (actor->Speed() > 0) && playerdata->Gold() && playerdata->Lumber();
}

bool CPlayerCapabilityRepair::CanApply(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    if((actor->Color() != target->Color())||(target->Speed())){
        return false;   
    }
    if(target->HitPoints() >= target->MaxHitPoints()){
        return false;   
    }
    return CanInitiate(actor, playerdata);
}

bool CPlayerCapabilityRepair::ApplyCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target){
    if(actor->TilePosition() != target->TilePosition()){
        SAssetCommand NewCommand;
        
        NewCommand.DAction = aaCapability;
        NewCommand.DCapability = AssetCapabilityType();
        NewCommand.DAssetTarget = target;
        NewCommand.DActivatedCapability = std::make_shared< CActivatedCapability >(actor, playerdata, target);
        actor->ClearCommand();
        actor->PushCommand(NewCommand);
        return true;
    }

    return false;
}

CPlayerCapabilityRepair::CActivatedCapability::CActivatedCapability(std::shared_ptr< CPlayerAsset > actor, std::shared_ptr< CPlayerData > playerdata, std::shared_ptr< CPlayerAsset > target) :
CActivatedPlayerCapability(actor, playerdata, target){

}


int CPlayerCapabilityRepair::CActivatedCapability::PercentComplete(int max){
    return 0;
}

bool CPlayerCapabilityRepair::CActivatedCapability::IncrementStep(){
    SAssetCommand AssetCommand;
    SGameEvent TempEvent;
    
    TempEvent.DType = etAcknowledge;
    TempEvent.DAsset = DActor;
    DPlayerData->AddGameEvent(TempEvent);
    
    AssetCommand.DAction = aaRepair;
    AssetCommand.DAssetTarget = DTarget;
    DActor->ClearCommand();
    DActor->PushCommand(AssetCommand);
    
    AssetCommand.DAction = aaWalk;
    if(!DActor->TileAligned()){
        DActor->Direction((EDirection)((DActor->Position().TileOctant() + dMax/2) % dMax));
    }
    DActor->PushCommand(AssetCommand);
    return true;
}

void CPlayerCapabilityRepair::CActivatedCapability::Cancel(){

    DActor->PopCommand();
}
