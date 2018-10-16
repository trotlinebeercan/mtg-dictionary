//! ----------------------------------------------------------------------------
//! DownloadCards_Window.cpp
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#include "DownloadCards_Window.h"

#include <curl/curl.h>
#include <qjson/parser.h>

#include "Log.h"
#include "ui_DownloadCards_Window.h"

mtg::DownloadCards_Window::DownloadCards_Window() :
    QMainWindow(),
    mUi(new Ui::DownloadCardsMainWindow),
    mState(mtg::DownloadCards_Window::State::SELECTING_SET),
    mCardInfoNetworkManager(this),
    mCardImageNetworkManager(this),
    mCurrentSetSelected(0),
    mTotalNumberOfCardsInSet(-1),
    mCurrentCardDownloaded(-1),
    mCurrentCardSetPageNumber(1),
    mUserForcedShutdown(false)
{
    mUi->setupUi(this);

    // connect signals and slots for UI elements
    QMainWindow::connect(mUi->button_StartDownload, SIGNAL(clicked()),
                         this, SLOT(slot_downloadButtonClicked()));
    QMainWindow::connect(mUi->combo_AvailableSets, SIGNAL(currentIndexChanged(int)),
                         this, SLOT(slot_newSetSelected(int)));
    QMainWindow::connect(&mCardInfoNetworkManager, SIGNAL(finished(QNetworkReply *)),
                         this, SLOT(slot_replyReceivedFromCardInfoRequest(QNetworkReply *)));
    QMainWindow::connect(&mCardImageNetworkManager, SIGNAL(finished(QNetworkReply *)),
                         this, SLOT(slot_replyReceivedFromCardImageRequest(QNetworkReply *)));
    QMainWindow::connect(this, SIGNAL(signal_updateUi()), this, SLOT(slot_updateUi()));
    QMainWindow::connect(this, SIGNAL(signal_resetUi()), this, SLOT(slot_resetUi()));
    QMainWindow::connect(this, SIGNAL(signal_downloadCardsFromSet(int, QString)),
                         this, SLOT(slot_downloadCardsFromSet(int, QString)));
    QMainWindow::connect(this, SIGNAL(signal_downloadImageFromCard(QVariantMap)),
                         this, SLOT(slot_downloadImageFromCard(QVariantMap)));

    // initialize the progress bar
    setWindowTitle("MTG Gatherer-er");
    mUi->progbar_TotalProgress->setValue(0);

    // set up drop down menu for set selection
    QDirIterator availableSets("../MTGDictionary/data/cardlist/", QStringList() << "*.txt");
    while (availableSets.hasNext())
    {
        availableSets.next();
        QString baseName = availableSets.fileInfo().baseName();
        QString filePath = availableSets.filePath();

        mUi->combo_AvailableSets->addItem(baseName);
        mCardSetNames.push_back(baseName);
        mCardSetFiles.push_back(filePath);
    }

    QDirIterator testSets("./data/", QDir::AllEntries | QDir::NoDotAndDotDot);
    while (testSets.hasNext())
    {
        testSets.next();
        mtg_debug(testSets.fileInfo().baseName().toStdString());
    }
}

void mtg::DownloadCards_Window::processCardsFromChosenSetForDownload()
{
    QString baseName = mCardSetNames.at(mCurrentSetSelected);
    QString filePath = mCardSetFiles.at(mCurrentSetSelected);

    mtg_debug("Chosen set: " << baseName.toStdString() << ", location: " << filePath.toStdString());

    emit signal_downloadCardsFromSet(mCurrentCardSetPageNumber, baseName);

    mtg_debug("Downloading set " << baseName.toStdString() << "...");
    mState = mtg::DownloadCards_Window::State::DOWNLOADING_SET;
    while (mState != mtg::DownloadCards_Window::State::DOWNLOADED_SET && !mUserForcedShutdown)
    {
        QWaitCondition wc;
        QMutex mutex;
        QMutexLocker locker(&mutex);
        wc.wait(&mutex, 10);
    }

    if (mUserForcedShutdown) return;

    mTotalNumberOfCardsInSet = mDownloadedCards.size();

    mtg_debug("Total number of cards in set " << baseName.toStdString() << ": " << mTotalNumberOfCardsInSet);

    QVariantList::iterator cardIdx = mDownloadedCards.begin();
    while (cardIdx != mDownloadedCards.end())
    {
        QVariantMap card   = cardIdx->toMap();
        mCurrentCardName   = card["name"].toString();
        mCurrentCardSet    = card["set"].toString();
        mCurrentCardNumber = card["number"].toString();

        mtg_debug("Downloading image for card " << mCurrentCardName.toStdString() << "...");
        emit signal_downloadImageFromCard(card);
        mState = mtg::DownloadCards_Window::State::DOWNLOADING_IMAGE;
        while (mState != mtg::DownloadCards_Window::State::DOWNLOADED_IMAGE && !mUserForcedShutdown)
        {
            QWaitCondition wc;
            QMutex mutex;
            QMutexLocker locker(&mutex);
            wc.wait(&mutex, 10);
        }

        if (mUserForcedShutdown) return;

        // write the image to the disc
        QString saveDir = QString("./data/%1").arg(mCurrentCardSet);
        if (QDir().mkpath(saveDir))
        {
            QString saveName = QString("%1/%2.png").arg(saveDir).arg(mCurrentCardNumber);
            if (!mCurrentCardImage.save(saveName))
            {
                mtg_error("Unable to save image " << saveName.toStdString() << ".");
            }
        }
        else
        {
            mtg_error("Unable to save image...");
        }

        mCurrentCardDownloaded++;

        emit signal_updateUi();
        mState = mtg::DownloadCards_Window::State::UPDATING_UI;
        while (mState != mtg::DownloadCards_Window::State::UPDATED_UI && !mUserForcedShutdown)
        {
            QWaitCondition wc;
            QMutex mutex;
            QMutexLocker locker(&mutex);
            wc.wait(&mutex, 10);
        }

        if (mUserForcedShutdown) return;

        cardIdx++;
    }

    emit signal_resetUi();
}

void
mtg::DownloadCards_Window::closeEvent(QCloseEvent *event)
{
    mUserForcedShutdown = true;
    return QMainWindow::closeEvent(event);
}

void mtg::DownloadCards_Window::slot_downloadButtonClicked()
{
    if (mCurrentSetSelected >= 0)
    {
        mDownloadedCards.clear();
        mCurrentCardImage   = QPixmap();
        mCurrentCardName    = QString();
        mCurrentCardSet     = QString();
        mCurrentCardNumber  = QString();
        mUserForcedShutdown = false;
        mTotalNumberOfCardsInSet  = 0;
        mCurrentCardDownloaded    = 0;
        mCurrentCardSetPageNumber = 1;
        mState = mtg::DownloadCards_Window::State::SELECTING_SET;

        mUi->combo_AvailableSets->setEnabled(false);
        mUi->button_StartDownload->setEnabled(false);
        mUi->button_StartDownload->setText("Downloading...");
        mUi->progbar_TotalProgress->setValue(0);

        QtConcurrent::run(this, &mtg::DownloadCards_Window::processCardsFromChosenSetForDownload);
    }
}

void mtg::DownloadCards_Window::slot_newSetSelected(int _index)
{
    mCurrentSetSelected = _index;
}

void mtg::DownloadCards_Window::slot_updateUi()
{
    // display the information on the UI
    mUi->label_CardName->setText(mCurrentCardName);
    mUi->label_CardSet->setText(mCurrentCardSet);
    mUi->label_CardNumber->setText(mCurrentCardNumber);
    mUi->label_CardImage->setPixmap(mCurrentCardImage);

    float const totalProgress = (float)mCurrentCardDownloaded / (float)mTotalNumberOfCardsInSet;
    mUi->progbar_TotalProgress->setValue(totalProgress * 100.f);

    mState = mtg::DownloadCards_Window::State::UPDATED_UI;
}

void mtg::DownloadCards_Window::slot_resetUi()
{
    mUi->combo_AvailableSets->setEnabled(true);
    mUi->button_StartDownload->setEnabled(true);
    mUi->button_StartDownload->setText("Download Cards");
}

void mtg::DownloadCards_Window::slot_downloadCardsFromSet(int _page, QString _setName)
{
    // http://api.mtgapi.com/v2/cards

    QUrl targetUrl("http://api.mtgapi.com/v2/cards");
    targetUrl.addQueryItem("page", QString("%1").arg(_page));
    targetUrl.addQueryItem("set", _setName);

    QNetworkRequest request;
    request.setUrl(targetUrl);

    mCardInfoNetworkManager.get(request);
}

void mtg::DownloadCards_Window::slot_downloadImageFromCard(QVariantMap _card)
{
    QUrl cardImageUrl(_card["images"].toMap()["gatherer"].toString());
    mCardImageNetworkManager.get(QNetworkRequest(cardImageUrl));
}

void mtg::DownloadCards_Window::slot_replyReceivedFromCardInfoRequest(QNetworkReply *_reply)
{
    QByteArray fullResponse = _reply->readAll();

    // mtg_debug("Response: " << QString(fullResponse).toStdString());

    bool ok;
    QJson::Parser parser;
    QVariantMap root  = parser.parse(fullResponse, &ok).toMap();
    mDownloadedCards.append(root["cards"].toList());

    if (root["links"].toMap()["next"].isNull())
    {
        mState = mtg::DownloadCards_Window::State::DOWNLOADED_SET;
    }
    else
    {
        mCurrentCardSetPageNumber++;
        emit signal_downloadCardsFromSet(mCurrentCardSetPageNumber, mCardSetNames.at(mCurrentSetSelected));
    }
}

void mtg::DownloadCards_Window::slot_replyReceivedFromCardImageRequest(QNetworkReply *_reply)
{
    QByteArray fullResponse = _reply->readAll();

    // construct the pixmap from the JSON response
    QPixmap tempImage;
    if (!tempImage.loadFromData(fullResponse))
    {
        mtg_error("Unable to load the image from data.");
    }

    mCurrentCardImage = tempImage;

    mState = mtg::DownloadCards_Window::State::DOWNLOADED_IMAGE;
}
