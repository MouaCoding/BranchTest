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
#include "FileDataSource.h"
#include "Path.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

CFileDataSource::CFileDataSource(const std::string &filename) : CDataSource(){
    DFullPath = CPath::CurrentPath().Simplify(filename).ToString();
    //printf("CFileDataSource::CFileDataSource(\"%s\");\n",DFullPath.c_str());
    if(DFullPath.length()){
        DFileHandle = open(DFullPath.c_str(), O_RDONLY);
    }
}

CFileDataSource::~CFileDataSource(){
    if(0 <= DFileHandle){
        close(DFileHandle);   
    }
}

int CFileDataSource::Read(void *data, int length){
    if(0 <= DFileHandle){
        int BytesRead = read(DFileHandle, data, length);
        
        if(0 < BytesRead){
            return BytesRead;   
        }
    }
    return -1;
}

std::shared_ptr< CDataSourceContainer > CFileDataSource::Container(){
    std::string ContainerName = CPath(DFullPath).Containing().ToString();
    
    if(ContainerName.length()){
        return (std::shared_ptr< CDataSourceContainer >)(CDataSourceContainer *) new CDirectoryDataSourceContainer(ContainerName);
    }
    return nullptr;
}

CDirectoryDataSourceContainerIterator::CDirectoryDataSourceContainerIterator(const std::string &path) : CDataSourceContainerIterator(){
    //printf("CDirectoryDataSourceContainerIterator::CDirectoryDataSourceContainerIterator(\"%s\")\n",path.c_str());
    DDirectory = opendir(path.c_str());
    if(nullptr != DDirectory){
        if(0 != readdir_r(DDirectory, &DEntry, &DEntryResult)){
            DEntry.d_name[0] = '\0';
            DEntryResult = nullptr;
        }
        else{
            //printf("Name = %s\n",DEntry.d_name);   
        }
    }
    else{
        //printf("nullptr != DDirectory\n");
    }
}

CDirectoryDataSourceContainerIterator::~CDirectoryDataSourceContainerIterator(){
    if(nullptr != DDirectory){
        closedir(DDirectory);
    }
}

std::string CDirectoryDataSourceContainerIterator::Name(){
    return DEntry.d_name;
}

bool CDirectoryDataSourceContainerIterator::IsContainer(){
    return DT_DIR == DEntry.d_type;
}

bool CDirectoryDataSourceContainerIterator::IsValid(){
    return (nullptr != DEntryResult) || ('\0' != DEntry.d_name[0]);
}

void CDirectoryDataSourceContainerIterator::Next(){
    if(nullptr != DDirectory){
        if(nullptr == DEntryResult){
            DEntry.d_name[0] = '\0';   
        }
        else{
            if(0 != readdir_r(DDirectory, &DEntry, &DEntryResult)){
                DEntry.d_name[0] = '\0';
                DEntryResult = nullptr;
            }
        }
    }
}

CDirectoryDataSourceContainer::CDirectoryDataSourceContainer(const std::string &path) : CDataSourceContainer(){
    DFullPath = CPath::CurrentPath().Simplify(path).ToString();
    //printf("CDirectoryDataSourceContainer::CDirectoryDataSourceContainer(\"%s\");\n",DFullPath.c_str());
}

CDirectoryDataSourceContainer::~CDirectoryDataSourceContainer(){
    
}


std::shared_ptr< CDataSourceContainerIterator > CDirectoryDataSourceContainer::First(){
    if(DFullPath.length()){
        return (std::shared_ptr< CDataSourceContainerIterator >)(CDataSourceContainerIterator *)new CDirectoryDataSourceContainerIterator(DFullPath);
    }
    return nullptr;
}

std::shared_ptr< CDataSource > CDirectoryDataSourceContainer::DataSource(const std::string &name){
    std::string FileName = CPath(DFullPath).Simplify(CPath(name)).ToString();
    
    if(FileName.length()){
        return (std::shared_ptr< CDataSource >)(CDataSource *) new CFileDataSource(FileName);
    }
    return nullptr;
}

std::shared_ptr< CDataSourceContainer > CDirectoryDataSourceContainer::Container(){
    std::string ContainerName = CPath(DFullPath).Containing().ToString();
    
    if(ContainerName.length()){
        return (std::shared_ptr< CDataSourceContainer >)(CDataSourceContainer *) new CDirectoryDataSourceContainer(ContainerName);
    }
    return nullptr;
}

std::shared_ptr< CDataSourceContainer > CDirectoryDataSourceContainer::DataSourceContainer(const std::string &name){
    std::string ContainerName = CPath(DFullPath).Simplify(CPath(name)).ToString();
    
    if(ContainerName.length()){
        return (std::shared_ptr< CDataSourceContainer >)(CDataSourceContainer *) new CDirectoryDataSourceContainer(ContainerName);
    }
    return nullptr;
}


