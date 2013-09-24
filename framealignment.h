#ifndef FRAMEALIGNMENT_H
#define FRAMEALIGNMENT_H

#define _CVS_PIXEL_NEIGHBORHOOD_DIST    3
#define _CVS_IS_PIXEL_DIFFERENT_THRES  50
#define _CVS_IS_PIXEL_DIFFERENT_THRES_SQUARE 2500

#include <opencv2/opencv.hpp>
#include <vector>
#include "exception.h"


class FrameAlignmentSettings
{
public:
    int dummy;
};

class FrameAlignment
{
public:
    FrameAlignment();
    void set(FrameAlignmentSettings& _settings);
    void getSettings(FrameAlignmentSettings& _settings);
    void saveSettings();
    bool loadSettings();
    void process(cv::Mat& inputImage,cv::Mat& homography,cv::Mat &outputImage);

    /* TODO: daha sonra private a çekilecek */
    void calculateBinaryDiffImageAccording2pixelNeighborhood(cv::Mat &image1,
                                                             cv::Mat &image2,
                                                             cv::Mat &outputImage);

private:
    FrameAlignmentSettings settings;
    Exception               exc;


};

#endif // FRAMEALIGNMENT_H
