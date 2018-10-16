//! ----------------------------------------------------------------------------
//! CardMatcher.cpp
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#include "CardMatcher.h"

#include "Log.h"

void mtg::loadAllSets(QString const &_directory, std::vector<mtg::Card> &_cards)
{
    _cards.clear();

    QDirIterator setFolders(_directory, QDir::AllEntries | QDir::NoDotAndDotDot);
    while (setFolders.hasNext())
    {
        QDirIterator images(setFolders.next(), QStringList() << "*.png");
        while (images.hasNext())
        {
            images.next();

            mtg::Card card;
            card.fileName = images.filePath().toStdString();
            card.setName  = setFolders.fileInfo().baseName().toStdString();
            card.image = cv::imread(card.fileName.c_str());
            getImageDCTHash(card.image, card.dctHash);

            _cards.push_back(card);
        }
    }
}

void mtg::getImageDCTHash(cv::Mat const &_source, cv::Mat &_hash)
{
    cv::Mat sourceFloat;
    cv::cvtColor(_source, sourceFloat, CV_BGR2GRAY);
    sourceFloat.convertTo(sourceFloat, CV_32F, 1.f / 255.f);
    sourceFloat = cv::Mat(sourceFloat, cv::Rect(16, 31, 194, 144));

    cv::Mat cardArt(sourceFloat.size(), CV_32F);
    cv::resize(sourceFloat, cardArt, cv::Size(32, 32));

    cv::Mat dct(cv::Size(32, 32), CV_32F);
    cv::dct(cardArt, dct);
    dct = cv::Mat(dct, cv::Rect(1, 1, 8, 8));

    cv::Scalar avg = cv::mean(dct)[0];
    cv::Mat avg8Bit(dct.size(), CV_8UC1);
    cv::compare(dct, avg, avg8Bit, cv::CMP_GT);

    _hash = (avg8Bit == 255);
}

float mtg::getHammingDistance(cv::Mat const &_image0, cv::Mat const &_image1)
{
    assert(_image0.size() == _image1.size());

    float sum = 0.f;
    for (int32_t j = 0; j < _image0.size().height; j++)
    {
        for (int32_t i = 0; i < _image0.size().width; i++)
        {
            sum += (_image0.at<uint8_t>(j, i) != _image1.at<uint8_t>(j, i));
        }
    }

    return sum;
}

void mtg::getCandidateMatches(cv::Mat const &_cardImage, std::vector<mtg::Card> const &_cache, std::vector<mtg::Card> &_candidates)
{
    _candidates.clear();

    cv::Mat phash;
    getImageDCTHash(_cardImage, phash);

    std::map<float, mtg::Card> sortedDistances;
    for (auto const &card : _cache)
    {
        float const dist = getHammingDistance(card.dctHash, phash);
        sortedDistances[dist] = card;
    }

    std::map<float, mtg::Card>::const_iterator idx = sortedDistances.begin();
    while (idx != sortedDistances.end() || _candidates.size() == 20)
    {
        _candidates.push_back(idx->second);
        idx++;
    }
}
