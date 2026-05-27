#include "photo_mosaic.h"
#include <iostream>
#include <algorithm>

using namespace std;

Photo_Mosaic::Photo_Mosaic(int size) : tileSize(size){}

Photo_Mosaic::~Photo_Mosaic(){
    for (auto &tile : tile_db){
        delete tile.tile_img;
    }
    tile_db.clear();
}

// 計算一張圖的平均顏色
void Photo_Mosaic::CalculateAverageColor(RGBImage* img, int &r, int &g, int &b){
    if (!img) return; // 如果傳入空指標則直接返回
    long long sumR = 0, sumG = 0, sumB = 0;
    int w = img->get_width();
    int h = img->get_height();
    int count = w * h; // 計算整張圖的總像素數量

    // 如果圖像尺寸無效，則返回黑色
    if (w <= 0 || h <= 0) { 
        r = 0;
        g = 0;
        b = 0;
        return;
    }

    // 看整張圖片的每一個像素，累加 R、G、B 的值
    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            sumR += img->getR(x, y);
            sumG += img->getG(x, y);
            sumB += img->getB(x, y);
        }
    }

    // 將總和除以總像素數量，得到平均值，並透過傳址(Reference)回傳給 r, g, b
    r = (int)(sumR / count);
    g = (int)(sumG / count);
    b = (int)(sumB / count);
}

// 前置作業，把素材資料夾裡的圖轉化為可以使用的小圖資料庫
void Photo_Mosaic::LoadTiles(string folder_path){
    vector<string> all_files;
    Data_Loader loader;

    // 讀取資料夾內所有檔案的路徑，存入 all_files。若失敗則報錯並返回。
    if(loader.List_Directory(folder_path, all_files) != 0) {
        cerr << "Error: Cannot read folder:" << folder_path << "\n";
        return;
    }

    cout << "Loading tiles from" << folder_path << "...\n";

    for(const string& full_path : all_files){
        string ext = "";
        size_t dot_idx = full_path.find_last_of("."); // 找出檔名中最後一個小數點的位置
        if(dot_idx != string::npos){
            ext = full_path.substr(dot_idx + 1); // 擷取副檔名
            for(auto &c : ext) c = tolower(c); // 將副檔名全部轉為小寫，方便後續比對
        }

        if(ext == "jpg" || ext == "jpeg" || ext == "png"){
            RGBImage* raw_tile = new RGBImage();
            
            // 嘗試載入圖片，若成功則進行後續處理；若失敗則釋放記憶體並繼續下一個檔案
            if(raw_tile->LoadImage(full_path)){
                // 強制將圖片縮放成預設的正方形尺寸 (tileSize x tileSize)
                RGBImage* resized_tile = raw_tile->Resize(tileSize, tileSize);
                
                // 如果縮放失敗，清理記憶體並跳過這張圖
                if (resized_tile == nullptr) {
                    delete raw_tile;
                    continue;
                }

                int ar, ag, ab;
                CalculateAverageColor(resized_tile, ar, ag, ab); // 計算這張縮放後小圖的平均顏色
                
                // 將圖片指標與平均顏色打包成 TileInfo 結構，並加入 tile_db 資料庫中
                TileInfo info;
                info.tile_img = resized_tile;
                info.avgR = ar; info.avgG = ag; info.avgB = ab;
                tile_db.push_back(info);
                
                // 每載入 100 張圖就輸出一次進度
                if(tile_db.size() % 100 == 0){
                    cout << "\rLoaded " << tile_db.size() << " tiles..." << flush;
                }
            }
            delete raw_tile; 
        }
    }
    cout << "\nLoading completed! Total " << tile_db.size() << " tiles.\n";
}

// 將目標圖替換成很多小圖
void Photo_Mosaic::Process(RGBImage *target){
    // 如果圖庫是空的，根本沒東西可以拼，直接結束
    if(tile_db.empty()){
        cerr << "Error: Tile database is empty!\n";
        return;
    }

    int width = target->get_width();
    int height = target->get_height();

    cout << "Generating Photo Mosaic...\n";

    // 以tileSize為單位，將目標圖切成一塊一塊的區塊，對每個區塊找出最相似的小圖來替換
    for(int by = 0; by < height; by += tileSize){
        for(int bx = 0; bx < width; bx += tileSize){
            
            // 計算目前區塊的實際寬高。使用min是為了防止圖片邊緣無法被tileSize整除而發生越界
            int currentW = min(tileSize, width - bx);
            int currentH = min(tileSize, height - by);
            
            // 從大圖中將這個區裁切出來
            RGBImage* block = target->Crop(bx, by, currentW, currentH);
            int targetR, targetG, targetB;
            CalculateAverageColor(block, targetR, targetG, targetB); // 計算這個區塊的平均顏色
            delete block;

            // 在圖庫中尋找顏色最相近的小圖
            int best_idx = 0;
            long long min_dist = -1;
            for(size_t i = 0; i < tile_db.size(); i++){
                // 計算目標區塊顏色與圖庫中第i張圖顏色的差異
                long long dr = targetR - tile_db[i].avgR;
                long long dg = targetG - tile_db[i].avgG;
                long long db = targetB - tile_db[i].avgB;
                // 計算色彩空間中的歐氏距離平方
                long long dist = dr*dr + dg*dg + db*db;

                if(min_dist == -1 || dist < min_dist){
                    min_dist = dist;
                    best_idx = i;
                }
            }

            // 找到最相似的小圖後，將它貼到目標圖的對應位置上
            RGBImage* best_tile = tile_db[best_idx].tile_img;
            for(int ty = 0; ty < currentH; ty++){
                for(int tx = 0; tx < currentW; tx++){
                    target->setRGB(bx + tx, by + ty, 
                                   best_tile->getR(tx, ty), 
                                   best_tile->getG(tx, ty), 
                                   best_tile->getB(tx, ty));
                }
            }
        }
    }
    cout << "Photo Mosaic completed!\n";
}