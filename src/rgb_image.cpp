#include "rgb_image.h"

using namespace std;

RGBImage::RGBImage() : Image(0, 0), pixels(nullptr){}

RGBImage::RGBImage(int width, int height) : Image(width, height), pixels(nullptr){
    allocate(width, height);
}

RGBImage::~RGBImage(){
    deallocate();
}

void RGBImage::allocate(int w, int h) {
    if(w <= 0 || h <= 0){
        pixels = nullptr;
        return;
    }
    pixels = new int**[h];
    for(int i = 0; i < h; ++i) {
        pixels[i] = new int*[w];
        for(int j = 0; j < w; ++j) pixels[i][j] = new int[3]{0};
    }
}

void RGBImage::deallocate() {
    if (pixels) {
        for(int i = 0; i < height; ++i) {
            for(int j = 0; j < width; ++j) delete[] pixels[i][j];
            delete[] pixels[i];
        }
        delete[] pixels;
        pixels = nullptr;
    }
}

bool RGBImage::LoadImage(string filename){
    this->filename = filename;
    deallocate();
    pixels = dataLoader.Load_RGB(filename, &width, &height);
    if (!pixels) {
        width = 0;
        height = 0;
    }
    return (pixels != nullptr);
}

void RGBImage::DumpImage(string filename){
    if (pixels) dataLoader.Dump_RGB(width, height, pixels, filename);
}

void RGBImage::Display_X_Server(){
    if (pixels) dataLoader.Display_RGB_X_Server(width, height, pixels);
}

void RGBImage::Display_ASCII(){
    if (pixels) dataLoader.Display_RGB_ASCII(width, height, pixels);
}

void RGBImage::Display_CMD(){
    dataLoader.Display_RGB_CMD(this->filename);
}

int& RGBImage::operator()(int x, int y, int c) {
    return pixels[max(0, min(y, height-1))][max(0, min(x, width-1))][c];
}

RGBImage& RGBImage::operator=(const RGBImage& other){
    if (this == &other) return *this;
    deallocate();
    width = other.width; height = other.height;
    allocate(width, height);
    for(int y = 0; y < height; ++y)
        for(int x = 0; x < width; ++x)
            for(int c = 0; c < 3; ++c) pixels[y][x][c] = other.pixels[y][x][c];
    return *this;
}

RGBImage* RGBImage::Resize(int new_w, int new_h){
    RGBImage* res = new RGBImage(new_w, new_h);
    float sx = (float)width / new_w, sy = (float)height / new_h;
    for (int y = 0; y < new_h; ++y) {
        for (int x = 0; x < new_w; ++x) {
            // 找到原圖中對應的中心點座標
            int src_x = (int)(x * sx);
            int src_y = (int)(y * sy);
            
            // 確保索引不會變成負數或越界
            src_x = max(0, min(src_x, width - 1));
            src_y = max(0, min(src_y, height - 1));

            for (int c = 0; c < 3; ++c) {
                (*res)(x, y, c) = (*this)(src_x, src_y, c);
            }
        }
    }
    return res;
}

RGBImage* RGBImage::Crop(int x, int y, int w, int h){
    RGBImage* res = new RGBImage(w, h);
    for(int i = 0; i < h; ++i)
        for(int j = 0; j < w; ++j)
            for(int c = 0; c < 3; ++c) (*res)(j, i, c) = (*this)(x + j, y + i, c);
    return res;
}

int RGBImage::getR(int x, int y){
    // 邊界保護，避免存取到陣列外導致錯誤
    if (x < 0 || x >= width || y < 0 || y >= height) return 0; 
    return pixels[y][x][0]; 
}

int RGBImage::getG(int x, int y){
    if (x < 0 || x >= width || y < 0 || y >= height) return 0;
    return pixels[y][x][1];
}

int RGBImage::getB(int x, int y){
    if (x < 0 || x >= width || y < 0 || y >= height) return 0;
    return pixels[y][x][2];
}

void RGBImage::setRGB(int x, int y, int r, int g, int b){
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    pixels[y][x][0] = r;
    pixels[y][x][1] = g;
    pixels[y][x][2] = b;
}