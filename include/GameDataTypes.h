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
#ifndef GAMEDATATYPES_H
#define GAMEDATATYPES_H

typedef enum{
    pcNone = 0,
    pcBlue,
    pcRed,
    pcGreen,
    pcPurple,
    pcOrange,
    pcYellow,
    pcBlack,
    pcWhite,
    pcMax
} EPlayerColor, *EPlayerColorRef;

typedef enum{
    aaNone = 0,
    aaConstruct,
    aaBuild,
    aaRepair,
    aaWalk,
    aaStandGround,
    aaAttack,
    aaHarvestLumber,
    aaMineGold,
    aaConveyLumber,
    aaConveyGold,
    aaDeath,
    aaDecay,
    aaCapability
} EAssetAction, *EAssetActionRef;

typedef enum{
    actNone = 0,
    actBuildPeasant,
    actBuildFootman,
    actBuildArcher,
    actBuildRanger,
    actBuildFarm,
    actBuildTownHall,
    actBuildBarracks,
    actBuildLumberMill,
    actBuildBlacksmith,
    actBuildKeep,
    actBuildCastle,
    actBuildScoutTower,
    actBuildGuardTower,
    actBuildCannonTower,
    actMove,
    actRepair,
    actMine,
    actBuildSimple,
    actBuildAdvanced,
    actConvey,
    actCancel,
    actBuildWall,
    actAttack,
    actStandGround,
    actPatrol,
    actWeaponUpgrade1,
    actWeaponUpgrade2,
    actWeaponUpgrade3,
    actArrowUpgrade1,
    actArrowUpgrade2,
    actArrowUpgrade3,
    actArmorUpgrade1,
    actArmorUpgrade2,
    actArmorUpgrade3,
    actLongbow,
    actRangerScouting,
    actMarksmanship,
    actMax
} EAssetCapabilityType, *EAssetCapabilityTypeRef;

typedef enum{
    atNone = 0,
    atPeasant,
    atFootman,
    atArcher,
    atRanger,
    atGoldMine,
    atTownHall,
    atKeep,
    atCastle,
    atFarm,
    atBarracks,
    atLumberMill,
    atBlacksmith,
    atScoutTower,
    atGuardTower,
    atCannonTower,
    atMax
} EAssetType, *EAssetTypeRef;

typedef enum{
    dNorth = 0,
    dNorthEast,
    dEast,
    dSouthEast,
    dSouth,
    dSouthWest,
    dWest,
    dNorthWest,
    dMax
} EDirection, EDirectionRef;

#endif

