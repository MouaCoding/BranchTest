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
#ifndef TERRAINMAP_H
#define TERRAINMAP_H
#include "DataSource.h"
#include "GameDataTypes.h"
#include "Position.h"
#include <vector>

class CTerrainMap{
    public:
        typedef enum{
            ttNone = 0,
            ttGrass,
            ttDirt,
            ttRock,
            ttTree,
            ttStump,
            ttWater,
            ttWall,
            ttWallDamaged,
            ttRubble,
            ttMax
        } ETileType, *ETileTypeRef;
        
    protected:
        std::vector< std::vector< ETileType > > DMap;
        std::vector< std::string > DStringMap;
        int DPlayerCount;
        std::string DMapName;
        
    public:        
        CTerrainMap();
        CTerrainMap(const CTerrainMap &map);
        ~CTerrainMap();
        
        CTerrainMap &operator=(const CTerrainMap &map);
        
        std::string MapName() const;
        
        int Width() const;
        int Height() const;
        
        ETileType TileType(int xindex, int yindex) const{
            if((-1 > xindex)||(-1 > yindex)){
                return ttNone;    
            }
            if(DMap.size() <= yindex+1){
                return ttNone;   
            }
            if(DMap[yindex+1].size() <= xindex+1){
                return ttNone;   
            }
            return DMap[yindex+1][xindex+1];
        };
        ETileType TileType(const CPosition &pos) const{
            return TileType(pos.X(), pos.Y());
        };
        CPosition FindNearestTileType(const CPosition &pos, ETileType type);
        void ChangeTileType(int xindex, int yindex, ETileType type);
        void ChangeTileType(const CPosition &pos, ETileType type){
            ChangeTileType(pos.X(), pos.Y(), type);
        };
        
        virtual bool LoadMap(std::shared_ptr< CDataSource > source);
};

#endif

