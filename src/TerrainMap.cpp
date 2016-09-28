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
#include "TerrainMap.h"
#include "LineDataSource.h"
#include "Tokenizer.h"
#include "Debug.h"
#include <stdio.h>
#include <stdlib.h>
        
CTerrainMap::CTerrainMap(){

}

CTerrainMap::CTerrainMap(const CTerrainMap &map){
    DMap = map.DMap;
    DStringMap = map.DStringMap;
    DPlayerCount = map.DPlayerCount;
    DMapName = map.DMapName;
}

CTerrainMap::~CTerrainMap(){

}

CTerrainMap &CTerrainMap::operator=(const CTerrainMap &map){
    if(this != &map){
        DMap = map.DMap;
        DStringMap = map.DStringMap;
        DPlayerCount = map.DPlayerCount;
        DMapName = map.DMapName; 
    }
    return *this;
}

std::string CTerrainMap::MapName() const{
    return DMapName;   
}

int CTerrainMap::Width() const{
    if(DMap.size()){
        return DMap[0].size()-2;
    }
    return 0;
}

int CTerrainMap::Height() const{
    return DMap.size()-2;
}

CPosition CTerrainMap::FindNearestTileType(const CPosition &pos, ETileType type){
    int MaxDistance = Width() > Height() ? Width() : Height();
    int XOffset = pos.X() + 1;
    int YOffset = pos.Y() + 1;
    
    for(int SearchDistance = 0; SearchDistance < MaxDistance; SearchDistance++){
        int PositiveX = XOffset + SearchDistance;
        int NegativeX = XOffset - SearchDistance;
        int PositiveY = YOffset + SearchDistance;
        int NegativeY = YOffset - SearchDistance;
        bool SearchPX = true;
        bool SearchNX = true;
        bool SearchPY = true;
        bool SearchNY = true;
        
        if(0 >= NegativeX){
            NegativeX = 1;   
            SearchNX = false;
        }
        if(PositiveX + 1 >= DMap[0].size()){
            PositiveX = DMap[0].size() - 2;
            SearchPX = false;
        }
        if(0 >= NegativeY){
            NegativeY = 1;
            SearchNY = false;
        }
        if(PositiveY + 1 >= DMap.size()){
            PositiveY = DMap.size() - 2;
            SearchPY = false;
        }
        if(!SearchNX && !SearchNY && !SearchPX && !SearchPY){
            break;
        }
        if(SearchNY){
            for(int XOff = NegativeX; XOff <= PositiveX; XOff++){
                if(type == DMap[NegativeY][XOff]){
                    return CPosition(XOff-1, NegativeY-1);
                }
            }
        }
        if(SearchPX){
            for(int YOff = NegativeY; YOff <= PositiveY; YOff++){
                if(type == DMap[YOff][PositiveX]){
                    return CPosition(PositiveX-1, YOff-1);
                }
            }
        }
        if(SearchPY){
            for(int XOff = PositiveX; XOff >= NegativeX; XOff--){
                if(type == DMap[PositiveY][XOff]){
                    return CPosition(XOff-1, PositiveY-1);
                }
            }
        }
        if(SearchNX){
            for(int YOff = PositiveY; YOff >= NegativeY; YOff--){
                if(type == DMap[YOff][NegativeX]){
                    return CPosition(NegativeX-1, YOff-1);
                }
            }
        }
    }
    return CPosition(-1, -1);
}

void CTerrainMap::ChangeTileType(int xindex, int yindex, ETileType type){
    if((-1 > xindex)||(-1 > yindex)){
        return;    
    }
    if(DMap.size() <= yindex+1){
        return;   
    }
    if(DMap[yindex+1].size() <= xindex+1){
        return;   
    }
    DMap[yindex+1][xindex+1] = type;
}

bool CTerrainMap::LoadMap(std::shared_ptr< CDataSource > source){     
    CLineDataSource LineSource(source);
    std::string TempString;
    std::vector< std::string > Tokens;
    int MapWidth, MapHeight;
    bool ReturnStatus = false;
    //int PlayerColorsFound = 0;
    
    DMap.clear();
    

    if(!LineSource.Read(DMapName)){
        PrintError("Failed to read map line.\n");
        goto LoadMapExit;
    }
    
    if(!LineSource.Read(TempString)){
        PrintError("Failed to read map line.\n");
        goto LoadMapExit;
    }
    CTokenizer::Tokenize(Tokens, TempString);
    if(2 != Tokens.size()){
        PrintError("Invalid map dimensions %s (%zd).\n",TempString.c_str(), Tokens.size());
        for(int I = 0; I < Tokens.size(); I++){
            PrintError("Tokens[%d] = %s\n",I,Tokens[I].c_str());
        }
        goto LoadMapExit;
    }
    try{
        MapWidth = std::stoi(Tokens[0]);
        MapHeight = std::stoi(Tokens[1]);
            
        PrintDebug(DEBUG_LOW, "Map %s is %d x %d\n",DMapName.c_str(), MapWidth, MapHeight); 
        if((8 > MapWidth)||(8 > MapHeight)){
            PrintError("Invalid map dimensions.\n");
            goto LoadMapExit;        
        }
        while(DStringMap.size() < MapHeight + 2){
            if(!LineSource.Read(TempString)){
                PrintError("Failed to read map line.\n");
                goto LoadMapExit;
            }                                 
            DStringMap.push_back(TempString);
            if(MapWidth + 2 > DStringMap.back().length()){
                PrintError("Map line %zd too short!\n", DStringMap.size());
                goto LoadMapExit;
            }
        }
        if(MapHeight + 2 > DStringMap.size()){
            PrintError("Map has too few lines!\n");
            goto LoadMapExit;
        }
        DMap.resize(MapHeight+2);
        for(int Index = 0; Index < DMap.size(); Index++){
            DMap[Index].resize(MapWidth+2);
            for(int Inner = 0; Inner < MapWidth+2; Inner++){
                switch(DStringMap[Index][Inner]){
                    case 'G':   DMap[Index][Inner] = ttGrass;
                                break;
                    case 'F':   DMap[Index][Inner] = ttTree;
                                break;
                    case 'D':   DMap[Index][Inner] = ttDirt;
                                break;
                    case 'W':   DMap[Index][Inner] = ttWall;
                                break;
                    case 'w':   DMap[Index][Inner] = ttWallDamaged;
                                break;
                    case 'R':   DMap[Index][Inner] = ttRock;
                                break;
                    case ' ':   DMap[Index][Inner] = ttWater;
                                break;
                    default:    PrintError("Unknwon tile type '%c'on line %d\n", DStringMap[Index][Inner],Index+2);
                                goto LoadMapExit;
                                break;
                }
            }
        }    
        ReturnStatus = true;
    }
    catch(std::exception &E){
        PrintError("%s\n",E.what());
    }

LoadMapExit:
    return ReturnStatus;
}
/*
void CTerrainMap::DrawPreviewMap(GdkDrawable *drawable, GdkGC *gc, gint xoff, gint yoff){
    for(int YIndex = 0, YPos = yoff; YIndex < DMap.size(); YIndex++, YPos += 2){
        for(int XIndex = 0, XPos = xoff; XIndex < DMap[YIndex].size(); XIndex++, XPos += 2){
            DTileset->DrawPixelCorners(drawable, gc, XPos, YPos, DMap[YIndex][XIndex]);
        }
    }
}

void CTerrainMap::DrawMap(GdkDrawable *drawable, GdkGC *gc){
    int TileWidth, TileHeight;
    
    TileWidth = DTileset->TileWidth();
    TileHeight = DTileset->TileHeight();
    for(int YIndex = 0, YPos = 0; YIndex < DMap.size(); YIndex++, YPos += TileHeight){
        for(int XIndex = 0, XPos = 0; XIndex < DMap[YIndex].size(); XIndex++, XPos += TileWidth){
            DTileset->DrawTile(drawable, gc, XPos, YPos, DMap[YIndex][XIndex]);
        }
    }
}

*/
