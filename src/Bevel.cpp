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
#include "Bevel.h"

CBevel::CBevel(std::shared_ptr< CGraphicTileset > tileset){
    DTileset = tileset;
    DWidth = tileset->TileWidth();
    DTopIndices.resize(DWidth);
    DTopIndices[0] = DTileset->FindTile("tf");
    for(int Index = 1; Index < DWidth; Index++){
        DTopIndices[Index] = DTileset->FindTile(std::string("t") + std::to_string(Index));
    }
    
    DBottomIndices.resize(DWidth);
    DBottomIndices[0] = DTileset->FindTile("bf");
    for(int Index = 1; Index < DWidth; Index++){
        DBottomIndices[Index] = DTileset->FindTile(std::string("b") + std::to_string(Index));
    }
    
    DLeftIndices.resize(DWidth);
    DLeftIndices[0] = DTileset->FindTile("lf");
    for(int Index = 1; Index < DWidth; Index++){
        DLeftIndices[Index] = DTileset->FindTile(std::string("l") + std::to_string(Index));
    }
    
    DRightIndices.resize(DWidth);
    DRightIndices[0] = DTileset->FindTile("rf");
    for(int Index = 1; Index < DWidth; Index++){
        DRightIndices[Index] = DTileset->FindTile(std::string("r") + std::to_string(Index));
    }
    
    DCornerIndices.resize(4);
    DCornerIndices[0] = DTileset->FindTile("tl");
    DCornerIndices[1] = DTileset->FindTile("tr");
    DCornerIndices[2] = DTileset->FindTile("bl");
    DCornerIndices[3] = DTileset->FindTile("br");
}

CBevel::~CBevel(){
    
}

void CBevel::DrawBevel(GdkDrawable *drawable, gint xpos, gint ypos, gint width, gint height){
    GdkGC *TempGC = gdk_gc_new(drawable);
    int TopY = ypos - DWidth;
    int BottomY = ypos + height;
    int LeftX = xpos - DWidth;
    int RightX = xpos + width;
    
    DTileset->DrawTile(drawable, TempGC, LeftX, TopY, DCornerIndices[0]);
    DTileset->DrawTile(drawable, TempGC, RightX, TopY, DCornerIndices[1]);
    DTileset->DrawTile(drawable, TempGC, LeftX, BottomY, DCornerIndices[2]);
    DTileset->DrawTile(drawable, TempGC, RightX, BottomY, DCornerIndices[3]);
    
    for(int XOff = 0; XOff < width; XOff += DWidth){
        int Index = 0;
        if(XOff + DWidth > width){
            Index = width - XOff;
        }
        DTileset->DrawTile(drawable, TempGC, xpos + XOff, TopY, DTopIndices[Index]);
        DTileset->DrawTile(drawable, TempGC, xpos + XOff, BottomY, DBottomIndices[Index]);
    }
    for(int YOff = 0; YOff < height; YOff += DWidth){
        int Index = 0;
        if(YOff + DWidth > height){
            Index = height - YOff;
        }
        DTileset->DrawTile(drawable, TempGC, LeftX, ypos + YOff, DLeftIndices[Index]);
        DTileset->DrawTile(drawable, TempGC, RightX, ypos + YOff, DRightIndices[Index]);
    }
    g_object_unref(TempGC);   
}

