#include "pthread_for_photo_mosaic.h"
#include <iostream>
#include <algorithm>
#include <pthread.h>

using namespace std;

Pthread_for_Photo_Mosaic::Pthread_for_Photo_Mosaic(int size) : Photo_Mosaic(size) {}
Pthread_for_Photo_Mosaic::~Pthread_for_Photo_Mosaic() {}

void Pthread_for_Photo_Mosaic::Process(RGBImage *target){
    if(tile_db.empty()){
        cerr << "Error: Tile database is empty!\n";
        return;
    }

    int height = target->get_height();
    int total_block_rows = (height + tileSize - 1) / tileSize;

    int num_threads = 4; 
    pthread_t threads[num_threads];

    cout << "Initializing " << num_threads << " Pthreads for parallel processing...\n";

    // 平均分配任務
    for(int i = 0; i < num_threads; i++) {
        // 用Heap配置參數
        ThreadArg* arg = new ThreadArg(); 
        arg->instance = this;
        arg->target = target;
        
        // 切分的是區塊列的索引範圍
        arg->start_row = i * total_block_rows / num_threads;
        arg->end_row = (i + 1) * total_block_rows / num_threads;

        // 建立執行緒並傳遞Heap指標
        pthread_create(&threads[i], nullptr, Pthread_for_Photo_Mosaic::ProcessWorker, arg);
    }

    // 等待所有執行緒完成
    for(int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], nullptr);
    }

    cout << "Photo Mosaic parallel processing completed!\n";
}

void* Pthread_for_Photo_Mosaic::ProcessWorker(void* arg) {
    // 取得Heap上的參數
    ThreadArg* t_arg = (ThreadArg*)arg;
    Pthread_for_Photo_Mosaic* mosaic = t_arg->instance;
    RGBImage* target = t_arg->target;

    int width = target->get_width();
    int height = target->get_height();
    int size = mosaic->tileSize;

    //
    for(int b_row = t_arg->start_row; b_row < t_arg->end_row; b_row++) {
        int by = b_row * size; // 還原像素 Y 座標
        
        for(int bx = 0; bx < width; bx += size) {
            
            int currentW = min(size, width - bx);
            int currentH = min(size, height - by);
            
            // 裁剪區塊並計算色彩
            RGBImage* block = target->Crop(bx, by, currentW, currentH);
            int targetR, targetG, targetB;
            mosaic->CalculateAverageColor(block, targetR, targetG, targetB);
            delete block;

            int best_idx = 0;
            long long min_dist = -1;

            // 尋找最佳磁磚
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

            // 寫入目標像素
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

    delete t_arg; 
    return nullptr;
}