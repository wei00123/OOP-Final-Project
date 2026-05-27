#include "single_image_mosaic.h"
#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;

SingleImageMosaic::SingleImageMosaic(int size): Photo_Mosaic(size){}
SingleImageMosaic::~SingleImageMosaic(){}

// 利用重疊滑動視窗(Sliding Window)從單張圖片分出很多小圖
void SingleImageMosaic::LoadTilesFromSingleImage(RGBImage* source_img){
    if (!source_img) return;

    int srcW = source_img->get_width();
    int srcH = source_img->get_height();
    
    // 為了讓磁磚庫夠豐富，我們讓視窗重疊滑動(步長為tileSize的一半)
    int step = tileSize / 2;
    if(step < 1) step = 1;

    cout << "Analyzing single image and cutting into tiles...\n";

    for(int y = 0; y <= srcH - tileSize; y += step){
        for(int x = 0; x <= srcW - tileSize; x += step){

            // 裁剪出局部方塊
            RGBImage* raw_tile = source_img->Crop(x, y, tileSize, tileSize);
            if(!raw_tile) continue;

            int ar, ag, ab;
            CalculateAverageColor(raw_tile, ar, ag, ab);

            TileInfo info;
            info.tile_img = raw_tile; // 儲存切下來的方塊
            info.avgR = ar; 
            info.avgG = ag; 
            info.avgB = ab;
            
            tile_db.push_back(info);
        }
    }
    cout << "Successfully extracted " << tile_db.size() << " structural tiles from the single image!\n";
}


// 執行馬賽克渲染
void SingleImageMosaic::Process(RGBImage* target){
    if(tile_db.empty()){
        cerr << "Error: No tiles loaded!\n";
        return;
    }

    int width = target->get_width();
    int height = target->get_height();

    cout << "Analyzing single image and rendering mosaic...\n";

    for(int by = 0; by < height; by += tileSize){
        for(int bx = 0; bx < width; bx += tileSize){
            
            int currentW = min(tileSize, width - bx);
            int currentH = min(tileSize, height - by);
            
            // 取得目標區塊與平均顏色
            RGBImage* block = target->Crop(bx, by, currentW, currentH);
            int targetR, targetG, targetB;
            CalculateAverageColor(block, targetR, targetG, targetB);
            delete block;

            // 尋找結構最合適的母體磁磚 (以顏色最接近者為基底，減少色彩遷移時的溢位)
            int best_idx = 0;
            long long min_dist = -1;

            for(size_t i = 0; i < tile_db.size(); i++){
                long long dr = targetR - tile_db[i].avgR;
                long long dg = targetG - tile_db[i].avgG;
                long long db = targetB - tile_db[i].avgB;
                long long dist = dr*dr + dg*dg + db*db;

                if(min_dist == -1 || dist < min_dist){
                    min_dist = dist;
                    best_idx = i;
                }
            }

            RGBImage* best_tile = tile_db[best_idx].tile_img;
            int tileAvgR = tile_db[best_idx].avgR;
            int tileAvgG = tile_db[best_idx].avgG;
            int tileAvgB = tile_db[best_idx].avgB;

            // 換成最相似的小圖
            for(int ty = 0; ty < currentH; ty++){
                for(int tx = 0; tx < currentW; tx++){
                    int finalR = best_tile->getR(tx, ty);
                    int finalG = best_tile->getG(tx, ty);
                    int finalB = best_tile->getB(tx, ty);

                    target->setRGB(bx + tx, by + ty, finalR, finalG, finalB);
                }
            }
        }
    }
}