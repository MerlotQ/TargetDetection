#include "alignmentmatrixcalc.h"

AlignmentMatrixCalc::AlignmentMatrixCalc()
{
    exc.setModuleName("AlignmentMatrixCalc");

    //prevFrame = NULL;
    //currentFrame = NULL;

    hMethod = featureBased;
 //   hMethod = flowBased;
    keyRetainFactor=0.75;
    homographyCalcMethod=CV_RANSAC;
    ransacReprojThreshold = 3;
    setDetectorSimple("GridHARRIS");
    setDescriptorSimple("ORB");
    setMatcherSimple("BruteForce-L1");
    isHomographyCalc=false;

    if(!cv::initModule_nonfree())
    {
        exc.showException("Compiled without Non-free Option!" );
    }
}

void AlignmentMatrixCalc::process(cv::Mat &inputImage)
{
    if(!prevFrame.empty())
    {
        if(!currentFrame.empty())
        {
            if(hMethod == featureBased)
            {
                prevFrame = currentFrame;
                keypointsPrev = keypointsCurrent;
                descriptorsPrev = descriptorsCurrent;
            }
            else if(hMethod == flowBased)
            {
                prevFrame = currentFrame;
                pointsPrev = pointsCurrent;
            }
        }

        inputImage.copyTo(currentFrame);
        run();

        if(hMethod == featureBased)
        {
            featureBasedHomography();
        }
        else
        {
            flowBasedHomography();
        }
    }
    else // if(!prevFrame.empty()){
    {
        init(inputImage);
    }
}

void AlignmentMatrixCalc::init(cv::Mat &frame)
{
    frame.copyTo(prevFrame);

    if(hMethod == featureBased)
    {
        detector->detect(prevFrame,keypointsPrev);
        cv::KeyPointsFilter::retainBest(keypointsPrev,keyRetainFactor*keypointsPrev.size() );
        descriptor->compute(prevFrame,keypointsPrev,descriptorsPrev);
    }
    else if(hMethod == flowBased)
    {
        detector->detect(prevFrame, keypointsPrev);
        cv::KeyPointsFilter::retainBest(keypointsPrev, keyRetainFactor*keypointsPrev.size() );

        for(unsigned int i = 0; i<keypointsPrev.size(); i++)
        {
            pointsPrev.push_back(keypointsPrev[i].pt);
        }

        cv::cornerSubPix(prevFrame, pointsPrev, cv::Size(5,5), cv::Size(-1,-1),
                         cv::TermCriteria(cv::TermCriteria::MAX_ITER+cv::TermCriteria::EPS, 30, 0.1));

    }
}

void AlignmentMatrixCalc::run()
{
    if(hMethod == featureBased)
    {
        detector->detect(currentFrame, keypointsCurrent);
        cv::KeyPointsFilter::retainBest(keypointsCurrent, keyRetainFactor*keypointsCurrent.size() );
        descriptor->compute(currentFrame, keypointsCurrent, descriptorsCurrent);
    }
    else if(hMethod == flowBased)
    {
        if(pointsPrev.size() < minimumFlowPoint)
        {
            detector->detect(prevFrame, keypointsPrev);
            cv::KeyPointsFilter::retainBest(keypointsPrev, keyRetainFactor*keypointsPrev.size() );

            for(unsigned int i=0; i < keypointsPrev.size(); i++)
            {
                pointsPrev.push_back(keypointsPrev[i].pt);
            }

            cv::cornerSubPix(prevFrame,pointsPrev,cv::Size(5,5),cv::Size(-1,-1),cv::TermCriteria(cv::TermCriteria::MAX_ITER+cv::TermCriteria::EPS,30,0.1));
        }

    }

}

void AlignmentMatrixCalc::featureBasedHomography()
{
    std::vector< cv::DMatch > matches,matches12,matches21;
    // Simetri testi
    matcher->match( descriptorsPrev, descriptorsCurrent, matches12 );
    matcher->match( descriptorsCurrent, descriptorsPrev, matches21 );

    for( size_t i = 0; i < matches12.size(); i++ )
    {
        cv::DMatch forward = matches12[i];
        cv::DMatch backward = matches21[forward.trainIdx];
        if( backward.trainIdx == forward.queryIdx )
        {
            matches.push_back( forward );
        }
    }

    pointsPrev.clear();
    pointsCurrent.clear();

    for (int p = 0; p < (int)matches.size(); ++p)
    {
        pointsPrev.push_back(keypointsPrev[matches[p].queryIdx].pt);
        pointsCurrent.push_back(keypointsCurrent[matches[p].trainIdx].pt);
    }

    homography=cv::findHomography(pointsPrev, pointsCurrent, homographyCalcMethod, ransacReprojThreshold);
    isHomographyCalc=true;

}

void AlignmentMatrixCalc::flowBasedHomography()
{
    std::vector<uchar>status;
    std::vector<float>err;
    std::vector<cv::Point2f>tempPrev;
    std::vector<cv::Point2f>tempCurrent;

    calcOpticalFlowPyrLK(prevFrame, currentFrame, pointsPrev, pointsCurrent, status, err);

    for (unsigned int i=0; i<pointsPrev.size(); i++)
    {
        if(status[i] && err[i] <= flowErrorThreshold)
        {
            tempPrev.push_back(pointsPrev[i]);
            tempCurrent.push_back(pointsCurrent[i]);
        }
    }

    homography = cv::findHomography(tempPrev, tempCurrent, homographyCalcMethod, ransacReprojThreshold);
    pointsCurrent = tempCurrent;
    isHomographyCalc=true;
    // Burda bir kontrol eklenmeli filitrelenen noktaların belli bir sayı altına düştüğünde
    // yeniden feature bazlı nokta tespit ettirilmeli

}


void AlignmentMatrixCalc::setDetector(cv::Ptr<cv::FeatureDetector> idetector)
{
    detector = idetector;
}

void AlignmentMatrixCalc::setDetectorSimple(char *detectorName)
{
    setDetector(cv::FeatureDetector::create(detectorName ));
}

void AlignmentMatrixCalc::setDescriptor(cv::Ptr<cv::DescriptorExtractor> idescriptor)
{
    descriptor=idescriptor;
}

void AlignmentMatrixCalc::setDescriptorSimple(char *descriptorName)
{
    setDescriptor(cv::DescriptorExtractor::create(descriptorName));
}

void AlignmentMatrixCalc::setMatcher(cv::Ptr<cv::DescriptorMatcher> imatcher)
{
    matcher = imatcher;
}

void AlignmentMatrixCalc::setMatcherSimple(char *matcherName)
{
    setMatcher(cv::DescriptorMatcher::create(matcherName));
}

bool AlignmentMatrixCalc::getHomography(cv::Mat &gHomography)
{
    if(isHomographyCalc)
        gHomography=homography;
    return isHomographyCalc;
}
