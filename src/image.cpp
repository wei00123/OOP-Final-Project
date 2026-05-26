#include "image.h"
#include "data_loader.h"

using namespace std;

Image::Image(int width, int height) : width(width), height(height){}

Image::~Image(){}

bool Image::LoadImage(string filename){
    return false;
}

void Image::DumpImage(string filename){};

void Image::Display_X_Server(){};

void Image::Display_ASCII(){};

void Image::Display_CMD(){};

Image* Image::Resize(int new_w, int new_h){
    return 0;
}

Image* Image::Crop(int x, int y, int w, int h){
    return 0;
}

int Image::get_width() const{
    return width;
}

int Image::get_height() const{
    return height;
}