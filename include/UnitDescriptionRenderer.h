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
#ifndef UNITDESCRIPTIONRENDERER_H
#define UNITDESCRIPTIONRENDERER_H
#include "Bevel.h"
#include "FontTileset.h"
#include "GraphicMulticolorTileset.h"
#include "PlayerAsset.h"
#include <list>
#include <vector>

class CUnitDescriptionRenderer{
    public:
        typedef enum{
            fsSmall = 0,
            fsMedium,
            fsLarge,
            fsGiant,
            fsMax
        } EFontSize, *EFontSizeRef;
        
    protected:
        std::shared_ptr< CGraphicMulticolorTileset > DIconTileset;
        std::shared_ptr< CBevel > DBevel;
        std::shared_ptr< CFontTileset > DFonts[fsMax];
        std::vector< int > DAssetIndices;
        std::vector< int > DResearchIndices;
        std::vector< int > DFontColorIndices[fsMax];
        std::vector< GdkColor > DHealthColors;
        EPlayerColor DPlayerColor;
        GdkColor DHealthRectangleFG;
        GdkColor DHealthRectangleBG;
        GdkColor DConstructionRectangleFG;
        GdkColor DConstructionRectangleBG;
        GdkColor DConstructionRectangleCompletion;
        GdkColor DConstructionRectangleShadow;
        int DFullIconWidth;
        int DFullIconHeight;
        int DDisplayedWidth;
        int DDisplayedHeight;
        int DDisplayedIcons;
        
        static std::string AddAssetNameSpaces(const std::string &name);
        void DrawCompletionBar(GdkDrawable *drawable, GdkGC *gc, int percent);
        
    public:        
        CUnitDescriptionRenderer(std::shared_ptr< CBevel > bevel, std::shared_ptr< CGraphicMulticolorTileset > icons, std::shared_ptr< CFontTileset > fonts[fsMax], EPlayerColor color);
        ~CUnitDescriptionRenderer();
        
        int MinimumWidth() const;
        int MinimumHeight(int width, int count) const;
        
        int MaxSelection(int width, int height) const;
        int Selection(const CPosition &pos) const;
        
        void DrawUnitDescription(GdkDrawable *drawable, const std::list< std::weak_ptr< CPlayerAsset > > &selectionlist);
};

#endif

