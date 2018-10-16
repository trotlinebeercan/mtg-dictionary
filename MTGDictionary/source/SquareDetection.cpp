//! ----------------------------------------------------------------------------
//! SquareDetection.cpp
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#include "SquareDetection.h"

#include "Log.h"

float mtg::getAngleBetweenVectors(cv::Point pt1, cv::Point pt2, cv::Point pt0)
{
    float const dx1 = pt1.x - pt0.x;
    float const dy1 = pt1.y - pt0.y;
    float const dx2 = pt2.x - pt0.x;
    float const dy2 = pt2.y - pt0.y;

    float const a = dx1 * dx2 + dy1 * dy2;
    float const b = dx1 * dx1 + dy1 * dy1;
    float const c = dx2 * dx2 + dy2 * dy2;
    float const d = 1e-10;

    return a / std::sqrt(b * c + d);
}

void mtg::findSquares(cv::Mat const &input_image, int32_t threshold, int32_t levels, mtg::SquaresVector &squares)
{
    squares.clear();

    mtg::SquaresVector contours;
    cv::Mat pyr, tempImage, gray, gray0(input_image.size(), CV_8U);

    // downscale and upscale the image to filter out noise
    cv::pyrDown(input_image, pyr, cv::Size(input_image.size().width / 2, input_image.size().height / 2));
    cv::pyrUp(pyr, tempImage, input_image.size());

    // find squares in every color plane of the image
    for (int32_t chan = 0; chan < 3; chan++)
    {
        int32_t ch[] = { chan, 0 };
        cv::mixChannels(&tempImage, 1, &gray0, 1, ch, 1);

        // try several threshold levels
        for (int32_t lvl = 0; lvl < levels; lvl++)
        {
            // use canny instead on zero threshold level
            // helps to catch squares with gradient shading
            if (lvl == 0)
            {
                cv::Canny(gray0, gray, 0, threshold, 5);
                cv::dilate(gray, gray, cv::Mat(), cv::Point(-1, -1));
            }
            else
            {
                gray = gray0 >= (lvl + 1) * 255 / levels;
            }

            // detect contours on the image
            cv::findContours(gray, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

            mtg::Square approx;
            for (int32_t idx = 0; idx < (int32_t)contours.size(); idx++)
            {
                // approximate contour with accuracy proportional to the contour perimeter
                cv::approxPolyDP(cv::Mat(contours[idx]), approx, cv::arcLength(cv::Mat(contours[idx]), true) * 0.02f, true);

                // square contours should have 4 vertices, have a relatively large area, and be convex
                if (approx.size() == 4 && std::fabs(cv::contourArea(cv::Mat(approx))) > 1000 && cv::isContourConvex(cv::Mat(approx)))
                {
                    float maxCosine = 0.f;

                    for (int32_t jdx = 2; jdx < 5; jdx++)
                    {
                        // find the maximum cosine of the angle between joint edges
                        float const cosine = std::fabs(mtg::getAngleBetweenVectors(approx[jdx % 4], approx[jdx - 2], approx[jdx - 1]));
                        maxCosine = std::max(cosine, maxCosine);
                    }

                    // if cosines of all angles are small (~90 degrees)
                    if (maxCosine < 0.3f)
                    {
                        mtg::reorderSquareVertices(approx);
                        squares.push_back(approx);
                    }
                }
            }
        }
    }
}

void mtg::findLines(cv::Mat const &input_image, std::vector<cv::Vec4i> &lines)
{
    lines.clear();

    cv::HoughLinesP(input_image, lines, 1, CV_PI / 180.f, 100, 50, 10);
}

void mtg::reorderSquareVertices(mtg::Square &square)
{
    // to simulate in-place modification
    mtg::Square rect = square;

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

    // ensure the square will always hold the vertices in this order
    square.clear();
    square.push_back(topLeft);
    square.push_back(bottomLeft);
    square.push_back(bottomRight);
    square.push_back(topRight);
}

void mtg::warpImage(mtg::Square const &rect, cv::Mat const &input_image, cv::Mat &output_image)
{
    // order is guaranteed from mtg::reorderSquareVertices
    cv::Point2f topLeft     = rect.at(0);
    cv::Point2f bottomLeft  = rect.at(1);
    cv::Point2f bottomRight = rect.at(2);
    cv::Point2f topRight    = rect.at(3);

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

    std::vector<cv::Point> destTmp;
    destTmp.push_back(cv::Point2f(0, 0));
    destTmp.push_back(cv::Point2f(0, maxHeight));
    destTmp.push_back(cv::Point2f(maxWidth, maxHeight));
    destTmp.push_back(cv::Point2f(maxWidth, 0));

    // probably not needed, but here to be extra safe
    // ----------------------------------------------
    mtg::reorderSquareVertices(destTmp);
    std::vector<cv::Point2f> destRect;
    destRect.push_back(destTmp.at(0));
    destRect.push_back(destTmp.at(1));
    destRect.push_back(destTmp.at(2));
    destRect.push_back(destTmp.at(3));
    // ----------------------------------------------

    // allocate output image and perform perspective warp to rectify square image
    output_image = cv::Mat(cv::Size(maxWidth, maxHeight), input_image.type());
    cv::Mat perspective = cv::getPerspectiveTransform(cv::Mat(sourceRect), cv::Mat(destRect));
    cv::warpPerspective(input_image, output_image, perspective, cv::Size(maxWidth, maxHeight));
}

void mtg::warpImages(mtg::SquaresVector const &squares, cv::Mat const &input_image, std::vector<cv::Mat> &output_images)
{
    // ensure we're not populating a vector with images present
    output_images.clear();

    // iterate through each square and rectify
    for (int32_t sq = 0; sq < (int32_t)squares.size(); sq++)
    {
        cv::Mat rectifiedImage;
        mtg::warpImage(squares.at(sq), input_image, rectifiedImage);
        output_images.push_back(rectifiedImage);
    }
}

void mtg::findBestImage(std::vector<cv::Mat> const &rectified_images, cv::Mat const &input_image, cv::Mat &best_image)
{
    // load template image
    cv::Mat templateImage = cv::imread("../MTGDictionary/data/template.png");

    // iterate over all rectified images and find the best image
    for (int32_t img = 0; img < (int32_t)rectified_images.size(); img++)
    {
        // get the current image and store as grayscale
        cv::Mat image;
        cv::cvtColor(rectified_images.at(img), image, CV_BGR2GRAY);

        // resize the template image and store as grayscale
        cv::Mat resizedTemplateImage;
        cv::resize(templateImage, resizedTemplateImage, image.size());
        cv::cvtColor(resizedTemplateImage, resizedTemplateImage, CV_BGR2GRAY);

        cv::imshow("Rectified Image", image);
        cv::waitKey(0);
    }
}

void mtg::drawSquare(mtg::Square const &square, cv::Mat &output_image)
{
    mtg::SquaresVector tmp;
    tmp.push_back(square);
    drawSquares(tmp, output_image);
}

void mtg::drawSquares(mtg::SquaresVector const &squares, cv::Mat &output_image)
{
    // iterate through each square and draw on the output image
    for (int32_t idx = 0; idx < (int32_t)squares.size(); idx++)
    {
        const cv::Point *pt = &squares[idx][0];
        int32_t n = (int32_t)squares[idx].size();
        cv::polylines(output_image, &pt, &n, 1, true, cv::Scalar(0, 255, 0), 3);
    }
}

void mtg::drawLines(std::vector<cv::Vec4i> const &lines, cv::Mat &output_image)
{
    for (int32_t i = 0; i < (int32_t)lines.size(); i++)
    {
        cv::Vec4i const &line = lines.at(i);
        cv::line(output_image, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), cv::Scalar(0, 0, 255), 1, CV_AA);
    }
}
