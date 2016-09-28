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
#ifndef MAPRENDERER_H
#define MAPRENDERER_H
#include "GraphicTileset.h"
#include "TerrainMap.h"
#include "Rectangle.h"
#include <vector>
#include <unordered_map>

class CMapRenderer{
    protected:
        std::shared_ptr< CGraphicTileset > DTileset;
        std::shared_ptr< CTerrainMap > DMap;
        std::vector< int > DGrassIndices;
        std::vector< int > DTreeIndices;
        std::vector< int > DDirtIndices;
        std::vector< int > DWaterIndices;
        std::vector< int > DRockIndices;
        std::vector< int > DWallIndices;
        std::vector< int > DWallDamagedIndices;
        std::vector< int > DPixelIndices;
        
        std::unordered_map< int, int > DTreeUnknown;
        std::unordered_map< int, int > DWaterUnknown;
        std::unordered_map< int, int > DDirtUnknown;
        std::unordered_map< int, int > DRockUnknown;
        
        void MakeHammingSet(int value, std::vector< int > &hammingset);
        int FindUnknown(CTerrainMap::ETileType type, int known, int unknown);
        
    public:
        CMapRenderer(std::shared_ptr< CGraphicTileset > tileset, std::shared_ptr< CTerrainMap > map);
        
        int MapWidth() const;
        int MapHeight() const;
        int DetailedMapWidth() const;
        int DetailedMapHeight() const;
        
        void DrawMap(GdkDrawable *drawable, GdkDrawable *typedrawable, const SRectangle &rect, int level);
        void DrawMiniMap(GdkDrawable *drawable);
};

#endif

