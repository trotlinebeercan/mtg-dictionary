//! ----------------------------------------------------------------------------
//! CardScanner.h
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <cmath>
#include <deque>
#include <numeric>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace mtg
{
    class CardScanner
    {
    public:
        CardScanner(cv::VideoCapture *_camera);

    public:
        bool checkForCard(cv::Mat &_detectedCard);

    private:
        void grabFrame();
        void updateBackground();
        void checkForMovement();

        int32_t calculateBiggestDifference();
        float calculateBackgroundSimilarity();

        void detectCard(cv::Mat const &_gray, cv::Mat const &_grayBase, std::vector<cv::Point2f> &_corners);
        void getRectifiedCard(cv::Mat const &_inputColor, std::vector<cv::Point2f> const &_corners);
        void reorderCornerVertices(std::vector<cv::Point2f> &corners);

    private:
        std::deque<cv::Mat> mRecentFrames;
        std::deque<cv::Mat> mRecentFramesGray;
        cv::VideoCapture *mCamera;
        int32_t  mRecentFramesMax;
        int32_t  mNumPixels;
        cv::Mat  mLastFrame;
        cv::Mat  mLastFrameGray;
        cv::Mat  mLastFrameFlipped;
        cv::Mat  mSnapshot;
        cv::Mat  mBackground;
        cv::Mat  mBackgroundGray;
        cv::Mat  mBackgroundFlipped;
        cv::Size mImageSize;
        bool mHasMoved;
        bool mFound;
    };
}
