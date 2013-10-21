#include <Test/TEST_frameAllignment.h>


#define TEST_VIDEO_FILE_CNT 14

static long processVideoAndGetScores(QString &videoFileName, int startFrame);
static void reportScoresForVideoFile(int videoFileIndex, AllignementTestScore score);

//
int64 times[10];
bool measureStart[10]={false,false,false,false,false,
                       false,false,false,false,false};

void timeMeasure(int i)
{
    if(measureStart[i] == false)
    {
        times[i] = cv::getTickCount();
        measureStart[i] = true;
    }
    else
    {
        qDebug()<<"Time Measurement "<<i<<" : "<<((double)cv::getTickCount() - times[i]) / cv::getTickFrequency();
        measureStart[i] = false;
    }
}
//


typedef struct
{
    long TotalTimeSn;
    int fps;
    float homograpyFoundPercent;
    float whitePixelPerFrame;
    QString HomographyMethod;
    QString usedDetector;
    QString usedDescriptor;

}AllignementTestScore;

void TEST_frameAllignment()
{
    QString videoFileName;
    long scoreForSingleVideo;
    long totalScoreForVideos = 0;
    int startFrame = 200;

    //open videos sequentialy.

    for(int i = 1; i <= TEST_VIDEO_FILE_CNT; ++i)
    {
        //determine video fileName
#ifdef WIN32
        videoFileName = "D:/cvs/data/testavi/output";
#else
        videoFileName = "output";//"output";
#endif

        videoFileName +=  QString::number(i);
        videoFileName += ".avi";


        //process video and get results
        scoreForSingleVideo = processVideoAndGetScores(videoFileName, startFrame);

        //save scores to file
        reportScoresForVideoFile(i, scoreForSingleVideo);

        totalScoreForVideos += scoreForSingleVideo;

        scoreForSingleVideo = 0;
    }

    //report total score, variable 0 express total score
    reportScoresForVideoFile(0, totalScoreForVideos);

}


static void reportScoresForVideoFile(int videoFileIndex, AllignementTestScore score)
{
    QFile file("testScore.txt");

    if (file.open(QIODevice::Append) )
    {
        QTextStream out(&file);

        if(videoFileIndex == 0)
        {
            //total score for videos
            out<<"Total Score    :"<<score;
        }
        else
        {
                out<<videoFileIndex<<".avi    :"<<score;
        }

        file.close();
    }

}

static void processVideoAndGetScores(QString &videoFileName, int startFrame, AllignementTestScore& score)
{
    AlignmentMatrixCalc alignMatrixcalc;
    FrameAlignment frameAlligner;

    cv::VideoCapture videoCap;
    cv::Mat videoFrame;
    cv::Mat currentFrame;
    cv::Mat prevFrame;
    cv::Mat sequentalImageDiff;
    cv::Mat sequentalImageDiffBinary;
    cv::Mat sequentalImageDiffBinary1;

    cv::Mat prevFrameAlligned;
    long sumNonZero = 0;
    cv::Mat homograpyMatrix;

    long frameCount = 0;

    //alignMatrixcalc.setHomographyMethod(flowBased);
    //alignMatrixcalc.setDescriptorSimple("GFTT");

    alignMatrixcalc.setDetectorSimple("SURF");
    alignMatrixcalc.setDescriptorSimple("SURF");

    alignMatrixcalc.setHomographyCalcMethod(CV_LMEDS);
    alignMatrixcalc.setMatchingType(knnMatch);

    videoCap.open(videoFileName.toStdString());

    //burası düzeltilecek düzgün init fonksiyonu koyulacak.
    if(startFrame != 0)
    {
        videoCap.set(CV_CAP_PROP_POS_FRAMES, (double)startFrame);
        frameCount = startFrame;
    }

    videoCap.read(videoFrame);
    frameCount ++;

    cv::cvtColor(videoFrame, videoFrame, CV_BGR2GRAY);
    cv::resize(videoFrame, videoFrame, cv::Size(640,480));

    //buna neden gerek var. sadece getHomography olsa olmuyor mu?
    alignMatrixcalc.process(videoFrame);
    prevFrame=videoFrame.clone();

    while (videoCap.read(videoFrame))
    {
        cv::resize(videoFrame, videoFrame, cv::Size(640,480));
        currentFrame = videoFrame.clone();

        cv::cvtColor(currentFrame, currentFrame, CV_BGR2GRAY);

        timeMeasure(1);
        alignMatrixcalc.process(currentFrame);
        timeMeasure(1);

        if(alignMatrixcalc.getHomography(homograpyMatrix) == true)
        {
            cv::Mat mask(currentFrame.size(), CV_8U);
            mask = cv::Scalar(255);

            cv::Mat maskedVideoFrame(currentFrame.size(), CV_8U);
            maskedVideoFrame = cv::Scalar(255);

            frameAlligner.process(prevFrame, homograpyMatrix, prevFrameAlligned);

            frameAlligner.process(mask, homograpyMatrix, mask);

            maskedVideoFrame = currentFrame & mask;

            sequentalImageDiffBinary.create(maskedVideoFrame.size(), CV_8UC1);
            frameAlligner.calculateBinaryDiffImageAccording2pixelNeighborhood(prevFrameAlligned,
                                                                              maskedVideoFrame,
                                                                              sequentalImageDiffBinary);

            cv::imshow("sequentalImageDiffBinaryPixelNeigh", sequentalImageDiffBinary);
            sumNonZero += cv::countNonZero(prevFrameAlligned);


            cv::putText(videoFrame, "Homograpy status: OK", cvPoint(10,35),
                        cv::FONT_HERSHEY_COMPLEX_SMALL, 0.6, cvScalar(200,200,250), 1, CV_AA);

        }
        else
        {
            cv::putText(videoFrame, "Homograpy status: Not Found!", cvPoint(10,35),
                        cv::FONT_HERSHEY_COMPLEX_SMALL, 0.6, cvScalar(200,200,250), 1, CV_AA);
        }

        frameCount++;

        QString videoInfo;

        videoInfo = videoFileName + " @ " + QString::number(frameCount);

        cv::putText(videoFrame, videoInfo.toStdString(), cvPoint(10,20),
                    cv::FONT_HERSHEY_COMPLEX_SMALL, 0.6, cvScalar(200,200,250), 1, CV_AA);

        prevFrame = currentFrame.clone();

        cv::imshow("input", videoFrame);
        cv::waitKey(5);

    }

    return sumNonZero;
}

