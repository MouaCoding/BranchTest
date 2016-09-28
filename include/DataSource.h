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
#ifndef DATASOURCE_H
#define DATASOURCE_H

#include <string>
#include <memory>

class CDataSourceContainer;

class CDataSource{
    public:
        virtual int Read(void *data, int length) = 0;
        virtual std::shared_ptr< CDataSourceContainer > Container(){
            return nullptr;
        };
};

class CDataSourceContainerIterator{
    public:  
        virtual std::string Name() = 0;
        virtual bool IsContainer() = 0;
        virtual bool IsValid() = 0;
        virtual void Next() = 0;
};

class CDataSourceContainer{
    public:
        virtual std::shared_ptr< CDataSourceContainerIterator > First() = 0;
        virtual std::shared_ptr< CDataSource > DataSource(const std::string &name) = 0;
        virtual std::shared_ptr< CDataSourceContainer > Container(){
            return nullptr;
        };
        virtual std::shared_ptr< CDataSourceContainer > DataSourceContainer(const std::string &name) = 0;
};

#endif
