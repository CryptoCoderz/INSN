#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "clientmodel.h"
#include "darksend.h"
#include "darksendconfig.h"
#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"

#include "init.h"
#include "rpcserver.h"
#include "rpcclient.h"
#include "kernel.h"

#include "main.h"
#include "wallet.h"
#include "base58.h"
#include "transactionrecord.h"

using namespace json_spirit;

#include <sstream>
#include <string>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QScrollArea>
#include <QScroller>
#include <QSettings>
#include <QTimer>

#define DECORATION_SIZE 64
#define ICON_OFFSET 16
#define NUM_ITEMS 6

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(qVariantCanConvert<QColor>(value))
        {
            foreground = qvariant_cast<QColor>(value);
        }

        painter->setPen(fUseBlackTheme ? QColor(255, 255, 255) : foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(fUseBlackTheme ? QColor(255, 255, 255) : foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(fUseBlackTheme ? QColor(96, 101, 110) : option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentStake(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchOnlyStake(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listTransactions->setMinimumWidth(300);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelDarksendSyncStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");
    ui->labelNetStatus->setText("(" + tr("out of sync") + ")");

    fLiteMode = GetBoolArg("-litemode", false);

    if(fLiteMode){
        ui->frameDarksend->setVisible(false);
    } else {
        if(fMasterNode){
            ui->toggleDarksend->setText("(" + tr("Disabled") + ")");
            ui->darksendAuto->setText("(" + tr("Disabled") + ")");
            ui->darksendReset->setText("(" + tr("Disabled") + ")");
            ui->frameDarksend->setEnabled(false);
        } else {
            if(!fEnableDarksend){
                ui->toggleDarksend->setText(tr("Start Darksend Mixing"));
            } else {
                ui->toggleDarksend->setText(tr("Stop Darksend Mixing"));
            }
            timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(darkSendStatus()));
            if(!GetBoolArg("-reindexaddr", false))
                timer->start(60000);
        }
    }

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    if (fUseBlackTheme)
    {
        const char* whiteLabelQSS = "QLabel { color: rgb(255,255,255); }";
        ui->labelBalance->setStyleSheet(whiteLabelQSS);
        ui->labelStake->setStyleSheet(whiteLabelQSS);
        ui->labelUnconfirmed->setStyleSheet(whiteLabelQSS);
        ui->labelImmature->setStyleSheet(whiteLabelQSS);
        ui->labelTotal->setStyleSheet(whiteLabelQSS);
    }
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    if(!fLiteMode && !fMasterNode) disconnect(timer, SIGNAL(timeout()), this, SLOT(darkSendStatus()));
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& stake, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& anonymizedBalance, const CAmount& watchOnlyBalance, const CAmount& watchOnlyStake, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentAnonymizedBalance = anonymizedBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchOnlyStake = watchOnlyStake;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, balance));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, stake));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, unconfirmedBalance));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, immatureBalance));
    ui->labelAnonymized->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, anonymizedBalance));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(nDisplayUnit, balance + stake + unconfirmedBalance + immatureBalance));
    ui->labelWatchAvailable->setText(BitcoinUnits::floorWithUnit(nDisplayUnit, watchOnlyBalance));
    ui->labelWatchStake->setText(BitcoinUnits::floorWithUnit(nDisplayUnit, watchOnlyStake));
    ui->labelWatchPending->setText(BitcoinUnits::floorWithUnit(nDisplayUnit, watchUnconfBalance));
    ui->labelWatchImmature->setText(BitcoinUnits::floorWithUnit(nDisplayUnit, watchImmatureBalance));
    ui->labelWatchTotal->setText(BitcoinUnits::floorWithUnit(nDisplayUnit, watchOnlyBalance + watchOnlyStake + watchUnconfBalance + watchImmatureBalance));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    updateDarksendProgress();

    static int cachedTxLocks = 0;

    if(cachedTxLocks != nCompleteTXLocks){
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchStake->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly){
        ui->labelWatchImmature->hide();
    }
    else{
        ui->labelBalance->setIndent(20);
        ui->labelStake->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;

    setCntConnections(model->getNumConnections());
    connect(model, SIGNAL(numConnectionsChanged(int)), this, SLOT(setCntConnections(int)));

    setCntBlocks(model->getNumBlocks());
    connect(model, SIGNAL(numBlocksChanged(int)), this, SLOT(setCntBlocks(int)));

    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getAnonymizedBalance(),
             model->getWatchBalance(), model->getWatchStake(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        connect(ui->darksendAuto, SIGNAL(clicked()), this, SLOT(darksendAuto()));
        connect(ui->darksendReset, SIGNAL(clicked()), this, SLOT(darksendReset()));
        connect(ui->toggleDarksend, SIGNAL(clicked()), this, SLOT(toggleDarksend()));
        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {

        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if(currentBalance != -1)
            setBalance(currentBalance, currentStake, currentUnconfirmedBalance, currentImmatureBalance, currentAnonymizedBalance,
                currentWatchOnlyBalance, currentWatchOnlyStake, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelDarksendSyncStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
    ui->labelNetStatus->setVisible(fShow);
}



void OverviewPage::updateDarksendProgress()
{
    if(!darkSendPool.IsBlockchainSynced() || ShutdownRequested()) return;

    if(!pwalletMain) return;

    QString strAmountAndRounds;
    QString strAnonymizeINSaNeAmount = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, nAnonymizeINSaNeAmount * COIN, false, BitcoinUnits::separatorAlways);

    if(currentBalance == 0)
    {
        ui->darksendProgress->setValue(0);
        ui->darksendProgress->setToolTip(tr("No inputs detected"));
        // when balance is zero just show info from settings
        strAnonymizeINSaNeAmount = strAnonymizeINSaNeAmount.remove(strAnonymizeINSaNeAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strAnonymizeINSaNeAmount + " / " + tr("%n Rounds", "", nDarksendRounds);

        ui->labelAmountRounds->setToolTip(tr("No inputs detected"));
        ui->labelAmountRounds->setText(strAmountAndRounds);
        return;
    }

    CAmount nDenominatedConfirmedBalance;
    CAmount nDenominatedUnconfirmedBalance;
    CAmount nAnonymizableBalance;
    CAmount nNormalizedAnonymizedBalance;
    double nAverageAnonymizedRounds;

    {
        TRY_LOCK(cs_main, lockMain);
        if(!lockMain) return;

        nDenominatedConfirmedBalance = pwalletMain->GetDenominatedBalance();
        nDenominatedUnconfirmedBalance = pwalletMain->GetDenominatedBalance(true);
        nAnonymizableBalance = pwalletMain->GetAnonymizableBalance();
        nNormalizedAnonymizedBalance = pwalletMain->GetNormalizedAnonymizedBalance();
        nAverageAnonymizedRounds = pwalletMain->GetAverageAnonymizedRounds();
    }

    //Get the anon threshold
    CAmount nMaxToAnonymize = nAnonymizableBalance + currentAnonymizedBalance + nDenominatedUnconfirmedBalance;

    // If it's more than the anon threshold, limit to that.
    if(nMaxToAnonymize > nAnonymizeINSaNeAmount*COIN) nMaxToAnonymize = nAnonymizeINSaNeAmount*COIN;

    if(nMaxToAnonymize == 0) return;

    if(nMaxToAnonymize >= nAnonymizeINSaNeAmount * COIN) {
        ui->labelAmountRounds->setToolTip(tr("Found enough compatible inputs to anonymize %1")
                                          .arg(strAnonymizeINSaNeAmount));
        strAnonymizeINSaNeAmount = strAnonymizeINSaNeAmount.remove(strAnonymizeINSaNeAmount.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = strAnonymizeINSaNeAmount + " / " + tr("%n Rounds", "", nDarksendRounds);
    } else {
        QString strMaxToAnonymize = BitcoinUnits::formatHtmlWithUnit(nDisplayUnit, nMaxToAnonymize, false, BitcoinUnits::separatorAlways);
        ui->labelAmountRounds->setToolTip(tr("Not enough compatible inputs to anonymize <span style='color:red;'>%1</span>,<br>"
                                             "will anonymize <span style='color:red;'>%2</span> instead")
                                          .arg(strAnonymizeINSaNeAmount)
                                          .arg(strMaxToAnonymize));
        strMaxToAnonymize = strMaxToAnonymize.remove(strMaxToAnonymize.indexOf("."), BitcoinUnits::decimals(nDisplayUnit) + 1);
        strAmountAndRounds = "<span style='color:red;'>" +
                QString(BitcoinUnits::factor(nDisplayUnit) == 1 ? "" : "~") + strMaxToAnonymize +
                " / " + tr("%n Rounds", "", nDarksendRounds) + "</span>";
    }
    ui->labelAmountRounds->setText(strAmountAndRounds);

    // calculate parts of the progress, each of them shouldn't be higher than 1
    // progress of denominating
    float denomPart = 0;
    // mixing progress of denominated balance
    float anonNormPart = 0;
    // completeness of full amount anonimization
    float anonFullPart = 0;

    CAmount denominatedBalance = nDenominatedConfirmedBalance + nDenominatedUnconfirmedBalance;
    denomPart = (float)denominatedBalance / nMaxToAnonymize;
    denomPart = denomPart > 1 ? 1 : denomPart;
    denomPart *= 100;

    anonNormPart = (float)nNormalizedAnonymizedBalance / nMaxToAnonymize;
    anonNormPart = anonNormPart > 1 ? 1 : anonNormPart;
    anonNormPart *= 100;

    anonFullPart = (float)currentAnonymizedBalance / nMaxToAnonymize;
    anonFullPart = anonFullPart > 1 ? 1 : anonFullPart;
    anonFullPart *= 100;

    // apply some weights to them ...
    float denomWeight = 1;
    float anonNormWeight = nDarksendRounds;
    float anonFullWeight = 2;
    float fullWeight = denomWeight + anonNormWeight + anonFullWeight;
    // ... and calculate the whole progress
    float denomPartCalc = ceilf((denomPart * denomWeight / fullWeight) * 100) / 100;
    float anonNormPartCalc = ceilf((anonNormPart * anonNormWeight / fullWeight) * 100) / 100;
    float anonFullPartCalc = ceilf((anonFullPart * anonFullWeight / fullWeight) * 100) / 100;
    float progress = denomPartCalc + anonNormPartCalc + anonFullPartCalc;
    if(progress >= 100) progress = 100;

    ui->darksendProgress->setValue(progress);

    QString strToolPip = ("<b>" + tr("Overall progress") + ": %1%</b><br/>" +
                          tr("Denominated") + ": %2%<br/>" +
                          tr("Mixed") + ": %3%<br/>" +
                          tr("Anonymized") + ": %4%<br/>" +
                          tr("Denominated inputs have %5 of %n rounds on average", "", nDarksendRounds))
            .arg(progress).arg(denomPart).arg(anonNormPart).arg(anonFullPart)
            .arg(nAverageAnonymizedRounds);
    ui->darksendProgress->setToolTip(strToolPip);
}


void OverviewPage::darkSendStatus()
{
    static int64_t nLastDSProgressBlockTime = 0;

    int nBestHeight = pindexBest->nHeight;

    // we we're processing more then 1 block per second, we'll just leave
    if(((nBestHeight - darkSendPool.cachedNumBlocks) / (GetTimeMillis() - nLastDSProgressBlockTime + 1) > 1)) return;
    nLastDSProgressBlockTime = GetTimeMillis();

    if(!fEnableDarksend) {
        if(nBestHeight != darkSendPool.cachedNumBlocks)
        {
            darkSendPool.cachedNumBlocks = nBestHeight;
            updateDarksendProgress();

            ui->darksendEnabled->setText(tr("Disabled"));
            ui->darksendStatus->setText("");
            ui->toggleDarksend->setText(tr("Start Darksend Mixing"));
        }

        return;
    }

    // check darksend status and unlock if needed
    if(nBestHeight != darkSendPool.cachedNumBlocks)
    {
        // Balance and number of transactions might have changed
        darkSendPool.cachedNumBlocks = nBestHeight;

        updateDarksendProgress();

        ui->darksendEnabled->setText(tr("Enabled"));
    }

    QString strStatus = QString(darkSendPool.GetStatus().c_str());

    QString s = tr("Last Darksend message:\n") + strStatus;

    if(s != ui->darksendStatus->text())
        LogPrintf("Last Darksend message: %s\n", strStatus.toStdString());

    ui->darksendStatus->setText(s);

    if(darkSendPool.sessionDenom == 0){
        ui->labelSubmittedDenom->setText(tr("N/A"));
    } else {
        std::string out;
        darkSendPool.GetDenominationsToString(darkSendPool.sessionDenom, out);
        QString s2(out.c_str());
        ui->labelSubmittedDenom->setText(s2);
    }

    // Get DarkSend Denomination Status
}

void OverviewPage::darksendAuto(){
    darkSendPool.DoAutomaticDenominating();
}

void OverviewPage::darksendReset(){
    darkSendPool.Reset();
    darkSendStatus();

    QMessageBox::warning(this, tr("Darksend"),
        tr("Darksend was successfully reset."),
        QMessageBox::Ok, QMessageBox::Ok);
}

void OverviewPage::toggleDarksend(){

    QSettings settings;
    // Popup some information on first mixing
    QString hasMixed = settings.value("hasMixed").toString();
    if(hasMixed.isEmpty()){
        QMessageBox::information(this, tr("Darksend"),
                tr("If you don't want to see internal Darksend fees/transactions select \"Received By\" as Type on the \"Transactions\" tab."),
                QMessageBox::Ok, QMessageBox::Ok);
        settings.setValue("hasMixed", "hasMixed");
    }
    if(!fEnableDarksend){
        int64_t balance = currentBalance;
        float minAmount = 1.49 * COIN;
        if(balance < minAmount){
            QString strMinAmount(BitcoinUnits::formatWithUnit(nDisplayUnit, minAmount));
            QMessageBox::warning(this, tr("Darksend"),
                tr("Darksend requires at least %1 to use.").arg(strMinAmount),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        // if wallet is locked, ask for a passphrase
        if (walletModel->getEncryptionStatus() == WalletModel::Locked)
        {
            WalletModel::UnlockContext ctx(walletModel->requestUnlock());
            if(!ctx.isValid())
            {
                //unlock was cancelled
                darkSendPool.cachedNumBlocks = std::numeric_limits<int>::max();
                QMessageBox::warning(this, tr("Darksend"),
                    tr("Wallet is locked and user declined to unlock. Disabling Darksend."),
                    QMessageBox::Ok, QMessageBox::Ok);
                if (fDebug) LogPrintf("Wallet is locked and user declined to unlock. Disabling Darksend.\n");
                return;
            }
        }

    }

    fEnableDarksend = !fEnableDarksend;
    darkSendPool.cachedNumBlocks = std::numeric_limits<int>::max();

    if(!fEnableDarksend){
        ui->toggleDarksend->setText(tr("Start Darksend Mixing"));
        darkSendPool.UnlockCoins();
    } else {
        ui->toggleDarksend->setText(tr("Stop Darksend Mixing"));

        /* show darksend configuration if client has defaults set */

        if(nAnonymizeINSaNeAmount == 0){
            DarksendConfig dlg(this);
            dlg.setModel(walletModel);
            dlg.exec();
        }

    }
}

//
// Network logic rerporting
//
//

std::string getPoSHash(int Height)
{
    if(Height < 0) { return "351c6703813172725c6d660aa539ee6a3d7a9fe784c87fae7f36582e3b797058"; }
    int desiredheight;
    desiredheight = Height;
    if (desiredheight < 0 || desiredheight > nBestHeight)
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > desiredheight)
        pblockindex = pblockindex->pprev;
    return pblockindex->phashBlock->GetHex();
}


double getPoSHardness(int height)
{
    const CBlockIndex* blockindex = getPoSIndex(height);

    int nShift = (blockindex->nBits >> 24) & 0xff;

    double dDiff =
        (double)0x0000ffff / (double)(blockindex->nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    }
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    }

    return dDiff;

}

const CBlockIndex* getPoSIndex(int height)
{
    std::string hex = getPoSHash(height);
    uint256 hash(hex);
    return mapBlockIndex[hash];
}

int getPoSTime(int Height)
{
    std::string strHash = getPoSHash(Height);
    uint256 hash(strHash);

    if (mapBlockIndex.count(hash) == 0)
        return 0;

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    return pblockindex->nTime;
}

int PoSInPastHours(int hours)
{
    int wayback = hours * 3600;
    bool check = true;
    int height = pindexBest->nHeight;
    int heightHour = pindexBest->nHeight;
    int utime = (int)time(NULL);
    int target = utime - wayback;

    while(check)
    {
        if(getPoSTime(heightHour) < target)
        {
            check = false;
            return height - heightHour;
        } else {
            heightHour = heightHour - 1;
        }
    }

    return 0;
}

double convertPoSCoins(int64_t amount)
{
    return (double)amount / (double)COIN;
}

void OverviewPage::updatePoSstat(bool stat)
{
    if(stat)
    {
        uint64_t nWeight = 0;
        if (pwalletMain)
            nWeight = pwalletMain->GetStakeWeight();
        uint64_t nNetworkWeight = GetPoSKernelPS();
        bool staking = nLastCoinStakeSearchInterval && nWeight;
        uint64_t nExpectedTime = staking ? (GetTargetSpacing * nNetworkWeight / nWeight) : 0;
        QString Qseconds = " Second(s)";
        if(nExpectedTime > 86399)
        {
           nExpectedTime = nExpectedTime / 60 / 60 / 24;
           Qseconds = " Day(s)";
        }
        else if(nExpectedTime > 3599)
        {
           nExpectedTime = nExpectedTime / 60 / 60;
           Qseconds = " Hour(s)";
        }
        else if(nExpectedTime > 59)
        {
           nExpectedTime = nExpectedTime / 60;
           Qseconds = " Minute(s)";
        }
        ui->lbTime->show();

        int nHeight = pindexBest->nHeight;

        // Net weight percentage
        double nStakePercentage = (double)nWeight / (double)nNetworkWeight * 100;
        double nNetPercentage = (100 - (double)nStakePercentage);
        if(nWeight > nNetworkWeight)
        {
            nStakePercentage = (double)nNetworkWeight / (double)nWeight * 100;
            nNetPercentage = (100 - (double)nStakePercentage);
        }
        // Sync percentage
        // (1498887616 - 1498884018 / 100) / (1498887616 - 1467348018 / 100) * 100
        double nTimeLapse = (GetTime() - pindexBest->GetBlockTime()) / 100;
        double nTimetotalLapse = (GetTime() - Params().GenesisBlock().GetBlockTime()) / 100;
        double nSyncPercentage = 100 - (nTimeLapse / nTimetotalLapse * 100);

        QString QStakePercentage = QString::number(nStakePercentage, 'f', 2);
        QString QNetPercentage = QString::number(nNetPercentage, 'f', 2);
        QString QSyncPercentage = QString::number(nSyncPercentage, 'f', 2);
        QString QTime = clientModel->getLastBlockDate().toString();
        QString QBlockHeight = QString::number(nHeight);
        QString QExpect = QString::number(nExpectedTime, 'f', 0);
        QString QStaking = "Disabled";
        QString QStakeEN = "Not Staking";
        QString QSynced = "Ready (Synced)";
        QString QSyncing = "Not Ready (Syncing)";
        QString QFailed = "Not Connected";
        ui->estnxt->setText(QExpect + Qseconds);
        ui->stkstat->setText(QStakeEN);
        ui->lbHeight->setText(QBlockHeight);
        ui->clStat->setText("<font color=\"red\">" + QFailed + "</font>");
        if(nExpectedTime == 0)
        {
            QExpect = "Not Staking";
            ui->estnxt->setText(QExpect);
        }
        if(staking)
        {
            QStakeEN = "Staking";
            ui->stkstat->setText(QStakeEN);
        }
        if(GetBoolArg("-staking", true))
        {
            QStaking = "Enabled";
        }
        if((GetTime() - pindexBest->GetBlockTime()) < 45 * 60)
        {
            QSyncPercentage = "100";
            nSyncPercentage = 100;
        }
        ui->lbTime->setText(QTime);
        ui->stken->setText(QStaking);
        ui->urweight_2->setText(QStakePercentage + "%");
        ui->netweight_2->setText(QNetPercentage + "%");
        ui->sncStatus->setText(QSyncPercentage + "%");
        if(nSyncPercentage == 100)
            ui->clStat->setText("<font color=\"green\">" + QSynced + "</font>");
        if(nSyncPercentage != 100)
        {
            if(clientModel->getNumConnections() > 0)
                ui->clStat->setText("<font color=\"orange\">" + QSyncing + "</font>");
        }
        return;
    }
}

void OverviewPage::setCntBlocks(int pseudo)
{
    pseudo = pindexBest->GetBlockTime();

    if(pseudo > Params().GenesisBlock().GetBlockTime())
        updatePoSstat(true);

    return;
}

void OverviewPage::setCntConnections(int count)
{
    ui->lbcon->setText(QString::number(count));
}

