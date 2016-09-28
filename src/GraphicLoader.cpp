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
#include "GraphicLoader.h"
#include <vector>

GdkPixbuf *CGraphicLoader::LoadPixbuf(std::shared_ptr< CDataSource > source){
    GInputStream *InputStream = nullptr;
    GdkPixbuf *ReturnPixbuf = nullptr;
    std::vector< uint8_t > Buffer;
    int BytesToRead = 512;
    int BytesRead;
    
    do{
        Buffer.resize(Buffer.size() + BytesToRead);
        BytesRead = source->Read(Buffer.data() + (Buffer.size() - BytesToRead), BytesToRead);
        if(BytesRead < BytesToRead){
            if(0 < BytesRead){
                Buffer.resize(Buffer.size() - (BytesToRead - BytesRead));
            }
        }
    }while(BytesToRead == BytesRead);
    
    InputStream = g_memory_input_stream_new_from_data(Buffer.data(), Buffer.size(), nullptr);
    if(nullptr == InputStream){
        return nullptr;   
    }
    ReturnPixbuf = gdk_pixbuf_new_from_stream(InputStream, nullptr, nullptr); 
    
    g_object_unref(G_OBJECT(InputStream));
    return ReturnPixbuf;
}
