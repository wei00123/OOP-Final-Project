#ifndef _RGB_IMAGE_H_
#define _RGB_IMAGE_H_

#include "image.h"

class RGBImage : public Image{
private:
    int ***pixels;
    void allocate(int w, int h);
    void deallocate();
public:
    RGBImage();
    RGBImage(int width, int height);
    ~RGBImage();

    bool LoadImage(string filename) override;
    void DumpImage(string filename) override;
    void Display_X_Server() override;
    void Display_ASCII() override;
    void Display_CMD() override;
    
    int& operator()(int x, int y, int c);
    RGBImage& operator=(const RGBImage& other);
    RGBImage* Resize(int new_w, int new_h) override;
    RGBImage* Crop(int x, int y, int w, int h) override;
    
    int getR(int x, int y);
    int getG(int x, int y);
    int getB(int x, int y);
    void setRGB(int x, int y, int r, int g, int b);
};

#endif