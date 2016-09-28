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
#ifndef GRAPHICTILESET_H
#define GRAPHICTILESET_H
#include <gtk/gtk.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "DataSource.h"

class CGraphicTileset{
    protected:
        GdkPixbuf *DPixbufTileset;
        std::vector< GdkBitmap * > DClippingMasks;
        std::unordered_map< std::string, int > DMapping;
        int DTileCount;       
        int DTileWidth;
        int DTileHeight;
        int DTileHalfWidth;
        int DTileHalfHeight;
        int DPixelCount;
        std::vector< GdkColor > DPixelColors;
        std::unordered_map< std::string, int > DPixelMapping;
        
    public:
        CGraphicTileset();
        virtual ~CGraphicTileset();

        int TileCount() const{
            return DTileCount;
        };
        int TileCount(int count);
        
        int TileWidth() const{
            return DTileWidth;
        };
        int TileHeight() const{
            return DTileHeight;
        };
        int TileHalfWidth() const{
            return DTileHalfWidth;
        };
        int TileHalfHeight() const{
            return DTileHalfHeight;
        };
        
        int PixelCount() const{
            return DPixelCount;
        };
        GdkColor PixelColor(int index){
            if((0 > index)||(DPixelColors.size() <= index)){
                return GdkColor({0x000000, 0x0000, 0x0000, 0x0000});
            }
            return DPixelColors[index];
        };
        
        int FindTile(const std::string &tilename) const;
        int FindPixel(const std::string &pixelname) const;
        
        bool ClearTile(int index);
        bool DuplicateTile(int destindex, const std::string &tilename, int srcindex);
        bool OrAlphaTile(int destindex, int srcindex);

        bool LoadTileset(std::shared_ptr< CDataSource > source);
        
        void DrawTile(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int tileindex);
        void DrawTileCorner(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int tileindex);
        void DrawTileRectangle(GdkDrawable *drawable, gint xpos, gint ypos, gint width, gint height, int tileindex);
        void DrawClipped(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int tileindex, guint32 color);
        void DrawPixel(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int size, int pixelindex);
};

#endif

