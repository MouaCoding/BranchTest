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
#include "MapRenderer.h"
#include "PixelType.h"
#include "Debug.h"

CMapRenderer::CMapRenderer(std::shared_ptr< CGraphicTileset > tileset, std::shared_ptr< CTerrainMap > map){
    int Index;
    std::vector< int > Hamming;
    DTileset = tileset;
    DMap = map;
    
    DPixelIndices.resize(CTerrainMap::ttMax);
    DPixelIndices[CTerrainMap::ttGrass] = DTileset->FindPixel("grass");
    DPixelIndices[CTerrainMap::ttDirt] = DTileset->FindPixel("dirt");
    DPixelIndices[CTerrainMap::ttRock] = DTileset->FindPixel("rock");
    DPixelIndices[CTerrainMap::ttTree] = DTileset->FindPixel("tree");
    DPixelIndices[CTerrainMap::ttStump] = DTileset->FindPixel("stump");
    DPixelIndices[CTerrainMap::ttWater] = DTileset->FindPixel("water");
    DPixelIndices[CTerrainMap::ttWall] = DTileset->FindPixel("wall");
    DPixelIndices[CTerrainMap::ttWallDamaged] = DTileset->FindPixel("walldamaged");
    DPixelIndices[CTerrainMap::ttRubble] = DTileset->FindPixel("rubble");
    
    Index = 0;
    while(true){
        int Value = DTileset->FindTile(std::string("grass-") + std::to_string(Index));
        if(0 > Value){
            break;   
        }
        DGrassIndices.push_back(Value);
        Index++;
    }

    for(Index = 0; Index < 0x40; Index++){
        DTreeIndices.push_back(DTileset->FindTile(std::string("tree-") + std::to_string(Index)));
    }
    DTreeIndices[0x0A] = DTreeIndices[0x03];
    DTreeIndices[0x0B] = DTreeIndices[0x03];
    DTreeIndices[0x23] = DTreeIndices[0x03];
    DTreeIndices[0x2B] = DTreeIndices[0x03];
    DTreeIndices[0x0E] = DTreeIndices[0x06];
    DTreeIndices[0x22] = DTreeIndices[0x06];
    DTreeIndices[0x26] = DTreeIndices[0x06];
    DTreeIndices[0x2E] = DTreeIndices[0x06];
    DTreeIndices[0x0F] = DTreeIndices[0x07];
    DTreeIndices[0x27] = DTreeIndices[0x07];
    DTreeIndices[0x2F] = DTreeIndices[0x07];
    DTreeIndices[0x18] = DTreeIndices[0x11];
    DTreeIndices[0x19] = DTreeIndices[0x11];
    DTreeIndices[0x39] = DTreeIndices[0x11];
    DTreeIndices[0x30] = DTreeIndices[0x14];
    DTreeIndices[0x34] = DTreeIndices[0x14];
    DTreeIndices[0x32] = DTreeIndices[0x16];
    DTreeIndices[0x3A] = DTreeIndices[0x1A];
    
    DTreeIndices[0x11] = DTreeIndices[0x10];
    DTreeIndices[0x14] = DTreeIndices[0x10];
    DTreeIndices[0x15] = DTreeIndices[0x10];
    
    DTreeIndices[0x39] = DTreeIndices[0x38];
    DTreeIndices[0x3C] = DTreeIndices[0x38];
    DTreeIndices[0x3D] = DTreeIndices[0x38];
    for(Index = 0; Index < 0x100; Index++){
        DDirtIndices.push_back(DTileset->FindTile(std::string("dirt-") + std::to_string(Index)));
    }
    // Three in a row on side
    DDirtIndices[0x03] = DDirtIndices[0x02];
    DDirtIndices[0x05] = DDirtIndices[0x02];
    DDirtIndices[0x06] = DDirtIndices[0x02];
    DDirtIndices[0x07] = DDirtIndices[0x02];
    
    DDirtIndices[0x09] = DDirtIndices[0x08];
    DDirtIndices[0x21] = DDirtIndices[0x08];
    DDirtIndices[0x28] = DDirtIndices[0x08];
    DDirtIndices[0x29] = DDirtIndices[0x08];
    
    DDirtIndices[0x14] = DDirtIndices[0x10];
    DDirtIndices[0x84] = DDirtIndices[0x10];
    DDirtIndices[0x90] = DDirtIndices[0x10];
    DDirtIndices[0x94] = DDirtIndices[0x10];
    
    DDirtIndices[0x60] = DDirtIndices[0x40];
    DDirtIndices[0xA0] = DDirtIndices[0x40];
    DDirtIndices[0xC0] = DDirtIndices[0x40];
    DDirtIndices[0xE0] = DDirtIndices[0x40];
    
    // Corner three
    DDirtIndices[0x0A] = DDirtIndices[0x0B];
    DDirtIndices[0x0F] = DDirtIndices[0x0B];
    DDirtIndices[0x25] = DDirtIndices[0x0B];
    DDirtIndices[0x27] = DDirtIndices[0x0B];
    DDirtIndices[0x2D] = DDirtIndices[0x0B];
    DDirtIndices[0x2B] = DDirtIndices[0x0B];
    DDirtIndices[0x2F] = DDirtIndices[0x0B];
    
    DDirtIndices[0x12] = DDirtIndices[0x16];
    DDirtIndices[0x17] = DDirtIndices[0x16];
    DDirtIndices[0x85] = DDirtIndices[0x16];
    DDirtIndices[0x87] = DDirtIndices[0x16];
    DDirtIndices[0x95] = DDirtIndices[0x16];
    DDirtIndices[0x96] = DDirtIndices[0x16];
    DDirtIndices[0x97] = DDirtIndices[0x16];
    
    DDirtIndices[0x48] = DDirtIndices[0x68];
    DDirtIndices[0x69] = DDirtIndices[0x68];
    DDirtIndices[0xA1] = DDirtIndices[0x68];
    DDirtIndices[0xA9] = DDirtIndices[0x68];
    DDirtIndices[0xE1] = DDirtIndices[0x68];
    DDirtIndices[0xE8] = DDirtIndices[0x68];
    DDirtIndices[0xE9] = DDirtIndices[0x68];
    
    DDirtIndices[0x50] = DDirtIndices[0xD0];
    DDirtIndices[0xA4] = DDirtIndices[0xD0];
    DDirtIndices[0xB4] = DDirtIndices[0xD0];
    DDirtIndices[0xD4] = DDirtIndices[0xD0];
    DDirtIndices[0xE4] = DDirtIndices[0xD0];
    DDirtIndices[0xF0] = DDirtIndices[0xD0];
    DDirtIndices[0xF4] = DDirtIndices[0xD0];
    
    MakeHammingSet(0x5A, Hamming);
    for(auto Value : Hamming){
        if(-1 == DDirtIndices[Value | 0x24]){
            DDirtIndices[Value | 0x24] = DDirtIndices[0x7E];
        }
        if(-1 == DDirtIndices[Value | 0x81]){
            DDirtIndices[Value | 0x81] = DDirtIndices[0xDB];
        }
    }   

    
    DDirtIndices[0x1D] = DDirtIndices[0xFF];
    DDirtIndices[0x3D] = DDirtIndices[0xFF];
    DDirtIndices[0x9D] = DDirtIndices[0xFF];
    
    DDirtIndices[0x63] = DDirtIndices[0xFF];
    DDirtIndices[0x67] = DDirtIndices[0xFF];
    DDirtIndices[0xE3] = DDirtIndices[0xFF];
    
    DDirtIndices[0xB8] = DDirtIndices[0xFF];
    DDirtIndices[0xB9] = DDirtIndices[0xFF];
    DDirtIndices[0xBC] = DDirtIndices[0xFF];
    
    DDirtIndices[0xC6] = DDirtIndices[0xFF];
    DDirtIndices[0xC7] = DDirtIndices[0xFF];
    DDirtIndices[0xE6] = DDirtIndices[0xFF];
    
    DDirtIndices[0xBD] = DDirtIndices[0xFF];    
    DDirtIndices[0xE7] = DDirtIndices[0xFF];
    /*

    DDirtIndices[0x0F] = DDirtIndices[0x0B];
    DDirtIndices[0x2B] = DDirtIndices[0x0B];
    DDirtIndices[0x2F] = DDirtIndices[0x0B];
    DDirtIndices[0x17] = DDirtIndices[0x16];
    DDirtIndices[0x87] = DDirtIndices[0x16];
    DDirtIndices[0x96] = DDirtIndices[0x16];
    DDirtIndices[0x97] = DDirtIndices[0x16];
    DDirtIndices[0x69] = DDirtIndices[0x68];
    DDirtIndices[0xC8] = DDirtIndices[0x68];
    DDirtIndices[0xE8] = DDirtIndices[0x68];
    DDirtIndices[0xE9] = DDirtIndices[0x68];
    DDirtIndices[0xD4] = DDirtIndices[0xD0];
    DDirtIndices[0xF0] = DDirtIndices[0xD0];
    DDirtIndices[0xF4] = DDirtIndices[0xD0];
    
    DDirtIndices[0x63] = DDirtIndices[0xFF];
    DDirtIndices[0xC6] = DDirtIndices[0xFF];
    DDirtIndices[0xC7] = DDirtIndices[0xFF];
    DDirtIndices[0xE7] = DDirtIndices[0xFF];
    */
    for(Index = 0; Index < 0x100; Index++){
        DWaterIndices.push_back(DTileset->FindTile(std::string("water-") + std::to_string(Index)));
    }
    DWaterIndices[0x00] = DDirtIndices[0xFF];
    DWaterIndices[0x01] = DWaterIndices[0x00];
    DWaterIndices[0x02] = DWaterIndices[0x00];
    DWaterIndices[0x04] = DWaterIndices[0x00];
    DWaterIndices[0x08] = DWaterIndices[0x00];
    DWaterIndices[0x10] = DWaterIndices[0x00];
    DWaterIndices[0x20] = DWaterIndices[0x00];
    DWaterIndices[0x40] = DWaterIndices[0x00];
    DWaterIndices[0x80] = DWaterIndices[0x00];
    
    
    // Three ground in corner
    DWaterIndices[0x0F] = DWaterIndices[0x0B];
    DWaterIndices[0x2B] = DWaterIndices[0x0B];
    DWaterIndices[0x2F] = DWaterIndices[0x0B];
    
    DWaterIndices[0x17] = DWaterIndices[0x16];
    DWaterIndices[0x96] = DWaterIndices[0x16];
    DWaterIndices[0x97] = DWaterIndices[0x16];
    
    DWaterIndices[0x69] = DWaterIndices[0x68];
    DWaterIndices[0xE8] = DWaterIndices[0x68];
    DWaterIndices[0xE9] = DWaterIndices[0x68];
    
    DWaterIndices[0xD4] = DWaterIndices[0xD0];
    DWaterIndices[0xF0] = DWaterIndices[0xD0];
    DWaterIndices[0xF4] = DWaterIndices[0xD0];
    
    // Three ground straight
    DWaterIndices[0x3F] = DWaterIndices[0x1F];
    DWaterIndices[0x9F] = DWaterIndices[0x1F];
    
    DWaterIndices[0x6F] = DWaterIndices[0x6B];
    DWaterIndices[0xEB] = DWaterIndices[0x6B];
    
    DWaterIndices[0xD7] = DWaterIndices[0xD6];
    DWaterIndices[0xF6] = DWaterIndices[0xD6];
    
    DWaterIndices[0xF9] = DWaterIndices[0xF8];
    DWaterIndices[0xFC] = DWaterIndices[0xF8];
    
    Index = 0;
    for(Index = 0; Index < 0x100; Index++){
        int Value = DTileset->FindTile(std::string("rock-") + std::to_string(Index));
        DRockIndices.push_back(Value);
    }
    
    // Three rocks in corner
    MakeHammingSet(0x25, Hamming);
    for(auto Value : Hamming){
        if(-1 == DRockIndices[0x0A | Value]){
            DRockIndices[0x0A | Value] = DRockIndices[0x0B];
        }
    } 
    
    MakeHammingSet(0x85, Hamming);
    for(auto Value : Hamming){
        if(-1 == DRockIndices[0x12 | Value]){
            DRockIndices[0x12 | Value] = DRockIndices[0x16];
        }
    }
    
    MakeHammingSet(0xA1, Hamming);
    for(auto Value : Hamming){
        if(-1 == DRockIndices[0x48 | Value]){
            DRockIndices[0x48 | Value] = DRockIndices[0x68];
        }
    }
    
    MakeHammingSet(0xA4, Hamming);
    for(auto Value : Hamming){
        if(-1 == DRockIndices[0x50 | Value]){
            DRockIndices[0x50 | Value] = DRockIndices[0xD0];
        }
    } 
    // Three rocks straight
    DRockIndices[0x1B] = DRockIndices[0x1F];
    DRockIndices[0x1E] = DRockIndices[0x1F];
    DRockIndices[0x3F] = DRockIndices[0x1F];
    DRockIndices[0x9F] = DRockIndices[0x1F];
    
    DRockIndices[0x6F] = DRockIndices[0x6B];
    DRockIndices[0xEB] = DRockIndices[0x6B];
    
    DRockIndices[0xD7] = DRockIndices[0xD6];
    DRockIndices[0xF6] = DRockIndices[0xD6];
    
    DRockIndices[0x78] = DRockIndices[0xF8];
    DRockIndices[0xD8] = DRockIndices[0xF8];
    DRockIndices[0xF9] = DRockIndices[0xF8];
    DRockIndices[0xFC] = DRockIndices[0xF8];
    
    // Ends
    DRockIndices[0x09] = DRockIndices[0x08];
    DRockIndices[0x28] = DRockIndices[0x08];
    DRockIndices[0x29] = DRockIndices[0x08];
    
    DRockIndices[0x14] = DRockIndices[0x10];
    DRockIndices[0x90] = DRockIndices[0x10];
    DRockIndices[0x94] = DRockIndices[0x10];

    MakeHammingSet(0xA5, Hamming);
    for(auto Value : Hamming){
        if(-1 == DRockIndices[Value]){
            DRockIndices[Value] = DRockIndices[0x02];
        }
    } 
    
    Index = 0;
    for(Index = 0; Index < 0x10; Index++){
        int Value = DTileset->FindTile(std::string("wall-") + std::to_string(Index));
        DWallIndices.push_back(Value);
    }
    Index = 0;
    for(Index = 0; Index < 0x10; Index++){
        int Value = DTileset->FindTile(std::string("wall-damaged-") + std::to_string(Index));
        DWallDamagedIndices.push_back(Value);
    }
}

void CMapRenderer::MakeHammingSet(int value, std::vector< int > &hammingset){
    int BitCount, Anchor, LastEnd;
    hammingset.clear();
    for(int Index = 0; Index < 8; Index++){
        int Value = 1<<Index;
        if(value & Value){
            hammingset.push_back(Value);
        }
    }
    LastEnd = BitCount = hammingset.size();
    Anchor = 0;
    for(int TotalBits = 1; TotalBits < BitCount; TotalBits++){
        for(int LastIndex = Anchor; LastIndex < LastEnd; LastIndex++){
            for(int BitIndex = 0; BitIndex < BitCount; BitIndex++){
                int NewValue = hammingset[LastIndex] | hammingset[BitIndex];
                if(NewValue != hammingset[LastIndex]){
                    bool Found = false;
                    for(int Index = LastEnd; Index < hammingset.size(); Index++){
                        if(NewValue == hammingset[Index]){
                            Found = true;
                            break;
                        }
                    }
                    if(!Found){
                        hammingset.push_back(NewValue);   
                    }
                }
            }
        }
        Anchor = LastEnd + 1;
        LastEnd = hammingset.size();
    }
}

int CMapRenderer::FindUnknown(CTerrainMap::ETileType type, int known, int unknown){
    int ReturnValue = -1;
    if(CTerrainMap::ttTree == type){
        auto Iterator = DTreeUnknown.find((known<<8) | unknown);
        if(Iterator != DTreeUnknown.end()){
            return Iterator->second;
        }
        std::vector< int > HammingSet;
        MakeHammingSet(unknown, HammingSet);
        for(auto &Value : HammingSet){
            if(-1 != DTreeIndices[known | Value]){
                DTreeUnknown[(known<<8) | unknown] = DTreeIndices[known | Value];
                return DTreeIndices[known | Value];
            }
        }
    }
    else if(CTerrainMap::ttWater == type){
        auto Iterator = DWaterUnknown.find((known<<8) | unknown);
        if(Iterator != DWaterUnknown.end()){
            return Iterator->second;
        }
        std::vector< int > HammingSet;
        MakeHammingSet(unknown, HammingSet);
        for(auto &Value : HammingSet){
            if(-1 != DWaterIndices[known | Value]){
                DWaterUnknown[(known<<8) | unknown] = DWaterIndices[known | Value];
                return DWaterIndices[known | Value];
            }
        }
    }
    else if(CTerrainMap::ttDirt == type){
        auto Iterator = DDirtUnknown.find((known<<8) | unknown);
        if(Iterator != DDirtUnknown.end()){
            return Iterator->second;
        }
        std::vector< int > HammingSet;
        MakeHammingSet(unknown, HammingSet);
        for(auto &Value : HammingSet){
            if(-1 != DDirtIndices[known | Value]){
                DDirtUnknown[(known<<8) | unknown] = DDirtIndices[known | Value];
                return DDirtIndices[known | Value];
            }
        }
    }
    else if(CTerrainMap::ttRock == type){
        auto Iterator = DRockUnknown.find((known<<8) | unknown);
        if(Iterator != DRockUnknown.end()){
            return Iterator->second;
        }
        std::vector< int > HammingSet;
        MakeHammingSet(unknown, HammingSet);
        for(auto &Value : HammingSet){
            if(-1 != DRockIndices[known | Value]){
                DRockUnknown[(known<<8) | unknown] = DRockIndices[known | Value];
                return DRockIndices[known | Value];
            }
        }
    }
    return ReturnValue;
}

int CMapRenderer::MapWidth() const{
    return DMap->Width();
}

int CMapRenderer::MapHeight() const{
    return DMap->Height();
}

int CMapRenderer::DetailedMapWidth() const{
    return DMap->Width() * DTileset->TileWidth();
}

int CMapRenderer::DetailedMapHeight() const{
    return DMap->Height() * DTileset->TileHeight();
}

void CMapRenderer::DrawMap(GdkDrawable *drawable, GdkDrawable *typedrawable, const SRectangle &rect, int level){
    GdkGC *TempGC = gdk_gc_new(drawable);
    GdkGC *TempTypeGC = gdk_gc_new(typedrawable);
    int TileWidth, TileHeight;
    static std::vector< bool > UnknownTree, UnknownWater, UnknownDirt, UnknownRock;
    static std::unordered_map< int, bool > UnknownUnknownTree, UnknownUnknownWater, UnknownUnknownDirt, UnknownUnknownRock;
    
    
    if(UnknownTree.empty()){
        UnknownTree.resize(0x100);   
        UnknownWater.resize(0x100);   
        UnknownDirt.resize(0x100);  
        UnknownRock.resize(0x100);  
    }
    
    TileWidth = DTileset->TileWidth();
    TileHeight = DTileset->TileHeight();
    if(0 == level){
        GdkColor NoneColor;
        
        NoneColor.pixel = 0;
        NoneColor.red = 0;
        NoneColor.green = 0;
        NoneColor.blue = 0;
        gdk_gc_set_rgb_fg_color(TempTypeGC, &NoneColor);
        gdk_gc_set_rgb_bg_color(TempTypeGC, &NoneColor);
        gdk_draw_rectangle(typedrawable, TempTypeGC, TRUE, 0, 0, rect.DWidth, rect.DHeight);
        for(int YIndex = rect.DYPosition / TileHeight, YPos = -(rect.DYPosition % TileHeight); YPos < rect.DHeight; YIndex++, YPos += TileHeight){
            for(int XIndex = rect.DXPosition / TileWidth, XPos = -(rect.DXPosition % TileWidth); XPos < rect.DWidth; XIndex++, XPos += TileWidth){
                CPixelType PixelType(DMap->TileType(XIndex, YIndex)); 
                CTerrainMap::ETileType ThisTileType = DMap->TileType(XIndex, YIndex);
                if(CTerrainMap::ttTree == ThisTileType){//||(CTerrainMap::ttTree == DMap->TileType(XIndex, YIndex+1))){
                    int TreeIndex = 0, TreeMask = 0x1, UnknownMask = 0, DisplayIndex = -1;
                    for(int YOff = 0; YOff < 2; YOff++){
                        for(int XOff = -1; XOff < 2; XOff++){
                            CTerrainMap::ETileType Tile = DMap->TileType(XIndex + XOff, YIndex + YOff);
                            if(CTerrainMap::ttTree == Tile){
                                TreeIndex |= TreeMask;   
                            }
                            else if(CTerrainMap::ttNone == Tile){
                                UnknownMask |= TreeMask;
                            }
                            TreeMask <<= 1;
                        }
                    }
                    if(-1 == DTreeIndices[TreeIndex]){
                        if(!UnknownTree[TreeIndex] && !UnknownMask){
                            PrintError("Unknown tree 0x%02X @ (%d, %d)\n",TreeIndex, XIndex, YIndex);   
                            UnknownTree[TreeIndex] = true;
                        }
                        DisplayIndex = FindUnknown(CTerrainMap::ttTree, TreeIndex, UnknownMask);
                        if(-1 == DisplayIndex){
                            if(UnknownUnknownTree.end() == UnknownUnknownTree.find((TreeIndex<<8) | UnknownMask)){
                                UnknownUnknownTree[(TreeIndex<<8) | UnknownMask] = true;
                                PrintError("Unknown tree 0x%02X/%02X @ (%d, %d)\n",TreeIndex, UnknownMask, XIndex, YIndex);   
                            }
                        }
                    }
                    else{
                        DisplayIndex = DTreeIndices[TreeIndex];  
                    }
                    if(-1 != DisplayIndex){
                        DTileset->DrawTile(drawable, TempGC, XPos, YPos, DisplayIndex);
                        DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DisplayIndex, PixelType.ToPixelColor());
                    }
                }
                else if(CTerrainMap::ttWater == ThisTileType){
                    int WaterIndex = 0, WaterMask = 0x1, UnknownMask = 0, DisplayIndex = -1;
                    
                    for(int YOff = -1; YOff < 2; YOff++){
                        for(int XOff = -1; XOff < 2; XOff++){
                            if(YOff || XOff){
                                CTerrainMap::ETileType Tile = DMap->TileType(XIndex + XOff, YIndex + YOff);
                                if(CTerrainMap::ttWater == Tile){
                                    WaterIndex |= WaterMask;   
                                }
                                else if(CTerrainMap::ttNone == Tile){
                                    UnknownMask |= WaterMask;
                                }
                                WaterMask <<= 1;
                            }
                        }
                    }
                    if(-1 == DWaterIndices[WaterIndex]){
                        if(!UnknownWater[WaterIndex] && !UnknownMask){
                            PrintError("Unknown water 0x%02X @ (%d, %d)\n",WaterIndex, XIndex, YIndex);   
                            UnknownWater[WaterIndex] = true;
                        }
                        DisplayIndex = FindUnknown(CTerrainMap::ttWater, WaterIndex, UnknownMask);
                        if(-1 == DisplayIndex){
                            if(UnknownUnknownWater.end() == UnknownUnknownWater.find((WaterIndex<<8) | UnknownMask)){
                                UnknownUnknownWater[(WaterIndex<<8) | UnknownMask] = true;
                                PrintError("Unknown water 0x%02X/%02X @ (%d, %d)\n",WaterIndex, UnknownMask, XIndex, YIndex);   
                            }
                        }
                    }
                    else{
                        DisplayIndex = DWaterIndices[WaterIndex];  
                    }
                    if(-1 != DisplayIndex){
                        DTileset->DrawTile(drawable, TempGC, XPos, YPos, DisplayIndex);
                        DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DisplayIndex, PixelType.ToPixelColor());
                    }
                }
                else if(CTerrainMap::ttGrass == ThisTileType){
                    int OtherIndex = 0, OtherMask = 0x1, UnknownMask = 0, DisplayIndex = -1;
                    for(int YOff = -1; YOff < 2; YOff++){
                        for(int XOff = -1; XOff < 2; XOff++){
                            if(YOff || XOff){
                                CTerrainMap::ETileType Tile = DMap->TileType(XIndex + XOff, YIndex + YOff);
                                if((CTerrainMap::ttWater == Tile)||(CTerrainMap::ttDirt == Tile)||(CTerrainMap::ttRock == Tile)){
                                    OtherIndex |= OtherMask;   
                                }
                                else if(CTerrainMap::ttNone == Tile){
                                    UnknownMask |= OtherMask;
                                }
                                OtherMask <<= 1;
                            }
                        }
                    }
                    if(OtherIndex){
                        if(-1 == DDirtIndices[OtherIndex]){
                            if(!UnknownDirt[OtherIndex] && !UnknownMask){
                                PrintError("Unknown dirt 0x%02X @ (%d, %d)\n",OtherIndex, XIndex, YIndex);   
                                UnknownDirt[OtherIndex] = true;
                            }
                            DisplayIndex = FindUnknown(CTerrainMap::ttDirt, OtherIndex, UnknownMask);
                            if(-1 == DisplayIndex){
                                if(UnknownUnknownDirt.end() == UnknownUnknownDirt.find((OtherIndex<<8) | UnknownMask)){
                                    UnknownUnknownDirt[(OtherIndex<<8) | UnknownMask] = true;
                                    PrintError("Unknown dirt 0x%02X/%02X @ (%d, %d)\n",OtherIndex, UnknownMask, XIndex, YIndex);   
                                }
                            }
                        }
                        else{
                            DisplayIndex = DDirtIndices[OtherIndex];
                        }
                        
                        DTileset->DrawTile(drawable, TempGC, XPos, YPos, DisplayIndex);   
                        DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DisplayIndex, PixelType.ToPixelColor());
                    }
                    else{
                        DTileset->DrawTile(drawable, TempGC, XPos, YPos, DGrassIndices[0x00]);
                        DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DGrassIndices[0x00], PixelType.ToPixelColor());
                    }
                }
                else if(CTerrainMap::ttRock == ThisTileType){
                    int RockIndex = 0, RockMask = 0x1, UnknownMask = 0, DisplayIndex = -1;
                    
                    for(int YOff = -1; YOff < 2; YOff++){
                        for(int XOff = -1; XOff < 2; XOff++){
                            if(YOff || XOff){
                                CTerrainMap::ETileType Tile = DMap->TileType(XIndex + XOff, YIndex + YOff);
                                if(CTerrainMap::ttRock == Tile){
                                    RockIndex |= RockMask;   
                                }
                                else if(CTerrainMap::ttNone == Tile){
                                    UnknownMask |= RockMask;
                                }
                                RockMask <<= 1;
                            }
                        }
                    }
                    if(-1 == DRockIndices[RockIndex]){
                        if(!UnknownRock[RockIndex] && !UnknownMask){
                            PrintError("Unknown rock 0x%02X @ (%d, %d)\n",RockIndex, XIndex, YIndex);   
                            UnknownRock[RockIndex] = true;
                        }
                        DisplayIndex = FindUnknown(CTerrainMap::ttRock, RockIndex, UnknownMask);
                        if(-1 == DisplayIndex){
                            if(UnknownUnknownRock.end() == UnknownUnknownRock.find((RockIndex<<8) | UnknownMask)){
                                UnknownUnknownRock[(RockIndex<<8) | UnknownMask] = true;
                                PrintError("Unknown rock 0x%02X/%02X @ (%d, %d)\n",RockIndex, UnknownMask, XIndex, YIndex);   
                            }
                        }
                    }
                    else{
                        DisplayIndex = DRockIndices[RockIndex];  
                    }
                    if(-1 != DisplayIndex){
                        DTileset->DrawTile(drawable, TempGC, XPos, YPos, DisplayIndex);
                        DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DisplayIndex, PixelType.ToPixelColor());
                    }
                }
                else if((CTerrainMap::ttWall == ThisTileType)||(CTerrainMap::ttWallDamaged == ThisTileType)){
                    int WallIndex = 0, WallMask = 0x1, DisplayIndex = -1;
                    int XOffsets[] = {0, 1, 0, -1};
                    int YOffsets[] = {-1, 0, 1, 0};
                    for(int Index = 0; Index < sizeof(XOffsets) / sizeof(int); Index++){
                        CTerrainMap::ETileType Tile = DMap->TileType(XIndex + XOffsets[Index], YIndex + YOffsets[Index]);
                        if((CTerrainMap::ttWall == Tile)||(CTerrainMap::ttWallDamaged == Tile)||(CTerrainMap::ttRubble == Tile)){
                            WallIndex |= WallMask;
                        }
                        WallMask <<= 1;
                    }
                    DisplayIndex = CTerrainMap::ttWall == ThisTileType ? DWallIndices[WallIndex] : DWallDamagedIndices[WallIndex];
                    if(-1 != DisplayIndex){
                        DTileset->DrawTile(drawable, TempGC, XPos, YPos, DisplayIndex);
                        DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DisplayIndex, PixelType.ToPixelColor());
                    }
                }
                else{
                    
                    switch(DMap->TileType(XIndex, YIndex)){
                        case CTerrainMap::ttGrass:          DTileset->DrawTile(drawable, TempGC, XPos, YPos, DGrassIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DGrassIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttDirt:           DTileset->DrawTile(drawable, TempGC, XPos, YPos, DDirtIndices[0xFF]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DDirtIndices[0xFF], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttRock:           DTileset->DrawTile(drawable, TempGC, XPos, YPos, DRockIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DRockIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttTree:           DTileset->DrawTile(drawable, TempGC, XPos, YPos, DTreeIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DTreeIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttStump:          DTileset->DrawTile(drawable, TempGC, XPos, YPos, DTreeIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DTreeIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttWater:          DTileset->DrawTile(drawable, TempGC, XPos, YPos, DWaterIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DWaterIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttWall:           DTileset->DrawTile(drawable, TempGC, XPos, YPos, DWallIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DWallIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttWallDamaged:    DTileset->DrawTile(drawable, TempGC, XPos, YPos, DWallIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DWallIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        case CTerrainMap::ttRubble:         DTileset->DrawTile(drawable, TempGC, XPos, YPos, DWallIndices[0x00]);
                                                            DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DWallIndices[0x00], PixelType.ToPixelColor());
                                                            break;
                        default:                            break;
                    }
                }
            }
        }
    }
    else{
        for(int YIndex = rect.DYPosition / TileHeight, YPos = -(rect.DYPosition % TileHeight); YPos < rect.DHeight; YIndex++, YPos += TileHeight){
            for(int XIndex = rect.DXPosition / TileWidth, XPos = -(rect.DXPosition % TileWidth); XPos < rect.DWidth; XIndex++, XPos += TileWidth){
                if((CTerrainMap::ttTree == DMap->TileType(XIndex, YIndex+1))&&(CTerrainMap::ttTree != DMap->TileType(XIndex, YIndex))){
                    CPixelType PixelType(CTerrainMap::ttTree); 
                    int TreeIndex = 0, TreeMask = 0x1;
                    
                    for(int YOff = 0; YOff < 2; YOff++){
                        for(int XOff = -1; XOff < 2; XOff++){
                            if(CTerrainMap::ttTree == DMap->TileType(XIndex + XOff, YIndex + YOff)){
                                TreeIndex |= TreeMask;   
                            }
                            TreeMask <<= 1;
                        }
                    }
                    DTileset->DrawTile(drawable, TempGC, XPos, YPos, DTreeIndices[TreeIndex]);
                    DTileset->DrawClipped(typedrawable, TempTypeGC, XPos, YPos, DTreeIndices[TreeIndex], PixelType.ToPixelColor());
                }
            }
        }
    }
    g_object_unref(TempTypeGC);
    g_object_unref(TempGC);
}

void CMapRenderer::DrawMiniMap(GdkDrawable *drawable){
    GdkGC *TempGC = gdk_gc_new(drawable);
    for(int YPos = 0; YPos < DMap->Height(); YPos++){
        for(int XPos = 0; XPos < DMap->Width(); XPos++){
            CTerrainMap::ETileType TileType = DMap->TileType(XPos, YPos);
            if(CTerrainMap::ttNone != TileType){
                DTileset->DrawPixel(drawable, TempGC, XPos, YPos, 1, DPixelIndices[TileType]);
            }
        }
    }
    g_object_unref(TempGC);
}
