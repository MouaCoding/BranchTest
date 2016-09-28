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
#include "GraphicRecolorMap.h"
#include "GraphicLoader.h"
        
CGraphicRecolorMap::CGraphicRecolorMap(){
    
}

CGraphicRecolorMap::~CGraphicRecolorMap(){
    
}


bool CGraphicRecolorMap::Load(std::shared_ptr< CDataSource > source){
    GdkPixbuf *ColorPixbuf;
    bool ReturnStatus = false;
    int NumberChannels, RowStride;
    
    if(nullptr == source){
        return false;   
    }

    ColorPixbuf = CGraphicLoader::LoadPixbuf(source);
    if(nullptr == ColorPixbuf){
        printf("Failed to load file colormap.\n");
        return false;
    }
    NumberChannels = gdk_pixbuf_get_n_channels(ColorPixbuf);
    RowStride = gdk_pixbuf_get_rowstride(ColorPixbuf);
    if(GDK_COLORSPACE_RGB != gdk_pixbuf_get_colorspace(ColorPixbuf)){
        printf("Colormap not RGB.\n");
        goto LoadExit;
    }
    if(8 != gdk_pixbuf_get_bits_per_sample(ColorPixbuf)){
        printf("Colormap not 8-bit samples.\n");
        goto LoadExit;
    }
    DColors.resize(gdk_pixbuf_get_height(ColorPixbuf));
    for(size_t Index = 0; Index < DColors.size(); Index++){
        for(int XPos = 0; XPos < gdk_pixbuf_get_width(ColorPixbuf); XPos++){
            guchar *Pixel = gdk_pixbuf_get_pixels(ColorPixbuf) + RowStride * Index + XPos * NumberChannels;
            SRGBColor TempColor;
            
            TempColor.DRed = Pixel[0];
            TempColor.DGreen = Pixel[1];
            TempColor.DBlue = Pixel[2];
            DColors[Index].push_back(TempColor);
        }   
    }
    ReturnStatus = true;
LoadExit:
    g_object_unref(G_OBJECT(ColorPixbuf));
    return ReturnStatus;
}


bool CGraphicRecolorMap::Recolor(int index, guchar &red, guchar &green, guchar &blue){
    if((0 <= index) && (index < DColors.size())){
        for(int ColIndex = 0; ColIndex < DColors[0].size(); ColIndex++){
            if((DColors[0][ColIndex].DRed == red)&&(DColors[0][ColIndex].DGreen == green)&&(DColors[0][ColIndex].DBlue == blue)){
                red = DColors[index][ColIndex].DRed;
                green = DColors[index][ColIndex].DGreen;
                blue = DColors[index][ColIndex].DBlue;
                return true;
            }
        }
    }
    return false;
}

