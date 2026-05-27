#ifndef PTHREAD_FOR_PHOTO_MOSAIC_H
#define PTHREAD_FOR_PHOTO_MOSAIC_H

#include "rgb_image.h"
#include "data_loader.h"
#include "photo_mosaic.h"
#include <vector>
#include <string>

using namespace std;

class Pthread_for_Photo_Mosaic: public Photo_Mosaic{
private:
    struct ThreadArg {
        Pthread_for_Photo_Mosaic* instance; // 讓靜態函式可以存取類別成員
        RGBImage* target; // 目標圖片指針
        int start_row; // 該執行緒負責的起始區塊列數
        int end_row; // 該執行緒負責的結束區塊列數
    };

    // Pthread 規定入口函式必須是static且傳回void*
    static void* ProcessWorker(void* arg); 

public:
    Pthread_for_Photo_Mosaic(int size);
    ~Pthread_for_Photo_Mosaic();
    void Process(RGBImage *target) override;
};

#endif