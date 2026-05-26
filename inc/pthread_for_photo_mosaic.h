#ifndef PTHREAD_FOR_PHOTO_MOSAIC_H
#define PTHREAD_FOR_PHOTO_MOSAIC_H

#include "rgb_image.h"
#include "data_loader.h"
#include <vector>
#include <string>

class Pthread_for_Photo_Mosaic {
private:
    int tileSize;
    struct TileInfo {
        RGBImage* tile_img;
        int avgR, avgG, avgB;
    };
    std::vector<TileInfo> tile_db;

    // ====== Pthread 所需的私有結構與靜態函式 ======
    struct ThreadArg {
        Pthread_for_Photo_Mosaic* instance; // 讓靜態函式可以存取類別成員
        RGBImage* target;       // 目標圖片指針
        int start_row;          // 該執行緒負責的起始區塊列數
        int end_row;            // 該執行緒負責的結束區塊列數
    };

    // Pthread 規定入口函式必須是 static 且傳回 void*
    static void* ProcessWorker(void* arg); 
    // ===========================================

public:
    Pthread_for_Photo_Mosaic(int size);
    ~Pthread_for_Photo_Mosaic();
    void CalculateAverageColor(RGBImage* img, int &r, int &g, int &b);
    void LoadTiles(std::string folder_path);
    void Process(RGBImage *target);
};

#endif