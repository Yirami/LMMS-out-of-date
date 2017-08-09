#include "cbaseeditdlg.h"

QBrush CBaseEditDlg::normalForeground(QBrush(Qt::black));
QBrush CBaseEditDlg::warningForeground(QBrush(Qt::green));
QBrush CBaseEditDlg::errorForeground(QBrush(Qt::red));
QBrush CBaseEditDlg::normal0Background(QBrush(Qt::white));
QBrush CBaseEditDlg::normal1Background(QBrush(Qt::lightGray));
QBrush CBaseEditDlg::errorBackground(QBrush(Qt::red));
QRegExp CBaseEditDlg::allCapitalRE("^[A-Z]+$");
QRegExp CBaseEditDlg::allChineseRE("^[\u4e00-\u9fa5]+$");
QRegExp CBaseEditDlg::beginDigitRE("^[0-9]*[1-9][0-9]*[A-Za-z\u4e00-\u9fa5]+$");
CLineEditDelegate *CBaseEditDlg::agentLineDelegate = new CLineEditDelegate(CBaseEditDlg::allCapitalRE,true);
CLineEditDelegate *CBaseEditDlg::nameLineDelegate = new CLineEditDelegate(CBaseEditDlg::allChineseRE,true);
CLineEditDelegate *CBaseEditDlg::madeLineDelegate = new CLineEditDelegate(CBaseEditDlg::allChineseRE,true);
CLineEditDelegate *CBaseEditDlg::specLineDelegate = new CLineEditDelegate(CBaseEditDlg::beginDigitRE);
CComboDelegate *CBaseEditDlg::nameComboDelegate = new CComboDelegate;
CComboDelegate *CBaseEditDlg::madeComboDelegate = new CComboDelegate;
CSpinDelegate *CBaseEditDlg::ioDelegate = new CSpinDelegate;
CSpinDelegate *CBaseEditDlg::stockDelegate = new CSpinDelegate;
CDoubleSpinDelegate *CBaseEditDlg::priceDelegate = new CDoubleSpinDelegate;

CBaseEditDlg::CBaseEditDlg(CDatabasePackage *dbPackage, QWidget *parent)
    :QDialog(parent), mainPackage(dbPackage)
{
    setUI();
    configModel();
    connect(addB,SIGNAL(clicked()),this,SLOT(on_addB_clicked()));
    connect(deleteB,SIGNAL(clicked()),this,SLOT(on_deleteB_clicked()));
    connect(cancelB,SIGNAL(clicked()),this,SLOT(on_cancelB_clicked()));
    connect(checkB,SIGNAL(clicked()),this,SLOT(on_checkB_clicked()));

    on_addB_clicked();
}

void CBaseEditDlg::setUI()
{
    this->setFixedSize(1000,600);
    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowMinimizeButtonHint;
    flags |=Qt::WindowCloseButtonHint;
    setWindowFlags(flags);

    actionBox = new QGroupBox(this);
    actionBox->setFixedWidth(150);
    actionBox->setTitle("操作");
    addB = new QPushButton(this);
    addB->setMinimumHeight(50);
    addB->setText("添加");
    deleteB = new QPushButton(this);
    deleteB->setMinimumHeight(50);
    deleteB->setText("删除");
    cancelB = new QPushButton(this);
    cancelB->setMinimumHeight(50);
    cancelB->setText("取消");
    checkB = new QPushButton(this);
    checkB->setMinimumHeight(50);
    checkB->setText("检查");
    tabV = new QTableView(this);

    QVBoxLayout *actboxlayout = new QVBoxLayout(this);
    actboxlayout->addStretch(1);
    actboxlayout->addWidget(addB);
    actboxlayout->addStretch(2);
    actboxlayout->addWidget(deleteB);
    actboxlayout->addStretch(8);
    actboxlayout->addWidget(cancelB);
    actboxlayout->addStretch(2);
    actboxlayout->addWidget(checkB);
    actboxlayout->addStretch(1);

    actionBox->setLayout(actboxlayout);

    QHBoxLayout *mlayout = new QHBoxLayout(this);
    mlayout->addWidget(tabV);
    mlayout->addWidget(actionBox);

    this->setLayout(mlayout);
}

void CBaseEditDlg::configModel()
{
    dataModel = new QStandardItemModel(0,7);
    tabV->setModel(dataModel);
    tabV->setColumnWidth(0,100);
    tabV->setColumnWidth(1,180);
    tabV->setColumnWidth(2,180);
    tabV->setColumnWidth(3,100);
    tabV->setColumnWidth(4,80);
    tabV->setColumnWidth(5,80);
    tabV->setColumnWidth(6,60);
    tabV->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    tabV->verticalHeader()->setMinimumWidth(22);
    tabV->setSelectionBehavior(QAbstractItemView::SelectRows);
    tabV->setSelectionMode( QAbstractItemView::MultiSelection);
    tabV->show();
}

void CBaseEditDlg::on_cancelB_clicked()
{
    reject();
}

void CBaseEditDlg::on_addB_clicked()
{
    int i = dataModel->rowCount();
    dataModel->insertRow(i);
    if (i%2 == 0)
    {
        for (int j=0;j<dataModel->columnCount();j++)
            dataModel->setData(dataModel->index(i,j),CBaseEditDlg::normal1Background,Qt::BackgroundColorRole);
    }
    else
    {
        for (int j=0;j<dataModel->columnCount();j++)
            dataModel->setData(dataModel->index(i,j),CBaseEditDlg::normal0Background,Qt::BackgroundColorRole);
    }

}

void CBaseEditDlg::on_deleteB_clicked()
{
    QItemSelectionModel *thisSelectionM = tabV->selectionModel();
    if (thisSelectionM->hasSelection())
    {
        QModelIndexList selections = thisSelectionM->selectedIndexes();
        QMap<int, int> rowMap;
        foreach (QModelIndex index, selections)
        {
            rowMap.insert(index.row(), 0);
        }
        QMapIterator<int, int> rowMapIterator(rowMap);
        rowMapIterator.toBack();
        while (rowMapIterator.hasPrevious())
        {
            rowMapIterator.previous();
            int rowToDel = rowMapIterator.key();
            dataModel->removeRow(rowToDel);
        }
        thisSelectionM->clearSelection();
        recoveryBackground();
    }
}

void CBaseEditDlg::on_checkB_clicked()
{
    // 清除所有选中
    QItemSelectionModel *thisSelectionM = tabV->selectionModel();
    thisSelectionM->clearSelection();
    //
    if (dataModel->rowCount() != 0)
    {
        addB->setEnabled(false);
        deleteB->setEnabled(false);
        tabV->setEditTriggers(QAbstractItemView::NoEditTriggers);
        bool hasError = false;
        recoveryBackground();
        for (int nextOne = 0;nextOne<dataModel->rowCount();nextOne++)
            hasError = checkOneRecord(nextOne) || hasError;
        if (hasError)
        {
            QMessageBox::critical(NULL,QString("警告"),QString("当前列表存在严重问题，请修改确认后再次检查并提交！"));
            addB->setEnabled(true);
            deleteB->setEnabled(true);
            tabV->setEditTriggers(QAbstractItemView::AllEditTriggers);
        }
        else
        {
            switch(QMessageBox::information(this,QString("确认"),QString("确定提交吗？Yes：准备提交；No：再瞅瞅！"),QMessageBox::Yes|QMessageBox::No,QMessageBox::No))
            {
            case QMessageBox::Yes:
                submitData();
                reject();
            default:
                addB->setEnabled(true);
                deleteB->setEnabled(true);
                tabV->setEditTriggers(QAbstractItemView::AllEditTriggers);
                break;
            }
        }
    }
}

void CBaseEditDlg::setDelegateForIN()
{
    tabV->setItemDelegateForColumn(0,agentLineDelegate);
    tabV->setItemDelegateForColumn(1,nameLineDelegate);
    tabV->setItemDelegateForColumn(2,madeLineDelegate);
    tabV->setItemDelegateForColumn(3,specLineDelegate);
    tabV->setItemDelegateForColumn(4,ioDelegate);
    tabV->setItemDelegateForColumn(5,stockDelegate);
    tabV->setItemDelegateForColumn(6,priceDelegate);
}

void CBaseEditDlg::setDelegateForOUT()
{
    tabV->setItemDelegateForColumn(0,agentLineDelegate);
    tabV->setItemDelegateForColumn(1,nameComboDelegate);
    tabV->setItemDelegateForColumn(2,madeComboDelegate);
    tabV->setItemDelegateForColumn(3,specLineDelegate);
    tabV->setItemDelegateForColumn(4,ioDelegate);
    tabV->setItemDelegateForColumn(5,stockDelegate);
    tabV->setItemDelegateForColumn(6,priceDelegate);
}

void CBaseEditDlg::refreshList(QModelIndex TL)
{
    qDebug()<<QString("信号发生行：")<<TL.row();
    qDebug("This indicates that the virtual function is called abnormally");
}

void CBaseEditDlg::submitData()
{
    qDebug("This indicates that the virtual function is called abnormally");
}

bool CBaseEditDlg::checkOneRecord(const int idx)
{
    qDebug("This indicates that the virtual function is called abnormally and input Var: %d",idx);
    return true;
}

void CBaseEditDlg::recoveryBackground()
{
    for (int i=0;i<dataModel->rowCount();i++)
    {
        if (i%2 == 0)
        {
            for (int j=0;j<dataModel->columnCount();j++)
                dataModel->setData(dataModel->index(i,j),CBaseEditDlg::normal1Background,Qt::BackgroundColorRole);
        }
        else
        {
            for (int j=0;j<dataModel->columnCount();j++)
                dataModel->setData(dataModel->index(i,j),CBaseEditDlg::normal0Background,Qt::BackgroundColorRole);
        }
    }
}

CBaseEditDlg::~CBaseEditDlg()
{

}
