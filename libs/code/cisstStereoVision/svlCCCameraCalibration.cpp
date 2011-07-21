/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
$Id: cameraCalibration.cpp 2426 2011-05-21 00:53:58Z wliu25 $

Author(s):  Wen P. Liu
Created on: 2011

(C) Copyright 2006-2007 Johns Hopkins University (JHU), All Rights
Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---
*/

#include <cisstStereoVision/svlCCCameraCalibration.h>
#include <sstream>

svlCCCameraCalibration::svlCCCameraCalibration(int boardWidth, int boardHeight, float squareSize, int originDetectorColorModeFlag)
{
    minCornerThreshold = 5;
    maxCalibrationIteration = 10;
    maxNumberOfGrids = 25;
    debug = false;
    groundTruthTest = false;
    visibility = new int[maxNumberOfGrids];
    groundTruthCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    groundTruthDistCoeffs  = cv::Mat::zeros(5, 1, CV_64F);
    this->boardSize = cv::Size(boardWidth,boardHeight);
    this->squareSize = squareSize;
    calCornerDetector = new svlCCCornerDetector(boardSize.width,boardSize.height);
    calOriginDetector = new svlCCOriginDetector(originDetectorColorModeFlag);
    cameraGeometry = new svlSampleCameraGeometry();
    minHandEyeAvgError = std::numeric_limits<double>::max( );
}

bool svlCCCameraCalibration::runCameraCalibration(bool runHandEye)
{
    cameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    distCoeffs  = cv::Mat::zeros(5, 1, CV_64F);
    objectPoints.clear();
    projectedObjectPoints.clear();
    imagePoints.clear();
    projectedImagePoints.clear();
    pointFiles.clear();
    rootMeanSquaredThreshold = 1;
    refineThreshold = 2;
    pointsCount = 0;
    maxPointsCount = 0;
    avgErr = std::numeric_limits<double>::max( );
    this->runHandEye = runHandEye;
    calHandEye = new svlCCHandEyeCalibration(calibrationGrids);
    // Calibrate
    return calibration();

}

void svlCCCameraCalibration::reset()
{
    images.clear();
    calibrationGrids.clear();
    rvecs.clear();
    tvecs.clear();
    svlFilterImageRectifier *rectifier;

}

void svlCCCameraCalibration::printCalibrationParameters()
{
    for(int i=0;i<cameraMatrix.rows;i++)
    {
        for(int j=0;j<cameraMatrix.cols;j++)
        {
            std::cout << "Camera matrix: " << cameraMatrix.at<double>(i,j) << std::endl;
        }
    }

    for(int i=0;i<distCoeffs.rows;i++)
    {
        for(int j=0;j<distCoeffs.cols;j++)
        {
            std::cout << "Distortion _coefficients: " << distCoeffs.at<double>(i,j) << std::endl;
        }
    }

    std::cout << "Handeye error: "<< minHandEyeAvgError<<std::endl;
    std::cout << tcpTCamera << std::endl;
    //for(int i=0;i<rvecs.size();i++)
    //{
    //    std::cout << "rvect: " << i << ": " << rvecs.at(i).at<double>(0,0) <<","<< rvecs.at(i).at<double>(0,1) <<","<< rvecs.at(i).at<double>(0,2) <<","<< std::endl;
    //    std::cout << "tvect: " << i << ": " << tvecs.at(i).at<double>(0,0) <<","<< tvecs.at(i).at<double>(0,1) <<","<< tvecs.at(i).at<double>(0,2) <<","<< std::endl;
    //}
}

/**************************************************************************************************
* computeReprojectionErrors()					
*	Compute the average L1 reprojection error
*	
* Input
*	objectPoints			const vector<vector<Point3f> >&				- 3D object points of chessboard plane
*	imagePoints				const vector<vector<Point3f> >&				- 2D image points
*	rvecs					const vector<Mat>&							- camera rotation vectors
*	tvecs					const vector<Mat>& 							- camera translation vectors
*	cameraMatrix			const Mat&									- camera intrinsic matrix
*	distCoeffs				const Mat&									- camera distortion coefficients
*	perViewErrors			vector<float>&								- average error per camera
*	projected				bool										- Indicator whether or not to use projected or standard points
*
* Output:
*	double																- Average L1 reprojection error 					
*
***********************************************************************************************************/
double svlCCCameraCalibration::computeReprojectionErrors(
    const std::vector<std::vector<cv::Point3f> >& objectPoints,
    const std::vector<std::vector<cv::Point2f> >& imagePoints,
    const std::vector<cv::Mat>& rvecs, const std::vector<cv::Mat>& tvecs,
    const cv::Mat& cameraMatrix, const cv::Mat& distCoeffs,
    std::vector<float>& perViewErrors, bool projected )
{
    std::vector<cv::Point2f> imagePoints2, projectedImgPoints;
    std::vector<cv::Point3f> projectedObjPoints;
    int i, totalPoints = 0;
    double totalErr = 0, err;
    int validIndex = 0;
    perViewErrors.resize(objectPoints.size());

    for( i = 0; i < (int)objectPoints.size(); i++ )
    {
        projectPoints(cv::Mat(objectPoints[i]), rvecs[i], tvecs[i],
            cameraMatrix, distCoeffs, imagePoints2);
        projectedImagePoints.push_back(imagePoints2);
        projectedObjectPoints = objectPoints;
        if(projected)
        {
            while(!calibrationGrids.at(validIndex)->valid)
                validIndex++;

            for(int j=0;j<(int)imagePoints2.size();j++)
            {
                //orange, original projected points
                //cvCircle( images.at(validIndex).IplImageRef(), imagePoints.at(i).at(j), 3, cvScalar(55,0,255,0), 1, 8, 0 );
                //pink, projected final projected points
                cvCircle( images.at(validIndex).IplImageRef(), imagePoints2.at(j), 3, cvScalar(255,0,255,0), 1, 8, 0 );
            }
            validIndex++;
        }
        err = norm(cv::Mat(imagePoints[i]), cv::Mat(imagePoints2), CV_L1 );
        int n = (int)objectPoints[i].size();
        perViewErrors[i] = (float)(err/n);
        totalErr += err;
        totalPoints += n;
    }

    return totalErr/totalPoints;
}

void svlCCCameraCalibration::updateCalibrationGrids()
{
    int validIndex = 0;
    for(int i=0;i<(int)calibrationGrids.size();i++)
    {
        if(visibility[i]){
            //std::cout << "Updating grid " << i << " with parameters " << validIndex <<std::endl;
            //std::cout << "rvect: " << i << ": " << rvecs.at(validIndex).at<double>(0,0) <<","<< rvecs.at(validIndex).at<double>(0,1) <<","<< rvecs.at(validIndex).at<double>(0,2) <<","<< std::endl;
            //std::cout << "tvect: " << i << ": " << tvecs.at(validIndex).at<double>(0,0) <<","<< tvecs.at(validIndex).at<double>(0,1) <<","<< tvecs.at(validIndex).at<double>(0,2) <<","<< std::endl;
            calibrationGrids.at(i)->cameraMatrix = cameraMatrix;
            calibrationGrids.at(i)->distCoeffs = distCoeffs;
            calibrationGrids.at(i)->rvec = rvecs[validIndex];
            calibrationGrids.at(i)->tvec = tvecs[validIndex];
            validIndex++;
        }
    }
}

/**************************************************************************************************
* runOpenCVCalibration()					
*	Calibration with OpenCV function
*	
* Input
*	projected		bool				- Indicator whether or not to use the projected points from last
*										calibration, with known improvement or standard calibration
*										with current points
*
* Output:
*	double								- RMS error 					
*
***********************************************************************************************************/
double svlCCCameraCalibration::runOpenCVCalibration(bool projected)
{
    flags = 0;
    double rms;
    rvecs.clear();
    tvecs.clear();

    if(projected)
    {
        printf("============calibrateCamera: running projected============\n");
        rms = calibrateCamera(projectedObjectPoints, projectedImagePoints, imageSize, cameraMatrix,
            distCoeffs, rvecs, tvecs, flags);//cv::CALIB_FIX_K1|cv::CALIB_FIX_K2|cv::CALIB_FIX_K3);
    }
    else
    {
        printf("============calibrateCamera: running standard============\n");
        rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix,
            distCoeffs, rvecs, tvecs, flags);//cv::CALIB_FIX_K1|cv::CALIB_FIX_K2|cv::CALIB_FIX_K3);
    }
    //if(debug)
    printf("RMS error reported by calibrateCamera: %g\n", rms);



    return rms;
}

/**************************************************************************************************
* checkCalibration()					
*	Check for reprojection error from current calibration
*	
* Input
*	projected		bool				- Indicator whether or not to use the projected points from last
*										calibration, with known improvement or standard calibration
*										with current points
*
* Output:
*	bool								- Indicator of success 					
*
***********************************************************************************************************/
bool svlCCCameraCalibration::checkCalibration(bool projected)
{
    bool ok = false;
    std::vector<float> reprojErrs;
    ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

    if(projected)
    {
        avgErr = computeReprojectionErrors(projectedObjectPoints, projectedImagePoints,
            rvecs, tvecs, cameraMatrix, distCoeffs, reprojErrs, projected);
    }else
    {
        avgErr = computeReprojectionErrors(objectPoints, imagePoints,
            rvecs, tvecs, cameraMatrix, distCoeffs, reprojErrs, projected);
    }
    std::cout << "Range check " << ok << ", average L1 Norm error: " << avgErr <<std::endl;

    if(ok && debug)
    {
        printCalibrationParameters();
    }

    return ok;
}

/**************************************************************************************************
* calibrate()					
*	Calibrate with current points
*	
* Input
*	projected		bool				- Indicator whether or not to use the projected points from last
*										calibration, with known improvement or standard calibration
*										with current points
*
* Output:
*	void 					
*
***********************************************************************************************************/
double svlCCCameraCalibration::calibrate(bool projected, bool groundTruthTest)
{
    // empty points vectors
    imagePoints.clear();
    objectPoints.clear();
    pointsCount = 0;
    std::vector<cv::Point2f> points2D;
    std::vector<cv::Point3f> points3D;

    if(!projected)
    {
        //empty projected points
        projectedImagePoints.clear();
        projectedObjectPoints.clear();

        //get Points
        for(int i=0;i<(int)calibrationGrids.size();i++)
        {
            if((!groundTruthTest && calibrationGrids.at(i)->valid)|| (groundTruthTest && calibrationGrids.at(i)->validGroundTruth)){
                visibility[i] = 1;
                points2D.clear();
                points3D.clear();
                if(groundTruthTest)
                {
                    points2D = calibrationGrids.at(i)->groundTruthImagePoints;
                    points3D = calibrationGrids.at(i)->groundTruthCalibrationGridPoints;
                }
                else
                {
                    points2D = calibrationGrids.at(i)->getGoodImagePoints();
                    points3D = calibrationGrids.at(i)->getGoodCalibrationGridPoints3D();
                }

                if (points2D.size() > calibrationGrids.at(i)->minGridPoints && points3D.size() > calibrationGrids.at(i)->minGridPoints && points2D.size() == points3D.size())
                {
                    std::cout << "image " << i << " using " << points2D.size() <<" points." << std::endl;
                    imagePoints.push_back(points2D);
                    objectPoints.push_back(points3D);
                    pointsCount += points2D.size();
                }
            }else
                visibility[i] = 0;
        }
    }

    double rms = std::numeric_limits<double>::max( );
    bool check = false;
    double handEyeAvgError = std::numeric_limits<double>::max( );

    if(projected)
    {
        if(projectedObjectPoints.size() <= 0)
            return rms;
    }
    else
    {
        if(objectPoints.size() <= 0)
            return rms;
    }

    if(!projected)
        std::cout << "Calibrating using " << pointsCount <<" points." << std::endl;
   
    rms = runOpenCVCalibration(projected);
    check = checkCalibration(projected);
    updateCalibrationGrids();
    if(this->runHandEye)
    {
        handEyeAvgError = calHandEye->calibrate();
        if(handEyeAvgError < minHandEyeAvgError)
        {
            minHandEyeAvgError = handEyeAvgError;
            tcpTCamera = calHandEye->tcp_T_camera;
        }
    }

    if(!check)
        return std::numeric_limits<double>::max( );
    else
    {
        //if(this->runHandEye)
        //    return handEyeAvgError;
        //else
            return rms;
    }
}

void svlCCCameraCalibration::refineGrids(int localThreshold)
{
    //refine
    int validIndex = 0;
    for(int i=0;i<(int)calibrationGrids.size();i++)
    {
        if(visibility[i])
        {
            //cout << "refining grid " << i << " with parameters " << validIndex <<endl;
            //cout << "rvect: " << i << ": " << rvecs.at(validIndex).at<double>(0,0) <<","<< rvecs.at(validIndex).at<double>(0,1) <<","<< rvecs.at(validIndex).at<double>(0,2) <<","<< endl;
            //cout << "tvect: " << i << ": " << tvecs.at(validIndex).at<double>(0,0) <<","<< tvecs.at(validIndex).at<double>(0,1) <<","<< tvecs.at(validIndex).at<double>(0,2) <<","<< endl;
            //calibrationGrids.at(i)->refine(rvecs[validIndex],tvecs[validIndex],cameraMatrix,distCoeffs,threshold,true,false);
            calibrationGrids.at(i)->refine(rvecs[validIndex],tvecs[validIndex],cameraMatrix,distCoeffs,localThreshold,false,true);
            validIndex++;
        }
    }
}

/**************************************************************************************************
* optimizeCalibration()					
*	Optimize on calibration by projection and refining each grid
*
* Output:
*	void 					
*
***********************************************************************************************************/
void svlCCCameraCalibration::optimizeCalibration()
{
    double rms;
    double prevRMS = std::numeric_limits<double>::max( );
    double handEyeAvgError;
    double prevHandEyeAvgError = std::numeric_limits<double>::max( );
    int prevPointsCount = 0;
    int iteration = 0;
    cv::Mat pPrevCameraMatrix;
    cv::Mat pPrevDistCoeffs;
    std::vector<cv::Mat> pPrevRvecs;
    std::vector<cv::Mat> pPrevTvecs;
    cv::Mat prevCameraMatrix;
    cv::Mat prevDistCoeffs;
    std::vector<cv::Mat> prevRvecs;
    std::vector<cv::Mat> prevTvecs;
    std::vector<cv::Mat> originalCameraMatrix;
    std::vector<cv::Mat> originalDistCoeffs;
    std::vector<cv::Mat> originalRvecs;
    std::vector<cv::Mat> originalTvecs;
    int* originalThresholds = new int[maxNumberOfGrids];
    int* prevVisibility = new int[maxNumberOfGrids];
    int* pPrevVisibility = new int[maxNumberOfGrids];
    int prevThreshold, pPrevThreshold;
    int validIndex = 0;
    int maxPointsCount = 0;

    //if(max(imageSize.height,imageSize.width) > 1000)
    //	refineThreshold = 4;

    //save original
    for(int i=0;i<(int)calibrationGrids.size();i++)
    {
        pPrevVisibility[i] = calibrationGrids.at(i)->valid;
        if(calibrationGrids.at(i)->valid)
        {
            originalCameraMatrix.push_back(calibrationGrids.at(i)->cameraMatrix);
            originalDistCoeffs.push_back(calibrationGrids.at(i)->distCoeffs);
            originalRvecs.push_back(calibrationGrids.at(i)->rvec);
            originalTvecs.push_back(calibrationGrids.at(i)->tvec);
            originalThresholds[i] = calibrationGrids.at(i)->refineThreshold;
        }
    }

    //refine
    rms = calibrate(false,false);
    prevCameraMatrix = cameraMatrix;
    prevDistCoeffs = distCoeffs;
    prevRvecs = rvecs;
    prevTvecs = tvecs;
    prevThreshold = refineThreshold;
    pPrevCameraMatrix = prevCameraMatrix;
    pPrevDistCoeffs = prevDistCoeffs;
    pPrevRvecs = prevRvecs;
    pPrevTvecs = prevTvecs;
    pPrevThreshold = prevThreshold;

    for(int i=0;i<(int)calibrationGrids.size();i++)
    {
        if(debug)
            std::cout << "Grid " << i << " valid: " << visibility[i] << std::endl; 
        if(calibrationGrids.at(i)->valid)
        {
            visibility[i] = 1;
        }
        else
            visibility[i] = 0;
        prevVisibility[i] = visibility[i];
    }
    maxPointsCount = std::max(pointsCount,maxPointsCount);
    int pointIncreaseIteration = 0;

    // check for bad calibration
    if(rms == std::numeric_limits<double>::max( ))
        return;

    while((rms < std::numeric_limits<double>::max( )) && (rms > rootMeanSquaredThreshold)&& (iteration < maxCalibrationIteration))
    {
 
        // Lower threshold for higher iteration of optimization
        if(iteration > 1)
            refineThreshold = 2;

        if(rms > prevRMS && pointsCount > (maxPointsCount + minCornerThreshold*calibrationGrids.size()))
        {
            refineThreshold = 1;
            pointIncreaseIteration++;
        }

        if(rms < prevRMS || (pointsCount > (maxPointsCount + pointIncreaseIteration*minCornerThreshold*calibrationGrids.size())))
        {
            std::cout << "Iteration: " << iteration << " rms delta: " << prevRMS-rms << " count delta: " <<  pointsCount-maxPointsCount << " pointIteration " << pointIncreaseIteration <<std::endl;
            pPrevCameraMatrix = prevCameraMatrix;
            pPrevDistCoeffs = prevDistCoeffs;
            pPrevRvecs = prevRvecs;
            pPrevTvecs = prevTvecs;
            pPrevThreshold = prevThreshold;
            prevCameraMatrix = cameraMatrix;
            prevDistCoeffs = distCoeffs;
            prevRvecs = rvecs;
            prevTvecs = tvecs;
            prevRMS = rms;
            prevPointsCount = pointsCount;
            prevThreshold = refineThreshold;
            for(int i=0;i<(int)calibrationGrids.size();i++)
            {
                pPrevVisibility[i] = prevVisibility[i];
                if(calibrationGrids.at(i)->valid)
                {
                    visibility[i] = 1;
                }
                else
                    visibility[i] = 0;
                prevVisibility[i] = visibility[i];
            }
            maxPointsCount = std::max(pointsCount,maxPointsCount);
            refineGrids(refineThreshold);
            rms = calibrate(false,false);

        }else
        {
            break;
        }
        iteration++;
    }

    //if(debug)
    std::cout <<std::endl << "==========Optimize Calibration stopped at " << iteration << " iterations=========" <<std::endl;
    if(debug)
        printCalibrationParameters();

    if(iteration > 0)
    {
        cameraMatrix = pPrevCameraMatrix;
        distCoeffs = pPrevDistCoeffs;
        rvecs = pPrevRvecs;
        tvecs = pPrevTvecs;
        refineThreshold = pPrevThreshold;

        if(debug)
            printCalibrationParameters();

        for(int i=0;i<(int)calibrationGrids.size();i++)
        {
            visibility[i] = pPrevVisibility[i];
        }
        refineGrids(refineThreshold);
        rms = calibrate(false, false);
    }else{
        //No iteration, set calibration grids back to original
        for(int i=0;i<(int)calibrationGrids.size();i++)
        {
            if(pPrevVisibility[i]){
                calibrationGrids.at(i)->refine(originalRvecs.at(validIndex),originalTvecs.at(validIndex),originalCameraMatrix.at(validIndex),originalDistCoeffs.at(validIndex),originalThresholds[i],false,false);
                validIndex++;
            }
        }

        if(debug)
            printCalibrationParameters();
        rms = calibrate(false, false);
    }

}

bool svlCCCameraCalibration::calibration()
{
    double rms;

    ///////////////////////optimize
    optimizeCalibration();
    //updateCalibrationGrids();

    ///////////////////////projected
    //rms = calibrate(true, false);
    //updateCalibrationGrids();
    //refineGrids(refineThreshold);

    ///////////////////////update camera geometry
    if(rms < std::numeric_limits<double>::max( ))
    {
        updateCameraGeometry();
        return true;
    }

    return false;
}

bool svlCCCameraCalibration::processImage(std::string imageDirectory, std::string imagePrefix, std::string imageType, int index)
{
    std::stringstream path;
    std::string currentFileName;

    svlCCCalibrationGrid* calibrationGrid;
    bool ok = false;

    // image file
    path << imageDirectory;
    path << imagePrefix;
    path.fill('0');
    path << std::setw(3) << index << std::setw(1);
    path << "." << imageType;

    if(debug)
        std::cout << "Attempting to load image, " << path.str() << std::endl;

    ok = svlImageIO::Read(image, 0, path.str());
    if(ok != SVL_OK)
    {
        std::cout << "ERROR: svl Failed to load image, " << path.str() << std::endl;
        return false;
    }
    //cv::Mat matImage;
    //matImage=cv::imread(path.str(), 1);
    cv::Mat matImage(image.IplImageRef());
    if(!(matImage.data))
    {
        std::cout << "ERROR: cv::Mat Failed to convert image, " << path.str() << std::endl;
        return false;
    }

    imageSize =  matImage.size();

    // find origin (must preceed corners, additional draws throws off threshold)
    calOriginDetector->detectOrigin(image.IplImageRef());

    // find corners
    calCornerDetector->detectCorners(matImage,image.IplImageRef());

    // find corner correlation
    calibrationGrid = new svlCCCalibrationGrid(image.IplImageRef(), boardSize,squareSize);
    calibrationGrid->correlate(calOriginDetector, calCornerDetector);

    // tracker coords file
    path.str(std::string());
    path << imageDirectory;
    path << imagePrefix;
    path.fill('0');
    path << std::setw(3) << index << std::setw(1);
    path << ".coords";
    currentFileName = path.str();

    if(debug)
        std::cout << "Reading coords for " << currentFileName << std::endl;

    svlCCTrackerCoordsFileIO coordsFileIO(currentFileName.c_str());
    ok = coordsFileIO.parseFile();
    if(ok)
    {
        coordsFileIO.repackData();
        calibrationGrid->worldToTCP = coordsFileIO.worldToTCP;
        calibrationGrid->hasTracking = true;
    }else{
        calibrationGrid->hasTracking = false;
    }

    if(groundTruthTest)
    {
        // points file
        path.str(std::string());
        path << imageDirectory;
        path << "shot";
        path.fill('0');
        path << std::setw(1) << index << std::setw(1);
        path << ".pts";
        currentFileName = path.str();
        if(debug)
            std::cout << "Reading points for " << currentFileName << std::endl;

        svlCCPointsFileIO pointsFileIO(currentFileName.c_str(),svlCCFileIO::IMPROVED);
        pointsFileIO.parseFile();
        pointsFileIO.repackData(image.IplImageRef());

        //save ground truth
        calibrationGrid->groundTruthImagePoints = pointsFileIO.imagePoints;
        calibrationGrid->groundTruthCalibrationGridPoints = pointsFileIO.calibrationGridPoints;
        calibrationGrid->setGroundTruthTransformation(dlrFileIO->cameraMatrices[index]);
        if(dlrFileIO->cameraMatrices.size() < index || (dlrFileIO->cameraMatrices[index]->data.fl[0] <= -1999))
            calibrationGrid->validGroundTruth = false;
        else
            calibrationGrid->validGroundTruth = true;
    }

    //save images and calibration grids
    images.push_back(image);
    calibrationGrids.push_back(calibrationGrid);
    if (!calibrationGrids.back()->valid)
    {
        std::cout << "svlCCCameraCalibration.processImage() - IMAGE " << currentFileName << " NOT VALID!"<< std::endl;
    }
    return calibrationGrids.back()->valid;
}

/**************************************************************************************************
* process				
*	Process images individually for calibration
*	
* Input:
*	imageDirectory		string						- Directory where images are
*	imagePrefix			string						- Common prefix for images
*	imageType			string						- Commen appendix for images
*	startIndex			int							- Image index to start
*	stopIndex			int							- Image index to end
*
* Output:
*	bool											- Success indicator						
*
***********************************************************************************************************/
bool svlCCCameraCalibration::process(std::string imageDirectory, std::string imagePrefix, std::string imageType, int startIndex, int stopIndex)
{
    reset();
    bool validImage = false;
    bool valid = false;
    std::string currentFileName;
    std::string currentImagePrefix;

    // DLR calibration
    if(groundTruthTest)
    {
        currentFileName = imageDirectory;
        currentImagePrefix = "camera_calibration_callab_matlab";
        currentFileName.append(currentImagePrefix.append(".m"));
        if(debug)
            std::cout << "Reading DLR calibration info for " << currentFileName << std::endl;

        dlrFileIO = new svlCCDLRCalibrationFileIO(currentFileName.c_str());
        dlrFileIO->parseFile();
        dlrFileIO->repackData(stopIndex-startIndex+1);
        this->groundTruthCameraMatrix = dlrFileIO->cameraMatrix;
        this->groundTruthDistCoeffs = dlrFileIO->distCoeffs;
    }

    for(int i=startIndex;i<stopIndex+1;i++){
        validImage = processImage(imageDirectory, imagePrefix, imageType, i);
        valid = validImage || valid;
    }

    if (!valid)
    {
        std::cout << "svlCCCameraCalibration.process() - NO VALID IMAGES! Please acquire more images and try again! " << currentFileName << std::endl;
    }

    return valid;
}

void svlCCCameraCalibration::updateCameraGeometry()
{
    double alpha = 0.0;//assumed to be square pixels
    f = vct2(cameraMatrix.at<double>(0,0),cameraMatrix.at<double>(1,1));
    c = vct2(cameraMatrix.at<double>(0,2),cameraMatrix.at<double>(1,2));
    k = vctFixedSizeVector<double,7>(distCoeffs.at<double>(0,0),distCoeffs.at<double>(1,0),distCoeffs.at<double>(2,0),distCoeffs.at<double>(3,0),distCoeffs.at<double>(4,0),0.0,0.0);//-0.36,0.1234,0.0,0,0);//
    //cameraGeometry = new svlSampleCameraGeometry();
    cameraGeometry->SetIntrinsics(f,c,alpha,k);
}

int svlCCCameraCalibration::setRectifier(svlFilterImageRectifier *rectifier)
{
    //if(debug)
    std::cout << "==========setRectifier==============" << std::endl;

    //int result = rectifier->GetInput("calibration")->PushSample(cameraGeometry);

    int result = rectifier->SetTableFromCameraCalibration(imageSize.height,imageSize.width, vct3x3::Eye(),f,c,k,0,0);

    printf("rectifier returns: %d\n", result);
    return result;
}

int svlCCCameraCalibration::setFilterSourceDummy(svlFilterSourceDummy* source, int index)
{
    if(index > images.size()-1)
        return SVL_FAIL;

    source->SetImageOverwrite(images.at(index));

    return SVL_OK;
}

int svlCCCameraCalibration::setImageVisibility(int index, int visible)
{
    if(index > calibrationGrids.size()-1)
        return SVL_FAIL;

    calibrationGrids.at(index)->valid = (visible == 1);
    return SVL_OK;
}

void svlCCCameraCalibration::runTest()
{
    std::cout << std::endl;
    std::cout << "=============Test==============" << std::endl;
    //////////////////////compare to ground truth
    if(groundTruthTest)
    {
        projectedObjectPoints.clear();
        projectedImagePoints.clear();

        for(int i=0;i<(int)calibrationGrids.size();i++)
        {
            if(calibrationGrids.at(i)->valid && calibrationGrids.at(i)->validGroundTruth)
            {    
                std::cout << "Compare ground truth for grid: " <<i<< std::endl;
                calibrationGrids.at(i)->compareGroundTruth();
            }
        }
    }

    calibrate(false, true);

    std::vector<std::vector<cv::Point3f> > testObjectPoints;
    std::vector<std::vector<cv::Point2f> > testImagePoints;
    std::vector<cv::Mat> testRvecs, testTvecs;
    cv::Mat testCameraMatrix, testDistCoeffs;
    bool projected = false;
    std::vector<float> reprojErrs;
    testCameraMatrix = cv::Mat::eye(3, 3, CV_64F);
    testDistCoeffs  = cv::Mat::zeros(5, 1, CV_64F);
    testCameraMatrix = this->groundTruthCameraMatrix;
    testDistCoeffs = this->groundTruthDistCoeffs;

    for(int i=0;i<(int)calibrationGrids.size();i++)
    {
        if(calibrationGrids.at(i)->validGroundTruth && calibrationGrids.at(i)->groundTruthCalibrationGridPoints.size() > 5)
        {
            testObjectPoints.push_back(calibrationGrids.at(i)->groundTruthCalibrationGridPoints);
            testImagePoints.push_back(calibrationGrids.at(i)->groundTruthImagePoints);
            testRvecs.push_back(calibrationGrids.at(i)->groundTruthRvec);
            testTvecs.push_back(calibrationGrids.at(i)->groundTruthTvec);
        }
    }

    if(testObjectPoints.size() > 0)
        avgErr = computeReprojectionErrors(testObjectPoints, testImagePoints,
                 testRvecs, testTvecs, testCameraMatrix, testDistCoeffs, reprojErrs, projected);  

    std::cout << "GroundTruth's average L1 Norm error: " << avgErr <<std::endl;

    std::cout << std::endl;

}
