//! ----------------------------------------------------------------------------
//! SquareDetection.h
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#pragma once

#include <cmath>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <vector>

namespace mtg
{
    //! For the sake of brevity
    typedef std::vector< cv::Point > Square;
    typedef std::vector<   Square  > SquaresVector;

    //! Finds the cosine of angle between vectors from p0->p1 and from p0->p2
    float getAngleBetweenVectors(cv::Point p1, cv::Point p2, cv::Point p0);

    //! Returns sequence of squares detected on the input image
    void findSquares(cv::Mat const &input_image, int32_t threshold, int32_t levels, mtg::SquaresVector &squares);

    //! Returns sequence of lines detected on the input image
    void findLines(cv::Mat const &input_image, std::vector<cv::Vec4i> &lines);

    //! Reorders the vertices of a square to always contain { TL, BL, BR, TR }
    void reorderSquareVertices(mtg::Square &square);

    //! Performs perspective warping to rectify the square image from the input
    void warpImage(mtg::Square const &rect, cv::Mat const &input_image, cv::Mat &output_image);

    //! Overloaded version of warpImage to warp a collection of squares
    void warpImages(mtg::SquaresVector const &squares, cv::Mat const &input_image, std::vector<cv::Mat> &output_images);

    //! Iterates through a list of rectified images and finds the best fit image
    void findBestImage(std::vector <cv::Mat> const &rectified_images, cv::Mat const &input_image, cv::Mat &best_image);

    //! DEBUG: Draws a square onto an image
    void drawSquare(mtg::Square const &square, cv::Mat &output_image);

    //! DEBUG: Overloaded version of drawSquare to draw a collection of squares
    void drawSquares(mtg::SquaresVector const &squares, cv::Mat &output_image);

    //! DEBUG: Draws a collection of lines onto an image
    void drawLines(std::vector<cv::Vec4i> const &lines, cv::Mat &output_image);
};

