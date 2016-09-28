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
#include "ViewportRenderer.h"
   
CViewportRenderer::CViewportRenderer(std::shared_ptr< CMapRenderer > maprender, std::shared_ptr< CAssetRenderer > assetrender, std::shared_ptr< CFogRenderer > fogrender){
    DMapRenderer = maprender;
    DAssetRenderer = assetrender;
    DFogRenderer = fogrender;
    DViewportX = 0;
    DViewportY = 0;
    DLastViewportWidth = maprender->DetailedMapWidth();
    DLastViewportHeight = maprender->DetailedMapHeight();
}

CViewportRenderer::~CViewportRenderer(){

}


int CViewportRenderer::ViewportX(int x){
    DViewportX = x;
    if(DViewportX + DLastViewportWidth >= DMapRenderer->DetailedMapWidth()){
        DViewportX = DMapRenderer->DetailedMapWidth() - DLastViewportWidth;
    }
    if(0 > DViewportX){
        DViewportX = 0;
    }
    return DViewportX;
}

int CViewportRenderer::ViewportY(int y){
    DViewportY = y;
    if(DViewportY + DLastViewportHeight >= DMapRenderer->DetailedMapHeight()){
        DViewportY = DMapRenderer->DetailedMapHeight() - DLastViewportHeight;
    }
    if(0 > DViewportY){
        DViewportY = 0;
    }
    return DViewportY;
}
                          
void CViewportRenderer::CenterViewport(const CPosition &pos){
    ViewportX(pos.X() - DLastViewportWidth/2);
    ViewportY(pos.Y() - DLastViewportHeight/2);
}

void CViewportRenderer::PanNorth(int pan){
    DViewportY -= pan;
    if(0 > DViewportY){
        DViewportY = 0;
    }
}

void CViewportRenderer::PanEast(int pan){
    ViewportX(DViewportX + pan);
}

void CViewportRenderer::PanSouth(int pan){
    ViewportY(DViewportY + pan);
}

void CViewportRenderer::PanWest(int pan){
    DViewportX -= pan;
    if(0 > DViewportX){
        DViewportX = 0;
    }
}


void CViewportRenderer::DrawViewport(GdkDrawable *drawable, GdkDrawable *typedrawable, const std::list< std::weak_ptr< CPlayerAsset > > &selectionmarkerlist, const SRectangle &selectrect, EAssetCapabilityType curcapability){
    SRectangle TempRectangle;
    EAssetType PlaceType = atNone;
    std::shared_ptr< CPlayerAsset > Builder;
    gdk_pixmap_get_size(drawable, &DLastViewportWidth, &DLastViewportHeight); 
    
    if(DViewportX + DLastViewportWidth >= DMapRenderer->DetailedMapWidth()){
        DViewportX = DMapRenderer->DetailedMapWidth() - DLastViewportWidth;
    }
    if(DViewportY + DLastViewportHeight >= DMapRenderer->DetailedMapHeight()){
        DViewportY = DMapRenderer->DetailedMapHeight() - DLastViewportHeight;
    }
    
    TempRectangle.DXPosition = DViewportX;
    TempRectangle.DYPosition = DViewportY;
    TempRectangle.DWidth = DLastViewportWidth;
    TempRectangle.DHeight = DLastViewportHeight;
    
    switch(curcapability){
        case actBuildFarm:          PlaceType = atFarm;
                                    break;
        case actBuildTownHall:      PlaceType = atTownHall;
                                    break;
        case actBuildBarracks:      PlaceType = atBarracks;
                                    break;
        case actBuildLumberMill:    PlaceType = atLumberMill;
                                    break;
        case actBuildBlacksmith:    PlaceType = atBlacksmith;
                                    break;
        case actBuildScoutTower:    PlaceType = atScoutTower;
                                    break;
        default:                    break;
    }
    DMapRenderer->DrawMap(drawable, typedrawable, TempRectangle, 0);
    DAssetRenderer->DrawSelections(drawable, TempRectangle, selectionmarkerlist, selectrect, atNone != PlaceType);
    DAssetRenderer->DrawAssets(drawable, typedrawable, TempRectangle);
    DMapRenderer->DrawMap(drawable, typedrawable, TempRectangle, 1);
    DAssetRenderer->DrawOverlays(drawable, TempRectangle);
    
    if(selectionmarkerlist.size()){
        Builder = selectionmarkerlist.front().lock();  
    }
    DAssetRenderer->DrawPlacement(drawable, TempRectangle, CPosition(selectrect.DXPosition, selectrect.DYPosition), PlaceType, Builder);
    DFogRenderer->DrawMap(drawable, TempRectangle);
}


