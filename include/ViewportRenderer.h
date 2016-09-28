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
#ifndef VIEWPORTRENDERER_H
#define VIEWPORTRENDERER_H
#include "MapRenderer.h"
#include "AssetRenderer.h"
#include "FogRenderer.h"

class CViewportRenderer{        
    protected:
        std::shared_ptr< CMapRenderer > DMapRenderer;
        std::shared_ptr< CAssetRenderer > DAssetRenderer;
        std::shared_ptr< CFogRenderer > DFogRenderer;
        int DViewportX;
        int DViewportY;
        gint DLastViewportWidth;
        gint DLastViewportHeight;
        
    public:        
        CViewportRenderer(std::shared_ptr< CMapRenderer > maprender, std::shared_ptr< CAssetRenderer > assetrender, std::shared_ptr< CFogRenderer > fogrender);
        ~CViewportRenderer();
        
        void InitViewportDimensions(int width, int height){
            DLastViewportWidth = width;
            DLastViewportHeight = height;
        };
        
        int ViewportX() const{
            return DViewportX;  
        };
        int ViewportX(int x);
        
        int ViewportY() const{
            return DViewportY;  
        };
        int ViewportY(int y);
        
        gint LastViewportWidth() const{
            return DLastViewportWidth;
        };
        gint LastViewportHeight() const{
            return DLastViewportHeight;
        };
        void CenterViewport(const CPosition &pos);
        
        CPosition DetailedPosition(const CPosition &pos) const{
            return CPosition(pos.X() + DViewportX, pos.Y() + DViewportY);  
        };
        
        void PanNorth(int pan);
        void PanEast(int pan);
        void PanSouth(int pan);
        void PanWest(int pan);
        
        void DrawViewport(GdkDrawable *drawable, GdkDrawable *typedrawable, const std::list< std::weak_ptr< CPlayerAsset > > &selectionmarkerlist, const SRectangle &selectrect, EAssetCapabilityType curcapability);
};

#endif

