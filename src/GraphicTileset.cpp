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
#include "GraphicTileset.h"
#include "GraphicLoader.h"
#include "LineDataSource.h"
#include "Tokenizer.h"
#include "Debug.h"
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

CGraphicTileset::CGraphicTileset(){
    DPixbufTileset = nullptr;
    DTileCount = 0;
    DTileWidth = 0;
    DTileHeight = 0;
    DTileHalfWidth = 0;
    DTileHalfHeight = 0;
    DPixelCount = 0;
}

CGraphicTileset::~CGraphicTileset(){
    if(nullptr != DPixbufTileset){
        g_object_unref(G_OBJECT(DPixbufTileset));
    }
    for(auto &Bitmap : DClippingMasks){
        g_object_unref(G_OBJECT(Bitmap));
    }
}

int CGraphicTileset::TileCount(int count){
    GdkPixbuf *TempPixbufTileset;
    guchar *Pixels;
    guint PixelLength;
    
    if(0 > count){
        return false;   
    }
    if(!DTileWidth || !DTileHeight){
        return false;
    }
    if(count < DTileCount){
        auto Iterator = DMapping.begin();
        DTileCount = count;
        
        while(DMapping.end() != Iterator){
            if(Iterator->second >= DTileCount){
                Iterator = DMapping.erase(Iterator);
            }
            else{
                Iterator++;   
            }
        }
        return true;
    }
    TempPixbufTileset = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, DTileWidth, count * DTileHeight);
    if(nullptr == TempPixbufTileset){
        return false;   
    }
    Pixels = gdk_pixbuf_get_pixels_with_length(TempPixbufTileset, &PixelLength);
    memset(Pixels, 0, PixelLength);
    if(nullptr != DPixbufTileset){
        int NumberChannels;
        NumberChannels = gdk_pixbuf_get_n_channels(DPixbufTileset);
        
        memcpy(Pixels, gdk_pixbuf_get_pixels(DPixbufTileset), NumberChannels * DTileHeight * DTileWidth * DTileCount);
        g_object_unref(G_OBJECT(DPixbufTileset));
    }
    DPixbufTileset = TempPixbufTileset;
    DTileCount = count;
    return true;
}

bool CGraphicTileset::ClearTile(int index){
    int NumberChannels, RowStride;
    
    if((0 > index)||(index >= DTileCount)){
        return false;
    }
    if(nullptr == DPixbufTileset){
        return false;
    }    
    NumberChannels = gdk_pixbuf_get_n_channels(DPixbufTileset);
    RowStride = gdk_pixbuf_get_rowstride(DPixbufTileset);

    memset(gdk_pixbuf_get_pixels(DPixbufTileset) + (index * DTileHeight) * RowStride, 0, NumberChannels * DTileHeight * DTileWidth);
    
    return true;
}

bool CGraphicTileset::DuplicateTile(int destindex, const std::string &tilename, int srcindex){
    guchar *Pixels;
    int NumberChannels, RowStride;
    
    if((0 > srcindex)||(0 > destindex)||(srcindex >= DTileCount)||(destindex >= DTileCount)){
        return false;
    }
    if(tilename.empty()){
        return false;
    }
    ClearTile(destindex);
    
    NumberChannels = gdk_pixbuf_get_n_channels(DPixbufTileset);
    RowStride = gdk_pixbuf_get_rowstride(DPixbufTileset);
    Pixels = gdk_pixbuf_get_pixels(DPixbufTileset);
    
    memcpy(Pixels + (destindex * DTileHeight) * RowStride, Pixels + (srcindex * DTileHeight) * RowStride, NumberChannels * DTileHeight * DTileWidth);

    return true;
}

bool CGraphicTileset::OrAlphaTile(int destindex, int srcindex){
    guchar *PixelSrc, *PixelDest;
    int NumberChannels, RowStride, TilePixels;
    
    if((0 > srcindex)||(0 > destindex)||(srcindex >= DTileCount)||(destindex >= DTileCount)){
        return false;
    }

    NumberChannels = gdk_pixbuf_get_n_channels(DPixbufTileset);
    RowStride = gdk_pixbuf_get_rowstride(DPixbufTileset);
    PixelSrc = PixelDest = gdk_pixbuf_get_pixels(DPixbufTileset);
    
    PixelSrc += (srcindex * DTileHeight) * RowStride;
    PixelDest += (destindex * DTileHeight) * RowStride;
    TilePixels = DTileHeight * DTileWidth;
    for(int Index = 0; Index < TilePixels; Index++){
        if(0 == PixelSrc[3]){
            PixelDest[3] = 0;   
        }
        
        PixelSrc += NumberChannels;
        PixelDest += NumberChannels;
    }
    
    return true;
}

int CGraphicTileset::FindTile(const std::string &tilename) const{
    auto Iterator = DMapping.find(tilename);
    if(DMapping.end() != Iterator){
        return Iterator->second;
    }
    return -1;
}

int CGraphicTileset::FindPixel(const std::string &pixelname) const{
    auto Iterator = DPixelMapping.find(pixelname);
    if(DPixelMapping.end() != Iterator){
        return Iterator->second;
    }
    return -1;
}

bool CGraphicTileset::LoadTileset(std::shared_ptr< CDataSource > source){
    CLineDataSource LineSource(source);
    std::string PNGPath, TempString;
    std::vector< std::string > Tokens;
    bool ReturnStatus = false;
    
    if(nullptr == source){
        return false;   
    }
    if(!LineSource.Read(PNGPath)){
        PrintError("Failed to get path.\n");
        goto LoadTilesetExit;
    }
    DPixbufTileset = CGraphicLoader::LoadPixbuf(source->Container()->DataSource(PNGPath));
    if(nullptr == DPixbufTileset){
        PrintError("Failed to load file %s.\n", PNGPath.c_str());
        goto LoadTilesetExit;        
    }
    DTileWidth = gdk_pixbuf_get_width(DPixbufTileset);
    DTileHeight = gdk_pixbuf_get_height(DPixbufTileset);
    if(gdk_pixbuf_get_has_alpha(DPixbufTileset)){
        PrintDebug(DEBUG_LOW,"File %s has alpha channel\n", PNGPath.c_str());    
    }

    if(!LineSource.Read(TempString)){
        goto LoadTilesetExit;
    }

    DTileCount = atoi(TempString.c_str());
    DTileHeight /= DTileCount;
    for(int Index = 0; Index < DTileCount; Index++){
        if(!LineSource.Read(TempString)){
            goto LoadTilesetExit;
        }
        DMapping[TempString] = Index;
    }
    DTileHalfWidth = DTileWidth / 2;
    DTileHalfHeight = DTileHeight / 2;
    for(int Index = 0; Index < DTileCount; Index++){
        GdkBitmap *NewClipMask;
    
        NewClipMask = gdk_pixmap_new(NULL, DTileWidth, DTileHeight, 1);
        gdk_pixbuf_render_threshold_alpha(DPixbufTileset, NewClipMask, 0, Index * DTileHeight, 0, 0, DTileWidth, DTileHeight, 1);
        
        DClippingMasks.push_back(NewClipMask);
    }
    DPixelColors.clear();
    DPixelMapping.clear();
    PrintDebug(DEBUG_LOW, "Tile Dims %d x %d\n", DTileWidth, DTileHeight);
    if(!LineSource.Read(TempString)){
        ReturnStatus = true;
        goto LoadTilesetExit;
    }
    
    try{
        DPixelCount = std::stoi(TempString);
        for(int Index = 0; Index < DPixelCount; Index++){
            GdkColor TempColor;
            std::size_t NextPos;
            if(!LineSource.Read(TempString)){
                PrintError("Failed to read %d pixel color.\n", Index);
                goto LoadTilesetExit;
            } 
            CTokenizer::Tokenize(Tokens, TempString);
            if(Tokens.size() != 2){
                PrintError("Failed pixel color %d does not have 2 tokens.\n", Index);
                goto LoadTilesetExit;
            }
            TempColor.pixel = std::stoi(Tokens[1], &NextPos, 16);
            TempColor.red = (TempColor.pixel>>8) & 0xFF00;
            TempColor.green = TempColor.pixel & 0xFF00;
            TempColor.blue = (TempColor.pixel<<8) & 0xFF00;
            DPixelMapping[Tokens[0]] = DPixelColors.size();
            DPixelColors.push_back(TempColor);
        }
    }
    catch(std::exception &E){
        PrintError("%s\n",E.what());
    }
    ReturnStatus = true;
LoadTilesetExit:
    return ReturnStatus;
}

void CGraphicTileset::DrawTile(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int tileindex){
    if((0 > tileindex)||(tileindex >= DTileCount)){
        return;
    }
    gdk_draw_pixbuf(drawable, gc, DPixbufTileset, 0, tileindex * DTileHeight, xpos, ypos, DTileWidth, DTileHeight, GDK_RGB_DITHER_NONE, 0, 0);
}

void CGraphicTileset::DrawTileCorner(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int tileindex){
    if((0 > tileindex)||(tileindex >= DTileCount)){
        return;
    }
    gdk_draw_pixbuf(drawable, gc, DPixbufTileset, 0, tileindex * DTileHeight, xpos, ypos, 1, 1, GDK_RGB_DITHER_NONE, 0, 0);
}

void CGraphicTileset::DrawTileRectangle(GdkDrawable *drawable, gint xpos, gint ypos, gint width, gint height, int tileindex){
    if((0 > tileindex)||(tileindex >= DTileCount)){
        return;
    }
    GdkGC *TempGC = gdk_gc_new(drawable);
    GdkRectangle TempRect({xpos, ypos, width, height});
    int MaxXPos = xpos + width;
    int MaxYPos = ypos + height;
    int TileOffset = tileindex * DTileHeight;
    
    gdk_gc_set_clip_rectangle(TempGC, &TempRect);
    for(int YPos = ypos; YPos < MaxYPos; YPos += DTileHeight){
        for(int XPos = xpos; XPos < MaxXPos; XPos += DTileWidth){
            gdk_draw_pixbuf(drawable, TempGC, DPixbufTileset, 0, TileOffset, XPos, YPos, DTileWidth, DTileHeight, GDK_RGB_DITHER_NONE, 0, 0);
        }
    }
    g_object_unref(TempGC);
}

void CGraphicTileset::DrawClipped(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int tileindex, guint32 color){
    GdkColor ClipColor;
    
    ClipColor.pixel = color;
    ClipColor.red = (color>>8) & 0xFF00;
    ClipColor.green = color & 0xFF00;
    ClipColor.blue = (color<<8) & 0xFF00;
    
    if((0 > tileindex)||(tileindex >= DTileCount)){
        return;
    }
    gdk_gc_set_clip_mask(gc, DClippingMasks[tileindex]);
    gdk_gc_set_clip_origin(gc, xpos, ypos);
    gdk_gc_set_rgb_fg_color(gc, &ClipColor);
    gdk_gc_set_rgb_bg_color(gc, &ClipColor);
    
    gdk_draw_rectangle(drawable, gc, TRUE, xpos, ypos, DTileWidth, DTileHeight);
}

void CGraphicTileset::DrawPixel(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int size, int pixelindex){

    if((0 > pixelindex)||(pixelindex >= DPixelCount)){
        return;
    }
    if(0 >= size){
        return;
    }
    gdk_gc_set_rgb_fg_color(gc, &DPixelColors[pixelindex]);
    gdk_gc_set_rgb_bg_color(gc, &DPixelColors[pixelindex]);
    
    gdk_draw_rectangle(drawable, gc, TRUE, xpos, ypos, size, size);
}

