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
#include "FogRenderer.h"

static int HammingDistance(int v1, int v2){
    int Delta = v1 ^ v2;
    int Distance = 0;
    
    while(Delta){
        if(Delta & 0x01){
            Distance++;   
        }
        Delta >>= 1;
    }
    return Distance;
}

CFogRenderer::CFogRenderer(std::shared_ptr< CGraphicTileset > tileset, std::shared_ptr< CVisibilityMap > map){
    int Index, VisibleIndex, NextIndex;
    std::vector< int > OriginialValues({ 0x0B, 0x16, 0xD0, 0x68, 0x07, 0x94, 0xE0, 0x29, 0x03, 0x06, 0x14, 0x90, 0x60, 0xC0, 0x09, 0x28, 0x01, 0x02, 0x04, 0x10, 0x80, 0x40, 0x20, 0x08});
    
    DTileset = tileset;
    DMap = map;
    VisibleIndex = DTileset->FindTile("visible");
    DNoneIndex = DTileset->FindTile("none");
    DSeenIndex = DTileset->FindTile("seen");
    DPartialIndex = DTileset->FindTile("partial");
    for(Index = 0; Index < 0x100; Index++){
        DFogIndices.push_back(DTileset->FindTile(std::string("pf-") + std::to_string(Index)));
    }
    DFogIndices[0x00] = DSeenIndex;
    DFogIndices[0x03] = DFogIndices[0x07];
    DFogIndices[0x06] = DFogIndices[0x07];
    DFogIndices[0x14] = DFogIndices[0x94];
    DFogIndices[0x90] = DFogIndices[0x94];
    DFogIndices[0x60] = DFogIndices[0xE0];
    DFogIndices[0xC0] = DFogIndices[0xE0];
    DFogIndices[0x09] = DFogIndices[0x29];
    DFogIndices[0x28] = DFogIndices[0x29];

    /*
    DFogIndices[0x00] = DSeenIndex;
    DFogIndices[0x03] = DFogIndices[0x07];
    DFogIndices[0x06] = DFogIndices[0x07];
    
    DFogIndices[0x0F] = DFogIndices[0x0B];
    DFogIndices[0x2B] = DFogIndices[0x0B];
    DFogIndices[0x2F] = DFogIndices[0x0B];
    
    DFogIndices[0x17] = DFogIndices[0x16];
    DFogIndices[0x96] = DFogIndices[0x16];
    DFogIndices[0x97] = DFogIndices[0x16];
    
    DFogIndices[0x09] = DFogIndices[0x29];
    DFogIndices[0x28] = DFogIndices[0x29];
    
    DFogIndices[0x69] = DFogIndices[0x68];
    DFogIndices[0xE8] = DFogIndices[0x68];
    DFogIndices[0xE9] = DFogIndices[0x68];
    
    DFogIndices[0x14] = DFogIndices[0x94];
    DFogIndices[0x90] = DFogIndices[0x94];
    
    DFogIndices[0xD4] = DFogIndices[0xD0];
    DFogIndices[0xF0] = DFogIndices[0xD0];
    DFogIndices[0xF4] = DFogIndices[0xD0];
    
    DFogIndices[0x60] = DFogIndices[0xE0];
    DFogIndices[0xC0] = DFogIndices[0xE0];
    */
    
    for(Index = 0; Index < 0x100; Index++){
        DBlackIndices.push_back(DTileset->FindTile(std::string("pb-") + std::to_string(Index)));
    }
    DBlackIndices[0x00] = DNoneIndex;
    DBlackIndices[0x03] = DBlackIndices[0x07];
    DBlackIndices[0x06] = DBlackIndices[0x07];
    DBlackIndices[0x14] = DBlackIndices[0x94];
    DBlackIndices[0x90] = DBlackIndices[0x94];
    DBlackIndices[0x60] = DBlackIndices[0xE0];
    DBlackIndices[0xC0] = DBlackIndices[0xE0];
    DBlackIndices[0x09] = DBlackIndices[0x29];
    DBlackIndices[0x28] = DBlackIndices[0x29];
    /*
    DBlackIndices[0xA7] = VisibleIndex;
    DBlackIndices[0xAD] = VisibleIndex;
    DBlackIndices[0xB5] = VisibleIndex;
    DBlackIndices[0xE5] = VisibleIndex;
    
    
    DBlackIndices[0x00] = DNoneIndex;
    DBlackIndices[0x03] = DBlackIndices[0x07];
    DBlackIndices[0x06] = DBlackIndices[0x07];
    
    DBlackIndices[0x0F] = DBlackIndices[0x0B];
    DBlackIndices[0x2B] = DBlackIndices[0x0B];
    DBlackIndices[0x2F] = DBlackIndices[0x0B];
    
    DBlackIndices[0x17] = DBlackIndices[0x16];
    DBlackIndices[0x96] = DBlackIndices[0x16];
    DBlackIndices[0x97] = DBlackIndices[0x16];
    
    DBlackIndices[0x09] = DBlackIndices[0x29];
    DBlackIndices[0x28] = DBlackIndices[0x29];
    
    DBlackIndices[0x69] = DBlackIndices[0x68];
    DBlackIndices[0xE8] = DBlackIndices[0x68];
    DBlackIndices[0xE9] = DBlackIndices[0x68];
    
    DBlackIndices[0x14] = DBlackIndices[0x94];
    DBlackIndices[0x90] = DBlackIndices[0x94];
    
    DBlackIndices[0xD4] = DBlackIndices[0xD0];
    DBlackIndices[0xF0] = DBlackIndices[0xD0];
    DBlackIndices[0xF4] = DBlackIndices[0xD0];
    
    DBlackIndices[0x60] = DBlackIndices[0xE0];
    DBlackIndices[0xC0] = DBlackIndices[0xE0];
    */
    NextIndex = DTileset->TileCount();
    DTileset->TileCount(DTileset->TileCount() + (0x100 - OriginialValues.size()) * 2);
    for(int AllowedHamming = 1; AllowedHamming < 8; AllowedHamming++){
        for(int Value = 0; Value < 0x100; Value++){
            if(-1 == DFogIndices[Value]){
                int BestMatch = -1;
                int BestHamming = 8;    
                
                for(auto &Orig : OriginialValues){
                    int CurHamming = HammingDistance(Orig, Value);
                    
                    if(CurHamming == HammingDistance(0, ~Orig & Value)){
                        if(CurHamming < BestHamming){
                            BestHamming = CurHamming;
                            BestMatch = Orig;
                        }
                    }
                }
                if(BestHamming <= AllowedHamming){
                    int CurVal;
                    DTileset->DuplicateTile(NextIndex, std::string("pf-") + std::to_string(Value), DFogIndices[BestMatch]);
                    DFogIndices[Value] = NextIndex;
                    DTileset->DuplicateTile(NextIndex+1, std::string("pb-") + std::to_string(Value), DBlackIndices[BestMatch]);
                    DBlackIndices[Value] = NextIndex+1;
                    CurVal = Value & ~BestMatch;
                    BestMatch = -1;
                    BestHamming = 8;    
                    while(CurVal){
                        for(auto &Orig : OriginialValues){
                            int CurHamming = HammingDistance(Orig, CurVal);
                            
                            if(CurHamming == HammingDistance(0, ~Orig & CurVal)){
                                if(CurHamming < BestHamming){
                                    BestHamming = CurHamming;
                                    BestMatch = Orig;
                                }
                            }
                        }
                        DTileset->OrAlphaTile(NextIndex, DFogIndices[BestMatch]);
                        DTileset->OrAlphaTile(NextIndex+1, DBlackIndices[BestMatch]);
                        CurVal &= ~BestMatch;
                    }
                    NextIndex += 2;
                }
            }
        }
    }
}


void CFogRenderer::DrawMap(GdkDrawable *drawable, const SRectangle &rect){
    GdkGC *TempGC = gdk_gc_new(drawable);
    int TileWidth, TileHeight;
    static std::vector< bool > UnknownFog, UnknownBlack;
    
    if((UnknownFog.size() < 0x100)||(UnknownBlack.size() < 0x100)){
        UnknownFog.resize(0x100);
        UnknownBlack.resize(0x100);
    }
    
    TileWidth = DTileset->TileWidth();
    TileHeight = DTileset->TileHeight();

    for(int YIndex = rect.DYPosition / TileHeight, YPos = -(rect.DYPosition % TileHeight); YPos < rect.DHeight; YIndex++, YPos += TileHeight){
        for(int XIndex = rect.DXPosition / TileWidth, XPos = -(rect.DXPosition % TileWidth); XPos < rect.DWidth; XIndex++, XPos += TileWidth){
            CVisibilityMap::ETileVisibility TileType = DMap->TileType(XIndex, YIndex);
            
            if(CVisibilityMap::tvNone == TileType){
                DTileset->DrawTile(drawable, TempGC, XPos, YPos, DNoneIndex);
                continue;
            }
            else if(CVisibilityMap::tvVisible == TileType){
                continue;
            }
            if((CVisibilityMap::tvSeen == TileType)||(CVisibilityMap::tvSeenPartial == TileType)){
                DTileset->DrawTile(drawable, TempGC, XPos, YPos, DSeenIndex);
            }
            if((CVisibilityMap::tvPartialPartial == TileType)||(CVisibilityMap::tvPartial == TileType)){
                int VisibilityIndex = 0, VisibilityMask = 0x1;
                
                for(int YOff = -1; YOff < 2; YOff++){
                    for(int XOff = -1; XOff < 2; XOff++){
                        if(YOff || XOff){
                            CVisibilityMap::ETileVisibility VisTile = DMap->TileType(XIndex + XOff, YIndex + YOff);
                            
                            if(CVisibilityMap::tvVisible == VisTile){
                                VisibilityIndex |= VisibilityMask;   
                            }
                            VisibilityMask <<= 1;
                        }
                    }
                }
                if(-1 == DFogIndices[VisibilityIndex]){
                    if(!UnknownFog[VisibilityIndex]){
                        printf("Unknown fog 0x%02X @ (%d, %d)\n",VisibilityIndex, XIndex, YIndex);   
                        UnknownFog[VisibilityIndex] = true;
                    }
                }
                DTileset->DrawTile(drawable, TempGC, XPos, YPos, DFogIndices[VisibilityIndex]);
            }
            
            if((CVisibilityMap::tvPartialPartial == TileType)||(CVisibilityMap::tvSeenPartial == TileType)){
                int VisibilityIndex = 0, VisibilityMask = 0x1;
                
                for(int YOff = -1; YOff < 2; YOff++){
                    for(int XOff = -1; XOff < 2; XOff++){
                        if(YOff || XOff){
                            CVisibilityMap::ETileVisibility VisTile = DMap->TileType(XIndex + XOff, YIndex + YOff);
                            
                            if((CVisibilityMap::tvVisible == VisTile)||(CVisibilityMap::tvPartial == VisTile)||(CVisibilityMap::tvSeen == VisTile)){
                                VisibilityIndex |= VisibilityMask;   
                            }
                            VisibilityMask <<= 1;
                        }
                    }
                }
                if(-1 == DBlackIndices[VisibilityIndex]){
                    if(!UnknownBlack[VisibilityIndex]){
                        printf("Unknown black 0x%02X @ (%d, %d)\n",VisibilityIndex, XIndex, YIndex);   
                        UnknownBlack[VisibilityIndex] = true;
                    }
                }
                DTileset->DrawTile(drawable, TempGC, XPos, YPos, DBlackIndices[VisibilityIndex]);
            }
        }
    }
    g_object_unref(TempGC);
}

void CFogRenderer::DrawMiniMap(GdkDrawable *drawable){
    GdkGC *TempGC = gdk_gc_new(drawable);

    for(int YIndex = 0; YIndex < DMap->Height(); YIndex++){
        for(int XIndex = 0; XIndex < DMap->Width(); XIndex++){
            CVisibilityMap::ETileVisibility TileType = DMap->TileType(XIndex, YIndex);
            
            if(CVisibilityMap::tvNone == TileType){
                DTileset->DrawTileCorner(drawable, TempGC, XIndex, YIndex, DNoneIndex);
                continue;
            }
            else if(CVisibilityMap::tvVisible == TileType){
                continue;
            }
            else if((CVisibilityMap::tvSeen == TileType)||(CVisibilityMap::tvSeenPartial == TileType)){
                DTileset->DrawTileCorner(drawable, TempGC, XIndex, YIndex, DSeenIndex);
            }
            else{
                DTileset->DrawTileCorner(drawable, TempGC, XIndex, YIndex, DPartialIndex);
            }
        }
    }
    g_object_unref(TempGC);
}

