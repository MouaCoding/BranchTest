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
#include "UnitDescriptionRenderer.h"
#include "TextFormatter.h"

#define FG_COLOR        0
#define BG_COLOR        1
#define MAX_HP_COLOR    3
#define HEALTH_HEIGHT   5

CUnitDescriptionRenderer::CUnitDescriptionRenderer(std::shared_ptr< CBevel > bevel, std::shared_ptr< CGraphicMulticolorTileset > icons, std::shared_ptr< CFontTileset > fonts[fsMax], EPlayerColor color){
    gint TextWidth, TextHeight;
    
    DPlayerColor = color;
    DIconTileset = icons;
    DBevel = bevel;
    for(int Index = 0; Index < fsMax; Index++){
        DFonts[Index] = fonts[Index];
        
        DFontColorIndices[Index].resize(2);
        DFontColorIndices[Index][FG_COLOR] = DFonts[Index]->FindPixel("white");
        DFontColorIndices[Index][BG_COLOR] = DFonts[Index]->FindPixel("black");
    }
    DFonts[fsSmall]->MeasureText("0123456789", TextWidth, TextHeight);
    DFullIconWidth = DIconTileset->TileWidth() + DBevel->Width() * 2;
    DFullIconHeight = DIconTileset->TileHeight() + DBevel->Width() * 3 + HEALTH_HEIGHT + 2 + TextHeight;
    
    DHealthColors.resize(MAX_HP_COLOR);
    DHealthColors[0] = {0xFC0000, 0xFC00, 0x0000, 0x0000};
    DHealthColors[1] = {0xFCFC00, 0xFC00, 0xFC00, 0x0000};
    DHealthColors[2] = {0x307000, 0x3000, 0x7000, 0x0000};
    
    DHealthRectangleFG = GdkColor({0x000000, 0x0000, 0x0000, 0x0000});
    DHealthRectangleBG = GdkColor({0x303030, 0x3000, 0x3000, 0x3000});
    
    DConstructionRectangleFG = GdkColor({0xA0A060, 0xA000, 0xA000, 0x6000});
    DConstructionRectangleBG = GdkColor({0x505050, 0x5000, 0x5000, 0x5000});
    DConstructionRectangleCompletion = GdkColor({0x307000, 0x3000, 0x7000, 0x0000});
    DConstructionRectangleShadow = GdkColor({0x000000, 0x0000, 0x0000, 0x0000});
    
    DAssetIndices.resize(atMax);

    DAssetIndices[atPeasant] = DIconTileset->FindTile("peasant");
    DAssetIndices[atFootman] = DIconTileset->FindTile("footman");
    DAssetIndices[atArcher] = DIconTileset->FindTile("archer");
    DAssetIndices[atRanger] = DIconTileset->FindTile("ranger");
    DAssetIndices[atGoldMine] = DIconTileset->FindTile("gold-mine");
    DAssetIndices[atTownHall] = DIconTileset->FindTile("town-hall");
    DAssetIndices[atKeep] = DIconTileset->FindTile("keep");
    DAssetIndices[atCastle] = DIconTileset->FindTile("castle");
    DAssetIndices[atFarm] = DIconTileset->FindTile("chicken-farm");
    DAssetIndices[atBarracks] = DIconTileset->FindTile("human-barracks");
    DAssetIndices[atLumberMill] = DIconTileset->FindTile("human-lumber-mill");
    DAssetIndices[atBlacksmith] = DIconTileset->FindTile("human-blacksmith");
    DAssetIndices[atScoutTower] = DIconTileset->FindTile("scout-tower");
    DAssetIndices[atGuardTower] = DIconTileset->FindTile("human-guard-tower");
    DAssetIndices[atCannonTower] = DIconTileset->FindTile("human-cannon-tower");
    
    DResearchIndices.resize(actMax);

    DResearchIndices[actWeaponUpgrade1] = DIconTileset->FindTile("human-weapon-1");
    DResearchIndices[actWeaponUpgrade2] = DIconTileset->FindTile("human-weapon-2");
    DResearchIndices[actWeaponUpgrade3] = DIconTileset->FindTile("human-weapon-3");
    DResearchIndices[actArrowUpgrade1] = DIconTileset->FindTile("human-arrow-1");
    DResearchIndices[actArrowUpgrade2] = DIconTileset->FindTile("human-arrow-2");
    DResearchIndices[actArrowUpgrade3] = DIconTileset->FindTile("human-arrow-3");
    DResearchIndices[actArmorUpgrade1] = DIconTileset->FindTile("human-armor-1");
    DResearchIndices[actArmorUpgrade2] = DIconTileset->FindTile("human-armor-2");
    DResearchIndices[actArmorUpgrade3] = DIconTileset->FindTile("human-armor-3");
    DResearchIndices[actLongbow] = DIconTileset->FindTile("longbow");
    DResearchIndices[actRangerScouting] = DIconTileset->FindTile("ranger-scouting");
    DResearchIndices[actMarksmanship] = DIconTileset->FindTile("marksmanship");
    DResearchIndices[actBuildRanger] = DIconTileset->FindTile("ranger");
}

CUnitDescriptionRenderer::~CUnitDescriptionRenderer(){
    
}

std::string CUnitDescriptionRenderer::AddAssetNameSpaces(const std::string &name){
    std::string ReturnString;
    
    for(auto CurChar : name){
        if(ReturnString.size() && (('A' <= CurChar)&&('Z' >= CurChar))){
            ReturnString += ' ';   
        }
        ReturnString += CurChar;
    }
    
    return ReturnString;
}

int CUnitDescriptionRenderer::MinimumWidth() const{
    return DFullIconWidth * 3 + DBevel->Width() * 2;
}

int CUnitDescriptionRenderer::MinimumHeight(int width, int count) const{
    int Columns = (width / DFullIconWidth);
    int Rows = (count + Columns-1) / Columns;
    return Rows * DFullIconHeight + (Rows - 1) * DBevel->Width();
}

int CUnitDescriptionRenderer::MaxSelection(int width, int height) const{
    return (width / DFullIconWidth) * (height / DFullIconHeight);
}

int CUnitDescriptionRenderer::Selection(const CPosition &pos) const{
    int HorizontalIcons, VerticalIcons;
    int HorizontalGap, VerticalGap;
    int SelectedIcon = -1;
    
    HorizontalIcons = DDisplayedWidth / DFullIconWidth;
    VerticalIcons = DDisplayedHeight / DFullIconHeight;
    HorizontalGap = (DDisplayedWidth - (HorizontalIcons * DFullIconWidth)) / (HorizontalIcons - 1);
    VerticalGap = (DDisplayedHeight - (VerticalIcons * DFullIconHeight)) / (VerticalIcons - 1);
    if(((pos.X() % (DFullIconWidth + HorizontalGap)) < DFullIconWidth)&&((pos.Y() % (DFullIconHeight + VerticalGap)) < DFullIconHeight)){
        SelectedIcon = (pos.X() / (DFullIconWidth + HorizontalGap)) + HorizontalIcons * (pos.Y() / (DFullIconHeight + VerticalGap));
        if(DDisplayedIcons <= SelectedIcon){
            SelectedIcon = -1;    
        }
    }
    
    return SelectedIcon;
}

void CUnitDescriptionRenderer::DrawCompletionBar(GdkDrawable *drawable, GdkGC *gc, int percent){
    int TextWidth, TextHeight, DisplayWidth, TextTop, TextBottom;
    GdkColor BlackColor({0x000000, 0x0000, 0x0000, 0x0000});
    DFonts[fsLarge]->MeasureTextDetailed("% Complete", TextWidth, TextHeight, TextTop, TextBottom);
    
    TextHeight = TextBottom - TextTop + 1;
    gdk_gc_set_rgb_fg_color(gc, &BlackColor);
    gdk_gc_set_rgb_bg_color(gc, &BlackColor);
    gdk_draw_rectangle(drawable, gc, TRUE, 0, DDisplayedHeight - (TextHeight + 12),  DDisplayedWidth, TextHeight + 12);
    gdk_gc_set_rgb_fg_color(gc, &DConstructionRectangleFG);
    
    gdk_draw_rectangle(drawable, gc, FALSE, 1, DDisplayedHeight - (TextHeight + 11),  DDisplayedWidth-2, TextHeight + 10);
    gdk_draw_rectangle(drawable, gc, TRUE, 2, DDisplayedHeight - (TextHeight + 10),  DDisplayedWidth-4, TextHeight + 8);
    gdk_gc_set_rgb_fg_color(gc, &DConstructionRectangleBG);
    gdk_gc_set_rgb_bg_color(gc, &DConstructionRectangleBG);
    gdk_draw_rectangle(drawable, gc, TRUE, 4, DDisplayedHeight - (TextHeight + 8),  DDisplayedWidth-8, TextHeight + 4);
    
    gdk_gc_set_rgb_fg_color(gc, &DConstructionRectangleShadow);
    gdk_gc_set_rgb_bg_color(gc, &DConstructionRectangleShadow);
    gdk_draw_rectangle(drawable, gc, TRUE, 4, DDisplayedHeight - (TextHeight + 8),  DDisplayedWidth-8, TextHeight/2 + 2);
    
    DisplayWidth = percent * (DDisplayedWidth-8) / 100;
    gdk_gc_set_rgb_fg_color(gc, &DConstructionRectangleCompletion);
    gdk_gc_set_rgb_bg_color(gc, &DConstructionRectangleCompletion);
    gdk_draw_rectangle(drawable, gc, TRUE, 4, DDisplayedHeight - (TextHeight + 8), DisplayWidth, TextHeight + 4);
    
    DFonts[fsLarge]->DrawTextWithShadow(drawable, gc, 
                        DDisplayedWidth/2 - TextWidth/2, 
                        DDisplayedHeight  - (TextHeight + TextTop + 6), 
                        DFontColorIndices[fsLarge][FG_COLOR], DFontColorIndices[fsLarge][BG_COLOR], 1, "% Complete");
    
}

void CUnitDescriptionRenderer::DrawUnitDescription(GdkDrawable *drawable, const std::list< std::weak_ptr< CPlayerAsset > > &selectionlist){
    DDisplayedIcons = 0;
    if(selectionlist.size()){
        GdkGC *TempGC = gdk_gc_new(drawable);
        int HorizontalIcons, VerticalIcons;
        int HorizontalGap, VerticalGap;
        
        gdk_pixmap_get_size(drawable, &DDisplayedWidth, &DDisplayedHeight); 
        HorizontalIcons = DDisplayedWidth / DFullIconWidth;
        VerticalIcons = DDisplayedHeight / DFullIconHeight;
        HorizontalGap = (DDisplayedWidth - (HorizontalIcons * DFullIconWidth)) / (HorizontalIcons - 1);
        VerticalGap = (DDisplayedHeight - (VerticalIcons * DFullIconHeight)) / (VerticalIcons - 1);

        if(1 == selectionlist.size()){
            DDisplayedIcons = 1;
            if(auto Asset = selectionlist.front().lock()){
                int HPColor = (Asset->HitPoints() - 1) * MAX_HP_COLOR / Asset->MaxHitPoints();
                gint TextWidth, TextHeight;
                gint TextCenter, TextTop;
                std::string AssetName = AddAssetNameSpaces(CPlayerAssetType::TypeToName(Asset->Type()));
                std::string TempString;
                
                DBevel->DrawBevel(drawable, DBevel->Width(), DBevel->Width(), DIconTileset->TileWidth(), DIconTileset->TileHeight());
                DIconTileset->DrawTile(drawable, TempGC, DBevel->Width(), DBevel->Width(), DAssetIndices[Asset->Type()], Asset->Color() ? Asset->Color() - 1 : 0);
                
                DFonts[fsMedium]->MeasureText(AssetName, TextWidth, TextHeight);
                
                TextCenter = (DDisplayedWidth + DIconTileset->TileWidth() + DBevel->Width() * 2)/ 2;
                DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                    TextCenter - TextWidth/2, 
                    (DIconTileset->TileHeight()/2 + DBevel->Width()) - TextHeight/2, 
                    DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, AssetName);

                if(pcNone != Asset->Color()){
                    gdk_gc_set_rgb_fg_color(TempGC, &DHealthRectangleFG);
                    gdk_gc_set_rgb_bg_color(TempGC, &DHealthRectangleBG);
                    gdk_draw_rectangle(drawable, TempGC, TRUE, 0, DIconTileset->TileHeight() + DBevel->Width() * 3,  DIconTileset->TileWidth() + DBevel->Width() * 2, HEALTH_HEIGHT + 2);
                    
                    gdk_gc_set_rgb_fg_color(TempGC, &DHealthColors[HPColor]);
                    gdk_gc_set_rgb_bg_color(TempGC, &DHealthColors[HPColor]);
                    gdk_draw_rectangle(drawable, TempGC, TRUE, 1, DIconTileset->TileHeight() + DBevel->Width() * 3 + 1,  (DIconTileset->TileWidth() + DBevel->Width() * 2 - 2) * Asset->HitPoints()/ Asset->MaxHitPoints() , HEALTH_HEIGHT);
                    
                    TempString = std::to_string(Asset->HitPoints()) + std::string(" / ") + std::to_string(Asset->MaxHitPoints());
                    DFonts[fsSmall]->MeasureText(TempString, TextWidth, TextHeight);
                
                    TextTop = DIconTileset->TileHeight() + DBevel->Width() * 4 + HEALTH_HEIGHT + 2;
                    DFonts[fsSmall]->DrawTextWithShadow(drawable, TempGC, 
                        (DIconTileset->TileWidth()/2 + DBevel->Width()) - TextWidth/2, 
                        TextTop, 
                        DFontColorIndices[fsSmall][FG_COLOR], DFontColorIndices[fsSmall][BG_COLOR], 1, TempString);
                
                    TextTop += TextHeight;
                }
                if(DPlayerColor == Asset->Color()){
                    if(Asset->Speed()){
                        gint TextLineHeight;
                        int UpgradeValue;
                        
                        TempString = "Armor: ";
                        DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                        TextLineHeight = TextHeight;
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter - TextWidth, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        UpgradeValue = Asset->ArmorUpgrade();
                        TempString = std::to_string(Asset->Armor());
                        if(UpgradeValue){
                            TempString += " + ";
                            TempString += std::to_string(UpgradeValue);
                        }
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        
                        TextTop += TextLineHeight;
                        TempString = "Damage: ";
                        DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter - TextWidth, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        UpgradeValue = Asset->BasicDamageUpgrade() + Asset->PiercingDamageUpgrade();
                        TempString.clear();
                        if(UpgradeValue){
                            TempString += " + ";
                            TempString += std::to_string(UpgradeValue);
                        }
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, std::to_string(Asset->PiercingDamage()/2) + "-" + std::to_string(Asset->PiercingDamage() + Asset->BasicDamage()) + TempString);
                        
                        TextTop += TextLineHeight;
                        TempString = "Range: ";
                        DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter - TextWidth, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        UpgradeValue = Asset->RangeUpgrade();
                        TempString = std::to_string(Asset->Range());
                        if(UpgradeValue){
                            TempString += " + ";
                            TempString += std::to_string(UpgradeValue);
                        }
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        TextTop += TextLineHeight;
                        TempString = "Sight: ";
                        DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter - TextWidth, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        UpgradeValue = Asset->SightUpgrade();
                        TempString = std::to_string(Asset->Sight());
                        if(UpgradeValue){
                            TempString += " + ";
                            TempString += std::to_string(UpgradeValue);
                        }
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        TextTop += TextLineHeight;
                        TempString = "Speed: ";
                        DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter - TextWidth, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        UpgradeValue = Asset->SpeedUpgrade();
                        TempString = std::to_string(Asset->Speed());
                        if(UpgradeValue){
                            TempString += " + ";
                            TempString += std::to_string(UpgradeValue);
                        }
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                    }
                    else{
                        if(aaConstruct == Asset->Action()){
                            auto Command = Asset->CurrentCommand();
                            int PercentComplete = 0;
                            if(Command.DAssetTarget){
                                Command = Command.DAssetTarget->CurrentCommand();
                                if(Command.DActivatedCapability){
                                    PercentComplete = Command.DActivatedCapability->PercentComplete(100);
                                }
                            }
                            else if(Command.DActivatedCapability){
                                PercentComplete = Command.DActivatedCapability->PercentComplete(100);
                            }
                            DrawCompletionBar(drawable, TempGC, PercentComplete);
                        }
                        else if(aaCapability == Asset->Action()){
                            auto Command = Asset->CurrentCommand();
                            int PercentComplete = 0;
                            if(Command.DActivatedCapability){
                                PercentComplete = Command.DActivatedCapability->PercentComplete(100);
                            }
                            if(Command.DAssetTarget){
                                //int HPColor = (Command.DAssetTarget->HitPoints() - 1) * MAX_HP_COLOR / Command.DAssetTarget->MaxHitPoints();
                                int HorizontalOffset = DBevel->Width(), VerticalOffset = DBevel->Width();
                                
                                HorizontalOffset += 2 * (DFullIconWidth + HorizontalGap);
                                VerticalOffset += DFullIconHeight + VerticalGap;
                                
                                DBevel->DrawBevel(drawable, HorizontalOffset, VerticalOffset, DIconTileset->TileWidth(), DIconTileset->TileHeight());
                                DIconTileset->DrawTile(drawable, TempGC, HorizontalOffset, VerticalOffset, DAssetIndices[Command.DAssetTarget->Type()], Command.DAssetTarget->Color() ? Command.DAssetTarget->Color() - 1 : 0);
                                /*
                                gdk_gc_set_rgb_fg_color(TempGC, &DHealthRectangleFG);
                                gdk_gc_set_rgb_bg_color(TempGC, &DHealthRectangleBG);
                                gdk_draw_rectangle(drawable, TempGC, TRUE, HorizontalOffset - DBevel->Width(), VerticalOffset + DIconTileset->TileHeight() + DBevel->Width() * 3,  DIconTileset->TileWidth() + DBevel->Width() * 2, HEALTH_HEIGHT + 2);
                                
                                gdk_gc_set_rgb_fg_color(TempGC, &DHealthColors[HPColor]);
                                gdk_gc_set_rgb_bg_color(TempGC, &DHealthColors[HPColor]);
                                gdk_draw_rectangle(drawable, TempGC, TRUE, HorizontalOffset - DBevel->Width() + 1, VerticalOffset + DIconTileset->TileHeight() + DBevel->Width() * 3 + 1,  (DIconTileset->TileWidth() + DBevel->Width() * 2 - 2) * Command.DAssetTarget->HitPoints()/ Command.DAssetTarget->MaxHitPoints() , HEALTH_HEIGHT);
                                
                                TempString = std::to_string(Command.DAssetTarget->HitPoints()) + std::string(" / ") + std::to_string(Command.DAssetTarget->MaxHitPoints());
                                DFonts[fsSmall]->MeasureText(TempString, TextWidth, TextHeight);
                                
                                TextTop = VerticalOffset + DIconTileset->TileHeight() + DBevel->Width() * 4 + HEALTH_HEIGHT + 2;
                                DFonts[fsSmall]->DrawTextWithShadow(drawable, TempGC, 
                                    HorizontalOffset + (DIconTileset->TileWidth()/2 + DBevel->Width()) - TextWidth/2, 
                                    TextTop, 
                                    DFontColorIndices[fsSmall][FG_COLOR], DFontColorIndices[fsSmall][BG_COLOR], 1, TempString);
                                
                                */
                                TempString = "Training: ";
                                DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                                DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                                    HorizontalOffset - TextWidth - DBevel->Width(), 
                                    (VerticalOffset + DIconTileset->TileHeight()/2) - TextHeight/2, 
                                    DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                            }
                            else{
                                int HorizontalOffset = DBevel->Width(), VerticalOffset = DBevel->Width();
                                
                                HorizontalOffset += 2 * (DFullIconWidth + HorizontalGap);
                                VerticalOffset += DFullIconHeight + VerticalGap;
                                
                                DBevel->DrawBevel(drawable, HorizontalOffset, VerticalOffset, DIconTileset->TileWidth(), DIconTileset->TileHeight());
                                DIconTileset->DrawTile(drawable, TempGC, HorizontalOffset, VerticalOffset, DResearchIndices[Command.DCapability], Asset->Color() ? Asset->Color() - 1 : 0);

                                TempString = "Researching: ";
                                DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                                DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                                    HorizontalOffset - TextWidth - DBevel->Width(), 
                                    (VerticalOffset + DIconTileset->TileHeight()/2) - TextHeight/2, 
                                    DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                            }
                            DrawCompletionBar(drawable, TempGC, PercentComplete);
                        }
                    }
                }
                else{
                    if(atGoldMine == Asset->Type()){
                        gint TextLineHeight;
                        
                        TextTop = DIconTileset->TileHeight() + DBevel->Width() * 4 + HEALTH_HEIGHT + 2;
                        
                        TempString = "Gold: ";
                        DFonts[fsMedium]->MeasureText(TempString, TextWidth, TextHeight);
                        TextLineHeight = TextHeight;
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter - TextWidth, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                        
                        TempString = CTextFormatter::IntegerToPrettyString(Asset->Gold());
                        DFonts[fsMedium]->DrawTextWithShadow(drawable, TempGC, 
                            TextCenter, 
                            TextTop, 
                            DFontColorIndices[fsMedium][FG_COLOR], DFontColorIndices[fsMedium][BG_COLOR], 1, TempString);
                    }
                }
            }
        }
        else{
            DDisplayedIcons = 0;
            int HorizontalOffset = DBevel->Width(), VerticalOffset = DBevel->Width();
            for(auto &Item : selectionlist){
                if(auto Asset = Item.lock()){
                    int HPColor = (Asset->HitPoints() - 1) * MAX_HP_COLOR / Asset->MaxHitPoints();
                    gint TextWidth, TextHeight;
                    gint TextTop;
                    std::string TempString;
                    
                    DBevel->DrawBevel(drawable, HorizontalOffset, VerticalOffset, DIconTileset->TileWidth(), DIconTileset->TileHeight());
                    DIconTileset->DrawTile(drawable, TempGC, HorizontalOffset, VerticalOffset, DAssetIndices[Asset->Type()], Asset->Color() ? Asset->Color() - 1 : 0);
                    
                    gdk_gc_set_rgb_fg_color(TempGC, &DHealthRectangleFG);
                    gdk_gc_set_rgb_bg_color(TempGC, &DHealthRectangleBG);
                    gdk_draw_rectangle(drawable, TempGC, TRUE, HorizontalOffset - DBevel->Width(), VerticalOffset + DIconTileset->TileHeight() + DBevel->Width() * 3,  DIconTileset->TileWidth() + DBevel->Width() * 2, HEALTH_HEIGHT + 2);
                    
                    gdk_gc_set_rgb_fg_color(TempGC, &DHealthColors[HPColor]);
                    gdk_gc_set_rgb_bg_color(TempGC, &DHealthColors[HPColor]);
                    gdk_draw_rectangle(drawable, TempGC, TRUE, HorizontalOffset - DBevel->Width() + 1, VerticalOffset + DIconTileset->TileHeight() + DBevel->Width() * 3 + 1,  (DIconTileset->TileWidth() + DBevel->Width() * 2 - 2) * Asset->HitPoints()/ Asset->MaxHitPoints() , HEALTH_HEIGHT);
                    
                    TempString = std::to_string(Asset->HitPoints()) + std::string(" / ") + std::to_string(Asset->MaxHitPoints());
                    DFonts[fsSmall]->MeasureText(TempString, TextWidth, TextHeight);
                    
                    TextTop = VerticalOffset + DIconTileset->TileHeight() + DBevel->Width() * 4 + HEALTH_HEIGHT + 2;
                    DFonts[fsSmall]->DrawTextWithShadow(drawable, TempGC, 
                        HorizontalOffset + (DIconTileset->TileWidth()/2 + DBevel->Width()) - TextWidth/2, 
                        TextTop, 
                        DFontColorIndices[fsSmall][FG_COLOR], DFontColorIndices[fsSmall][BG_COLOR], 1, TempString);
                    
                    
                    HorizontalOffset += DFullIconWidth + HorizontalGap;
                    DDisplayedIcons++;
                    if(0 == (DDisplayedIcons % HorizontalIcons)){
                        HorizontalOffset = DBevel->Width();
                        VerticalOffset +=  DFullIconHeight + VerticalGap;
                    }
                }
            }
        }
        g_object_unref(TempGC);
    }
}

