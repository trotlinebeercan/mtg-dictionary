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

#include <QApplication>

#include "DownloadCards_Window.h"

int
main(int argc, char **argv)
{
    QApplication qt(argc, argv);

    mtg::DownloadCards_Window window;
    window.show();

    return qt.exec();
}