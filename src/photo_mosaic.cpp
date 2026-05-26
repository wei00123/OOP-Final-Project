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

void Photo_Mosaic::CalculateAverageColor(RGBImage* img, int &r, int &g, int &b){
    if (!img) return;
    long long sumR = 0, sumG = 0, sumB = 0;
    int w = img->get_width();
    int h = img->get_height();
    int count = w * h;

    if (w <= 0 || h <= 0) {
        r = 0;
        g = 0;
        b = 0;
        return;
    }

    for (int y = 0; y < h; y++){
        for (int x = 0; x < w; x++){
            sumR += img->getR(x, y);
            sumG += img->getG(x, y);
            sumB += img->getB(x, y);
        }
    }
    r = (int)(sumR / count);
    g = (int)(sumG / count);
    b = (int)(sumB / count);
}

void Photo_Mosaic::LoadTiles(string folder_path){
    vector<string> all_files;
    Data_Loader loader;

    if(loader.List_Directory(folder_path, all_files) != 0) {
        cerr << "無法讀取資料夾: " << folder_path << "\n";
        return;
    }

    cout << "正在從 " << folder_path << " 載入磁磚...\n";

    for(const string& full_path : all_files){
        string ext = "";
        size_t dot_idx = full_path.find_last_of(".");
        if(dot_idx != string::npos){
            ext = full_path.substr(dot_idx + 1);
            for(auto &c : ext) c = tolower(c);
        }

        if(ext == "jpg" || ext == "jpeg" || ext == "png"){
            RGBImage* raw_tile = new RGBImage();
            
            if(raw_tile->LoadImage(full_path)){
                RGBImage* resized_tile = raw_tile->Resize(tileSize, tileSize);
                
                if (resized_tile == nullptr) {
                    delete raw_tile;
                    continue;
                }

                int ar, ag, ab;
                CalculateAverageColor(resized_tile, ar, ag, ab);
                
                TileInfo info;
                info.tile_img = resized_tile;
                info.avgR = ar; info.avgG = ag; info.avgB = ab;
                tile_db.push_back(info);
                
                if(tile_db.size() % 100 == 0){
                    cout << "\r已載入 " << tile_db.size() << " 張磁磚..." << flush;
                }
            }
            delete raw_tile; 
        }
    }
    cout << "\n載入完成！共計 " << tile_db.size() << " 張有效磁磚。\n";
}

void Photo_Mosaic::Process(RGBImage *target){
    if(tile_db.empty()){
        cerr << "Error: Tile database is empty!\n";
        return;
    }

    int width = target->get_width();
    int height = target->get_height();

    cout << "Generating Photo Mosaic...\n";

    for(int by = 0; by < height; by += tileSize){
        for(int bx = 0; bx < width; bx += tileSize){
            
            int currentW = min(tileSize, width - bx);
            int currentH = min(tileSize, height - by);
            
            RGBImage* block = target->Crop(bx, by, currentW, currentH);
            int targetR, targetG, targetB;
            CalculateAverageColor(block, targetR, targetG, targetB);
            delete block;

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