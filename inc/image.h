#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <iostream>
#include <string>
#include <algorithm>
#include "data_loader.h"

using namespace std;

class Image{
protected:
    int width; // 圖片寬度
    int height; // 圖片高度
    string filename; // 圖片來源檔案路徑
    Data_Loader dataLoader; //處理圖片I/O的物件

public:
    Image(int width, int height);
    virtual ~Image();

    virtual bool LoadImage(string filename); // 從檔案載入圖片
    virtual void DumpImage(string filename); // 將圖片輸出到檔案
    virtual void Display_X_Server(); // 在 X Server 上顯示圖片
    virtual void Display_ASCII(); // 以 ASCII 字元在終端機顯示圖片
    virtual void Display_CMD(); // 在終端機顯示圖片資訊
    
    virtual Image* Resize(int new_w, int new_h);
    virtual Image* Crop(int x, int y, int w, int h);

    int get_width() const;
    int get_height() const;
};

#endif