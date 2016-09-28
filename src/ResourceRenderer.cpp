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
#include "ResourceRenderer.h"
#include "TextFormatter.h"
#include "Debug.h"

typedef enum{
    mitGold = 0,
    mitLumber,
    mitFood,
    mitMax
} EMiniIconTypes, *Ref;

        
CResourceRenderer::CResourceRenderer(std::shared_ptr< CGraphicTileset > icons, std::shared_ptr< CFontTileset > font, std::shared_ptr< CPlayerData > player){
    gint Width;
    
    DIconTileset = icons;
    DFont = font;
    DPlayer = player;
    DForegroundColor = DFont->FindPixel("white");
    DBackgroundColor = DFont->FindPixel("black");
    DInsufficientColor = DFont->FindPixel("red");
    DLastGoldDisplay = 0;
    DLastLumberDisplay = 0;
    
    DIconIndices.resize(mitMax);
    DIconIndices[mitGold] = DIconTileset->FindTile("gold");
    DIconIndices[mitLumber] = DIconTileset->FindTile("lumber");
    DIconIndices[mitFood] = DIconTileset->FindTile("food");
    DFont->MeasureText("0123456789", Width, DTextHeight);
}

CResourceRenderer::~CResourceRenderer(){

}

void CResourceRenderer::DrawResources(GdkDrawable *drawable, GdkGC *gc){
    gint Width, Height;
    int TextYOffset, ImageYOffset;
    int WidthSeparation, XOffset;
    int DeltaGold = DPlayer->Gold() - DLastGoldDisplay;
    int DeltaLumber = DPlayer->Lumber() - DLastLumberDisplay;
    
    DeltaGold /= 5;
    if((-3 < DeltaGold) && (3 > DeltaGold)){
        DLastGoldDisplay = DPlayer->Gold();
    }
    else{
        DLastGoldDisplay += DeltaGold; 
    }
    DeltaLumber /= 5;
    if((-3 < DeltaLumber) && (3 > DeltaLumber)){
        DLastLumberDisplay = DPlayer->Lumber();
    }
    else{
        DLastLumberDisplay += DeltaLumber; 
    }    
    gdk_pixmap_get_size(drawable, &Width, &Height); 
    TextYOffset = Height/2 - DTextHeight/2;
    ImageYOffset = Height/2 - DIconTileset->TileHeight()/2;
    WidthSeparation = Width/4;
    XOffset = Width / 8;
    
    DIconTileset->DrawTile(drawable, gc, XOffset, ImageYOffset, DIconIndices[mitGold]);
    DFont->DrawTextWithShadow(drawable, gc, XOffset +  DIconTileset->TileWidth(), TextYOffset, DForegroundColor, DBackgroundColor, 1, std::string(" ") + CTextFormatter::IntegerToPrettyString(DLastGoldDisplay));
    XOffset += WidthSeparation;
    
    DIconTileset->DrawTile(drawable, gc, XOffset, ImageYOffset, DIconIndices[mitLumber]);
    DFont->DrawTextWithShadow(drawable, gc, XOffset +  DIconTileset->TileWidth(), TextYOffset, DForegroundColor, DBackgroundColor, 1, std::string(" ") + CTextFormatter::IntegerToPrettyString(DLastLumberDisplay));
    XOffset += WidthSeparation;
    
    DIconTileset->DrawTile(drawable, gc, XOffset, ImageYOffset, DIconIndices[mitFood]);
    
    if(DPlayer->FoodConsumption() > DPlayer->FoodProduction()){
        int SecondTextWidth, TotalTextWidth, TextHeight;
        //DFont->MeasureText( std::string(" ") + std::to_string(DPlayer->FoodConsumption()), FirstTextWidth, TextHeight);
        DFont->MeasureText( std::string(" / ") + std::to_string(DPlayer->FoodProduction()), SecondTextWidth, TextHeight);
        DFont->MeasureText( std::string(" ") + std::to_string(DPlayer->FoodConsumption()) + std::string(" / ") + std::to_string(DPlayer->FoodProduction()), TotalTextWidth, TextHeight);
        DFont->DrawTextWithShadow(drawable, gc, XOffset +  DIconTileset->TileWidth(), TextYOffset, DInsufficientColor, DBackgroundColor, 1, std::string(" ") + std::to_string(DPlayer->FoodConsumption()));
        DFont->DrawTextWithShadow(drawable, gc, XOffset +  DIconTileset->TileWidth() + TotalTextWidth - SecondTextWidth, TextYOffset, DForegroundColor, DBackgroundColor, 1, std::string(" / ") + std::to_string(DPlayer->FoodProduction()));
    }
    else{
        DFont->DrawTextWithShadow(drawable, gc, XOffset +  DIconTileset->TileWidth(), TextYOffset, DForegroundColor, DBackgroundColor, 1, std::string(" ") + std::to_string(DPlayer->FoodConsumption()) + std::string(" / ") + std::to_string(DPlayer->FoodProduction()));   
    }
}

