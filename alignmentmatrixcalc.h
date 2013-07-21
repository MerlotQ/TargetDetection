#ifndef ALIGNMENTMATRIXCALC_H
#define ALIGNMENTMATRIXCALC_H

#include "opencv2/nonfree/nonfree.hpp"
#include <opencv2/opencv.hpp>
#include "exception.h"

enum HomograpyMethod
{
    featureBased,
    flowBased
};



class AlignmentMatrixCalcSettings
{
public:
    int dummy;
};

class AlignmentMatrixCalc
{
public:
    AlignmentMatrixCalc();
    void set(AlignmentMatrixCalcSettings& _settings);
    void getSettings(AlignmentMatrixCalcSettings& _settings);
    void saveSettings();
    bool loadSettings();
    void process(cv::Mat& inputImage);
    bool getHomography(cv::Mat& gHomography);

private:
    AlignmentMatrixCalcSettings settings;
    Exception                   exc;

    //
    cv::Mat currentFrame;
    cv::Mat prevFrame;
    cv::Mat descriptorsCurrent;
    cv::Mat descriptorsPrev;
    cv::Mat homography;
    int homographyCalcMethod;
    double ransacReprojThreshold;
    float flowErrorThreshold;
    unsigned int minimumFlowPoint;
    HomograpyMethod hMethod;
    cv::Ptr<cv::FeatureDetector> detector;
    cv::Ptr<cv::DescriptorExtractor> descriptor;
    cv::Ptr<cv::DescriptorMatcher> matcher;
    std::vector<cv::KeyPoint> keypointsCurrent;
    std::vector<cv::KeyPoint> keypointsPrev;
    cv::vector<cv::Point2f> pointsCurrent;
    cv::vector<cv::Point2f> pointsPrev;
    float keyRetainFactor;
    bool isHomographyCalc;

    //
    void featureBasedHomography();
    void flowBasedHomography();
    void init(cv::Mat &frame);
    void run();
    //
public:
    void setDetector(cv::Ptr<cv::FeatureDetector> idetector);
    void setDetectorSimple(const char *detectorName);
    void setDescriptor(cv::Ptr<cv::DescriptorExtractor> idescriptor);
    void setDescriptorSimple(const char* descriptorName);
    void setMatcher(cv::Ptr<cv::DescriptorMatcher> imatcher);
    void setMatcherSimple(const char* matcherName);
    void setHomographyMethod(HomograpyMethod ihMethod);
    void setHomographyCalcMethod(int ihomographyCalcMethod);
    //


};

#endif // ALIGNMENTMATRIXCALC_H
