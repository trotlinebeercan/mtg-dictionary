//! ----------------------------------------------------------------------------
//! CardMatcher.h
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#pragma once

#include <map>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>
#include <QtCore>

#include "Log.h"

std::string const kAllAvailableSets[] = {
    "BFZ", "BNG", "DTK",
    "FRF", "JOU", "KTK",
    "M15", "ORI", "THS"
};

namespace mtg
{
    typedef struct Card
    {
        std::string fileName;
        std::string setName;
        cv::Mat image;
        cv::Mat dctHash;
    } Card;

    void loadAllSets(QString const &_directory, std::vector<mtg::Card> &_cards);
    void getImageDCTHash(cv::Mat const &_source, cv::Mat &_hash);
    float getHammingDistance(cv::Mat const &_image0, cv::Mat const &_image1);
    void getCandidateMatches(cv::Mat const &_cardImage, std::vector<mtg::Card> const &_cache, std::vector<mtg::Card> &_candidates);
}
