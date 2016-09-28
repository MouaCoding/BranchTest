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
#include "ListViewRenderer.h"

CListViewRenderer::CListViewRenderer(std::shared_ptr< CGraphicTileset > icons, std::shared_ptr< CFontTileset > font){
    DIconTileset = icons;
    DFont = font;
    DFontHeight = 1;
    DLastItemCount = 0;
    DLastItemOffset = 0;
    DLastViewWidth = 0;
    DLastViewHeight = 0;
    DLastUndisplayed = false;
}

CListViewRenderer::~CListViewRenderer(){
    
}

int CListViewRenderer::ItemAt(int x, int y){
    if((0 > x)||(0 > y)){
        return lvoNone;   
    }
    if((DLastViewWidth <= x)||(DLastViewHeight <= y)){
        return lvoNone;    
    }
    if(x < (DLastViewWidth - DIconTileset->TileWidth())){
        if((y / DFontHeight) < DLastItemCount){
            return DLastItemOffset + (y / DFontHeight);
        }
    }
    else if(y < DIconTileset->TileHeight()){
        if(DLastItemOffset){
            return lvoUpArrow;
        }
    }
    else if(y > DLastViewHeight - DIconTileset->TileHeight()){
        if(DLastUndisplayed){
            return lvoDownArrow;    
        }
    }
    return lvoNone;
}

void CListViewRenderer::DrawListView(GdkDrawable *drawable, int selectedindex, int offsetindex, std::vector< std::string > &items){
    GdkGC *TempGC = gdk_gc_new(drawable);
    gint TextWidth, TextHeight;
    int MaxTextWidth;
    gdk_pixmap_get_size(drawable, &DLastViewWidth, &DLastViewHeight); 
    int BlackIndex = DFont->FindPixel("black");
    int WhiteIndex = DFont->FindPixel("white");
    int GoldIndex = DFont->FindPixel("gold");
    int TextYOffset = 0;
    
    DLastItemCount = 0;
    DLastItemOffset = offsetindex;
    MaxTextWidth = DLastViewWidth - DIconTileset->TileWidth();
    DIconTileset->DrawTileRectangle(drawable, 0, 0, DLastViewWidth, DLastViewHeight, DIconTileset->FindTile("background"));
    DIconTileset->DrawTile(drawable, TempGC, MaxTextWidth, 0, offsetindex ? DIconTileset->FindTile("up-active") : DIconTileset->FindTile("up-inactive"));
    DLastUndisplayed = false;
    while((offsetindex < items.size())&&(TextYOffset < DLastViewHeight)){
        std::string TempString = items[offsetindex];
        
        DFont->MeasureText(TempString, TextWidth, TextHeight);
        if(TextWidth >= MaxTextWidth){
            while(TempString.length()){
                TempString = TempString.substr(0,TempString.length()-1);
                DFont->MeasureText(TempString + "...", TextWidth, TextHeight);
                if(TextWidth < MaxTextWidth){
                    TempString += "...";
                    break;
                }
            }
        }
        DFont->DrawTextWithShadow(drawable, TempGC, 0, TextYOffset, offsetindex == selectedindex ? WhiteIndex : GoldIndex, BlackIndex, 1, TempString);
        DFontHeight = TextHeight;
        TextYOffset += DFontHeight;
        DLastItemCount++;
        offsetindex++;    
    }
    if(DLastItemCount + DLastItemOffset < items.size()){
        DLastUndisplayed = true;    
    }
    DIconTileset->DrawTile(drawable, TempGC, MaxTextWidth, DLastViewHeight - DIconTileset->TileWidth(), DLastUndisplayed ? DIconTileset->FindTile("down-active") : DIconTileset->FindTile("down-inactive"));
    
    g_object_unref(TempGC);
}

