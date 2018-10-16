//! ----------------------------------------------------------------------------
//! CardScanner.cpp
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#include "CardScanner.h"

#include "Log.h"
#include "OpenCVUtility.h"

mtg::CardScanner::CardScanner(cv::VideoCapture *_camera) :
    mCamera(_camera),
    mRecentFramesMax(3),
    mNumPixels(-1),
    mHasMoved(false),
    mFound(false)
{
}

bool mtg::CardScanner::checkForCard(cv::Mat &_detectedCard)
{
    mFound = false;

    grabFrame();
    updateBackground();
    checkForMovement();

    if (mFound)
    {
        _detectedCard = mSnapshot.clone();
    }

    cv::Mat smallerCameraFeedFrame;
    cv::resize(mLastFrame, smallerCameraFeedFrame, cv::Size(640, 480));
    cv::imshow("Camera Feed", smallerCameraFeedFrame);

    return mFound;
}

void mtg::CardScanner::grabFrame()
{
    cv::Mat frame, frameGray, frameFlipped;
    mCamera->operator>>(frame);

    cv::cvtColor(frame, frameGray, CV_BGR2GRAY);
    mtg::flipImage(frame, frameFlipped);

    if (mLastFrame.empty())
    {
        mImageSize = frame.size();
        mNumPixels = mImageSize.width * mImageSize.height;
    }

    mRecentFrames.push_back(frame);
    mRecentFramesGray.push_back(frameGray);

    if ((int32_t)mRecentFrames.size() > mRecentFramesMax)
    {
        mRecentFrames.pop_front();
        mRecentFramesGray.pop_front();
    }

    mLastFrame = frame.clone();
    mLastFrameGray = frameGray;
    mLastFrameFlipped = frameFlipped;
}

void mtg::CardScanner::updateBackground()
{
    if (mBackground.empty())
    {
        mBackground = mLastFrame.clone();
        cv::cvtColor(mBackground, mBackgroundGray, CV_BGR2GRAY);
        mtg::flipImage(mBackground, mBackgroundFlipped);
    }
}

void mtg::CardScanner::checkForMovement()
{
    if (calculateBiggestDifference() > 10)
    {
        mHasMoved = true;

        mtg_debug("movement detected inside calculateBiggestDifference");
    }
    else if (mHasMoved)
    {
        if (calculateBackgroundSimilarity() > 0.75f)
        {
            mHasMoved = false;
            mtg_debug("false alarm...");
        }
        else
        {
            std::vector<cv::Point2f> corners;
            mtg_debug("running detectCard...");
            detectCard(mLastFrameGray, mBackgroundGray, corners);
            if (corners.size() > 0)
            {
                std::vector<cv::Point2f>::const_iterator cornersIdx = corners.begin();
                while (cornersIdx != corners.end())
                {
                    mtg_debug("Corner: " << cornersIdx->x << ", " << cornersIdx->y);
                    cornersIdx++;
                }
                getRectifiedCard(mLastFrame, corners);
                mFound = true;
            }
            else
            {
                mtg_debug("a card was not found");
            }

            mHasMoved = false;
        }
    }
}

void mtg::CardScanner::detectCard(cv::Mat const &_gray, cv::Mat const &_grayBase, std::vector<cv::Point2f> &_corners)
{
    typedef struct Line {
        cv::Point2f c0;
        cv::Point2f c1;
        float length;
        float angle;
    } Line;

    _corners.clear();

    // initial filtering
    mtg_debug("filtering");
    cv::Mat difference = _gray.clone();
    cv::absdiff(_gray, _grayBase, difference);

    cv::Mat edges = _gray.clone();
    cv::Canny(difference, edges, 100, 100);

    // find contours
    mtg_debug("finding contours");
    std::vector< std::vector<cv::Point> > contours;
    cv::findContours(edges, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    // extract edge points
    std::vector<cv::Point2f> edgePoints;
    for (int32_t c = 0; c < (int32_t)contours.size(); c++)
    {
        if (contours.at(c).size() > 10)
        {
            for (int32_t cc = 0; cc < (int32_t)contours.at(c).size(); cc++)
            {
                edgePoints.push_back(contours.at(c).at(cc));
            }
        }
    }

    if (edgePoints.size() == 0)
    {
        mtg_debug("edgePoints.size() == 0");
        return;
    }
    else
    {
        mtg_debug("we may have found a card");
    }

    // wrap convex hull around contours
    mtg_debug("convex hull");
    std::vector<cv::Point2f> hull;
    cv::convexHull(edgePoints, hull, true);

    // convert into lines
    mtg_debug("converting lines");
    std::vector<Line> lines(hull.size());
    for (int32_t l = 0; l < (int32_t)hull.size(); l++)
    {
        cv::Point2f const p0 = cv::Point2f(hull.at(l).x, hull.at(l).y);
        cv::Point2f const p1 = cv::Point2f(hull.at((l + 1) % hull.size()).x, hull.at((l + 1) % hull.size()).y);

        Line line;
        line.c0 = p0;
        line.c1 = p1;
        line.length = std::sqrt(std::pow(p1.x - p0.x, 2.f) + std::pow(p1.y - p0.y, 2.f));
        line.angle  = std::atan2(p1.y - p0.y, p1.x - p0.x);
        lines.at(l) = line;
    }

    // straighten out lines
    mtg_debug("straightening out lines");
    int32_t lIdx = 0;
    while (lIdx + 1 < lines.size())
    {
        Line l0 = lines.at(lIdx);
        Line l1 = lines.at((lIdx + 1) % lines.size());

        if (std::fabs(l0.angle - l1.angle) / (CV_PI * 2.f) < 0.0027)
        {
            cv::Point2f const &p0 = l0.c0;
            cv::Point2f const &p1 = l1.c1;

            Line line;
            line.c0 = p0;
            line.c1 = p1;
            line.length = std::sqrt(std::pow(p1.x - p0.x, 2.f) + std::pow(p1.y - p0.y, 2.f));
            line.angle  = std::atan2(p1.y - p0.y, p1.x - p0.x);
            lines.at(lIdx) = line;
            lines.erase(lines.begin() + lIdx + 1);
        }
        else
        {
            lIdx++;
        }
    }

    // sort the lines
    mtg_debug("sorting lines");
    std::sort(lines.begin(), lines.end(),
        [](Line const &lhs, Line const &rhs) {
            return lhs.length > rhs.length;
        });

    // compute perimeter
    mtg_debug("computing perimeter");
    float perimeter = std::accumulate(lines.begin(), lines.end(), float{},
        [](float result, Line const &line) {
            return result + line.length;
        });

    mtg_debug("Perimeter after detection = " << perimeter);

    if (perimeter > 700)
    {
        std::vector<Line> firstFourLines;
        firstFourLines.push_back(lines.at(0));
        firstFourLines.push_back(lines.at(1));
        firstFourLines.push_back(lines.at(2));
        firstFourLines.push_back(lines.at(3));

        float const firstFourSum = std::accumulate(firstFourLines.begin(), firstFourLines.end(), float{},
            [](float result, const Line &line) {
                return result + line.length;
            });

        if (firstFourSum / perimeter > 0.7f)
        {
            std::vector<cv::Point2f> corners(4);
            std::vector<Line> sides = firstFourLines;
            std::sort(sides.begin(), sides.end(),
                [](Line const &lhs, Line const &rhs) {
                    return lhs.angle > rhs.angle;
                });

            for (int32_t i = 0; i < 4; i++)
            {
                // find where the lines intersect to get true corner points for the rectangle
                auto intersectLine = [&](Line const &s0, Line const &s1)
                {
                    float const x1 = s0.c0.x, y1 = s0.c0.y;
                    float const x2 = s0.c1.x, y2 = s0.c1.y;
                    float const x3 = s1.c0.x, y3 = s1.c0.y;
                    float const x4 = s1.c1.x, y4 = s1.c1.y;
                    float const denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

                    if (denom == 0)
                    {
                        return cv::Point2f(-1, -1);
                    }

                    float const x = ((x1 * y2 - y1 * x2) * (x3 - x4) - (x1 - x2) * (x3 * y4 - y3 * x4)) / float(denom);
                    float const y = ((x1 * y2 - y1 * x2) * (y3 - y4) - (y1 - y2) * (x3 * y4 - y3 * x4)) / float(denom);

                    return cv::Point2f(x, y);
                };

                corners.at(i) = intersectLine(sides.at(i), sides.at((i + 1) % 4));
            }

            std::vector<cv::Point2f>::const_iterator cornersIdx = corners.begin();
            while (cornersIdx != corners.end())
            {
                if (cornersIdx->x == -1 || cornersIdx->y == -1)
                {
                    mtg_debug("corners were -1, -1");
                }
                cornersIdx++;
            }

            reorderCornerVertices(corners);
            _corners = corners;
        }
        else
        {
            mtg_debug("Made it all the way to the last step, but the perimeter wasn't a sufficient size.");
        }
    }
}

void mtg::CardScanner::getRectifiedCard(cv::Mat const &_inputColor, std::vector<cv::Point2f> const &_corners)
{
    // order is guaranteed from mtg::reorderSquareVertices
    cv::Point2f topLeft     = _corners.at(0);
    cv::Point2f bottomLeft  = _corners.at(1);
    cv::Point2f bottomRight = _corners.at(2);
    cv::Point2f topRight    = _corners.at(3);

    // gather target width and height based on rect dimensions
    float const width0  = std::sqrt(std::powf(bottomRight.x - bottomLeft.x, 2.f) + std::powf(bottomRight.y - bottomLeft.y, 2.f));
    float const width1  = std::sqrt(std::powf(topRight.x - topLeft.x, 2.f) + std::powf(topRight.y - topLeft.y, 2.f));
    float const height0 = std::sqrt(std::powf(topRight.x - bottomRight.x, 2.f) + std::powf(topRight.y - bottomRight.y, 2.f));
    float const height1 = std::sqrt(std::powf(topLeft.x - bottomLeft.x, 2.f) + std::powf(topLeft.y - bottomLeft.y, 2.f));

    // NOTE: why is this transposed?
    int32_t const maxWidth = std::roundf(std::max(height0, height1));
    int32_t const maxHeight = std::roundf(std::max(width0, width1));

    std::vector<cv::Point2f> sourceRect;
    sourceRect.push_back(topLeft);
    sourceRect.push_back(bottomLeft);
    sourceRect.push_back(bottomRight);
    sourceRect.push_back(topRight);

    std::vector<cv::Point2f> destRect;
    destRect.push_back(cv::Point2f(0, 0));
    destRect.push_back(cv::Point2f(0, maxHeight));
    destRect.push_back(cv::Point2f(maxWidth, maxHeight));
    destRect.push_back(cv::Point2f(maxWidth, 0));

    // probably not needed, but here to be extra safe
    // ----------------------------------------------
    reorderCornerVertices(destRect);
    // ----------------------------------------------

    // allocate output image and perform perspective warp to rectify square image
    mSnapshot = cv::Mat(cv::Size(maxWidth, maxHeight), _inputColor.type());
    cv::Mat perspective = cv::getPerspectiveTransform(cv::Mat(sourceRect), cv::Mat(destRect));
    cv::warpPerspective(_inputColor, mSnapshot, perspective, cv::Size(maxWidth, maxHeight));
    cv::resize(mSnapshot, mSnapshot, cv::Size(222, 311));
    mtg::flipImage(mSnapshot, mSnapshot);
}

int32_t mtg::CardScanner::calculateBiggestDifference()
{
    float maxDifference = 0;
    for (int32_t img = 0; img < (int32_t)mRecentFramesGray.size(); img++)
    {
        cv::Mat const &image = mRecentFramesGray.at(img);
        maxDifference = std::max(mtg::sumSquared(mLastFrameGray, image) / mNumPixels, maxDifference);
    }

    return maxDifference;
}

float mtg::CardScanner::calculateBackgroundSimilarity()
{
    float minSim = FLT_MAX / 2;
    for (int32_t img = 0; img < (int32_t)mRecentFramesGray.size(); img++)
    {
        cv::Mat const &image = mRecentFramesGray.at(img);
        minSim = std::min(mtg::coeffNormed(mBackgroundGray, image), minSim);
    }

    return minSim;
}

void mtg::CardScanner::reorderCornerVertices(std::vector<cv::Point2f> &_corners)
{
    // to simulate in-place modification
    std::vector<cv::Point2f> rect = _corners;

    // topLeft will always have the smallest sum
    // bottomLeft will always have the largest difference
    // bottomRight will always have the largest sum
    // topRight will always have the smallest difference

    int32_t smallSum  = +INT_MAX / 2, smallSumIndex  = -1;
    int32_t largeSum  = -INT_MAX / 2, largeSumIndex  = -1;
    int32_t smallDiff = +INT_MAX / 2, smallDiffIndex = -1;
    int32_t largeDiff = -INT_MAX / 2, largeDiffIndex = -1;
    for (int32_t pt = 0; pt < (int32_t)rect.size(); pt++)
    {
        int32_t const sum  = rect.at(pt).x + rect.at(pt).y;
        int32_t const diff = rect.at(pt).x - rect.at(pt).y;
        if (sum < smallSum)
        {
            smallSum = sum;
            smallSumIndex = pt;
        }

        if (sum > largeSum)
        {
            largeSum = sum;
            largeSumIndex = pt;
        }

        if (diff < smallDiff)
        {
            smallDiff = diff;
            smallDiffIndex = pt;
        }

        if (diff > largeDiff)
        {
            largeDiff = diff;
            largeDiffIndex = pt;
        }
    }

    cv::Point2f topLeft     = rect.at(smallSumIndex);
    cv::Point2f bottomLeft  = rect.at(largeDiffIndex);
    cv::Point2f bottomRight = rect.at(largeSumIndex);
    cv::Point2f topRight    = rect.at(smallDiffIndex);

    // ensure the corners will always hold the vertices in this order
    _corners.clear();
    _corners.push_back(topLeft);
    _corners.push_back(bottomLeft);
    _corners.push_back(bottomRight);
    _corners.push_back(topRight);
}
