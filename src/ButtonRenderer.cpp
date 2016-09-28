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
#include "ButtonRenderer.h"
 
        
CButtonRenderer::CButtonRenderer(std::shared_ptr< CGraphicTileset > colors, std::shared_ptr< CBevel > innerbevel, std::shared_ptr< CBevel > outerbevel, std::shared_ptr< CFontTileset > font){
    DColorTileset = colors;
    DOuterBevel = outerbevel;
    DInnerBevel = innerbevel;
    DFont = font;
    DButtonColor = pcNone;
    DTextOffsetX = 0;
    DTextOffsetY = 0;
    DWidth = DOuterBevel->Width() * 2;
    DHeight = DWidth;
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
    DBlackIndex = DFont->FindPixel("black");
}

std::string CButtonRenderer::Text(const std::string &text, bool minimize){
    gint TotalWidth, TotalHeight, Top, Bottom;
    DText = text;
    DFont->MeasureTextDetailed(DText, TotalWidth, TotalHeight, Top, Bottom);
    
    TotalHeight = Bottom - Top + 1;
    if(TotalHeight + DOuterBevel->Width() * 2 > DHeight){
        DHeight = TotalHeight + DOuterBevel->Width() * 2;  
    }
    else if(minimize){
        DHeight = TotalHeight + DOuterBevel->Width() * 2;  
    }
    if(TotalWidth + DOuterBevel->Width() * 2 > DWidth){
        DWidth = TotalWidth + DOuterBevel->Width() * 2;
    }
    else if(minimize){
        DWidth = TotalWidth + DOuterBevel->Width() * 2;
    }
    DTextOffsetX = DWidth/2 - TotalWidth/2;
    DTextOffsetY = DHeight/2 - TotalHeight/2 - Top;
    return DText;
}

int CButtonRenderer::Width(int width){
    if(width > DWidth){
        gint TotalWidth, TotalHeight, Top, Bottom;
        
        DFont->MeasureTextDetailed(DText, TotalWidth, TotalHeight, Top, Bottom);
        DWidth = width;  
        DTextOffsetX = DWidth/2 - TotalWidth/2;
    }
    return DWidth;
}

int CButtonRenderer::Height(int height){
    if(height > DHeight){
        gint TotalWidth, TotalHeight, Top, Bottom;
        
        DFont->MeasureTextDetailed(DText, TotalWidth, TotalHeight, Top, Bottom);
        TotalHeight = Bottom - Top + 1;
        DHeight = height;
        DTextOffsetY = DHeight/2 - TotalHeight/2 - Top;
        
    }
    return DHeight;    
}

void CButtonRenderer::DrawButton(GdkDrawable *drawable, int x, int y, EButtonState state){
    GdkGC *TempGC = gdk_gc_new(drawable);
    
    if(bsPressed == state){
        int BevelWidth = DInnerBevel->Width();
        DColorTileset->DrawTileRectangle(drawable, x, y, DWidth, DHeight, DDarkIndices[DButtonColor]);
        DFont->DrawTextWithShadow(drawable, TempGC, x + DTextOffsetX, y + DTextOffsetY, DWhiteIndex, DBlackIndex, 1, DText);
        DInnerBevel->DrawBevel(drawable, x + BevelWidth, y + BevelWidth, DWidth - BevelWidth * 2, DHeight - BevelWidth * 2);
    }
    else if(bsInactive == state){
        int BevelWidth = DOuterBevel->Width();
        DColorTileset->DrawTileRectangle(drawable, x, y, DWidth, DHeight, DDarkIndices[pcBlack]);
        DFont->DrawTextWithShadow(drawable, TempGC, x + DTextOffsetX, y + DTextOffsetY, DBlackIndex, DWhiteIndex, 1, DText);
        DOuterBevel->DrawBevel(drawable, x + BevelWidth, y + BevelWidth, DWidth - BevelWidth * 2, DHeight - BevelWidth * 2);
    }
    else{
        int BevelWidth = DOuterBevel->Width();
        DColorTileset->DrawTileRectangle(drawable, x, y, DWidth, DHeight, DLightIndices[DButtonColor]);
        DFont->DrawTextWithShadow(drawable, TempGC, x + DTextOffsetX, y + DTextOffsetY, bsHover == state ? DWhiteIndex : DGoldIndex, DBlackIndex, 1, DText);
        DOuterBevel->DrawBevel(drawable, x + BevelWidth, y + BevelWidth, DWidth - BevelWidth * 2, DHeight - BevelWidth * 2);
    }
    
    g_object_unref(TempGC);
}


