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
#include "MiniMapRenderer.h"

CMiniMapRenderer::CMiniMapRenderer(std::shared_ptr< CMapRenderer > maprender, std::shared_ptr< CAssetRenderer > assetrender, std::shared_ptr< CFogRenderer > fogrender, std::shared_ptr< CViewportRenderer > viewport, gint depth){
    GdkColor ColorBlack = {0,0,0,0};
    GdkGC *TempGC;
    
    DMapRenderer = maprender;
    DAssetRenderer = assetrender;
    DFogRenderer = fogrender;
    DViewportRenderer = viewport;
    DWorkingPixbuf = nullptr;
    DViewportColor.pixel = 0xFFFFFF;
    DViewportColor.red = 0xFFFF;
    DViewportColor.green = 0xFFFF;
    DViewportColor.blue = 0xFFFF;
    
    DVisibleWidth = DMapRenderer->MapWidth();
    DVisibleHeight = DMapRenderer->MapHeight();
    DWorkingPixmap = gdk_pixmap_new(NULL, DMapRenderer->MapWidth(), DMapRenderer->MapHeight(), depth);
    TempGC = gdk_gc_new(DWorkingPixmap);
    gdk_gc_set_rgb_fg_color(TempGC, &ColorBlack);
    gdk_gc_set_rgb_bg_color(TempGC, &ColorBlack);
    gdk_draw_rectangle(DWorkingPixmap, TempGC, TRUE, 0, 0, DMapRenderer->MapWidth(), DMapRenderer->MapHeight());
    g_object_unref(TempGC);
}

CMiniMapRenderer::~CMiniMapRenderer(){
    if(!DWorkingPixmap){
        g_object_unref(DWorkingPixmap);
    }
    if(nullptr != DWorkingPixbuf){
        g_object_unref(DWorkingPixbuf);
    }
}

guint32 CMiniMapRenderer::ViewportColor() const{
    return DViewportColor.pixel;
}

guint32 CMiniMapRenderer::ViewportColor(guint32 color){
    return DViewportColor.pixel = color;
}

gint CMiniMapRenderer::VisibleWidth() const{
    return DVisibleWidth;
}

gint CMiniMapRenderer::VisibleHeight() const{
    return DVisibleHeight;
}

void CMiniMapRenderer::DrawMiniMap(GdkDrawable *drawable){
    GdkPixbuf *ScaledPixbuf;
    GdkGC *TempGC = gdk_gc_new(drawable);
    gint MiniMapViewportX, MiniMapViewportY;
    gint MiniMapViewportWidth, MiniMapViewportHeight;
    gint MiniMapWidth, MiniMapHeight;
    gint MMW_MH, MMH_MW;
    
    gdk_pixmap_get_size(drawable, &MiniMapWidth, &MiniMapHeight); 
    MMW_MH = MiniMapWidth * DMapRenderer->MapHeight();
    MMH_MW = MiniMapHeight * DMapRenderer->MapWidth();
    
    if(MMH_MW > MMW_MH){
        DVisibleWidth = MiniMapWidth;
        DVisibleHeight = (DMapRenderer->MapHeight() * MiniMapWidth) / DMapRenderer->MapWidth();        
    }
    else if(MMH_MW < MMW_MH){
        DVisibleWidth = (DMapRenderer->MapWidth() * MiniMapHeight) / DMapRenderer->MapHeight();
        DVisibleHeight = MiniMapHeight;
    }
    else{
        DVisibleWidth = MiniMapWidth;
        DVisibleHeight = MiniMapHeight;
    }
    
    DMapRenderer->DrawMiniMap(DWorkingPixmap);
    DAssetRenderer->DrawMiniAssets(DWorkingPixmap);
    if(nullptr != DFogRenderer){
        DFogRenderer->DrawMiniMap(DWorkingPixmap);
    }
    
    DWorkingPixbuf = gdk_pixbuf_get_from_drawable(DWorkingPixbuf, DWorkingPixmap, nullptr, 0, 0, 0, 0, -1, -1);
    
    ScaledPixbuf = gdk_pixbuf_scale_simple(DWorkingPixbuf, DVisibleWidth, DVisibleHeight, GDK_INTERP_BILINEAR);
    gdk_draw_pixbuf(drawable, TempGC, ScaledPixbuf, 0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
    g_object_unref(ScaledPixbuf);

    if(nullptr != DViewportRenderer){
        gdk_gc_set_rgb_fg_color(TempGC, &DViewportColor);
        MiniMapViewportX = (DViewportRenderer->ViewportX() * DVisibleWidth) / DMapRenderer->DetailedMapWidth();
        MiniMapViewportY = (DViewportRenderer->ViewportY() * DVisibleHeight) / DMapRenderer->DetailedMapHeight();
        MiniMapViewportWidth = (DViewportRenderer->LastViewportWidth() * DVisibleWidth) / DMapRenderer->DetailedMapWidth();
        MiniMapViewportHeight = (DViewportRenderer->LastViewportHeight() * DVisibleHeight) / DMapRenderer->DetailedMapHeight();
        gdk_draw_rectangle(drawable, TempGC, FALSE, MiniMapViewportX, MiniMapViewportY, MiniMapViewportWidth,  MiniMapViewportHeight);
    }
    
    g_object_unref(TempGC);
}

