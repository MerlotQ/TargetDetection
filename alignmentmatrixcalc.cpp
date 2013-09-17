#include "alignmentmatrixcalc.h"

AlignmentMatrixCalc::AlignmentMatrixCalc()
{
    exc.setModuleName("AlignmentMatrixCalc");

    hMethod = featureBased;
    keyRetainFactor=0.75;
    homographyCalcMethod=CV_RANSAC;
    ransacReprojThreshold = 3;

    matchType=normalMatch;
    maxRatio=0.50;
    maxRadius=100;

    setDetectorSimple("SURF");
    setDescriptorSimple("SURF");

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

void AlignmentMatrixCalc::reset()
{
    //delete buffers
    //set AlignmentMatrixCalc to initial state
    prevFrame.release();
}

void AlignmentMatrixCalc::init(cv::Mat &frame)
{
    frame.copyTo(prevFrame);

    if(hMethod == featureBased)
    {
        detector->detect(prevFrame, keypointsPrev);
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

/*
void convertRMatches(std::vector<std::vector<cv::DMatch>> rmatches,std::vector<cv::DMatch> matchesPassed)
{
    std::cout<<"Radius Matches :"<<rmatches.size()<<"\n ";
    for(std::vector<std::vector<cv::DMatch> >::iterator mi=rmatches.begin() ; mi != rmatches.end(); ++mi)
    {
        std::cout<<"Radius Matches :"<<(*mi).size()<<"\n ";
        matchesPassed.push_back((*mi)[0]);
    }
    std::cout<<"Matches Passed : "<<matchesPassed.size()<<"\n ";
}
*/
void AlignmentMatrixCalc::featureBasedHomography()
{
    std::vector<cv::DMatch> matchesPrevToCurrent;
    std::vector<cv::DMatch> matchesCurrentToPrev;
    std::vector<std::vector<cv::DMatch> > kmatchesPrevToCurrent;
    std::vector<std::vector<cv::DMatch> > kmatchesCurrentToPrev;
    std::vector<cv::DMatch> matchesPassed;

    if( matchType == normalMatch )
    {
        matcher->match( descriptorsPrev, descriptorsCurrent, matchesPrevToCurrent );
        matcher->match( descriptorsCurrent, descriptorsPrev, matchesCurrentToPrev );
        // Symmetry Test start
        symmetryTest(matchesPrevToCurrent,matchesCurrentToPrev,matchesPassed);

    }
    else if( matchType == knnMatch)
    {
        matcher->knnMatch(descriptorsPrev,descriptorsCurrent,kmatchesPrevToCurrent,2);
        std::cout<<"Ratio Test 1 :"<<kmatchesPrevToCurrent.size()<<"\n";
        ratioTest(kmatchesPrevToCurrent);
        std::cout<<"Ratio Test 1 End :"<<kmatchesPrevToCurrent.size()<<"\n";
        matcher->knnMatch(descriptorsCurrent,descriptorsPrev,kmatchesCurrentToPrev,2);
        std::cout<<"Ratio Test 2 :"<<kmatchesCurrentToPrev.size()<<"\n";
        ratioTest(kmatchesCurrentToPrev);
        std::cout<<"Ratio Test 2 End :"<<kmatchesCurrentToPrev.size()<<"\n";
        // Symmetry Test not working for knn
        //matchesPassed=matchesPrevToCurrent;
        symmetryTest(kmatchesPrevToCurrent,kmatchesCurrentToPrev,matchesPassed);
        std::cout<<"Sym Test  :"<<matchesPassed.size()<<"\n";

    }
    else if( matchType == radiusMatch)
    {
        // there is no documentation
        //matcher->radiusMatch(descriptorsPrev, descriptorsCurrent, kmatchesPrevToCurrent,maxRadius );
        // work but there is no matching back for any maxRadius
        //convertRMatches(kmatchesCurrentToPrev,matchesPassed);
        exc.showException("radiusMatch not working Dont use it!" );

    }


    // Matching Section end

    pointsPrev.clear();
    pointsCurrent.clear();

    for (int p = 0; p < (int)matchesPassed.size(); ++p)
    {
        pointsPrev.push_back(keypointsPrev[matchesPassed[p].queryIdx].pt);
        pointsCurrent.push_back(keypointsCurrent[matchesPassed[p].trainIdx].pt);
    }




    if(pointsPrev.size() !=0 && pointsCurrent.size() != 0)
    {
        // Sub-pixsel Accuracy

        cv::cornerSubPix(prevFrame, pointsPrev, cv::Size(5,5), cv::Size(-1,-1), cv::TermCriteria(cv::TermCriteria::MAX_ITER+cv::TermCriteria::EPS,30,0.1));
        cv::cornerSubPix(currentFrame, pointsCurrent, cv::Size(5,5), cv::Size(-1,-1), cv::TermCriteria(cv::TermCriteria::MAX_ITER+cv::TermCriteria::EPS,30,0.1));

        homography = cv::findHomography(pointsPrev, pointsCurrent, homographyCalcMethod, ransacReprojThreshold);
        /*
         cv::findHomography can return empty matrix in some cases.
         This seems happen only when cv::RANSAC flag is passed.
         check the computed homography before using it
         */
        if(!homography.empty())
        {
           isHomographyCalc = true;
        }
        else
        {
            isHomographyCalc = false;
        }

    }
    else
    {
        isHomographyCalc = false;
    }
    //If Homogrphy not valid
    if(!isHomographyValid())
    {
        wayBack();
    }


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
    if(pointsCurrent.size()<50)
    {
        init(currentFrame);
    }

    //If Homogrphy not valid !
    if(isHomographyValid())
    {
        wayBack();
    }

}


void AlignmentMatrixCalc::setDetector(cv::Ptr<cv::FeatureDetector> idetector)
{
    detector = idetector;
}

void AlignmentMatrixCalc::setDetectorSimple(const char *detectorName)
{
    setDetector(cv::FeatureDetector::create(detectorName ));
}

void AlignmentMatrixCalc::setDescriptor(cv::Ptr<cv::DescriptorExtractor> idescriptor)
{
    descriptor=idescriptor;
}

void AlignmentMatrixCalc::setDescriptorSimple(const char *descriptorName)
{
    setDescriptor(cv::DescriptorExtractor::create(descriptorName));
}

void AlignmentMatrixCalc::setMatcher(cv::Ptr<cv::DescriptorMatcher> imatcher)
{
    matcher = imatcher;
}

void AlignmentMatrixCalc::setMatcherSimple(const char *matcherName)
{
    setMatcher(cv::DescriptorMatcher::create(matcherName));
}

void AlignmentMatrixCalc::setHomographyMethod(HomograpyMethod ihMethod)
{
    hMethod=ihMethod;
}

void AlignmentMatrixCalc::setHomographyCalcMethod(int ihomographyCalcMethod)
{
    /*
    – 0 - a regular method using all the points
    – CV_RANSAC - RANSAC-based robust method
    – CV_LMEDS - Least-Median robust method
    */

    homographyCalcMethod=ihomographyCalcMethod;
}

bool AlignmentMatrixCalc::getHomography(cv::Mat &gHomography)
{
    if(isHomographyCalc)
    {
        gHomography = homography;
    }

    return isHomographyCalc;
}

void AlignmentMatrixCalc::setMatchingType(MatchingType iType)
{
    matchType=iType;
}

void AlignmentMatrixCalc::symmetryTest(std::vector<cv::DMatch> &matchesPrevToCurrent, std::vector<cv::DMatch> &matchesCurrentToPrev, std::vector<cv::DMatch> &matchesPassed)
{
   // std::cout<<"Prev2Cur :"<<matchesPrevToCurrent.size()<<"\n Cur2Prev :"<<matchesCurrentToPrev.size()<<"\n";
    for( size_t i = 0; i < matchesPrevToCurrent.size(); i++ )
    {

        cv::DMatch forward = matchesPrevToCurrent[i];
      //  std::cout<<i<<")"<<forward.trainIdx<<" - "<<forward.distance<<"\n"; // for debugging
      //  if(forward.trainIdx >= matchesCurrentToPrev.size()) continue;
        cv::DMatch backward = matchesCurrentToPrev[forward.trainIdx];
        if( backward.trainIdx == forward.queryIdx && forward.trainIdx==backward.queryIdx)
        {
            matchesPassed.push_back( forward );

        }

    }
 //   std::cout<<"Matches Passed Symmetry Test :"<<matchesPassed.size()<<"\n";
}

void AlignmentMatrixCalc::symmetryTest(std::vector<std::vector<cv::DMatch> >&kmatchesPrevToCurrent,std::vector<std::vector<cv::DMatch> >&kmatchesCurrentToPrev,std::vector< cv::DMatch >& matchesPassed)
{

    for(std::vector<std::vector<cv::DMatch> >::iterator mPi= kmatchesPrevToCurrent.begin(); mPi != kmatchesPrevToCurrent.end(); ++mPi)
    {
        if(mPi->size() < 2 )
            continue;
        for(std::vector<std::vector<cv::DMatch> >::iterator mCi= kmatchesCurrentToPrev.begin(); mCi != kmatchesCurrentToPrev.end(); ++mCi)
        {

            if(mCi->size() < 2 )
                continue;
            cv::DMatch forward = (*mPi)[0];
            cv::DMatch backward = (*mCi)[0];
            if((forward.queryIdx == backward.trainIdx) && (backward.queryIdx == forward.trainIdx) )
            {
                matchesPassed.push_back(forward);
                break;
            }
        }
    }

}

void AlignmentMatrixCalc::ratioTest(std::vector<std::vector<cv::DMatch> > &kmatches)
{
    for(std::vector<std::vector<cv::DMatch> >::iterator mi=kmatches.begin() ; mi != kmatches.end(); ++mi)
    {
        if(mi->size()>1)
        {
            const cv::DMatch& best = (*mi)[0];
            const cv::DMatch& good = (*mi)[1];

            assert(best.distance <= good.distance);
            float ratio = (best.distance / good.distance);
            //     std::cout<<ratio<<"\n";

            if (ratio > maxRatio)
            {
               // std::cout<<ratio<<"\n";
                mi->clear();
            }
        }
        else
            mi->clear();

    }
}

bool AlignmentMatrixCalc::isHomographyValid()
{
    std::vector<cv::Point2f> inputCorners(4);
    inputCorners[0] = cvPoint(0,0);
    inputCorners[1] = cvPoint( prevFrame.cols, 0 );
    inputCorners[2] = cvPoint( prevFrame.cols, prevFrame.rows );
    inputCorners[3] = cvPoint( 0, prevFrame.rows );
    std::vector<cv::Point2f> alignedCorners(4);

    perspectiveTransform( inputCorners, alignedCorners, homography);

    float upDeltaX = fabs(alignedCorners[0].x-alignedCorners[1].x);
    float downDeltaX = fabs(alignedCorners[2].x-alignedCorners[3].x);
    float upDeltaY = fabs(alignedCorners[0].y-alignedCorners[3].y);
    float downDeltaY = fabs(alignedCorners[1].y-alignedCorners[2].y);
    float alignedCols=(upDeltaX+downDeltaX)/2;
    float alignedRows=(upDeltaY+downDeltaY)/2;
    float colsDifference=fabs(alignedCols-prevFrame.cols)/prevFrame.cols;
    float rowsDifference=fabs(alignedRows-prevFrame.rows)/prevFrame.rows;


/*
    for(int i = 0; i < 4; i++){
        x=alignedCorners[i].x;
        y=alignedCorners[i].y;
  */
 //   std::cout<<"\nNew Delta X , Ys \n"<<upDeltaX<<" - "<<upDeltaY<<"\n"<<downDeltaX<<" - "<<downDeltaY<<"\n";
 //   std::cout<<"\nCols and Rows Differenece \n"<<colsDifference<<" - "<<rowsDifference<<"\n";

    if( colsDifference < 0.1 && rowsDifference < 0.1 )
       isHomographyCalc=true;
    else
       isHomographyCalc=false;

    return isHomographyCalc;

}

void AlignmentMatrixCalc::wayBack()
{
    if(hMethod == featureBased)
    {
        return;
        currentFrame = prevFrame;
        keypointsCurrent = keypointsPrev;
        descriptorsCurrent = descriptorsPrev;
    }
    else if(hMethod == flowBased)
    {
        currentFrame = prevFrame;
        pointsCurrent = pointsPrev;
    }
}
