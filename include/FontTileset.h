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
#ifndef FONTTILESET_H
#define FONTTILESET_H
#include "GraphicTileset.h"
#include "DataSource.h"
#include <vector>

class CFontTileset : public CGraphicTileset{
    protected:
        std::vector< GdkPixbuf * > DPixbufTilesets;
        std::vector< gint > DCharacterWidths;
        std::vector< std::vector < gint > > DDeltaWidths;
        std::vector< gint > DCharacterTops;
        std::vector< gint > DCharacterBottoms;
        gint DCharacterBaseline;
        
    public:
        CFontTileset();
        virtual ~CFontTileset();
        
        bool LoadFont(std::shared_ptr< CDataSource > source);
        
        gint CharacterBaseline() const{
            return DCharacterBaseline;  
        };
        
        void DrawText(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, const std::string &str);
        void DrawTextColor(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int colorindex, const std::string &str);
        void DrawTextWithShadow(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int color, int shadowcol, int shadowwidth, const std::string &str);
        void MeasureText(const std::string &str, gint &width, gint &height);
        void MeasureTextDetailed(const std::string &str, gint &width, gint &height, gint &top, gint &bottom);
};

#endif

