#ifndef SINGLE_IMAGE_MOSAIC_H
#define SINGLE_IMAGE_MOSAIC_H

#include "rgb_image.h"
#include "photo_mosaic.h"
#include <vector>
#include <string>

class SingleImageMosaic: public Photo_Mosaic {
public:
    SingleImageMosaic(int size);
    ~SingleImageMosaic();
    void LoadTilesFromSingleImage(RGBImage* source_img); // 從單一張圖片中利用重疊滑動視窗切碎並載入磁磚
    
    void Process(RGBImage* target) override; //執行單圖馬賽克處理 (包含色彩遷移與微幅混合演算法)
};
#endif