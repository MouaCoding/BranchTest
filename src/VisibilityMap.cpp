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
#include "VisibilityMap.h"

CVisibilityMap::CVisibilityMap(int width, int height, int maxvisibility){
    DMaxVisibility = maxvisibility;
    DMap.resize(height + 2 * DMaxVisibility);
    for(auto &Row : DMap){
        Row.resize(width + 2 * DMaxVisibility);
        for(auto &Cell : Row){
            Cell = tvNone;   
        }
    }
    DTotalMapTiles = width * height;
    DUnseenTiles = DTotalMapTiles; 
}

CVisibilityMap::CVisibilityMap(const CVisibilityMap &map){
    DMaxVisibility = map.DMaxVisibility;
    DMap = map.DMap;
    DTotalMapTiles = map.DTotalMapTiles;
    DUnseenTiles = map.DUnseenTiles;
}

CVisibilityMap::~CVisibilityMap(){
    
}

CVisibilityMap &CVisibilityMap::operator=(const CVisibilityMap &map){
    if(this != &map){
        DMaxVisibility = map.DMaxVisibility;
        DMap = map.DMap;   
        DTotalMapTiles = map.DTotalMapTiles;
        DUnseenTiles = map.DUnseenTiles;
    }
    return *this;
}

int CVisibilityMap::Width() const{
    if(DMap.size()){
        return DMap[0].size() - 2 * DMaxVisibility;
    }
    return 0;
}

int CVisibilityMap::Height() const{
    return DMap.size() - 2 * DMaxVisibility;
}

int CVisibilityMap::SeenPercent(int max) const{
    return (max * (DTotalMapTiles - DUnseenTiles)) / DTotalMapTiles;
}
        
void CVisibilityMap::Update(const std::list< std::weak_ptr< CPlayerAsset > > &assets){
    for(auto &Row : DMap){
        for(auto &Cell : Row){
            if((tvVisible == Cell)||(tvPartial == Cell)){
                Cell = tvSeen;
            }
            else if(tvPartialPartial == Cell){
                Cell = tvSeenPartial;   
            }
        }
    }
    for(auto WeakAsset : assets){
        if(auto CurAsset = WeakAsset.lock()){
            CPosition Anchor = CurAsset->TilePosition();
            int Sight = CurAsset->EffectiveSight() + CurAsset->Size()/2;
            int SightSquared = Sight * Sight;
            Anchor.X(Anchor.X() + CurAsset->Size()/2);
            Anchor.Y(Anchor.Y() + CurAsset->Size()/2);
            for(int X = 0; X <= Sight; X++){
                int XSquared = X * X;
                int XSquared1 = X ? (X - 1) * (X - 1) : 0;
                
                for(int Y = 0; Y <= Sight; Y++){
                    int YSquared = Y * Y;
                    int YSquared1 = Y ? (Y - 1) * (Y - 1) : 0;
                    
                    if((XSquared + YSquared) < SightSquared){
                        // Visible
                        DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility] = tvVisible;
                        DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility] = tvVisible;
                        DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility] = tvVisible;
                        DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility] = tvVisible;
                    }
                    else if((XSquared1 + YSquared1) < SightSquared){
                        // Partial   
                        ETileVisibility CurVis = DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility];
                        if(tvSeen == CurVis){
                            DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility] = tvPartial;
                        }
                        else if((tvNone == CurVis)||(tvSeenPartial == CurVis)){
                            DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility] = tvPartialPartial;
                        }
                        CurVis = DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility];
                        if(tvSeen == CurVis){
                            DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility] = tvPartial;
                        }
                        else if((tvNone == CurVis)||(tvSeenPartial == CurVis)){
                            DMap[Anchor.Y() - Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility] = tvPartialPartial;
                        }
                        CurVis = DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility];
                        if(tvSeen == CurVis){
                            DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility] = tvPartial;
                        }
                        else if((tvNone == CurVis)||(tvSeenPartial == CurVis)){
                            DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() - X + DMaxVisibility] = tvPartialPartial;
                        }
                        CurVis = DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility];
                        if(tvSeen == CurVis){
                            DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility] = tvPartial;
                        }
                        else if((tvNone == CurVis)||(tvSeenPartial == CurVis)){
                            DMap[Anchor.Y() + Y + DMaxVisibility][Anchor.X() + X + DMaxVisibility] = tvPartialPartial;
                        }
                    }
                }
            }
        }
    }
    int MinX, MinY, MaxX, MaxY;
    MinY = DMaxVisibility;
    MaxY = DMap.size() - DMaxVisibility;
    MinX = DMaxVisibility;
    MaxX = DMap[0].size() - DMaxVisibility;
    DUnseenTiles = 0;
    for(int Y = MinY; Y < MaxY; Y++){
        for(int X = MinX; X < MaxX; X++){
            if(tvNone == DMap[Y][X]){
                DUnseenTiles++;
            }
        }
    }
    
}

