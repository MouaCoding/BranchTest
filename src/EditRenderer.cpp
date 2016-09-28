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
#include "EditRenderer.h"

CEditRenderer::CEditRenderer(std::shared_ptr< CGraphicTileset > colors, std::shared_ptr< CBevel > innerbevel, std::shared_ptr< CFontTileset > font){
    DColorTileset = colors;
    DInnerBevel = innerbevel;
    DFont = font;
    DBackgroundColor = pcNone;

    DLightIndices.resize(pcMax);
    DDarkIndices.resize(pcMax);             
    DDarkIndices[pcNone] = DDarkIndices[pcBlue] = DColorTileset->FindTile("blue-dark");
    DDarkIndices[pcRed] = DColorTileset->FindTile("red-dark");
    DDarkIndices[pcGreen] = DColorTileset->FindTile("green-dark");
    DDarkIndices[pcPurple] = DColorTileset->FindTile("purple-dark");
    DDarkIndices[pcOrange] = DColorTileset->FindTile("orange-dark");
    DDarkIndices[pcYellow] = DColorTileset->FindTile("yellow-dark");
    DDarkIndices[pcBlack] = DColorTileset->FindTile("black-dark");
    DDarkIndices[pcWhite] = DColorTileset->FindTile("white-dark");

    DLightIndices[pcNone] = DLightIndices[pcBlue] = DColorTileset->FindTile("blue-light");
    DLightIndices[pcRed] = DColorTileset->FindTile("red-light");
    DLightIndices[pcGreen] = DColorTileset->FindTile("green-light");
    DLightIndices[pcPurple] = DColorTileset->FindTile("purple-light");
    DLightIndices[pcOrange] = DColorTileset->FindTile("orange-light");
    DLightIndices[pcYellow] = DColorTileset->FindTile("yellow-light");
    DLightIndices[pcBlack] = DColorTileset->FindTile("black-light");
    DLightIndices[pcWhite] = DColorTileset->FindTile("white-light");
    
    DWhiteIndex = DFont->FindPixel("white");
    DGoldIndex = DFont->FindPixel("gold");
    DRedIndex = DFont->FindPixel("red");
    DBlackIndex = DFont->FindPixel("black");
    
    DWidth = 0;
    DHeight = 0;
    MinimumCharacters(16);
}

std::string CEditRenderer::Text(const std::string &text, bool valid){
    DText = text;
    DTextValid = valid;
    return DText;
}

int CEditRenderer::MinimumCharacters(int minchar){
    if(0 < minchar){
        gint TextWidth, TextHeight;
        std::string TempText;
        
        for(int Index = 0; Index < minchar; Index++){
            TempText += "X";
        }
        TempText += "|";
        DFont->MeasureText(TempText, TextWidth, TextHeight);    
        DMinimumCharacters = minchar;
    
        DMinimumWidth = TextWidth + DInnerBevel->Width() * 2;
        DMinimumHeight = TextHeight + DInnerBevel->Width() * 2;
        if(DWidth < DMinimumWidth){
            DWidth = DMinimumWidth;   
        }
        if(DHeight < DMinimumHeight){
            DHeight = DMinimumHeight;   
        }
    }
    return DMinimumCharacters;
}

int CEditRenderer::Width(int width){
    if(DMinimumWidth <= width){
        DWidth = width;   
    }
    return DWidth;
}

int CEditRenderer::Height(int height){
    if(DMinimumHeight <= height){
        DHeight = height;   
    }
    return DHeight;
}

void CEditRenderer::DrawEdit(GdkDrawable *drawable, int x, int y, int cursorpos){
    GdkGC *TempGC = gdk_gc_new(drawable);
    int BevelWidth = DInnerBevel->Width();
    int TextColorIndex;
    gint TextWidth, TextHeight;
    std::string RenderText;
    
    DColorTileset->DrawTileRectangle(drawable, x, y, DWidth, DHeight, 0 <= cursorpos ? DLightIndices[DBackgroundColor] : DDarkIndices[DBackgroundColor]);
    if(0 <= cursorpos){
        if(DText.length()){
            bool RemoveCharacters = false;
            RenderText = DText.substr(0,cursorpos);
            RenderText += "|";
            do{
                DFont->MeasureText(RenderText, TextWidth, TextHeight); 
                TextWidth += BevelWidth * 2;
                if(TextWidth > DWidth){
                    RenderText = RenderText.substr(1, RenderText.length()-1);
                    RemoveCharacters = true;
                }
            }while(DWidth < TextWidth);
            if(!RemoveCharacters){
                if(cursorpos < DText.length()){
                    RenderText += DText.substr(cursorpos);    
                }
            }
        }
        else{
            RenderText = "|";   
        }
    }
    else{
        RenderText = DText;
    }
    do{
        DFont->MeasureText(RenderText, TextWidth, TextHeight); 
        TextWidth += BevelWidth * 2;
        if(TextWidth > DWidth){
            RenderText = RenderText.substr(0, RenderText.length()-1);
        }
    }while(DWidth < TextWidth);
    if(DTextValid){
        TextColorIndex = 0 <= cursorpos ? DWhiteIndex : DGoldIndex;
    }
    else{
        TextColorIndex = DRedIndex;   
    }
    DFont->DrawTextWithShadow(drawable, TempGC, x + BevelWidth, y + BevelWidth, TextColorIndex, DBlackIndex, 1, RenderText);
    DInnerBevel->DrawBevel(drawable, x + BevelWidth, y + BevelWidth, DWidth - BevelWidth * 2, DHeight - BevelWidth * 2);
    g_object_unref(TempGC);
}

