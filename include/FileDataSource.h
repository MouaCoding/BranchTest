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
#ifndef FILEDATASOURCE_H
#define FILEDATASOURCE_H

#include "DataSource.h"
#include <dirent.h>

class CFileDataSource : public CDataSource{
    protected:
        int DFileHandle;
        std::string DFullPath;
        
    public:
        CFileDataSource(const std::string &filename);
        ~CFileDataSource();
        
        int Read(void *data, int length);
        std::shared_ptr< CDataSourceContainer > Container();
};

class CDirectoryDataSourceContainerIterator : public CDataSourceContainerIterator{
    friend class CDirectoryDataSourceContainer;
    
    protected:
        CDirectoryDataSourceContainerIterator(const std::string &path);
        ~CDirectoryDataSourceContainerIterator();
        DIR *DDirectory;
        struct dirent DEntry;
        struct dirent *DEntryResult;
        
    public:  
        std::string Name();
        bool IsContainer();
        bool IsValid();
        void Next();
};

class CDirectoryDataSourceContainer : public CDataSourceContainer{
    protected:
        std::string DFullPath;
        
    public:
        CDirectoryDataSourceContainer(const std::string &path);
        ~CDirectoryDataSourceContainer();
        
        std::shared_ptr< CDataSourceContainerIterator > First();
        std::shared_ptr< CDataSource > DataSource(const std::string &name);
        std::shared_ptr< CDataSourceContainer > Container();
        std::shared_ptr< CDataSourceContainer > DataSourceContainer(const std::string &name);
};

#endif
