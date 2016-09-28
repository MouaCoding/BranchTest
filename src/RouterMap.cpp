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
#include "RouterMap.h"
//#include <stdlib.h>
#include <cstdlib>
#include <queue>

#define SEARCH_STATUS_UNVISITED -1
#define SEARCH_STATUS_VISITED   -2
#define SEARCH_STATUS_OCCUPIED  -3

EDirection CRouterMap::DIdealSearchDirection = dNorth;
int CRouterMap::DMapWidth = 1;

bool CRouterMap::SearchTargetCompare(const SSearchTarget &first, const SSearchTarget &second){
    if(first.DTargetDistanceSquared == second.DTargetDistanceSquared){
        if(first.DSteps == second.DSteps){
            return (first.DX + first.DY * DMapWidth) <= (second.DX + second.DY * DMapWidth);
        }
        return first.DSteps < second.DSteps;
    }
    return first.DTargetDistanceSquared < second.DTargetDistanceSquared;
/*    
    if(first.DTargetDistanceSquared == second.DTargetDistanceSquared){
        if(first.DSteps == second.DSteps){
            if(first.DTileType == second.DTileType){
                int DeltaDir1 = DIdealSearchDirection - first.DInDirection;
                int DeltaDir2 = DIdealSearchDirection - second.DInDirection;
                
                if(-(dMax/2) >= DeltaDir1){
                    DeltaDir1 += dMax;   
                }
                if((dMax/2) < DeltaDir1){
                    DeltaDir1 -= dMax;   
                }
                if(-(dMax/2) >= DeltaDir2){
                    DeltaDir2 += dMax;   
                }
                if((dMax/2) < DeltaDir2){
                    DeltaDir2 -= dMax;   
                }
                if((DeltaDir1 == DeltaDir2)||(DeltaDir1 == -DeltaDir2)){
                    return DeltaDir1 <= DeltaDir2;    
                }
                DeltaDir1 = DeltaDir1 < 0 ? -DeltaDir1 : DeltaDir1;
                DeltaDir2 = DeltaDir2 < 0 ? -DeltaDir2 : DeltaDir2;
                return DeltaDir1 <= DeltaDir2; 
            }
        }
        if(first.DTileType == second.DTileType){
            return first.DSteps < second.DSteps;
        }
        if(CTerrainMap::ttNone == second.DTileType){
            return false;   
        }
        if(CTerrainMap::ttNone == first.DTileType){
            return true;   
        }
        return first.DSteps < second.DSteps;
    }
    return first.DTargetDistanceSquared <= second.DTargetDistanceSquared;
*/
}

bool CRouterMap::MovingAway(EDirection dir1, EDirection dir2){
    int Value;
    if((0 > dir2)||(dMax <= dir2)){
        return false;    
    }
    Value = ((dMax + dir2) - dir1) % dMax;
    if((1 >= Value)||(dMax - 1 <= Value)){
        return true;   
    }
    return false;
}

EDirection CRouterMap::FindRoute(const CAssetDecoratedMap &resmap, const CPlayerAsset &asset, const CPosition &target){
    int MapWidth = resmap.Width();
    int MapHeight = resmap.Height();
    int StartX = asset.TilePositionX();
    int StartY = asset.TilePositionY();
    SSearchTarget CurrentSearch, BestSearch, TempSearch;
    CPosition CurrentTile, TargetTile, TempTile;
    EDirection SearchDirecitons[] = {dNorth,dEast,dSouth,dWest};
    int ResMapXOffsets[] = {0,1,0,-1};
    int ResMapYOffsets[] = {-1,0,1,0};
    int DiagCheckXOffset[] = {0,1,1,1,0,-1,-1,-1};
    int DiagCheckYOffset[] = {-1,-1,0,1,1,1,0,-1};
    int SearchDirectionCount = sizeof(SearchDirecitons) / sizeof(EDirection);
    EDirection LastInDirection, DirectionBeforeLast;
    std::queue< SSearchTarget > SearchQueue;
    
    //printf("Initializing Map\n");
    TargetTile.SetToTile(target);
    if((DMap.size() != MapHeight + 2)||(DMap[0].size() != MapWidth + 2)){
        int LastYIndex = MapHeight + 1;
        int LastXIndex = MapWidth + 1;
        DMap.resize(MapHeight + 2);
        for(auto &Row : DMap){
            Row.resize(MapWidth + 2);   
        }
        for(int Index = 0; Index < DMap.size(); Index++){
            DMap[Index][0] = SEARCH_STATUS_VISITED;
            DMap[Index][LastXIndex] = SEARCH_STATUS_VISITED; 
        }
        for(int Index = 0; Index < MapWidth; Index++){
            DMap[0][Index+1] = SEARCH_STATUS_VISITED;
            DMap[LastYIndex][Index+1] = SEARCH_STATUS_VISITED;
        }
        DMapWidth = MapWidth + 2;
    }
    //printf("Comparing target to current\n");
    if(asset.TilePosition() == TargetTile){
        int DeltaX = target.X() - asset.PositionX();
        int DeltaY = target.Y() - asset.PositionY();
        
        if(0 < DeltaX){
            if(0 < DeltaY){
                return dNorthEast;
            }
            else if(0 > DeltaY){
                return dSouthEast;
            }
            return dEast;
        }
        else if(0 > DeltaX){
            if(0 < DeltaY){
                return dNorthWest;
            }
            else if(0 > DeltaY){
                return dSouthWest;
            }
            return dWest;            
        }
        if(0 < DeltaY){
            return dNorth;
        }
        else if(0 > DeltaY){
            return dSouth;
        }
        
        return dMax;
    }
    //printf("Initializing unvisited\n");
    for(int Y = 0; Y < MapHeight; Y++){
        for(int X = 0; X < MapWidth; X++){
            DMap[Y+1][X+1] = SEARCH_STATUS_UNVISITED;
        }
    }
    //printf("Marking assets visited\n");
    for(auto &Res : resmap.Assets()){
        if(&asset != Res.get()){
            if(atNone != Res->Type()){
                if((aaWalk != Res->Action())||(asset.Color() != Res->Color())){
                    if((asset.Color() != Res->Color())||((aaConveyGold != Res->Action())&&(aaConveyLumber != Res->Action())&&(aaMineGold != Res->Action()))){
                        for(int YOff = 0; YOff < Res->Size(); YOff++){
                            for(int XOff = 0; XOff < Res->Size(); XOff++){
                                DMap[Res->TilePositionY() + YOff + 1][Res->TilePositionX() + XOff + 1] = SEARCH_STATUS_VISITED;
                            }
                        }
                    }
                }
                else{
                    DMap[Res->TilePositionY() + 1][Res->TilePositionX() + 1] = SEARCH_STATUS_OCCUPIED - Res->Direction();   
                }
            }
        }
    }
    
    //DSearchTargets.clear();
    DIdealSearchDirection = asset.Direction();//(asset.Direction()/2) * 2;
    CurrentTile = asset.TilePosition();
    CurrentSearch.DX = BestSearch.DX = CurrentTile.X();
    CurrentSearch.DY = BestSearch.DY = CurrentTile.Y();
    CurrentSearch.DSteps = 0;
    CurrentSearch.DTargetDistanceSquared = BestSearch.DTargetDistanceSquared = CurrentTile.DistanceSquared(TargetTile);
    CurrentSearch.DInDirection = BestSearch.DInDirection = dMax;
    DMap[StartY+1][StartX+1] = SEARCH_STATUS_VISITED;
    //printf("Searching Map (%d, %d) -> (%d, %d)\n", CurrentTile.X(), CurrentTile.Y(), TargetTile.X(), TargetTile.Y());
    while(true){
        if(CurrentTile == TargetTile){
            BestSearch = CurrentSearch;
            break;
        }
        if(CurrentSearch.DTargetDistanceSquared < BestSearch.DTargetDistanceSquared){
            BestSearch = CurrentSearch;
        }
        for(int Index = 0; Index < SearchDirectionCount; Index++){
            TempTile.X(CurrentSearch.DX + ResMapXOffsets[Index]);
            TempTile.Y(CurrentSearch.DY + ResMapYOffsets[Index]);
            //printf("Tile(%d, %d) = %d\n",TempTile.X(), TempTile.Y(), DMap[TempTile.Y() + 1][TempTile.X() + 1]);
            if((SEARCH_STATUS_UNVISITED == DMap[TempTile.Y() + 1][TempTile.X() + 1])||MovingAway(SearchDirecitons[Index], (EDirection)(SEARCH_STATUS_OCCUPIED - DMap[TempTile.Y() + 1][TempTile.X() + 1]))){
                DMap[TempTile.Y() + 1][TempTile.X() + 1] = Index;
                CTerrainMap::ETileType CurTileType = resmap.TileType(TempTile.X(), TempTile.Y());
                if((CTerrainMap::ttGrass == CurTileType)||(CTerrainMap::ttDirt == CurTileType)||(CTerrainMap::ttStump == CurTileType)||(CTerrainMap::ttRubble == CurTileType)||(CTerrainMap::ttNone == CurTileType)){
                    TempSearch.DX = TempTile.X();
                    TempSearch.DY = TempTile.Y();
                    TempSearch.DSteps = CurrentSearch.DSteps + 1;
                    TempSearch.DTileType = CurTileType;
                    TempSearch.DTargetDistanceSquared = TempTile.DistanceSquared(TargetTile);
                    TempSearch.DInDirection = SearchDirecitons[Index];
                    //DSearchTargets.push_back(TempSearch);
                    SearchQueue.push(TempSearch);
                }
            }
        }
        //DSearchTargets.sort(SearchTargetCompare);
        //if(DSearchTargets.empty()){
        //    break;    
        //}
        if(SearchQueue.empty()){
            break;   
        }
        //CurrentSearch = DSearchTargets.front();
        //DSearchTargets.pop_front();
        CurrentSearch = SearchQueue.front();
        SearchQueue.pop();
        CurrentTile.X(CurrentSearch.DX);
        CurrentTile.Y(CurrentSearch.DY);
    }
    //printf("Finding backpath (%d,%d) %d\n", BestSearch.DX, BestSearch.DY, BestSearch.DInDirection);
    DirectionBeforeLast = LastInDirection = BestSearch.DInDirection;
    CurrentTile.X(BestSearch.DX);
    CurrentTile.Y(BestSearch.DY);
    while((CurrentTile.X() != StartX)||(CurrentTile.Y() != StartY)){
        int Index = DMap[CurrentTile.Y()+1][CurrentTile.X()+1];
        
        //printf("DMap[%d][%d] == %d\n",CurrentTile.Y()+1, CurrentTile.X()+1, Index);
        if((0 > Index)||(SearchDirectionCount <= Index)){
            exit(0);   
        }
        DirectionBeforeLast = LastInDirection;
        LastInDirection = SearchDirecitons[Index];
        CurrentTile.DecrementX(ResMapXOffsets[Index]);
        CurrentTile.DecrementY(ResMapYOffsets[Index]);
    }
    //printf("Returning direction\n");
    if(DirectionBeforeLast != LastInDirection){
        
        //printf("D %d -> %d\n", LastInDirection, DirectionBeforeLast);  
        CTerrainMap::ETileType CurTileType = resmap.TileType(StartX + DiagCheckXOffset[DirectionBeforeLast], StartY + DiagCheckYOffset[DirectionBeforeLast]);
        if((CTerrainMap::ttGrass == CurTileType)||(CTerrainMap::ttDirt == CurTileType)||(CTerrainMap::ttStump == CurTileType)||(CTerrainMap::ttRubble == CurTileType)||(CTerrainMap::ttNone == CurTileType)){
            int Sum = LastInDirection + DirectionBeforeLast;
            if((6 == Sum)&&(!LastInDirection || !DirectionBeforeLast)){ // NW wrap around
                Sum += 8;
            }
            Sum /= 2;
            //printf("Can go diag %d\n", Sum);  
            LastInDirection = (EDirection)Sum;
        }
    }
    
    return LastInDirection;
}

