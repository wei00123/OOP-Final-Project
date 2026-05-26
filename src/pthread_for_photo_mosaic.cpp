#include "pthread_for_photo_mosaic.h"
#include <iostream>
#include <algorithm>
#include <pthread.h>

using namespace std;

Pthread_for_Photo_Mosaic::Pthread_for_Photo_Mosaic(int size) : tileSize(size){}

Pthread_for_Photo_Mosaic::~Pthread_for_Photo_Mosaic(){
    for (auto &tile : tile_db){
        delete tile.tile_img;
    }
    tile_db.clear();
}

void Pthread_for_Photo_Mosaic::CalculateAverageColor(RGBImage* img, int &r, int &g, int &b){
    if (!img) return;
    long long sumR = 0, sumG = 0, sumB = 0;
    int w = img->get_width();
    int h = img->get_height();
    int count = w * h;

    if (w <= 0 || h <= 0) {
        r = 0; g = 0; b = 0;
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

void Pthread_for_Photo_Mosaic::LoadTiles(string folder_path){
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

// =================================================================
// 3. 多執行緒主控端：Process 函式
// =================================================================
void Pthread_for_Photo_Mosaic::Process(RGBImage *target){
    if(tile_db.empty()){
        cerr << "Error: Tile database is empty!\n";
        return;
    }

    int height = target->get_height();
    
    // 計算 Y 軸方向總共有多少個區塊列
    int total_block_rows = (height + tileSize - 1) / tileSize;

    // 定義要啟動的核心執行緒數量 (通常設 4 或 8，依你的 CPU 而定)
    int num_threads = 4; 
    pthread_t threads[num_threads];
    ThreadArg thread_args[num_threads];

    cout << "正在啟動 " << num_threads << " 個 Pthread 進行多核心平行加速...\n";

    // 平均分配任務
    for(int i = 0; i < num_threads; i++) {
        thread_args[i].instance = this;
        thread_args[i].target = target;
        
        // 區間切分邏輯
        thread_args[i].start_row = i * total_block_rows / num_threads;
        thread_args[i].end_row = (i + 1) * total_block_rows / num_threads;

        // 建立並運作執行緒
        pthread_create(&threads[i], nullptr, Pthread_for_Photo_Mosaic::ProcessWorker, &thread_args[i]);
    }

    // 等待所有執行緒完成 (Join)
    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }

    cout << "Photo Mosaic 平行運算完成！\n";
}

// =================================================================
// 4. 執行緒工作者：ProcessWorker (實際幹活的靜態函式)
// =================================================================
void* Pthread_for_Photo_Mosaic::ProcessWorker(void* arg) {
    ThreadArg* t_arg = (ThreadArg*)arg;
    Pthread_for_Photo_Mosaic* mosaic = t_arg->instance;
    RGBImage* target = t_arg->target;

    int width = target->get_width();
    int height = target->get_height();
    int size = mosaic->tileSize;

    // 將區塊列轉回實際的像素 Y 座標範圍
    int start_by = t_arg->start_row * size;
    int end_by = min(height, t_arg->end_row * size);

    // 每個執行緒只掃描自己被分配到的 Y 軸區間
    for(int by = start_by; by < end_by; by += size){
        for(int bx = 0; bx < width; bx += size){
            
            int currentW = min(size, width - bx);
            int currentH = min(size, height - by);
            
            // 裁剪區塊並計算色彩 (此處 block 創在各執行緒的 Stack/Heap 中，互不干擾)
            RGBImage* block = target->Crop(bx, by, currentW, currentH);
            int targetR, targetG, targetB;
            mosaic->CalculateAverageColor(block, targetR, targetG, targetB);
            delete block;

            int best_idx = 0;
            long long min_dist = -1;

            // 尋找最佳磁磚 (唯讀資料，併發存取安全)
            for(size_t i = 0; i < mosaic->tile_db.size(); i++){
                long long dr = targetR - mosaic->tile_db[i].avgR;
                long long dg = targetG - mosaic->tile_db[i].avgG;
                long long db = targetB - mosaic->tile_db[i].avgB;
                long long dist = dr*dr + dg*dg + db*db;

                if(min_dist == -1 || dist < min_dist){
                    min_dist = dist;
                    best_idx = i;
                }
            }

            // 寫入目標像素 (各執行緒負責的 by 區間不同，絕對不會寫到同一個坐標)
            RGBImage* best_tile = mosaic->tile_db[best_idx].tile_img;
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
    return nullptr;
}