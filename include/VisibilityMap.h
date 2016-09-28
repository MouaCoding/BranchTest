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
#ifndef VISIBILITYMAP_H
#define VISIBILITYMAP_H
#include "PlayerAsset.h"
#include <vector>
#include <list>

class CVisibilityMap{
    public:
        typedef enum{
            tvNone = 0,
            tvPartialPartial,
            tvPartial,
            tvVisible,
            tvSeenPartial,
            tvSeen
        } ETileVisibility, *ETileVisibilityRef;
        
    protected:
        std::vector< std::vector< ETileVisibility > > DMap;
        int DMaxVisibility;
        int DTotalMapTiles;
        int DUnseenTiles;
        
    public:        
        CVisibilityMap(int width, int height, int maxvisibility);
        CVisibilityMap(const CVisibilityMap &map);
        ~CVisibilityMap();
        
        CVisibilityMap &operator=(const CVisibilityMap &map);
        
        int Width() const;
        int Height() const;
        
        int SeenPercent(int max) const;
        
        ETileVisibility TileType(int xindex, int yindex) const{
            if((-DMaxVisibility > xindex)||(-DMaxVisibility > yindex)){
                return tvNone;    
            }
            if(DMap.size() <= yindex+DMaxVisibility){
                return tvNone;   
            }
            if(DMap[yindex+DMaxVisibility].size() <= xindex+DMaxVisibility){
                return tvNone;   
            }
            return DMap[yindex+DMaxVisibility][xindex+DMaxVisibility];
        };
        
        void Update(const std::list< std::weak_ptr< CPlayerAsset > > &resources);
        
};

#endif

