#include "gray_image.h"

using namespace std;

GrayImage::GrayImage() : Image(0, 0), pixels(nullptr){}

GrayImage::GrayImage(int width, int height) : Image(width, height), pixels(nullptr){
    allocate(width, height);
}

GrayImage::~GrayImage(){
    deallocate();
}

void GrayImage::allocate(int w, int h) {
    pixels = new int*[h];
    for(int i = 0; i < h; ++i) pixels[i] = new int[w]{0};
}

void GrayImage::deallocate() {
    if (pixels) {
        for(int i = 0; i < height; ++i) delete[] pixels[i];
        delete[] pixels;
        pixels = nullptr;
    }
}

bool GrayImage::LoadImage(string filename){
    this->filename = filename;
    pixels = dataLoader.Load_Gray(filename, &width, &height);
    return (pixels != nullptr);
}

void GrayImage::DumpImage(string filename){
    if (pixels) dataLoader.Dump_Gray(width, height, pixels, filename);
}

void GrayImage::Display_X_Server(){
    if (pixels) dataLoader.Display_Gray_X_Server(width, height, pixels);
}

void GrayImage::Display_ASCII(){
    if (pixels) dataLoader.Display_Gray_ASCII(width, height, pixels);
}

void GrayImage::Display_CMD(){
    dataLoader.Display_Gray_CMD(this->filename);
}

int& GrayImage::operator()(int x, int y){
    // 加了邊界檢查，確保不會存取越界
    return pixels[max(0, min(y, height-1))][max(0, min(x, width-1))];
}   


GrayImage& GrayImage::operator=(const GrayImage& other){
    if(this == &other) return *this;
    deallocate();
    width = other.width; height = other.height;
    allocate(width, height);
    for(int y = 0; y < height; ++y)
        for(int x = 0; x < width; ++x) pixels[y][x] = other.pixels[y][x];
    return *this;
}

GrayImage* GrayImage::Resize(int new_w, int new_h){
    GrayImage* res = new GrayImage(new_w, new_h);
    float sx = (float)width / new_w, sy = (float)height / new_h;
    for(int y = 0; y < new_h; ++y)
        for(int x = 0; x < new_w; ++x) (*res)(x, y) = (*this)((int)(x*sx), (int)(y*sy));
    return res;
}

GrayImage* GrayImage::Crop(int x, int y, int w, int h){
    GrayImage* res = new GrayImage(w, h);
    for(int i = 0; i < h; ++i)
        for(int j = 0; j < w; ++j) (*res)(j, i) = (*this)(x + j, y + i);
    return res;
}