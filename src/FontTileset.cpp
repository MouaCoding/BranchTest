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
#include "FontTileset.h"
#include "LineDataSource.h"
#include "Tokenizer.h"
#include <stdio.h>
#include <stdlib.h>


CFontTileset::CFontTileset(){
    
}

CFontTileset::~CFontTileset(){

}

bool CFontTileset::LoadFont(std::shared_ptr< CDataSource > source){
    CLineDataSource LineSource(source);
    std::string TempString;
    bool ReturnStatus = false;
    
    if(!LoadTileset(source)){
        return false;    
    }
    
    DCharacterWidths.resize(DTileCount);
    DDeltaWidths.resize(DTileCount);
    DCharacterTops.resize(DTileCount);
    DCharacterBottoms.resize(DTileCount);
    DCharacterBaseline = DTileHeight;
    try{
        for(int Index = 0; Index < DTileCount; Index++){
            if(!LineSource.Read(TempString)){
                goto LoadFontExit;
            }
            DCharacterWidths[Index] = std::stoi(TempString);
        }
        for(int FromIndex = 0; FromIndex < DTileCount; FromIndex++){
            std::vector< std::string > Values;
            DDeltaWidths[FromIndex].resize(DTileCount);
            if(!LineSource.Read(TempString)){
                goto LoadFontExit;
            }
            CTokenizer::Tokenize(Values, TempString);
            if(Values.size() != DTileCount){
                goto LoadFontExit;
            }
            for(int ToIndex = 0; ToIndex < DTileCount; ToIndex++){
                DDeltaWidths[FromIndex][ToIndex] = std::stoi(Values[ToIndex]);
            }
        }
        ReturnStatus = true;
    }
    catch(std::exception &E){
        printf("%s\n",E.what());
    }
    DPixbufTilesets.clear();
    DPixbufTilesets.push_back(DPixbufTileset);
    if(DPixelColors.size()){
        int Width, Height, NumberChannels, RowStride;
        guchar SrcRed, SrcGreen, SrcBlue;
        std::vector< gint > BottomOccurence;
        int BestLine = 0;
        
        Height = gdk_pixbuf_get_height(DPixbufTileset);
        Width = gdk_pixbuf_get_width(DPixbufTileset);
        NumberChannels = gdk_pixbuf_get_n_channels(DPixbufTileset);
        RowStride = gdk_pixbuf_get_rowstride(DPixbufTileset);
        
        SrcRed = (DPixelColors[0].pixel>>16) & 0xFF;
        SrcGreen = (DPixelColors[0].pixel>>8) & 0xFF;
        SrcBlue = DPixelColors[0].pixel & 0xFF;
        BottomOccurence.resize(DTileHeight + 1);
        for(int Index = 0; Index < BottomOccurence.size(); Index++){
            BottomOccurence[Index] = 0;
        }
        for(int Index = 0; Index < DTileCount; Index++){
            int TopOpaque = DTileHeight, BottomOpaque = 0;
            for(int Row = 0; Row < DTileHeight; Row++){
                bool RowClear = true;
                guchar *Pixel = gdk_pixbuf_get_pixels(DPixbufTileset) + (DTileHeight * Index + Row) * RowStride;
                for(int Column = 0; Column < DTileWidth; Column++){
                    if(Pixel[3]){
                        RowClear = false;
                        break;      
                    }
                    Pixel += NumberChannels;   
                }
                if(!RowClear){
                    if(Row < TopOpaque){
                        TopOpaque = Row;
                    }
                    BottomOpaque = Row;
                }
            }
            DCharacterTops[Index] = TopOpaque;
            DCharacterBottoms[Index] = BottomOpaque;
            BottomOccurence[BottomOpaque]++;
        }
        for(int Index = 1; Index < BottomOccurence.size(); Index++){
            if(BottomOccurence[BestLine] < BottomOccurence[Index]){
                BestLine = Index;
            }
        }
        DCharacterBaseline = BestLine;
        for(int Index = 1; Index < DPixelColors.size(); Index++){
            guchar DestRed, DestGreen, DestBlue;
        
            DestRed = (DPixelColors[Index].pixel>>16) & 0xFF;
            DestGreen = (DPixelColors[Index].pixel>>8) & 0xFF;
            DestBlue = DPixelColors[Index].pixel & 0xFF;
            DPixbufTilesets.push_back(gdk_pixbuf_copy(DPixbufTileset));
            
            for(int YPos = 0; YPos < Height; YPos++){
                for(int XPos = 0; XPos < Width; XPos++){
                    guchar *Pixel = gdk_pixbuf_get_pixels(DPixbufTilesets.back()) + YPos * RowStride + XPos * NumberChannels;
                    if((SrcRed == Pixel[0])&&(SrcGreen == Pixel[1])&&(SrcBlue == Pixel[2])){
                        Pixel[0] = DestRed;
                        Pixel[1] = DestGreen;
                        Pixel[2] = DestBlue;
                    }
                }
            }
        }
    }
    
LoadFontExit:
    return ReturnStatus;
}

void CFontTileset::DrawText(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, const std::string &str){
    int LastChar, NextChar;
    for(int Index = 0; Index < str.length(); Index++){
        NextChar = str[Index] - ' ';
        
        if(Index){
            xpos += DCharacterWidths[LastChar] + DDeltaWidths[LastChar][NextChar]; 
        }
        DrawTile(drawable, gc, xpos, ypos, NextChar);
        LastChar = NextChar;
    }
}

void CFontTileset::DrawTextColor(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int colorindex, const std::string &str){
    int LastChar, NextChar;
    if((0 > colorindex)||(colorindex >= DPixbufTilesets.size())){
        return;    
    }
    for(int Index = 0; Index < str.length(); Index++){
        NextChar = str[Index] - ' ';
        
        if(Index){
            xpos += DCharacterWidths[LastChar] + DDeltaWidths[LastChar][NextChar]; 
        }
        
        gdk_draw_pixbuf(drawable, gc, DPixbufTilesets[colorindex], 0, NextChar * DTileHeight, xpos, ypos, DTileWidth, DTileHeight, GDK_RGB_DITHER_NONE, 0, 0);
        LastChar = NextChar;
    }
}

void CFontTileset::DrawTextWithShadow(GdkDrawable *drawable, GdkGC *gc, gint xpos, gint ypos, int color, int shadowcol, int shadowwidth, const std::string &str){
    if((0 > color)||(color >= DPixbufTilesets.size())){
        return;    
    }
    if((0 > shadowcol)||(shadowcol >= DPixbufTilesets.size())){
        return;    
    }
    DrawTextColor(drawable, gc, xpos + shadowwidth, ypos + shadowwidth, shadowcol, str);
    DrawTextColor(drawable, gc, xpos, ypos, color, str);
}

void CFontTileset::MeasureText(const std::string &str, gint &width, gint &height){
    gint TempTop, TempBottom;
    MeasureTextDetailed(str, width, height, TempTop, TempBottom);
}

void CFontTileset::MeasureTextDetailed(const std::string &str, gint &width, gint &height, gint &top, gint &bottom){
    int LastChar, NextChar;
    width = 0;
    top = DTileHeight;
    bottom = 0;
    for(int Index = 0; Index < str.length(); Index++){
        NextChar = str[Index] - ' ';
        
        if(Index){
            width += DDeltaWidths[LastChar][NextChar]; 
        }
        width += DCharacterWidths[NextChar]; 
        if(DCharacterTops[NextChar] < top){
            top = DCharacterTops[NextChar];   
        }
        if(DCharacterBottoms[NextChar] > bottom){
            bottom = DCharacterBottoms[NextChar];   
        }
        LastChar = NextChar;
    }
    height = DTileHeight;
}

