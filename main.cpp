#include <iostream>
#include <string>
#include "image.h"
#include "gray_image.h"
#include "rgb_image.h"
#include "photo_mosaic.h"
#include "bit_field_filter.h"
#include "image_segmentation.h"

using namespace std;

int main(int argc, char *argv[]){
    // Image *step2Img1 = new GrayImage();
    // step2Img1->LoadImage("Image-Folder/mnist/img_100.jpg");
    // step2Img1->DumpImage("output/step2_GrayImage_img_100.jpg");
    // step2Img1->Display_X_Server();
    // step2Img1->Display_CMD();
    
    // Image *step2Img2 = new RGBImage();
    // step2Img2->LoadImage("Image-Folder/lena.jpg");
    // step2Img2->DumpImage("output/step2_RGBImage_lena.jpg");
    // step2Img2->Display_X_Server();
    // step2Img2->Display_CMD();

    // delete step2Img1;
    // delete step2Img2;

    // // some bit field filter design driven code here
    // cout << "\n>>> 開始 Bit Field Filter 測試 <<<\n";
    // Bit_Field_Filter filter;
    // string test_path = "Image-Folder/lena.jpg";

    // // 1. 單獨測試 Box Filter (bit 0 -> 1)
    // RGBImage *step3ImgBox = new RGBImage(0, 0);
    // if (step3ImgBox->LoadImage(test_path)){
    //     filter.Filter(step3ImgBox, 1);
    //     step3ImgBox->DumpImage("output/step3_1_box_lena.jpg");
    //     cout << "[完成] 1. Box Filter -> output/step3_1_box_lena.jpg" << std::endl;
    // }
    // delete step3ImgBox;

    // // 2. 單獨測試 Sobel Gradient (bit 1 -> 2)
    // RGBImage *step3ImgSobel = new RGBImage(0, 0);
    // if (step3ImgSobel->LoadImage(test_path)){
    //     filter.Filter(step3ImgSobel, 2);
    //     step3ImgSobel->DumpImage("output/step3_2_sobel_lena.jpg");
    //     cout << "[完成] 2. Sobel Gradient -> output/step3_2_sobel_lena.jpg" << std::endl;
    // }
    // delete step3ImgSobel;

    // // 3. 單獨測試 Contrast Stretching (bit 2 -> 4)
    // RGBImage *step3ImgContrast = new RGBImage(0, 0);
    // if (step3ImgContrast->LoadImage(test_path)){
    //     filter.Filter(step3ImgContrast, 4);
    //     step3ImgContrast->DumpImage("output/step3_3_contrast_lena.jpg");
    //     cout << "[完成] 3. Contrast Stretching -> output/step3_3_contrast_lena.jpg" << std::endl;
    // }
    // delete step3ImgContrast;

    // // 4. 單獨測試 Mosaic Filter (bit 3 -> 8)
    // RGBImage *step3ImgMosaic = new RGBImage(0, 0);
    // if (step3ImgMosaic->LoadImage(test_path)){
    //     filter.Filter(step3ImgMosaic, 8);
    //     step3ImgMosaic->DumpImage("output/step3_4_mosaic_lena.jpg");
    //     cout << "[完成] 4. Mosaic Filter -> output/step3_4_mosaic_lena.jpg" << std::endl;
    // }
    // delete step3ImgMosaic;

    // // // 5. 疊加測試 (1+2+4+8 = 15)
    // // RGBImage *step3ImgAll = new RGBImage(0, 0);
    // // if (step3ImgAll->LoadImage(test_path)){
    // //     filter.Filter(step3ImgAll, 15);
    // //     step3ImgAll->DumpImage("output/step3_stacked_all_5_lena.jpg");
    // //     cout << "[完成] 5. 疊加效果 (All Filters) -> output/step3_stacked_all_5_lena.jpg" << std::endl;
    // // }
    // // delete step3ImgAll;

    // // some photo mosaic driven code here
    // RGBImage* step4Img = new RGBImage();
    // // lena->LoadImage("lena.jpg");
    // if (!step4Img->LoadImage("Image-Folder/girl_2x.png")) {
    //     cerr << "找不到原始圖片 lena.jpg！" << endl;
    //     delete step4Img;
    //     return -1;
    // }

    // Photo_Mosaic mosaic(16); // 設定磁磚大小為 16x16
    // mosaic.LoadTiles("Image-Folder/cifar10"); // 磁磚資料夾路徑
    // mosaic.Process(step4Img);

    // step4Img->DumpImage("output/step4_photo_mosaic.jpg");
    // cout << "[完成] Photo Mosaic 已輸出至 output/step4_photo_mosaic.jpg" << endl;
    // delete step4Img;

    // more ...
    cout << "\n>>> 開始執行 Image Segmentation 測試 <<<\n";
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
        cout << "[完成] 主體圖已儲存至 output/add1_subject.jpg\n";
        cout << "[完成] 背景圖已儲存至 output/add1_background.jpg\n";
    }
    else {
        cerr << "分割失敗，未產生圖片。\n";
    }
    delete test_img;
    if (subject_only) delete subject_only;
    if (background_only) delete background_only;

    return 0;
}
