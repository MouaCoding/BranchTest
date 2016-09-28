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
#include "Position.h"

int CPosition::DTileWidth = 1;
int CPosition::DTileHeight = 1;
int CPosition::DHalfTileWidth = 0;
int CPosition::DHalfTileHeight = 0;
std::vector< std::vector < EDirection > > CPosition::DOctant({dMax});
std::vector< std::vector < EDirection > > CPosition::DTileDirections(
    {{dNorthWest, dNorth, dNorthEast},
    {dWest, dMax, dEast},
    {dSouthWest, dSouth, dSouthEast}});

CPosition::CPosition(int x, int y){
    DX = x;
    DY = y;
}

bool CPosition::operator==(const CPosition &pos) const{
    return (DX == pos.DX) && (DY == pos.DY);
}

bool CPosition::operator!=(const CPosition &pos) const{
    return (DX != pos.DX) || (DY != pos.DY);
}

void CPosition::SetTileDimensions(int width, int height){
    if((0 < width)&&(0 < height)){
        DTileWidth = width;   
        DTileHeight = height;
        DHalfTileWidth = width / 2;
        DHalfTileHeight = height / 2;
        
        DOctant.resize(DTileHeight);
        for(auto &Row : DOctant){
            Row.resize(DTileWidth);
        }
        for(int Y = 0; Y < DTileHeight; Y++){
            for(int X = 0; X < DTileWidth; X++){
                int XDistance = X - DHalfTileWidth;
                int YDistance = Y - DHalfTileHeight;
                bool NegativeX = XDistance < 0;
                bool NegativeY = YDistance > 0; // Top of screen is 0
                double SinSquared;
                
                XDistance *= XDistance;
                YDistance *= YDistance;
                
                if(0 == (XDistance + YDistance)){
                    DOctant[Y][X] = dMax;
                    continue;
                }
                SinSquared = (double)YDistance / (XDistance + YDistance);
                
                if(0.1464466094 > SinSquared){
                    // East or West
                    if(NegativeX){
                        DOctant[Y][X] = dWest; // West   
                    }
                    else{
                        DOctant[Y][X] = dEast; // East
                    }
                }
                else if(0.85355339059 > SinSquared){
                    // NE, SE, SW, NW
                    if(NegativeY){
                        if(NegativeX){
                            DOctant[Y][X] = dSouthWest; // SW
                        }
                        else{
                            DOctant[Y][X] = dSouthEast; // SE
                        }
                    }
                    else{
                        if(NegativeX){
                            DOctant[Y][X] = dNorthWest; // NW
                        }
                        else{
                            DOctant[Y][X] = dNorthEast; // NE
                        }
                    }
                }
                else{
                    // North or South
                    if(NegativeY){
                        DOctant[Y][X] = dSouth; // South    
                    }
                    else{
                        DOctant[Y][X] = dNorth; // North
                    }
                }
            }
        }  
        /*
        for(auto &Row : DOctant){
            for(auto &Cell : Row){
                printf("%d ", Cell);
            }
            printf("\n");
               
        }
        */
    }
}

void CPosition::SetFromTile(const CPosition &pos){
    DX = pos.DX * DTileWidth + DHalfTileWidth;
    DY = pos.DY * DTileHeight + DHalfTileHeight;
}

void CPosition::SetXFromTile(int x){
    DX = x * DTileWidth + DHalfTileWidth;
}

void CPosition::SetYFromTile(int y){
    DY = y * DTileHeight + DHalfTileHeight;
}

void CPosition::SetToTile(const CPosition &pos){
    DX = pos.DX / DTileWidth;
    DY = pos.DY / DTileHeight;
}

void CPosition::SetXToTile(int x){
    DX = x / DTileWidth;
}

void CPosition::SetYToTile(int y){
    DY = y / DTileHeight;
}
        
EDirection CPosition::TileOctant() const{
    return DOctant[DY % DTileHeight][DX % DTileWidth];
}

EDirection CPosition::AdjacentTileDirection(const CPosition &pos, int objsize) const{
    if(1 == objsize){
        int DeltaX = pos.DX - DX;
        int DeltaY = pos.DY - DY;
        
        if((1 < (DeltaX * DeltaX))||(1 < (DeltaY * DeltaY))){
            return dMax;   
        }
        
        return DTileDirections[DeltaY+1][DeltaX+1];
    }
    else{
         CPosition ThisPosition;
         CPosition TargetPosition;
         
         ThisPosition.SetFromTile(*this);
         TargetPosition.SetFromTile(pos);
         
         TargetPosition.SetToTile( ThisPosition.ClosestPosition(TargetPosition, objsize) );
         return AdjacentTileDirection(TargetPosition, 1);
    }
}

CPosition CPosition::ClosestPosition(const CPosition &objpos, int objsize) const{
    CPosition CurPosition(objpos);
    CPosition BestPosition;
    int BestDistance = -1;
    for(int YIndex = 0; YIndex < objsize; YIndex++){
        for(int XIndex = 0; XIndex < objsize; XIndex++){
            int CurDistance = CurPosition.DistanceSquared(*this);
            if((-1 == BestDistance)||(CurDistance < BestDistance)){
                BestDistance = CurDistance;
                BestPosition = CurPosition;
            }
            CurPosition.IncrementX(CPosition::TileWidth());
        }
        CurPosition.X(objpos.X());
        CurPosition.IncrementY(CPosition::TileHeight());
    }
    return BestPosition;
}

EDirection CPosition::DirectionTo(const CPosition &pos) const{
    CPosition DeltaPosition(pos.DX - DX, pos.DY - DY);
    int DivX = DeltaPosition.DX / HalfTileWidth();
    int DivY = DeltaPosition.DY / HalfTileHeight();
    int Div;
    DivX = 0 > DivX ? -DivX : DivX;
    DivY = 0 > DivY ? -DivY : DivY;
    Div = DivX > DivY ? DivX : DivY;

    if(Div){
        DeltaPosition.DX /= Div;
        DeltaPosition.DY /= Div;
    }
    DeltaPosition.DX += HalfTileWidth();
    DeltaPosition.DY += HalfTileHeight();
    if(0 > DeltaPosition.DX){
        DeltaPosition.DX = 0;
    }
    if(0 > DeltaPosition.DY){
        DeltaPosition.DY = 0;
    }
    if(TileWidth() <= DeltaPosition.DX){
        DeltaPosition.DX = TileWidth() - 1;
    }
    if(TileHeight() <= DeltaPosition.DY){
        DeltaPosition.DY = TileHeight() - 1;
    }
    return DeltaPosition.TileOctant();
}

int CPosition::DistanceSquared(const CPosition &pos){
    int DeltaX = pos.DX - DX;
    int DeltaY = pos.DY - DY;
    
    return DeltaX * DeltaX + DeltaY * DeltaY;
}

int CPosition::Distance(const CPosition &pos){
    // Code from http://www.codecodex.com/wiki/Calculate_an_integer_square_root
    unsigned int Op, Result, One;
    
    Op = DistanceSquared(pos);
    Result = 0;
    
    One = 1 << (sizeof(unsigned int) * 8 - 2);
    while(One > Op){
        One >>= 2;
    }
    while(0 != One){
        if(Op >= Result + One){
            Op -= Result + One;  
            Result += One << 1;  // <-- faster than 2 * one  
        }
        Result >>= 1;
        One >>= 2;
    }
    return Result;    
}

