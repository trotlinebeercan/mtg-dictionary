//! ----------------------------------------------------------------------------
//! Main.cpp
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#include "CardScanner.h"
#include "CardMatcher.h"
#include "Log.h"

#include <QApplication>

int32_t mainApplication(int argc, char **argv)
{
    QApplication qt(argc, argv);

    std::vector<mtg::Card> cardCache;
    mtg::loadAllSets("./data", cardCache);

    cv::VideoCapture camera(0);
    if (!camera.isOpened())
    {
        mtg_error("Was not able to find/open camera.");
        return EXIT_FAILURE;
    }

    camera.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(CV_CAP_PROP_FRAME_HEIGHT, 720);

    mtg_debug("Found camera, starting card detection...");

    mtg::CardScanner scanner(&camera);

    cv::Mat card;
    while (true)
    {
        qt.processEvents();

        if (scanner.checkForCard(card))
        {
            cv::imshow("Detected Card", card);

            std::vector<mtg::Card> candidates;
            mtg::getCandidateMatches(card, cardCache, candidates);
            std::for_each(candidates.begin(), candidates.end(), [](mtg::Card const &card) {
                mtg_debug(card.fileName);
            });

            cv::imshow("1st Place Candidate", candidates.at(0).image);
            cv::imshow("2nd Place Candidate", candidates.at(1).image);
            cv::imshow("3rd Place Candidate", candidates.at(2).image);
        }

        cv::waitKey(33);
    }

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
    return mainApplication(argc, argv);
}
