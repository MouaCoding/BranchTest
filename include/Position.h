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
#ifndef POSITION_H
#define POSITION_H
#include "GameDataTypes.h"
#include <vector>

class CPosition{
    protected:
        int DX;
        int DY;

        static int DTileWidth;
        static int DTileHeight;
        static int DHalfTileWidth;
        static int DHalfTileHeight;
        static std::vector< std::vector < EDirection > > DOctant;
        static std::vector< std::vector < EDirection > > DTileDirections;
        
    public:
        CPosition() = default;
        CPosition(int x, int y);
        CPosition(const CPosition &pos) = default;
        
        CPosition &operator=(const CPosition &pos) = default;
        
        bool operator==(const CPosition &pos) const;
        bool operator!=(const CPosition &pos) const;
        
        static void SetTileDimensions(int width, int height);
        
        static int TileWidth(){
            return DTileWidth;  
        };
        
        static int TileHeight(){
            return DTileHeight;  
        };
        
        static int HalfTileWidth(){
            return DHalfTileWidth;  
        };
        
        static int HalfTileHeight(){
            return DHalfTileHeight;  
        };
        
        int X() const{
            return DX;  
        };
        
        int X(int x){
            return DX = x;  
        };
        
        int IncrementX(int x){
            DX += x; 
            return DX;
        };
        
        int DecrementX(int x){
            DX -= x; 
            return DX;
        };
        
        int Y() const{
            return DY;
        };
        
        int Y(int y){
            return DY = y;
        };
        
        int IncrementY(int y){
            DY += y;
            return DY;
        };
        
        int DecrementY(int y){
            DY -= y; 
            return DY;
        };
        
        void SetFromTile(const CPosition &pos);
        void SetXFromTile(int x);
        void SetYFromTile(int y);
        void SetToTile(const CPosition &pos);
        void SetXToTile(int x);
        void SetYToTile(int y);
        
        bool TileAligned() const{
            return ((DX % DTileWidth) == DHalfTileWidth) && ((DY % DTileHeight) == DHalfTileHeight);
        };
        
        EDirection TileOctant() const;
        EDirection AdjacentTileDirection(const CPosition &pos, int objsize = 1) const;
        CPosition ClosestPosition(const CPosition &objpos, int objsize) const;
        EDirection DirectionTo(const CPosition &pos) const;
        
        int DistanceSquared(const CPosition &pos);
        int Distance(const CPosition &pos);
};

#endif

