//! ----------------------------------------------------------------------------
//! CardDetector.h
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#pragma once

#include <opencv2/core/core.hpp>

namespace mtg
{
    class CardDetector
    {
    public:
        CardDetector();

    public:
        void detectCard(cv::Mat const &_gray, cv::Mat const &_grayBase, std::vector<cv::Point2f> &_corners);
        void getRectifiedCard(cv::Mat const &_inputColor, std::vector<cv::Point2f> const &_corners);

    private:
        void reorderCornerVertices(std::vector<cv::Point2f> &corners);
    };
}