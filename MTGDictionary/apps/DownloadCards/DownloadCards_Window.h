//! ----------------------------------------------------------------------------
//! DownloadCards_Window.h
//!
//! MTGDictionary is licensed under a
//! Creative Commons Attribution-NonCommercial 4.0 International License.
//! You should have received a copy of the license along with this
//! work. If not, see http://creativecommons.org/licenses/by-nc-sa/4.0/.
//!
//! (c) Copyright Dustin Hopper 2015 - hopper.dustin@gmail.com
//! ----------------------------------------------------------------------------

#pragma once

#include <QtGui>
#include <QtNetwork>
#include <vector>

namespace Ui
{
    class DownloadCardsMainWindow;
}

namespace mtg
{
    class DownloadCards_Window : public QMainWindow
    {
        Q_OBJECT;

        enum State
        {
            SELECTING_SET,
            DOWNLOADING_SET,
            DOWNLOADED_SET,
            DOWNLOADING_IMAGE,
            DOWNLOADED_IMAGE,
            UPDATING_UI,
            UPDATED_UI
        };

    public:
        DownloadCards_Window();

    signals:
        void signal_downloadCardsFromSet(int _page, QString _setName);
        void signal_downloadImageFromCard(QVariantMap _card);
        void signal_updateUi();
        void signal_resetUi();

    public:
        void processCardsFromChosenSetForDownload();

    protected:
        void closeEvent(QCloseEvent *event);

    private slots:
        void slot_downloadButtonClicked();
        void slot_newSetSelected(int _index);
        void slot_updateUi();
        void slot_resetUi();
        void slot_downloadCardsFromSet(int _page, QString _setName);
        void slot_downloadImageFromCard(QVariantMap _card);
        void slot_replyReceivedFromCardInfoRequest(QNetworkReply *_reply);
        void slot_replyReceivedFromCardImageRequest(QNetworkReply *_reply);

    private:
        Ui::DownloadCardsMainWindow *mUi;
        mtg::DownloadCards_Window::State mState;
        QNetworkAccessManager mCardInfoNetworkManager;
        QNetworkAccessManager mCardImageNetworkManager;
        std::vector<QString> mCardSetNames;
        std::vector<QString> mCardSetFiles;
        QVariantList mDownloadedCards;
        QPixmap mCurrentCardImage;
        QString mCurrentCardName;
        QString mCurrentCardSet;
        QString mCurrentCardNumber;
        int32_t mCurrentSetSelected;
        int32_t mTotalNumberOfCardsInSet;
        int32_t mCurrentCardDownloaded;
        int32_t mCurrentCardSetPageNumber;
        bool mUserForcedShutdown;
    };
}