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
#include "PixelType.h"

GdkPixbuf *CPixelType::DPixbufTranslater = nullptr;

CPixelType::CPixelType(int red, int green, int blue){
    DColor = (EPlayerColor)red;
    DType = (EAssetTerrainType)green;
}

CPixelType::CPixelType(CTerrainMap::ETileType type){
    DColor = pcNone;
    switch(type){
        case CTerrainMap::ttGrass:          DType = attGrass;
                                            break;
        case CTerrainMap::ttDirt:           DType = attDirt;
                                            break;
        case CTerrainMap::ttRock:           DType = attRock;
                                            break;
        case CTerrainMap::ttTree:           DType = attTree;
                                            break;
        case CTerrainMap::ttStump:          DType = attStump;
                                            break;
        case CTerrainMap::ttWater:          DType = attWater;
                                            break;
        case CTerrainMap::ttWall:           DType = attWall;
                                            break;
        case CTerrainMap::ttWallDamaged:    DType = attWallDamaged;
                                            break;
        case CTerrainMap::ttRubble:         DType = attRubble;
                                            break;
        default:                            DType = attNone;
                                            break;
    }
}

CPixelType::CPixelType(const CPlayerAsset &asset){
    switch(asset.Type()){
        case atPeasant:         DType = attPeasant;
                                break;
        case atFootman:         DType = attFootman;
                                break;
        case atArcher:          DType = attArcher;
                                break;
        case atRanger:          DType = attRanger;
                                break;
        case atGoldMine:        DType = attGoldMine;
                                break;
        case atTownHall:        DType = attTownHall;
                                break;
        case atKeep:            DType = attKeep;
                                break;
        case atCastle:          DType = attCastle;
                                break;
        case atFarm:            DType = attFarm;
                                break;
        case atBarracks:        DType = attBarracks;
                                break;
        case atLumberMill:      DType = attLumberMill;
                                break;
        case atBlacksmith:      DType = attBlacksmith;
                                break;
        case atScoutTower:      DType = attScoutTower;
                                break;
        case atGuardTower:      DType = attGuardTower;
                                break;
        case atCannonTower:     DType = attCannonTower;
                                break;
        default:                DType = attNone;
                                break;
    }
    DColor = asset.Color();
}

CPixelType::CPixelType(const CPixelType &pixeltype){
    DType = pixeltype.DType;
    DColor = pixeltype.DColor;
}

CPixelType &CPixelType::operator=(const CPixelType &pixeltype){
    if(this != &pixeltype){
        DType = pixeltype.DType;
        DColor = pixeltype.DColor;   
    }
    return *this;
}
 

guint32 CPixelType::ToPixelColor() const{
    guint32 RetVal = DColor;
    
    RetVal <<= 16;
    RetVal |= ((guint32)DType)<<8;
    return RetVal;
}

EAssetType CPixelType::AssetType() const{
    switch(DType){
        case attPeasant:        return atPeasant;
        case attFootman:        return atFootman;
        case attArcher:         return atArcher;
        case attRanger:         return atRanger;
        case attGoldMine:       return atGoldMine;
        case attTownHall:       return atTownHall;
        case attKeep:           return atKeep;
        case attCastle:         return atCastle;
        case attFarm:           return atFarm;
        case attBarracks:       return atBarracks;
        case attLumberMill:     return atLumberMill;
        case attBlacksmith:     return atBlacksmith;
        case attScoutTower:     return atScoutTower;
        case attGuardTower:     return atGuardTower;
        case attCannonTower:    return atCannonTower;                          
        default:                return atNone;
    }
}

CPixelType CPixelType::GetPixelType(GdkDrawable *drawable, const CPosition &pos){
    return GetPixelType(drawable, pos.X(), pos.Y());
}

CPixelType CPixelType::GetPixelType(GdkDrawable *drawable, gint xpos, gint ypos){
    guchar *Pixel;
    
    DPixbufTranslater = gdk_pixbuf_get_from_drawable(DPixbufTranslater, drawable, nullptr, xpos, ypos, 0, 0, 1, 1);
    Pixel = gdk_pixbuf_get_pixels(DPixbufTranslater);
    
    return CPixelType(Pixel[0], Pixel[1], Pixel[2]);
}

