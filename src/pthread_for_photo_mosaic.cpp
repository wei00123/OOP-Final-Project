#include "pthread_for_photo_mosaic.h"
#include <iostream>
#include <algorithm>
#include <pthread.h>

using namespace std;

Pthread_for_Photo_Mosaic::Pthread_for_Photo_Mosaic(int size) : Photo_Mosaic(size) {}
Pthread_for_Photo_Mosaic::~Pthread_for_Photo_Mosaic() {}

// 計算一張圖的平均顏色
void Pthread_for_Photo_Mosaic::Process(RGBImage *target){
    if(tile_db.empty()){
        cerr << "Error: Tile database is empty!\n";
        return;
    }

    int height = target->get_height();
    
    // 計算Y軸方向總共有多少個區塊列
    int total_block_rows = (height + tileSize - 1) / tileSize;

    int num_threads = 4; // 定義要啟動 4 個執行緒
    pthread_t threads[num_threads]; // 宣告陣列，用來儲存每個執行緒的ID憑證
    ThreadArg thread_args[num_threads]; // 宣告陣列，用來儲存每個執行緒的參數 (包含指向類別實例的指標、目標圖指標、負責的區塊列範圍)

    cout << "Initializing " << num_threads << " Pthreads for parallel processing...\n";

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

    // 等待所有執行緒完成
    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }

    cout << "Photo Mosaic parallel processing completed!\n";
}

// Pthread執行緒的工作函式，負責處理分配給它的區塊列
void* Pthread_for_Photo_Mosaic::ProcessWorker(void* arg) {
    ThreadArg* t_arg = (ThreadArg*)arg;
    Pthread_for_Photo_Mosaic* mosaic = t_arg->instance;
    RGBImage* target = t_arg->target;

    int width = target->get_width();
    int height = target->get_height();
    int size = mosaic->tileSize;

    // 將區塊列轉回實際的像素Y座標範圍
    int start_by = t_arg->start_row * size;
    int end_by = min(height, t_arg->end_row * size);

    // 每個執行緒只掃描自己被分配到的Y軸區間
    for(int by = start_by; by < end_by; by += size){
        for(int bx = 0; bx < width; bx += size){
            
            int currentW = min(size, width - bx);
            int currentH = min(size, height - by);
            
            // 裁剪區塊並計算色彩(此處block創在各執行緒的Stack/Heap中，互不干擾)
            RGBImage* block = target->Crop(bx, by, currentW, currentH);
            int targetR, targetG, targetB;
            mosaic->CalculateAverageColor(block, targetR, targetG, targetB);
            delete block;

            int best_idx = 0;
            long long min_dist = -1;

            // 尋找最佳磁磚(唯讀資料，併發存取安全)
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

            // 寫入目標像素(各執行緒負責的by區間不同，絕對不會寫到同一個坐標)
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