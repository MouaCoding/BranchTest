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
#include "AssetDecoratedMap.h"
#include "LineDataSource.h"
#include "Tokenizer.h"
#include "Debug.h"
#include <queue>

#ifndef MIN
    #define MIN(a, b)               ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
    #define MAX(a, b)               ((a) > (b) ? (a) : (b))
#endif

std::map< std::string, int > CAssetDecoratedMap::DMapNameTranslation;
std::vector< std::shared_ptr< CAssetDecoratedMap > > CAssetDecoratedMap::DAllMaps;

CAssetDecoratedMap::CAssetDecoratedMap() : CTerrainMap(){
    
}

CAssetDecoratedMap::CAssetDecoratedMap(const CAssetDecoratedMap &map) : CTerrainMap(map){
    DAssets = map.DAssets;
    DAssetInitializationList = map.DAssetInitializationList;
    DResourceInitializationList = map.DResourceInitializationList;
}

CAssetDecoratedMap::CAssetDecoratedMap(const CAssetDecoratedMap &map, EPlayerColor newcolors[pcMax]) : CTerrainMap(map){
    DAssets = map.DAssets;

    for(auto &InitVal : map.DAssetInitializationList){
        auto NewInitVal = InitVal;
        NewInitVal.DColor = newcolors[NewInitVal.DColor];
        DAssetInitializationList.push_back(NewInitVal);
    }
    for(auto &InitVal : map.DResourceInitializationList){
        auto NewInitVal = InitVal;
        NewInitVal.DColor = newcolors[NewInitVal.DColor];
        DResourceInitializationList.push_back(NewInitVal);
    }
}

CAssetDecoratedMap::~CAssetDecoratedMap(){
    
}

CAssetDecoratedMap &CAssetDecoratedMap::operator=(const CAssetDecoratedMap &map){
    if(this != &map){
        CTerrainMap::operator=(map);
        DAssets = map.DAssets;
        DAssetInitializationList = map.DAssetInitializationList;
        DResourceInitializationList = map.DResourceInitializationList;
    }
    return *this;
}

bool CAssetDecoratedMap::LoadMaps(std::shared_ptr< CDataSourceContainer > container){
    auto FileIterator = container->First();
    if(FileIterator == nullptr){
        PrintError("FileIterator == nullptr\n");
        return false;
    }
    while((FileIterator != nullptr)&&(FileIterator->IsValid())){
        std::string Filename = FileIterator->Name();
        FileIterator->Next();
        if(Filename.rfind(".map") == (Filename.length() - 4)){
            std::shared_ptr< CAssetDecoratedMap > TempMap = std::make_shared< CAssetDecoratedMap >();

            if(!TempMap->LoadMap(container->DataSource(Filename))){
                PrintError("Failed to load map \"%s\".\n",Filename.c_str());
                continue;
            }
            else{
                PrintDebug(DEBUG_LOW,"Loaded map \"%s\".\n",Filename.c_str());
            }
            DMapNameTranslation[TempMap->MapName()] = DAllMaps.size();
            DAllMaps.push_back(TempMap);
        }
    }
    PrintDebug(DEBUG_LOW, "Maps loaded\n");
    return true;
}

int CAssetDecoratedMap::FindMapIndex(const std::string &name){
    auto Iterator = DMapNameTranslation.find(name);
    
    if(Iterator != DMapNameTranslation.end()){
        return Iterator->second;
    }
    return -1;
}

std::shared_ptr< const CAssetDecoratedMap > CAssetDecoratedMap::GetMap(int index){
    if((0 > index)||(DAllMaps.size() <= index)){
        return std::shared_ptr< const CAssetDecoratedMap >();
    }
    return std::const_pointer_cast< const CAssetDecoratedMap >(DAllMaps[index] );
}

std::shared_ptr< CAssetDecoratedMap > CAssetDecoratedMap::DuplicateMap(int index, EPlayerColor newcolors[pcMax]){
    if((0 > index)||(DAllMaps.size() <= index)){
        return std::shared_ptr< CAssetDecoratedMap >();
    }
    return std::make_shared< CAssetDecoratedMap >( *DAllMaps[index], newcolors );
}


bool CAssetDecoratedMap::AddAsset(std::shared_ptr< CPlayerAsset > asset){
    DAssets.push_back(asset);
    return true;
}

bool CAssetDecoratedMap::RemoveAsset(std::shared_ptr< CPlayerAsset > asset){
    DAssets.remove(asset);
    return true;    
}

bool CAssetDecoratedMap::CanPlaceAsset(const CPosition &pos, int size, std::shared_ptr< CPlayerAsset > ignoreasset){
    int RightX, BottomY;    

    for(int YOff = 0; YOff < size; YOff++){
        for(int XOff = 0; XOff < size; XOff++){
            auto TileTerrainType = TileType(pos.X() + XOff, pos.Y() + YOff);
            
            if((CTerrainMap::ttGrass != TileTerrainType)&&(CTerrainMap::ttDirt != TileTerrainType)&&(CTerrainMap::ttStump != TileTerrainType)&&(CTerrainMap::ttRubble != TileTerrainType)){
                return false;
            }
        }
    }
    RightX = pos.X() + size;
    BottomY = pos.Y() + size;
    if(RightX >= Width()){
        return false;   
    }
    if(BottomY >= Height()){
        return false;   
    }
    for(auto Asset : DAssets){
        int Offset = atGoldMine == Asset->Type() ? 1 : 0;

        if(atNone == Asset->Type()){
            continue;    
        }
        if(ignoreasset == Asset){
            continue;   
        }
        if(RightX <= Asset->TilePositionX() - Offset){
            continue;   
        }
        if(pos.X() >= (Asset->TilePositionX() + Asset->Size() + Offset)){
            continue;   
        }
        if(BottomY <= Asset->TilePositionY() - Offset){
            continue;   
        }
        if(pos.Y() >= (Asset->TilePositionY() + Asset->Size() + Offset)){
            continue;   
        }
        return false;
    }
    return true;
}

CPosition CAssetDecoratedMap::FindAssetPlacement(std::shared_ptr< CPlayerAsset > placeasset, std::shared_ptr< CPlayerAsset > fromasset, const CPosition &nexttiletarget){
    int TopY, BottomY, LeftX, RightX;
    int BestDistance = -1, CurDistance;
    CPosition BestPosition(-1, -1);
    TopY = fromasset->TilePositionY() - placeasset->Size();
    BottomY = fromasset->TilePositionY() + fromasset->Size();
    LeftX = fromasset->TilePositionX() - placeasset->Size();
    RightX = fromasset->TilePositionX() + fromasset->Size();

    while(true){
        int Skipped = 0;
        if(0 <= TopY){
            int ToX = MIN(RightX, Width() - 1);
            for(int CurX = MAX(LeftX, 0); CurX <= ToX; CurX++){
                if(CanPlaceAsset(CPosition(CurX, TopY), placeasset->Size(), placeasset)){
                    CPosition TempPosition(CurX, TopY);
                    CurDistance = TempPosition.DistanceSquared(nexttiletarget);
                    if((-1 == BestDistance)||(CurDistance < BestDistance)){
                        BestDistance = CurDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        else{
            Skipped++;   
        }
        if(Width() > RightX){
            int ToY = MIN(BottomY, Height() - 1);
            for(int CurY = MAX(TopY, 0); CurY <= ToY; CurY++){
                if(CanPlaceAsset(CPosition(RightX, CurY), placeasset->Size(), placeasset)){
                    CPosition TempPosition(RightX, CurY);
                    CurDistance = TempPosition.DistanceSquared(nexttiletarget);
                    if((-1 == BestDistance)||(CurDistance < BestDistance)){
                        BestDistance = CurDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        else{
            Skipped++;   
        }
        if(Height() > BottomY){
            int ToX = MAX(LeftX, 0);
            for(int CurX = MIN(RightX, Width() - 1); CurX >= ToX; CurX--){
                if(CanPlaceAsset(CPosition(CurX, BottomY), placeasset->Size(), placeasset)){
                    CPosition TempPosition(CurX, BottomY);
                    CurDistance = TempPosition.DistanceSquared(nexttiletarget);
                    if((-1 == BestDistance)||(CurDistance < BestDistance)){
                        BestDistance = CurDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        else{
            Skipped++;   
        }
        if(0 <= LeftX){
            int ToY = MAX(TopY, 0);
            for(int CurY = MIN(BottomY, Height() - 1); CurY >= ToY; CurY--){
                if(CanPlaceAsset(CPosition(LeftX, CurY), placeasset->Size(), placeasset)){
                    CPosition TempPosition(LeftX, CurY);
                    CurDistance = TempPosition.DistanceSquared(nexttiletarget);
                    if((-1 == BestDistance)||(CurDistance < BestDistance)){
                        BestDistance = CurDistance;
                        BestPosition = TempPosition;
                    }
                }
            }
        }
        else{
            Skipped++;   
        }
        if(4 == Skipped){
            break;   
        }
        if(-1 != BestDistance){
            break;    
        }
        TopY--;
        BottomY++;
        LeftX--;
        RightX++;
    }
    return BestPosition;
}

std::weak_ptr< CPlayerAsset > CAssetDecoratedMap::FindNearestAsset(const CPosition &pos, EPlayerColor color, EAssetType type){
    std::shared_ptr< CPlayerAsset > BestAsset;
    int BestDistanceSquared = -1;
    
    for(auto &Asset : DAssets){
        if((Asset->Type() == type)&&(Asset->Color() == color)&&(aaConstruct != Asset->Action())){
            int CurrentDistance = Asset->Position().DistanceSquared(pos);
            
            if((-1 == BestDistanceSquared)||(CurrentDistance < BestDistanceSquared)){
                BestDistanceSquared = CurrentDistance;
                BestAsset = Asset;
            }
        }
    }
    return BestAsset;
}

bool CAssetDecoratedMap::LoadMap(std::shared_ptr< CDataSource > source){
    CLineDataSource LineSource(source);
    std::string TempString;
    std::vector< std::string > Tokens;
    SResourceInitialization TempResourceInit;
    SAssetInitialization TempAssetInit;
    int ResourceCount, AssetCount;
    bool ReturnStatus = false;
    
    if(!CTerrainMap::LoadMap(source)){
        return false;
    }
    try{
        if(!LineSource.Read(TempString)){
            PrintError("Failed to read map resource count.\n");
            goto LoadMapExit;
        }
        ResourceCount = std::stoi(TempString);
        DResourceInitializationList.clear();
        for(int Index = 0; Index <= ResourceCount; Index++){
            if(!LineSource.Read(TempString)){
                PrintError("Failed to read map resource %d.\n", Index);
                goto LoadMapExit;
            }
            CTokenizer::Tokenize(Tokens, TempString);
            if(3 > Tokens.size()){
                PrintError("Too few tokens for resource %d.\n", Index);
                goto LoadMapExit;
            }
            TempResourceInit.DColor = (EPlayerColor)std::stoi(Tokens[0]);
            if((0 == Index)&&(pcNone != TempResourceInit.DColor)){
                PrintError("Expected first resource to be for color None.\n");
                goto LoadMapExit;
            }
            TempResourceInit.DGold = std::stoi(Tokens[1]);
            TempResourceInit.DLumber = std::stoi(Tokens[2]);
            
            DResourceInitializationList.push_back(TempResourceInit);
        }
        
        
        if(!LineSource.Read(TempString)){
            PrintError("Failed to read map asset count.\n");
            goto LoadMapExit;
        }
        AssetCount = std::stoi(TempString);
        DAssetInitializationList.clear();
        for(int Index = 0; Index < AssetCount; Index++){
            if(!LineSource.Read(TempString)){
                PrintError("Failed to read map asset %d.\n", Index);
                goto LoadMapExit;
            }
            CTokenizer::Tokenize(Tokens, TempString);
            if(4 > Tokens.size()){
                PrintError("Too few tokens for asset %d.\n", Index);
                goto LoadMapExit;
            }
            TempAssetInit.DType = Tokens[0];
            TempAssetInit.DColor = (EPlayerColor)std::stoi(Tokens[1]);
            TempAssetInit.DTilePosition.X(std::stoi(Tokens[2]));
            TempAssetInit.DTilePosition.Y(std::stoi(Tokens[3]));
            
            if((0 > TempAssetInit.DTilePosition.X())||(0 > TempAssetInit.DTilePosition.Y())){
                PrintError("Invalid resource position %d (%d, %d).\n", Index, TempAssetInit.DTilePosition.X(), TempAssetInit.DTilePosition.Y());
                goto LoadMapExit;
            }
            if((Width() <= TempAssetInit.DTilePosition.X())||(Height() <= TempAssetInit.DTilePosition.Y())){
                PrintError("Invalid resource position %d (%d, %d).\n", Index, TempAssetInit.DTilePosition.X(), TempAssetInit.DTilePosition.Y());
                goto LoadMapExit;
            }
            DAssetInitializationList.push_back(TempAssetInit);
        }
        ReturnStatus = true;
    }
    catch(std::exception &E){
        PrintError("%s\n",E.what());
    }

LoadMapExit:
    return ReturnStatus;
    
}

const std::list< std::shared_ptr< CPlayerAsset > > &CAssetDecoratedMap::Assets() const{
    return DAssets;
}

const std::list< CAssetDecoratedMap::SAssetInitialization > &CAssetDecoratedMap::AssetInitializationList() const{
    return DAssetInitializationList;
}

const std::list< CAssetDecoratedMap::SResourceInitialization > &CAssetDecoratedMap::ResourceInitializationList() const{
    return DResourceInitializationList;
}

std::shared_ptr< CAssetDecoratedMap > CAssetDecoratedMap::CreateInitializeMap() const{
    std::shared_ptr< CAssetDecoratedMap > ReturnMap = std::make_shared< CAssetDecoratedMap > ();
    
    if(ReturnMap->DMap.size() != DMap.size()){
        // Initialize to empty grass
        ReturnMap->DMap.resize(DMap.size());
        for(auto &Row : ReturnMap->DMap){
            Row.resize(DMap[0].size());
            for(auto &Cell : Row){
                Cell = ttNone;
            }
        }
    }
    return ReturnMap;
}

std::shared_ptr< CVisibilityMap > CAssetDecoratedMap::CreateVisibilityMap() const{
    return std::make_shared< CVisibilityMap >(Width(), Height(), CPlayerAssetType::MaxSight());
}

bool CAssetDecoratedMap::UpdateMap(const CVisibilityMap &vismap, const CAssetDecoratedMap &resmap){
    auto Iterator = DAssets.begin();
    
    if(DMap.size() != resmap.DMap.size()){
        DMap.resize(resmap.DMap.size());
        for(auto &Row : DMap){
            Row.resize(resmap.DMap[0].size());
            for(auto &Cell : Row){
                Cell = ttNone;
            }
        }
    }
    while(Iterator != DAssets.end()){
        CPosition CurPosition = (*Iterator)->TilePosition();
        int AssetSize = (*Iterator)->Size();
        bool RemoveAsset = false;
        if((*Iterator)->Speed()||(aaDecay == (*Iterator)->Action())||(aaAttack == (*Iterator)->Action())){  // Remove all movable units
            Iterator = DAssets.erase(Iterator);
            continue;
        }
        for(int YOff = 0; YOff < AssetSize; YOff++){
            int YPos = CurPosition.Y() + YOff;
            for(int XOff = 0; XOff < AssetSize; XOff++){
                int XPos = CurPosition.X() + XOff;

                CVisibilityMap::ETileVisibility VisType = vismap.TileType(XPos, YPos);
                if((CVisibilityMap::tvPartial == VisType)||(CVisibilityMap::tvPartialPartial == VisType)||(CVisibilityMap::tvVisible == VisType)){ // Remove visible so they can be updated
                    RemoveAsset = atNone != (*Iterator)->Type();
                    break;
                }
            }
            if(RemoveAsset){
                break;   
            }
        }
        if(RemoveAsset){
            Iterator = DAssets.erase(Iterator);
            continue;
        }
        Iterator++;
    }
    for(int YPos = 0; YPos < DMap.size(); YPos++){
        for(int XPos = 0; XPos < DMap[YPos].size(); XPos++){
            CVisibilityMap::ETileVisibility VisType = vismap.TileType(XPos-1, YPos-1);
            if((CVisibilityMap::tvPartial == VisType)||(CVisibilityMap::tvPartialPartial == VisType)||(CVisibilityMap::tvVisible == VisType)){
                DMap[YPos][XPos] = resmap.DMap[YPos][XPos];
            }
        }
    }
    for(auto &Asset : resmap.DAssets){
        CPosition CurPosition = Asset->TilePosition();
        int AssetSize = Asset->Size();
        bool AddAsset = false;
        
        for(int YOff = 0; YOff < AssetSize; YOff++){
            int YPos = CurPosition.Y() + YOff;
            for(int XOff = 0; XOff < AssetSize; XOff++){
                int XPos = CurPosition.X() + XOff;

                CVisibilityMap::ETileVisibility VisType = vismap.TileType(XPos, YPos);
                if((CVisibilityMap::tvPartial == VisType)||(CVisibilityMap::tvPartialPartial == VisType)||(CVisibilityMap::tvVisible == VisType)){ // Add visible resources
                    AddAsset = true;
                    break;
                }
            }
            if(AddAsset){
                DAssets.push_back(Asset);
                break;
            }
        }
    }
    
    return true;
}

#define SEARCH_STATUS_UNVISITED 0
#define SEARCH_STATUS_QUEUED    1
#define SEARCH_STATUS_VISITED   2

typedef struct{
    int DX;
    int DY;
} SSearchTile, *SSearchTileRef; 

CPosition CAssetDecoratedMap::FindNearestReachableTileType(const CPosition &pos, ETileType type){
    std::queue< SSearchTile > SearchQueue;
    SSearchTile CurrentSearch, TempSearch;
    int MapWidth = Width();
    int MapHeight = Height();
    int SearchXOffsets[] = {0,1,0,-1};
    int SearchYOffsets[] = {-1,0,1,0};
    
    if(DSearchMap.size() != DMap.size()){
        DSearchMap.resize(DMap.size());
        for(auto &Row : DSearchMap){
            Row.resize(DMap[0].size());
            for(auto &Cell : Row){
                Cell = 0;
            }
        }
        int LastYIndex = DMap.size() - 1;
        int LastXIndex = DMap[0].size() - 1;
        for(int Index = 0; Index < DMap.size(); Index++){
            DSearchMap[Index][0] = SEARCH_STATUS_VISITED;
            DSearchMap[Index][LastXIndex] = SEARCH_STATUS_VISITED; 
        }
        for(int Index = 1; Index < LastXIndex; Index++){
            DSearchMap[0][Index] = SEARCH_STATUS_VISITED;
            DSearchMap[LastYIndex][Index] = SEARCH_STATUS_VISITED;
        }
    }
    for(int Y = 0; Y < MapHeight; Y++){
        for(int X = 0; X < MapWidth; X++){
            DSearchMap[Y+1][X+1] = SEARCH_STATUS_UNVISITED;
        }
    }
    for(auto Asset : DAssets){
        if(Asset->TilePosition() != pos){
            for(int Y = 0; Y < Asset->Size(); Y++){
                for(int X = 0; X < Asset->Size(); X++){
                    DSearchMap[Asset->TilePositionY()+Y+1][Asset->TilePositionX()+X+1] = SEARCH_STATUS_VISITED;
                }
            }     
        }
    }
    
    CurrentSearch.DX = pos.X() + 1;
    CurrentSearch.DY = pos.Y() + 1;
    SearchQueue.push(CurrentSearch);
    while(SearchQueue.size()){
        CurrentSearch = SearchQueue.front();
        SearchQueue.pop();
        DSearchMap[CurrentSearch.DY][CurrentSearch.DX] = SEARCH_STATUS_VISITED;
        for(int Index = 0; Index < (sizeof(SearchXOffsets)/sizeof(int)); Index++){
            TempSearch.DX = CurrentSearch.DX + SearchXOffsets[Index];
            TempSearch.DY = CurrentSearch.DY + SearchYOffsets[Index];
            if(SEARCH_STATUS_UNVISITED == DSearchMap[TempSearch.DY][TempSearch.DX]){
                ETileType CurTileType =DMap[TempSearch.DY][TempSearch.DX];
                
                DSearchMap[TempSearch.DY][TempSearch.DX] = SEARCH_STATUS_QUEUED;
                if(type == CurTileType){
                    return CPosition(TempSearch.DX - 1, TempSearch.DY - 1);
                }
                if((ttGrass == CurTileType)||(ttDirt == CurTileType)||(ttStump == CurTileType)||(ttRubble == CurTileType)||(ttNone == CurTileType)){
                    SearchQueue.push(TempSearch);
                }
            }
        }
    }
    return CPosition(-1, -1);
}
