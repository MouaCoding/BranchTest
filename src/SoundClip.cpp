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
#include "SoundClip.h"
#include <cstdint>
#include <vector>
#include <sndfile.h>
#include <cstring>
                
class CSFVirtualIODataSource{
    protected:
      std::vector< uint8_t > DData;
      sf_count_t DOffset;
      
      static sf_count_t GetFileLength(CSFVirtualIODataSource *iosource);
      static sf_count_t Seek(sf_count_t offset, int whence, CSFVirtualIODataSource *iosource);
      static sf_count_t Read(void *ptr, sf_count_t count, CSFVirtualIODataSource *iosource);
      static sf_count_t Write(const void *ptr, sf_count_t count, CSFVirtualIODataSource *iosource);
      static sf_count_t Tell(CSFVirtualIODataSource *iosource);
      
    public:
        CSFVirtualIODataSource(std::shared_ptr< CDataSource > source);
        
        SF_VIRTUAL_IO SFVirtualIO() const;
};                                     

CSFVirtualIODataSource::CSFVirtualIODataSource(std::shared_ptr< CDataSource > source){
    uint8_t TempByte;
    
    while(0 < source->Read(&TempByte, 1)){
        DData.push_back(TempByte);   
    }
    DOffset = 0;
}

SF_VIRTUAL_IO CSFVirtualIODataSource::SFVirtualIO() const{
    SF_VIRTUAL_IO ReturnIO;
    
    ReturnIO.get_filelen = (sf_vio_get_filelen)GetFileLength;   
    ReturnIO.seek = (sf_vio_seek)Seek;
    ReturnIO.read = (sf_vio_read)Read;
    ReturnIO.write = (sf_vio_write)Write;   
    ReturnIO.tell = (sf_vio_tell)Tell;   
    return ReturnIO;
}

sf_count_t CSFVirtualIODataSource::GetFileLength(CSFVirtualIODataSource *iosource){
    return iosource->DData.size();
}

sf_count_t CSFVirtualIODataSource::Seek(sf_count_t offset, int whence, CSFVirtualIODataSource *iosource){
    if(SEEK_SET == whence){
        iosource->DOffset = offset;
    }
    else if(SEEK_CUR == whence){
        iosource->DOffset += offset;
    }
    else if(SEEK_END == whence){
        iosource->DOffset = iosource->DData.size() + offset;        
    }
    if(0 > iosource->DOffset){
        iosource->DOffset = 0;    
    }
    else if(iosource->DData.size() < iosource->DOffset){
        iosource->DOffset = iosource->DData.size();
    }
    return iosource->DOffset;
}

sf_count_t CSFVirtualIODataSource::Read(void *ptr, sf_count_t count, CSFVirtualIODataSource *iosource){
    if(iosource->DOffset + count > iosource->DData.size()){
        count = iosource->DData.size() - iosource->DOffset;
    }
    memcpy(ptr, iosource->DData.data() + iosource->DOffset, count);
    iosource->DOffset += count;
    return count;
}

sf_count_t CSFVirtualIODataSource::Write(const void *ptr, sf_count_t count, CSFVirtualIODataSource *iosource){
    return 0;
}

sf_count_t CSFVirtualIODataSource::Tell(CSFVirtualIODataSource *iosource){
    return iosource->DOffset;
}

CSoundClip::CSoundClip(){
    DData = NULL;
    DChannels = 0;
    DTotalFrames = 0;
    DSampleRate = 0;
}

CSoundClip::CSoundClip(const CSoundClip &clip){
    DChannels = clip.DChannels;
    DTotalFrames = clip.DTotalFrames;
    DSampleRate = clip.DSampleRate;
    DData = NULL;
    if(DChannels * DTotalFrames){
        DData = new float[DChannels * DTotalFrames];
        memcpy(DData, clip.DData, sizeof(float) * DTotalFrames * DChannels);
    }
}

CSoundClip::~CSoundClip(){
    if(NULL != DData){
        delete [] DData;   
    }
}

CSoundClip &CSoundClip::operator=(const CSoundClip &clip){
    if(&clip != this){
        DChannels = clip.DChannels;
        DTotalFrames = clip.DTotalFrames;
        DSampleRate = clip.DSampleRate;
        if(NULL != DData){
            delete [] DData;
            DData = NULL;
        }
        if(DChannels * DTotalFrames){
            DData = new float[DChannels * DTotalFrames];
            memcpy(DData, clip.DData, sizeof(float) * DTotalFrames * DChannels);
        }
    }
    return *this;
}

bool CSoundClip::Load(std::shared_ptr< CDataSource > source){
    CSFVirtualIODataSource VIODataSource(source);
    SF_VIRTUAL_IO SFVirtualIO = VIODataSource.SFVirtualIO();
    SNDFILE *SoundFilePtr;
    SF_INFO SoundFileInfo;
    
    //SoundFilePtr = sf_open(filename.c_str(), SFM_READ, &SoundFileInfo);
    SoundFilePtr = sf_open_virtual(&SFVirtualIO, SFM_READ, &SoundFileInfo, (void *)&VIODataSource);
    if(NULL == SoundFilePtr){
        return false;
    }
    if(1 == SoundFileInfo.channels){
        DChannels = 2;
        DTotalFrames = SoundFileInfo.frames;
        DSampleRate = SoundFileInfo.samplerate;
        DData = new float[SoundFileInfo.frames * 2];
        for(int Frame = 0; Frame < DTotalFrames; Frame++){
            sf_readf_float(SoundFilePtr, DData + Frame * 2, 1);   
            DData[Frame * 2 + 1] = DData[Frame * 2];
        }
    }
    else if(2 == SoundFileInfo.channels){
        DChannels = 2;
        DTotalFrames = SoundFileInfo.frames;
        DSampleRate = SoundFileInfo.samplerate;
        DData = new float[SoundFileInfo.frames * SoundFileInfo.channels];
        sf_readf_float(SoundFilePtr, DData, DTotalFrames);
    }
    else{
        sf_close(SoundFilePtr);
        return false;    
    }
    
    sf_close(SoundFilePtr);
    
    return true;
}
    
void CSoundClip::CopyStereoClip(float *data, int offset, int frames){
    if(offset + frames > DTotalFrames){
        int FramesToCopy = DTotalFrames - offset;
        
        if(0 > FramesToCopy){
            FramesToCopy = 0;
        }
        if(FramesToCopy){
            memcpy(data, DData + (offset * 2), sizeof(float) * FramesToCopy * 2);
        }
        memset(data, 0, sizeof(float) * (frames - FramesToCopy) * 2);
    }
    else{
        memcpy(data, DData + (offset * 2), sizeof(float) * frames * 2);
    }
}

void CSoundClip::MixStereoClip(float *data, int offset, int frames, float volume, float rightbias){
    float *DataPointer = DData + (offset * 2);
    int FramesToMix = frames;
    
    if(offset + frames > DTotalFrames){
        FramesToMix = DTotalFrames - offset;
        if(0 > FramesToMix){
            FramesToMix = 0;
        }
    }
    for(int Frame = 0; Frame < FramesToMix; Frame++){
        *data++ += volume * (1.0 - rightbias) * *DataPointer++;
        *data++ += volume * (1.0 + rightbias) * *DataPointer++;
    }
}

