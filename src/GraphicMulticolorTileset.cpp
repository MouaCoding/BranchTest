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
#include "GraphicMulticolorTileset.h"
#include "Debug.h"

CGraphicMulticolorTileset::CGraphicMulticolorTileset() : CGraphicTileset(){
    
}

CGraphicMulticolorTileset::~CGraphicMulticolorTileset(){
    for(int Index = 1; Index < DPixbufTilesets.size(); Index++){
        g_object_unref(G_OBJECT(DPixbufTilesets[Index]));   
    }
}

bool CGraphicMulticolorTileset::LoadTileset(std::shared_ptr< CGraphicRecolorMap > colormap, std::shared_ptr< CDataSource > source){
    int Width, Height, NumberChannels, RowStride;
    
    PrintDebug(DEBUG_LOW,"CGraphicMulticolorTileset::LoadTileset\n"); 
    if(!CGraphicTileset::LoadTileset(source)){
        return false;
    }
    Height = gdk_pixbuf_get_height(DPixbufTileset);
    Width = gdk_pixbuf_get_width(DPixbufTileset);
    NumberChannels = gdk_pixbuf_get_n_channels(DPixbufTileset);
    RowStride = gdk_pixbuf_get_rowstride(DPixbufTileset);
    
    PrintDebug(DEBUG_LOW,"Translating Colors\n"); 
    DPixbufTilesets.clear();
    DPixbufTilesets.push_back(DPixbufTileset);
    for(int Index = 1; Index < colormap->GroupCount(); Index++){
        DPixbufTilesets.push_back(gdk_pixbuf_copy(DPixbufTileset));
        
        for(int YPos = 0; YPos < Height; YPos++){
            for(int XPos = 0; XPos < Width; XPos++){
                guchar *Pixel = gdk_pixbuf_get_pixels(DPixbufTilesets.back()) + YPos * RowStride + XPos * NumberChannels;
                for(int ColIndex = 0; ColIndex < colormap->ColorCount(); ColIndex++){
                    if(colormap->Recolor(Index, Pixel[0], Pixel[1], Pixel[2])){
                        break;   
                    }
                }
            }
        }
    }
    
    return true;
}

void CGraphicMulticolorTileset::DrawTile(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int tileindex, int colorindex){
    if((0 > tileindex)||(tileindex >= DTileCount)){
        return;
    }
    if((0 > colorindex)||(colorindex >= DPixbufTilesets.size())){
        return;    
    }
    gdk_draw_pixbuf(drawable, gc, DPixbufTilesets[colorindex], 0, tileindex * DTileHeight, xpos, ypos, DTileWidth, DTileHeight, GDK_RGB_DITHER_NONE, 0, 0);
}

