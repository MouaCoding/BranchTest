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
#ifndef AIPLAYER_H
#define AIPLAYER_H

#include "GameModel.h"
#include "PlayerCommand.h"

class CAIPlayer{
    protected:
        std::shared_ptr< CPlayerData > DPlayerData;
        int DCycle;
        int DDownSample;
        
        bool SearchMap(SPlayerCommandRequest &command);
        bool FindEnemies(SPlayerCommandRequest &command);
        bool AttackEnemies(SPlayerCommandRequest &command);
        bool BuildTownHall(SPlayerCommandRequest &command);
        bool BuildBuilding(SPlayerCommandRequest &command, EAssetType buildingtype, EAssetType neartype);
        bool ActivatePeasants(SPlayerCommandRequest &command, bool trainmore);
        bool ActivateFighters(SPlayerCommandRequest &command);
        bool TrainFootman(SPlayerCommandRequest &command);
        bool TrainArcher(SPlayerCommandRequest &command);
        
    public:
        CAIPlayer(std::shared_ptr< CPlayerData > playerdata, int downsample);
        
        void CalculateCommand(SPlayerCommandRequest &command);
};

#endif
