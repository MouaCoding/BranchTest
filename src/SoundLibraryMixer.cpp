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
#include "SoundLibraryMixer.h"
#include "LineDataSource.h"
#include "Debug.h"
#include <fstream>
#include <math.h>
#include <unistd.h>
#include <cstring>

#define FRAMES_PER_BUFFER       paFramesPerBufferUnspecified
#ifndef M_PI
#define M_PI                    3.141592653589793
#endif
#define TEMP_SFNAME             "runtime.sf2"
CSoundLibraryMixer::CSoundLibraryMixer(){
    DStream = nullptr;
    DSineWave = nullptr;
    DNextClipID = 0;
    DNextToneID = 0;
    DPortAudioInitialized = paNoError == Pa_Initialize();
    //pthread_mutex_init(&DMutex, nullptr);
    for(int Index = 0; Index < 128; Index++){
        DFreeClipIDs.push_back(Index);   
    }
    
    DFluidSettings = nullptr;
    DFluidSynthesizer = nullptr;
    DFluidPlayer = nullptr;
    DFluidAudioDriver = nullptr;
}

CSoundLibraryMixer::~CSoundLibraryMixer(){
    if(nullptr != DStream){
        if(Pa_IsStreamActive(DStream)){
            Pa_StopStream(DStream);
        }
        Pa_CloseStream(DStream);
        DStream = nullptr;
    }
    if(nullptr != DSineWave){
        delete [] DSineWave;    
    }
    if(DPortAudioInitialized){
        Pa_Terminate();
    }
    //pthread_mutex_destroy(&DMutex);
    if(nullptr != DFluidAudioDriver){
        delete_fluid_audio_driver(DFluidAudioDriver);
        //unlink(TEMP_SFNAME);
    }
    if(nullptr != DFluidAudioDriver){
        delete_fluid_player(DFluidPlayer);
    }
    if(nullptr != DFluidAudioDriver){
        delete_fluid_synth(DFluidSynthesizer);
    }
    if(nullptr != DFluidAudioDriver){
        delete_fluid_settings(DFluidSettings);
    }
}

int CSoundLibraryMixer::FindClip(const std::string &clipname) const{
    std::map< std::string, int >::const_iterator Iterator = DMapping.find(clipname);
    if(DMapping.end() != Iterator){
        return Iterator->second;
    }
    return -1;
}

int CSoundLibraryMixer::FindSong(const std::string &songname) const{
    std::map< std::string, int >::const_iterator Iterator = DMusicMapping.find(songname);
    if(DMusicMapping.end() != Iterator){
        return Iterator->second;
    }
    return -1;
}

int CSoundLibraryMixer::ClipDurationMS(int index){
    if((0 > index)||(index >= DSoundClips.size())){
        return 0;    
    }
    return (DSoundClips[index].TotalFrames() * 1000) / DSoundClips[index].SampleRate();
}

int CSoundLibraryMixer::TimestepCallback(const void *in, void *out, unsigned long frames, const PaStreamCallbackTimeInfo* timeinfo, PaStreamCallbackFlags status, void *data){
    CSoundLibraryMixer *Mixer = (CSoundLibraryMixer *)data;
static bool FirstCall = true;
if(FirstCall){
    PrintDebug(DEBUG_LOW, "First TimestepCallback call\n");
    FirstCall = false;   
}
    return Mixer->Timestep(out, frames, timeinfo, status);
}

int CSoundLibraryMixer::Timestep(void *out, unsigned long frames, const PaStreamCallbackTimeInfo* timeinfo, PaStreamCallbackFlags status){
    float *DataPtr = (float *)out;
    
    memset(DataPtr, 0, sizeof(float) * frames * 2);
    //pthread_mutex_lock(&DMutex);
    DMutex.lock();
    for(std::list< SClipStatus >::iterator ClipIterator = DClipsInProgress.begin(); ClipIterator != DClipsInProgress.end(); ){
        bool Advance = true;
        
        DSoundClips[ClipIterator->DIndex].MixStereoClip(DataPtr, ClipIterator->DOffset, frames, ClipIterator->DVolume, ClipIterator->DRightBias);
        
        ClipIterator->DOffset += frames;
        if(ClipIterator->DOffset >= DSoundClips[ClipIterator->DIndex].TotalFrames()){
            DFreeClipIDs.push_back(ClipIterator->DIdentification);   
            ClipIterator = DClipsInProgress.erase(ClipIterator);
            Advance = false;
        }
        if(Advance){
            ClipIterator++;   
        }
    }
    
    for(std::list< SToneStatus >::iterator ToneIterator = DTonesInProgress.begin(); ToneIterator != DTonesInProgress.end(); ){
        bool Advance = true;
        float *CurOutPtr = DataPtr;
        
        for(int Frame = 0; Frame < frames; Frame++){
            int SineIndex = ToneIterator->DCurrentStep;
            
            if(0.5 <= (ToneIterator->DCurrentStep - SineIndex)){
                SineIndex++;
                if(DSampleRate <= SineIndex){
                    SineIndex = 0;   
                }
            }
            *CurOutPtr++ += ToneIterator->DVolume * (1.0 - ToneIterator->DRightBias) * DSineWave[SineIndex];
            *CurOutPtr++ += ToneIterator->DVolume * (1.0 + ToneIterator->DRightBias) * DSineWave[SineIndex];
            ToneIterator->DCurrentStep += ToneIterator->DCurrentFrequency;
            ToneIterator->DCurrentFrequency += ToneIterator->DFrequencyDecay;
            ToneIterator->DVolume += ToneIterator->DVolumeDecay;
            ToneIterator->DRightBias += ToneIterator->DRightShift;
            if(DSampleRate <= ToneIterator->DCurrentStep){
                ToneIterator->DCurrentStep -= DSampleRate;
            }
            if(0.0 > ToneIterator->DVolume){
                Advance = false;
                break;
            }
            if(20.0 > ToneIterator->DCurrentFrequency){
                Advance = false;
                break;
            }
            if(-1.0 > ToneIterator->DRightBias){
                ToneIterator->DRightBias = -1.0;
                ToneIterator->DRightShift = 0.0;
            }
            if(1.0 < ToneIterator->DRightBias){
                ToneIterator->DRightBias = -1.0;
                ToneIterator->DRightShift = 0.0;                
            }
        }        
        if(Advance){
            ToneIterator++;    
        }
        else{
            DFreeToneIDs.push_back(ToneIterator->DIdentification);   
            ToneIterator = DTonesInProgress.erase(ToneIterator);
        }
    }
    
    //pthread_mutex_unlock(&DMutex);    
    DMutex.unlock();
    frames *= 2;
    for(int Frame = 0; Frame < frames; Frame++){
        if(-1.0 > *DataPtr){
            *DataPtr = -1.0;
        }
        if(1.0 < *DataPtr){
            *DataPtr = 1.0;
        }
        DataPtr++;
    }
    
    return paContinue;
}

bool CSoundLibraryMixer::LoadLibrary(std::shared_ptr< CDataSource > source){
    CLineDataSource LineSource(source);
    int TotalClips, TotalSongs;
    bool ReturnStatus = false;
    std::string TempString;
    std::shared_ptr< CDataSource >  SFInput;
    std::ofstream SFOutput;
    char Buffer[1024];
    int BytesRead;
    
    if(!LineSource.Read(TempString)){
        PrintError("Failed to read clip count.\n");
        goto LoadLibraryExit;
    }
    try{
        TotalClips = std::stoi(TempString);
    }
    catch(std::exception &E){
        PrintError("%s\n",E.what());
        goto LoadLibraryExit;
    }
        
    DSoundClips.resize(TotalClips);
    for(int Index = 0; Index < TotalClips; Index++){
        if(!LineSource.Read(TempString)){
            PrintError("Failed to read clip name %d.\n", Index);
            goto LoadLibraryExit;
        }
        DMapping[TempString] = Index;
        if(!LineSource.Read(TempString)){
            PrintError("Failed to read clip path %d.\n", Index);
            goto LoadLibraryExit;
        }
        if(!DSoundClips[Index].Load(source->Container()->DataSource(TempString))){
            PrintError("Failed to load sound clip %d.\n", Index);
            goto LoadLibraryExit;    
        }
    }
    DSampleRate = DSoundClips[0].SampleRate();
    for(int Index = 1; Index < TotalClips; Index++){
        if(DSoundClips[0].SampleRate() != DSoundClips[Index].SampleRate()){
            PrintError("Incompatible sample rate %dHz of clip %d.\n", DSoundClips[Index].SampleRate(), Index);
            goto LoadLibraryExit;    
        }
    }
    if(NULL != DSineWave){
        delete [] DSineWave;    
    }
    DSineWave = new float[DSampleRate];
    for(int Index = 0; Index < DSampleRate; Index++){
        DSineWave[Index] = sin( ((double)Index/(double)DSampleRate) * M_PI * 2.0 );
    }
    if(!LineSource.Read(TempString)){
        PrintError("Failed to read sound font name!\n");
        goto LoadLibraryExit;
    }
    if(!fluid_is_soundfont(TEMP_SFNAME)){
        PrintDebug(DEBUG_LOW, "Copying sound font %s to %s\n",TempString.c_str(), TEMP_SFNAME);
        SFInput = source->Container()->DataSource(TempString);
        SFOutput.open(TEMP_SFNAME, std::ofstream::binary);    
        while(0 < (BytesRead = SFInput->Read(Buffer, sizeof(Buffer)))){
            SFOutput.write(Buffer, BytesRead);
        }
        SFOutput.close();
        PrintDebug(DEBUG_LOW, "Copying complete.\nChecking if valid sound font.\n");
    }
    if(!fluid_is_soundfont(TEMP_SFNAME)){
        PrintError("%s not a sound font!\n",TempString.c_str());
        goto LoadLibraryExit;
    }
    if(!LineSource.Read(TempString)){
        PrintError("Failed to read line\n");
        goto LoadLibraryExit;
    }
    try{
        TotalSongs = std::stoi(TempString);
    }
    catch(std::exception &E){
        PrintError("%s\n",E.what());
        goto LoadLibraryExit;
    }
    DMusicFilenames.resize(TotalSongs);
    for(int Index = 0; Index < TotalSongs; Index++){
        auto TempData = std::make_shared< std::vector< char > > ();
        
        if(!LineSource.Read(TempString)){
            PrintError("Failed to read song name %d\n", Index);
            goto LoadLibraryExit;
        }    
        DMusicMapping[TempString] = Index;
        if(!LineSource.Read(TempString)){
            PrintError("Failed to read song path %d\n", Index);
            goto LoadLibraryExit;
        }   
        auto MidiInput = source->Container()->DataSource(TempString);
        while(0 < (BytesRead = MidiInput->Read(Buffer, sizeof(Buffer)))){
            TempData->insert(TempData->end(), Buffer, Buffer + BytesRead);
        }
        DMusicData.push_back(TempData);
        /*
        if(!fluid_is_midifile(TempString.c_str())){
            PrintError("%s is not a midi file!\n", TempString.c_str());
            goto LoadLibraryExit;    
        }
        */
        DMusicFilenames[Index] = TempString;
    }
    
    DFluidSettings = new_fluid_settings();
    fluid_settings_setstr(DFluidSettings, "audio.driver", "portaudio");
    DFluidSynthesizer = new_fluid_synth(DFluidSettings);
    DFluidPlayer = new_fluid_player(DFluidSynthesizer);
    DFluidAudioDriver = new_fluid_audio_driver(DFluidSettings, DFluidSynthesizer);
    PrintDebug(DEBUG_LOW, "Loading sound font %s.\n", TEMP_SFNAME);
    if(FLUID_FAILED == fluid_synth_sfload(DFluidSynthesizer, TEMP_SFNAME, 1)){
        PrintError("Failed to load sound font %s\n", TEMP_SFNAME);
        goto LoadLibraryExit;
    }
    PrintDebug(DEBUG_LOW, "Sound font %s loaded.\n", TEMP_SFNAME);
    if(!DPortAudioInitialized){
        PaError PortAudioResult;
        PrintDebug(DEBUG_LOW, "Port Audio not initialized, try to reinitialize.\n");
        PortAudioResult = Pa_Initialize();    
        DPortAudioInitialized = paNoError == PortAudioResult;
        if(DPortAudioInitialized){
            PrintDebug(DEBUG_LOW, "Port Audio initialized.\n");
        }
        else{
            PrintDebug(DEBUG_LOW, "Port Audio failed to initialize with code %d.\n", PortAudioResult);
        }
    }
    PrintDebug(DEBUG_LOW, "Opening stream at %dHz with %d frames per buffer\n",DSampleRate, FRAMES_PER_BUFFER);
    if(paNoError != Pa_OpenDefaultStream(&DStream, 0, 2, paFloat32, DSampleRate, FRAMES_PER_BUFFER, TimestepCallback, this)){
        PrintError("Failed to open default sound stream\n");
        goto LoadLibraryExit;
    }
    PrintDebug(DEBUG_LOW, "Default sound stream opened.\n");
    if(paNoError != Pa_StartStream(DStream)){
        PrintError("Failed to start default sound stream\n");
        goto LoadLibraryExit;
    }
    PrintDebug(DEBUG_LOW, "Sound stream started.\n");
    return true;
    
LoadLibraryExit:
    DMusicFilenames.clear();
    DSoundClips.clear();
    DMapping.clear();

    return ReturnStatus;
}

int CSoundLibraryMixer::PlayClip(int index, float volume, float rightbias){
    SClipStatus TempClipStatus;
    
    if((0 > index)||(DSoundClips.size() <= index)){
        PrintError("Invalid Clip %d!!!!!\n",index);
        return -1;
    }
    TempClipStatus.DIndex = index;
    TempClipStatus.DOffset = 0;
    TempClipStatus.DVolume = volume;
    TempClipStatus.DRightBias = rightbias;
    
    //pthread_mutex_lock(&DMutex);
    DMutex.lock();
    if(DFreeClipIDs.size()){
        TempClipStatus.DIdentification = DFreeClipIDs.front();
        DFreeClipIDs.pop_front();
    }
    else{
        TempClipStatus.DIdentification = DNextClipID++;    
    }
    DClipsInProgress.push_back(TempClipStatus);
    //pthread_mutex_unlock(&DMutex);  
    DMutex.unlock();
    return TempClipStatus.DIdentification;
}

int CSoundLibraryMixer::PlayTone(float freq, float freqdecay, float volume, float volumedecay, float rightbias, float rightshift){
    SToneStatus TempToneStatus;
    

    TempToneStatus.DCurrentFrequency = freq;
    TempToneStatus.DCurrentStep = 0;
    TempToneStatus.DFrequencyDecay = freqdecay / DSampleRate;
    TempToneStatus.DVolume = volume;
    TempToneStatus.DVolumeDecay = volumedecay / DSampleRate;
    TempToneStatus.DRightBias = rightbias;
    TempToneStatus.DRightShift = rightshift / DSampleRate;
    
    //pthread_mutex_lock(&DMutex);
    DMutex.lock();
    if(DFreeToneIDs.size()){
        TempToneStatus.DIdentification = DFreeToneIDs.front();
        DFreeToneIDs.pop_front();
    }
    else{
        TempToneStatus.DIdentification = DNextToneID++;    
    }
    DTonesInProgress.push_back(TempToneStatus);
    //pthread_mutex_unlock(&DMutex);  
    DMutex.unlock();
    return TempToneStatus.DIdentification;
}

void CSoundLibraryMixer::StopTone(int id){
    std::list< SToneStatus >::iterator ToneIterator;
    
    //pthread_mutex_lock(&DMutex);
    DMutex.lock();
    ToneIterator = DTonesInProgress.begin();
    while(ToneIterator != DTonesInProgress.end()){
        if(ToneIterator->DIdentification == id){
            DTonesInProgress.erase(ToneIterator);
            DFreeToneIDs.push_back(id);
            break;
        }
        ToneIterator++;
    }
    //pthread_mutex_unlock(&DMutex);  
    DMutex.unlock();
}

bool CSoundLibraryMixer::ClipCompleted(int id){
    std::list< int >::iterator ClipIDIterator;
    bool FoundID = false;
    
    //pthread_mutex_lock(&DMutex);
    DMutex.lock();
    ClipIDIterator = DFreeClipIDs.begin();
    while(ClipIDIterator != DFreeClipIDs.end()){
        if(*ClipIDIterator == id){
            FoundID = true;
            break;
        }
        ClipIDIterator++;
    }
    //pthread_mutex_unlock(&DMutex); 
    DMutex.unlock();
    return FoundID;
}

void CSoundLibraryMixer::PlaySong(int index, float volume){
    if((0 > index)||(index >= DMusicData.size())){
        return;   
    }
    switch(fluid_player_get_status(DFluidPlayer)){
        case FLUID_PLAYER_PLAYING:  fluid_player_stop(DFluidPlayer);
        case FLUID_PLAYER_READY:    
        default:                    delete_fluid_player(DFluidPlayer);
                                    DFluidPlayer = new_fluid_player(DFluidSynthesizer);
                                    break;
    }
    fluid_synth_set_gain(DFluidSynthesizer, 0.2 * volume);
    //fluid_player_add(DFluidPlayer,DMusicFilenames[index].c_str());
    fluid_player_add_mem(DFluidPlayer, DMusicData[index]->data(), DMusicData[index]->size());
    fluid_player_set_loop(DFluidPlayer, -1);
    fluid_player_play(DFluidPlayer);
}

void CSoundLibraryMixer::StopSong(){
    fluid_player_stop(DFluidPlayer);
    fluid_synth_set_gain(DFluidSynthesizer, 0.0);
}

void CSoundLibraryMixer::SongVolume(float volume){
    fluid_synth_set_gain(DFluidSynthesizer, 0.2 * volume);
}

