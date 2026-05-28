#ifndef _GRAY_IMAGE_H_
#define _GRAY_IMAGE_H_

#include "image.h"

class GrayImage : public Image{
private:
    int **pixels; // // 二維動態陣列：pixels[y][x]存取像素值
    void allocate(int w, int h); // 分配記憶體
    void deallocate(); // 釋放記憶體
public:
    GrayImage();
    GrayImage(int width, int height);
    ~GrayImage();

    bool LoadImage(string filename) override;
    void DumpImage(string filename) override;
    void Display_X_Server() override;
    void Display_ASCII() override;
    void Display_CMD() override;

    int& operator()(int x, int y); // 以(x,y)存取像素值
    GrayImage& operator=(const GrayImage& other); // 賦值
    GrayImage* Resize(int new_w, int new_h) override; // 縮放
    GrayImage* Crop(int x, int y, int w, int h) override; // 裁切

};

#endif