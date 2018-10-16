//! ----------------------------------------------------------------------------
//! OpenCVUtility.h
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

namespace mtg
{
    float sumSquared(cv::Mat const &lhs, cv::Mat const &rhs)
    {
        cv::Mat tmp;
        cv::subtract(lhs, rhs, tmp);
        cv::pow(tmp, 2.f, tmp);
        return cv::sum(tmp)[0];
    }

    float coeffNormed(cv::Mat const &lhs, cv::Mat const &rhs)
    {
        cv::Mat tmp0, tmp1;
        lhs.convertTo(tmp0, CV_32F, 1.f / 255.f);
        rhs.convertTo(tmp1, CV_32F, 1.f / 255.f);

        cv::subtract(tmp0, cv::mean(tmp0), tmp0);
        cv::subtract(tmp1, cv::mean(tmp1), tmp1);

        cv::Mat norm0 = tmp0.clone();
        cv::Mat norm1 = tmp1.clone();
        cv::pow(tmp0, 2.f, norm0);
        cv::pow(tmp1, 2.f, norm1);

        return tmp0.dot(tmp1) / std::sqrtf(cv::sum(norm1)[0] * cv::sum(norm1)[0]);
    }

    void flipImage(cv::Mat const &source, cv::Mat &target)
    {
        cv::flip(source, target, -1);
    }
}