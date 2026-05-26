#include <iostream>
#include <string>
#include "image.h"
#include "gray_image.h"
#include "rgb_image.h"
#include "photo_mosaic.h"
#include "bit_field_filter.h"
#include "image_segmentation.h"
#include "single_image_mosaic.h"
#include <chrono>
#include "pthread_for_photo_mosaic.h"
#include "cuda_for_photo_mosaic.h"

using namespace std;

int main(int argc, char *argv[]){
    cout << "============= Step 2: Construct image inheritance and polymorphism =============\n";
    Image *step2Img1 = new GrayImage();
    step2Img1->LoadImage("Image-Folder/mnist/img_100.jpg");
    step2Img1->DumpImage("output/step2_GrayImage_img_100.jpg");
    step2Img1->Display_X_Server();
    step2Img1->Display_CMD();
    cout << "[done]  step2Img1-> output/step2_GrayImage_img_100.jpg\n";
    
    Image *step2Img2 = new RGBImage();
    step2Img2->LoadImage("Image-Folder/lena.jpg");
    step2Img2->DumpImage("output/step2_RGBImage_lena.jpg");
    step2Img2->Display_X_Server();
    step2Img2->Display_CMD();
    cout << "[done]  step2Img2-> output/step2_RGBImage_lena.jpg\n";

    delete step2Img1;
    delete step2Img2;
    cout << "\n";

    // some bit field filter design driven code here
    cout << "============= Step 3: Bit-field with image filter design =============\n";
    Bit_Field_Filter filter;
    string test_path = "Image-Folder/lena.jpg";

    // 1. 測試 Box Filter (bit 0 -> 1)
    RGBImage *step3ImgBox = new RGBImage(0, 0);
    if (step3ImgBox->LoadImage(test_path)){
        filter.Filter(step3ImgBox, 1);
        step3ImgBox->DumpImage("output/step3_1_box_lena.jpg");
        cout << "[done] 1. Box Filter -> output/step3_1_box_lena.jpg\n";
    }
    delete step3ImgBox;

    // 2. 測試 Sobel Gradient (bit 1 -> 2)
    RGBImage *step3ImgSobel = new RGBImage(0, 0);
    if (step3ImgSobel->LoadImage(test_path)){
        filter.Filter(step3ImgSobel, 2);
        step3ImgSobel->DumpImage("output/step3_2_sobel_lena.jpg");
        cout << "[done] 2. Sobel Gradient -> output/step3_2_sobel_lena.jpg\n";
    }
    delete step3ImgSobel;

    // 3. 測試 Contrast Stretching (bit 2 -> 4)
    RGBImage *step3ImgContrast = new RGBImage(0, 0);
    if (step3ImgContrast->LoadImage(test_path)){
        filter.Filter(step3ImgContrast, 4);
        step3ImgContrast->DumpImage("output/step3_3_contrast_lena.jpg");
        cout << "[done] 3. Contrast Stretching -> output/step3_3_contrast_lena.jpg\n";
    }
    delete step3ImgContrast;

    // 4. 測試 Mosaic Filter (bit 3 -> 8)
    RGBImage *step3ImgMosaic = new RGBImage(0, 0);
    if (step3ImgMosaic->LoadImage(test_path)){
        filter.Filter(step3ImgMosaic, 8);
        step3ImgMosaic->DumpImage("output/step3_4_mosaic_lena.jpg");
        cout << "[done] 4. Mosaic Filter -> output/step3_4_mosaic_lena.jpg\n";
    }
    delete step3ImgMosaic;
    cout << "\n";

    cout << "================== Step 4: Photo Mosaic ==================\n";
    auto start_time = chrono::high_resolution_clock::now();
    RGBImage* step4Img = new RGBImage();
    // lena->LoadImage("lena.jpg");
    if (!step4Img->LoadImage("Image-Folder/girl_2x.png")) {
        cerr << "找不到原始圖片 lena.jpg！" << endl;
        delete step4Img;
        return -1;
    }

    Photo_Mosaic mosaic(16); // 設定磁磚大小為 16x16
    mosaic.LoadTiles("Image-Folder/cifar10");
    mosaic.Process(step4Img);

    step4Img->DumpImage("output/step4_photo_mosaic.jpg");
    cout << "[done] Photo Mosaic -> output/step4_photo_mosaic.jpg\n";
    delete step4Img;

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end_time - start_time;
    cout << "運算耗時: " << elapsed.count() << " 秒\n";
    cout << "\n";

    // more ...
    cout << "============= Additional features 1: Image Segamentation =============\n";
    RGBImage* test_img = new RGBImage();
    if (!test_img->LoadImage("Image-Folder/bunny.png")) {
        cerr << "找不到測試圖片 bunny.png！\n";
        delete test_img;
        return -1;
    }

    RGBImage* subject_only = nullptr;
    RGBImage* background_only = nullptr;

    ImageSegmentation seg(50);
    cout << "正在分離主體與背景...\n";
    seg.Segment(test_img, subject_only, background_only);

    if (subject_only && background_only) {
        subject_only->DumpImage("output/add1_subject.jpg");
        background_only->DumpImage("output/add1_background.jpg");
        cout << "[done] Image Segmentation -> output/add1_subject.jpg\n";
        cout << "[done] Image Segmentation -> output/add1_background.jpg\n";
    }
    else {
        cerr << "分割失敗，未產生圖片。\n";
    }
    delete test_img;
    if (subject_only) delete subject_only;
    if (background_only) delete background_only;

    cout << "============= Additional features 2: Single Image Mosaic =============\n";
    RGBImage* target_img = new RGBImage();
    if (!target_img->LoadImage("Image-Folder/girl_2x.png")) {
        cerr << "無法載入目標圖片\n";
        return -1;
    }

    RGBImage* single_material = new RGBImage();
    if (!single_material->LoadImage("Image-Folder/ranbow_taipei.png")) {
        cerr << "無法載入素材圖片\n";
        delete target_img;
        return -1;
    }

    int tileSize = 16; 
    SingleImageMosaic mosaic_solver(tileSize);

    mosaic_solver.LoadTilesFromSingleImage(single_material);

    // blend_ratio 建議設在 0.1 ~ 0.3。數字越低，細看時單圖的特徵越明顯；數字越高，遠看時大圖輪廓越清晰。
    double blend_ratio = 0.2; 
    mosaic_solver.Process(target_img, blend_ratio);

    target_img->DumpImage("output/add2_single_image_mosaic.jpg");

    delete target_img;
    delete single_material;
    cout << "[done] Single Image Mosaic -> output/add2_single_image_mosaic.jpg\n";
    
    cout << "============= Additional features 3: Pthread for Photo Mosaic =============\n";
    auto start_time_pthread = chrono::high_resolution_clock::now();
    RGBImage* add3Img = new RGBImage();
    if (!add3Img->LoadImage("Image-Folder/girl_2x.png")) {
        cerr << "找不到原始圖片 lena.jpg！" << endl;
        delete add3Img;
        return -1;
    }

    Pthread_for_Photo_Mosaic mosaic_pthread(16); // 設定磁磚大小為 16x16
    mosaic_pthread.LoadTiles("Image-Folder/cifar10");
    mosaic_pthread.Process(add3Img);

    add3Img->DumpImage("output/add3_pthread_photo_mosaic.jpg");
    cout << "[done] Photo Mosaic -> output/add3_pthread_photo_mosaic.jpg\n";
    delete add3Img;

    auto end_time_pthread = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed_pthread = end_time_pthread - start_time_pthread;

    cout << "運算耗時: " << elapsed_pthread.count() << " 秒\n";
    cout << "\n";

    cout << "============= Additional features 4: CUDA for Photo Mosaic =============\n";
    auto start_time_cuda = chrono::high_resolution_clock::now();
    RGBImage* add4Img = new RGBImage();
    if (!add4Img->LoadImage("Image-Folder/girl_2x.png")) {
        cerr << "找不到原始圖片 lena.jpg！" << endl;
        delete add4Img;
        return -1;
    }

    CUDA_for_Photo_Mosaic mosaic_cuda(16); // 設定磁磚大小為 16x16
    mosaic_cuda.LoadTiles("Image-Folder/cifar10");
    mosaic_cuda.Process(add4Img);

    add4Img->DumpImage("output/add4_cuda_photo_mosaic.jpg");
    cout << "[done] Photo Mosaic -> output/add4_cuda_photo_mosaic.jpg\n";

    delete add4Img;

    auto end_time_cuda = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed_cuda = end_time_cuda - start_time_cuda;

    cout << "運算耗時: " << elapsed_cuda.count() << " 秒\n";
    cout << "\n";

    return 0;
}
