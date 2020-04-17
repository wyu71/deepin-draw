/*
 * Copyright (C) 2019 ~ %YEAR% Deepin Technology Co., Ltd.
 *
 * Author:     WangXin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ccentralwidget.h"
#include "clefttoolbar.h"
#include "drawshape/cdrawscene.h"
#include "drawshape/cgraphicsitem.h"
#include "widgets/progresslayout.h"
#include "drawshape/cpictureitem.h"
#include "cgraphicsview.h"
#include "drawshape/cpicturetool.h"
#include "drawshape/cgraphicstextitem.h"
#include "drawshape/cgraphicsellipseitem.h"
#include "drawshape/cgraphicstriangleitem.h"
#include "widgets/dialog/cexportimagedialog.h"
#include "widgets/dialog/cprintmanager.h"
#include "drawshape/cpicturetool.h"
#include "frame/cviewmanagement.h"
#include "drawshape/cdrawparamsigleton.h"
#include "frame/cmultiptabbarwidget.h"

#include <DMenu>
#include <DGuiApplicationHelper>
#include <QDebug>
#include <QGraphicsItem>
#include <QtConcurrent>
#include <QDesktopWidget>
#include <QApplication>
#include <QPdfWriter>

DGUI_USE_NAMESPACE

CCentralwidget::CCentralwidget(DWidget *parent)
    : DWidget(parent)
    , m_isCloseNow(false)
    , m_tabDefaultName(tr("Unnamed"))

{
    m_stackedLayout = new QStackedLayout();
    m_hLayout = new QHBoxLayout();
    m_exportImageDialog = new CExportImageDialog(this);
    m_printManager = new CPrintManager();
    m_pictureTool = new CPictureTool(this);
    m_leftToolbar = new CLeftToolBar();
    m_topMutipTabBarWidget = new CMultipTabBarWidget(this);
    m_topMutipTabBarWidget->setDefaultTabBarName(m_tabDefaultName);

    initUI();
    initConnect();

    // 创建一个标签页(标签页生成时会自动创建一个view)
    m_topMutipTabBarWidget->addTabBarItem(tr("Unnamed"), CDrawParamSigleton::creatUUID());

    //刷新标题或者tab标签的名字
    updateTitle();

}

CCentralwidget::CCentralwidget(QStringList filepaths): DWidget (),
    m_tabDefaultName(tr("Unnamed"))
{
    m_stackedLayout = new QStackedLayout();
    m_hLayout = new QHBoxLayout();
    m_exportImageDialog = new CExportImageDialog(this);
    m_printManager = new CPrintManager();
    m_pictureTool = new CPictureTool(this);
    m_leftToolbar = new CLeftToolBar();
    m_topMutipTabBarWidget = new CMultipTabBarWidget(this);
    m_topMutipTabBarWidget->setDefaultTabBarName(m_tabDefaultName);

    if (filepaths.count() > 0) {
        for (int i = 0; i < filepaths.count(); i++) {
            createNewScenseByscencePath(filepaths.at(i));
        }
    } else {
        CGraphicsView *pView = createNewScense(m_tabDefaultName);
        CManageViewSigleton::GetInstance()->setCurView(pView);
        initSceneRect();
        // 顶部菜单栏进行创建
        m_topMutipTabBarWidget->addTabBarItem(pView->getDrawParam()->getShowViewNameByModifyState(),
                                              pView->getDrawParam()->uuid(), false);
    }

    initUI();
    initConnect();

    //刷新标题或者tab标签的名字
    updateTitle();
}

CCentralwidget::~CCentralwidget()
{
    delete m_pictureTool;

}
CLeftToolBar *CCentralwidget::getLeftToolBar()
{
    return m_leftToolbar;
}

CGraphicsView *CCentralwidget::getGraphicsView() const
{
    return CManageViewSigleton::GetInstance()->getCurView();
}

CDrawScene *CCentralwidget::getDrawScene() const
{
    return static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene());
}

void CCentralwidget::switchTheme(int type)
{
    if (type == 1) {
        CManageViewSigleton::GetInstance()->getCurView()->scene()->setBackgroundBrush(QColor(248, 248, 251));
    } else if (type == 2) {
        CManageViewSigleton::GetInstance()->getCurView()->scene()->setBackgroundBrush(QColor(35, 35, 35));
    }
    systemTheme = type;
    static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->switchTheme(type);
}

void CCentralwidget::resetSceneBackgroundBrush()
{
    int themeType = CManageViewSigleton::GetInstance()->getThemeType();
    if (themeType == 1) {
        static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->setBackgroundBrush(QColor(248, 248, 251));
    } else if (themeType == 2) {
        static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->setBackgroundBrush(QColor(35, 35, 35));
    }
}

void CCentralwidget::initSceneRect()
{
    QSize size = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->getCutDefaultSize();
    QRectF rc = QRectF(0, 0, size.width(), size.height());
    static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->setSceneRect(rc);
}

CGraphicsView *CCentralwidget::createNewScenseByDragFile(QString ddfFile)
{
    // [0] 判断是否已经打开此文件,已经打开则显示此文件
    if (m_topMutipTabBarWidget->IsFileOpened(ddfFile)) {
        qDebug() << "create same name Scence,deepin-draw will not create.";
        return nullptr;
    }
    QFileInfo info(ddfFile);

    CGraphicsView *pView = createNewScense(info.completeBaseName(), CDrawParamSigleton::creatUUID(), false);

    m_topMutipTabBarWidget->addTabBarItem(pView->getDrawParam()->getShowViewNameByModifyState(),
                                          pView->getDrawParam()->uuid(), false);

    return pView;
}

void CCentralwidget::createNewScenseByscencePath(QString scencePath)
{
    QFileInfo info(scencePath);
    createNewScenseByDragFile(scencePath);

//    if (pCreatedNewView != nullptr) {
//        m_topMutipTabBarWidget->setTabBarTooltipName(pCreatedNewView->getDrawParam()->uuid(), info.fileName());
//    }
}

void CCentralwidget::setCurrentView(QString viewname)
{
    m_topMutipTabBarWidget->setCurrentTabBarWithName(viewname);
}

void CCentralwidget::setCurrentViewByUUID(QString uuid)
{
    m_topMutipTabBarWidget->setCurrentTabBarWithUUID(uuid);
}

QStringList CCentralwidget::getAllTabBarName()
{
    return m_topMutipTabBarWidget->getAllTabBarName();
}

QStringList CCentralwidget::getAllTabBarUUID()
{
    return m_topMutipTabBarWidget->getAllTabBarUUID();
}

CGraphicsView *CCentralwidget::createNewScense(QString scenceName, const QString &uuid, bool isModified)
{
    if (CManageViewSigleton::GetInstance()->getCurView() != nullptr) {
        CManageViewSigleton::GetInstance()->getCurView()->slotDoCutScene();
    }

    CGraphicsView *newview = new CGraphicsView(this);
    CManageViewSigleton::GetInstance()->addView(newview);
    auto curScene = new CDrawScene(newview, uuid, isModified);
    newview->setFrameShape(QFrame::NoFrame);
    newview->getDrawParam()->setViewName(scenceName);

    emit signalAddNewScence(curScene);

    //设置scene大小为屏幕分辨率
    //获取屏幕分辨率
    QDesktopWidget *desktopWidget = QApplication::desktop();
    QRect screenRect = desktopWidget->screenGeometry();
    newview->getDrawParam()->setCutDefaultSize(QSize(screenRect.width(), screenRect.height()));

    if (CManageViewSigleton::GetInstance()->getThemeType() == 1) {
        curScene->setBackgroundBrush(QColor(248, 248, 251));
    } else {
        curScene->setBackgroundBrush(QColor(35, 35, 35));
    }

    newview->setAlignment(Qt::AlignCenter);
    newview->setRenderHint(QPainter::Antialiasing);//设置反走样

    //自动设置滚动条
    newview->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    newview->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_stackedLayout->addWidget(newview);

    QSize size = newview->getDrawParam()->getCutDefaultSize();
    QRectF rc = QRectF(0, 0, size.width(), size.height());
    static_cast<CDrawScene *>(newview->scene())->setSceneRect(rc);

    connect(curScene, SIGNAL(signalQuitCutAndChangeToSelect()), m_leftToolbar, SLOT(slotAfterQuitCut()));
    connect(newview, SIGNAL(signalSetScale(const qreal)), this, SIGNAL(signalSetScale(const qreal)));
    connect(curScene, &CDrawScene::signalAttributeChanged, this, &CCentralwidget::signalAttributeChangedFromScene);
    connect(curScene, &CDrawScene::signalChangeToSelect, m_leftToolbar, &CLeftToolBar::slotShortCutSelect);
    //图片选中后相应操作
    connect(this, SIGNAL(signalPassPictureOper(int)), curScene, SLOT(picOperation(int )));

    connect(curScene, &CDrawScene::signalUpdateCutSize, this, &CCentralwidget::signalUpdateCutSize);
    connect(curScene, &CDrawScene::signalUpdateTextFont, this, &CCentralwidget::signalUpdateTextFont);

    connect(newview, SIGNAL(signalLoadDragOrPasteFile(QString)), this, SLOT(slotLoadDragOrPasteFile(QString)));

    connect(newview, SIGNAL(signalPastePixmap(QPixmap)), this, SLOT(slotPastePixmap(QPixmap)));

    connect(newview, SIGNAL(signalTransmitContinueDoOtherThing()), this, SIGNAL(signalContinueDoOtherThing()));
    connect(newview, SIGNAL(singalTransmitEndLoadDDF()), m_leftToolbar, SLOT(slotShortCutSelect()));

    //主菜单栏中点击打开导入图片
    connect(newview, SIGNAL(signalImportPicture(QString)), this, SLOT(openPicture(QString)));

    connect(m_leftToolbar, SIGNAL(setCurrentDrawTool(int)), curScene, SLOT(drawToolChange(int)));

    //如果是裁剪模式点击左边工具栏按钮则执行裁剪
//    connect(m_leftToolbar, SIGNAL(singalDoCutFromLeftToolBar()), newview, SLOT(slotDoCutScene()));

    //如果是裁剪模式点击工具栏的菜单则执行裁剪
    connect(this, SIGNAL(signalTransmitQuitCutModeFromTopBarMenu()), newview, SLOT(slotDoCutScene()));

    // 当场景内容被改变需要进行的操作
    connect(curScene, SIGNAL(signalIsModify(bool)), this, SLOT(currentScenseViewIsModify(bool)));

    // 连接view保存文件状态
    connect(newview, SIGNAL(signalSaveFileStatus(bool, QString, QFileDevice::FileError)), this, SLOT(slotSaveFileStatus(bool, QString, QFileDevice::FileError)));

    return newview;
}

void CCentralwidget::closeCurrentScenseView(bool ifTabOnlyOneCloseAqq)
{
    CGraphicsView *closeView = static_cast<CGraphicsView *>(m_stackedLayout->currentWidget());
    if (nullptr != closeView) {
        QString viewname = closeView->getDrawParam()->viewName();
        qDebug() << "closeCurrentScenseView:" << viewname;

        // 如果只剩一个画板并且没有进行修改且不是导入文件则不再创建新的画板
        if (1 == m_topMutipTabBarWidget->count() && ifTabOnlyOneCloseAqq) {

            qDebug() << "closeCurrentScenseView:" << viewname << " not modify";
            emit signalLastTabBarRequestClose();
            return;
        }

        closeView->setParent(nullptr);
        m_stackedLayout->removeWidget(closeView);
        CManageViewSigleton::GetInstance()->removeView(closeView);
        m_topMutipTabBarWidget->closeTabBarItemByUUID(closeView->getDrawParam()->uuid());
    }

    if (m_topMutipTabBarWidget->count() > 0) {
        m_leftToolbar->slotShortCutSelect();

        if (m_topMutipTabBarWidget->count() == 1) {
            m_topMutipTabBarWidget->hide();
        } else {
            m_topMutipTabBarWidget->show();
        }
    }

}

void CCentralwidget::currentScenseViewIsModify(bool isModify)
{
    QString viewName = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->viewName();
    qDebug() << "viewName：" << viewName << " modify:" << isModify;

    //1.更新tab标签（先更新标签页名再更新可能存在的主标题）
    bool drawParamCurModified = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->getModify();
    if (isModify != drawParamCurModified) {
        //已经修改的状态和drawParam中的状态不同那么就要刷新drawParam的状态
        QString uuid = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->uuid();
        CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->setModify(isModify);
        QString newVName = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->getShowViewNameByModifyState();

        updateTabName(uuid, newVName);
    }
}

void CCentralwidget::slotSaveFileStatus(bool status, QString errorString, QFileDevice::FileError error)
{
    if (status) {
        qDebug() << "Ctrl_S Save:" << m_isCloseNow;
        if (!m_isCloseNow) {
            m_isCloseNow = false;
            // 设置保存路径到标签的tooltip上，并且更新标签名字
            QString current_path = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->getDdfSavePath();
            QString current_file_name = current_path.split("/").last();
            if (!current_file_name.isEmpty()) {
                QString uuid = m_topMutipTabBarWidget->getCurrentTabBarUUID();
                current_file_name = current_file_name.left(current_file_name.length() - 4);
                CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->setViewName(current_file_name);
                updateTabName(uuid, current_file_name);
            }
        } else {
            closeCurrentScenseView();
        }
    } else {
        qDebug() << "save error:" << errorString << error;
    }
    emit signalSaveFileStatus(status);
}

void CCentralwidget::updateTabName(const QString &uuid, const QString &newTabName)
{
    if (m_topMutipTabBarWidget != nullptr) {

        //1.刷新标签也名字及其tooltip
        m_topMutipTabBarWidget->updateTabBarName(uuid, newTabName);
        m_topMutipTabBarWidget->setTabBarTooltipName(uuid, newTabName);

        //2.刷新可能要修改的主界面标题
        if (m_topMutipTabBarWidget->count() == 1) {
            emit signalScenceViewChanged(newTabName);
        } else {
            emit signalScenceViewChanged("");
        }
    }

}

void CCentralwidget::updateTitle()
{
    QString uuid = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->uuid();
    QString name = CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->getShowViewNameByModifyState();
    QMetaObject::invokeMethod(this, "updateTabName", Qt::QueuedConnection, Q_ARG(QString, uuid), Q_ARG(QString, name));
}

//进行图片导入
void CCentralwidget::importPicture()
{

    DFileDialog *fileDialog = new DFileDialog();
    //设置文件保存对话框的标题
    //fileDialog->setWindowTitle(tr("导入图片"));
    fileDialog->setWindowTitle(tr("Import Picture"));
    QStringList filters;
    filters << "*.png *.jpg *.bmp *.tif";
    fileDialog->setNameFilters(filters);
    fileDialog->setFileMode(QFileDialog::ExistingFiles);

    if (fileDialog->exec() ==   QDialog::Accepted) {
        QStringList filenames = fileDialog->selectedFiles();
        slotPastePicture(filenames);
    } else {
        m_leftToolbar->slotShortCutSelect();
    }

}

//点击图片进行导入
void CCentralwidget::openPicture(QString path)
{
    QPixmap pixmap = QPixmap(path);
    slotPastePixmap(pixmap);
}

//导入图片
void CCentralwidget::slotPastePicture(QStringList picturePathList)
{
    m_pictureTool->drawPicture(picturePathList, static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene()), this);
}

void CCentralwidget::slotPastePixmap(QPixmap pixmap)
{
    m_pictureTool->addImages(pixmap, 1, static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene()), this);
}

void CCentralwidget::initUI()
{
    m_hLayout->setMargin(0);
    m_hLayout->setSpacing(0);
    m_hLayout->addWidget(m_leftToolbar);
    m_hLayout->addLayout(m_stackedLayout);

    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->addWidget(m_topMutipTabBarWidget);
    vLayout->addLayout(m_hLayout);
    vLayout->setMargin(0);
    vLayout->setSpacing(0);
    setLayout(vLayout);

    // 只有一个标签需要隐藏多标签控件
    m_topMutipTabBarWidget->hide();
}

void CCentralwidget::slotResetOriginPoint()
{
    /*QRect rect = CManageViewSigleton::GetInstance()->getCurView()->viewport()->rect();
    CManageViewSigleton::GetInstance()->getCurView()->setSceneRect(rect);*/
}

void CCentralwidget::slotAttributeChanged()
{
    if (static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene()) != nullptr) {
        static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->attributeChanged();
    }
}

void CCentralwidget::slotZoom(qreal scale)
{
    CManageViewSigleton::GetInstance()->getCurView()->scale(scale);
}

void CCentralwidget::slotSaveToDDF(bool isCloseNow)
{
    // 是否保存后关闭该图元
    m_isCloseNow = isCloseNow;
    CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->setSaveDDFTriggerAction(ESaveDDFTriggerAction::SaveAction);
    CManageViewSigleton::GetInstance()->getCurView()->doSaveDDF();
    // 释放资源需要等待view提示是否保存成功
}

void CCentralwidget::slotDoNotSaveToDDF()
{
    // [0] 关闭当前view
    closeCurrentScenseView();

    //    if ( 1 == m_topMutipTabBarWidget->count()) {
    //        qDebug() << "close last one ScenseView.";
    //        emit signalLastTabBarRequestClose();
    //        return;
    //    }
}

void CCentralwidget::slotSaveAs()
{
    CManageViewSigleton::GetInstance()->getCurView()->showSaveDDFDialog(false);
}

void CCentralwidget::slotTextFontFamilyChanged()
{
    if (static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene()) != nullptr) {
        static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->textFontFamilyChanged();
    }
}

void CCentralwidget::slotTextFontSizeChanged()
{
    if (static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene()) != nullptr) {
        static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->textFontSizeChanged();
    }
}

void CCentralwidget::slotNew()
{
    m_topMutipTabBarWidget->addTabBarItem();
    updateTitle();
}

void CCentralwidget::slotPrint()
{
    static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->clearSelection();
    QImage image = getSceneImage(1);
    m_printManager->showPrintDialog(image, this);
}

void CCentralwidget::slotShowCutItem()
{
    CManageViewSigleton::GetInstance()->getCurView()->setContextMenuAndActionEnable(false);
    static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->showCutItem();
}

void CCentralwidget::onEscButtonClick()
{
    //如果当前是裁剪模式则退出裁剪模式　退出裁剪模式会默认设置工具栏为选中
    if (cut == CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->getCurrentDrawToolMode()) {
        CManageViewSigleton::GetInstance()->getCurView()->slotQuitCutMode();
    } else {
        m_leftToolbar->slotShortCutSelect();
    }
    ///清空场景中选中图元
    static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->clearSelection();
}

void CCentralwidget::slotCutLineEditeFocusChange(bool isFocus)
{
    CManageViewSigleton::GetInstance()->getCurView()->disableCutShortcut(isFocus);
}

void CCentralwidget::slotDoSaveImage(QString completePath)
{
    int type = m_exportImageDialog->getImageType();
    if (type == CExportImageDialog::PDF) {
        QImage image = getSceneImage(1);
        QPdfWriter writer(completePath);
        int ww = image.width();
        int wh = image.height();
        writer.setResolution(96);
        writer.setPageSizeMM(QSizeF(25.4 * ww / 96, 25.4 * wh / 96));
        QPainter painter(&writer);
        painter.drawImage(0, 0, image);
    } else if (type == CExportImageDialog::PNG) {
        QString format = m_exportImageDialog->getImageFormate();
        int quality = m_exportImageDialog->getQuality();
        QImage image = getSceneImage(2);
        image.save(completePath, format.toUpper().toLocal8Bit().data(), quality);
    } else {
        QString format = m_exportImageDialog->getImageFormate();
        int quality = m_exportImageDialog->getQuality();
        QImage image = getSceneImage(1);
        image.save(completePath, format.toUpper().toLocal8Bit().data(), quality);
    }
}

void CCentralwidget::addView(QString viewName, const QString &uuid)
{
    qDebug() << "addView:" << viewName;
    CGraphicsView *pNewView = createNewScense(viewName, uuid);
    CManageViewSigleton::GetInstance()->setCurView(pNewView);
}

void CCentralwidget::slotRectRediusChanged(int value)
{
    qDebug() << "value" << value;
}

void CCentralwidget::slotQuitApp()
{
    // 此函数没有再被使用，所有的操作在mainwindow中进行实现
    int count = m_topMutipTabBarWidget->count();
    for (int i = 0; i < count; i++) {
        QString current_name = m_topMutipTabBarWidget->tabText(m_topMutipTabBarWidget->currentIndex());
        QString current_uuid = m_topMutipTabBarWidget->tabData(m_topMutipTabBarWidget->currentIndex()).toString();
        CGraphicsView *closeView = CManageViewSigleton::GetInstance()->getViewByUUID(current_uuid);
        if (closeView == nullptr) {
            qDebug() << "close error view:" << current_name;
            continue;
        } else {

            // 如果只剩一个画板并且没有进行修改且不是导入文件则不再创建新的画板
            if ( !closeView->getDrawParam()->getModify()
                    && 1 == m_topMutipTabBarWidget->count()
                    && closeView->getDrawParam()->getDdfSavePath().isEmpty()) {
                emit signalLastTabBarRequestClose();
                return;
            }

            qDebug() << "close view:" << current_name;
            bool editFlag = closeView->getDrawParam()->getModify();

            this->tabItemCloseRequested(current_name, current_uuid);

            if (editFlag) {
                break;
            }
        }
    }
}

void CCentralwidget::viewChanged(QString viewName, const QString &uuid)
{
    qDebug() << "viewChanged" << viewName;

    // [0] 判断当前新显示的视图是否为空
    CGraphicsView *view = CManageViewSigleton::GetInstance()->getViewByUUID(uuid);
    if (nullptr == view) {
        qWarning() << "can not find viewName:" << viewName;

//        // 判断标签栏是否还有元素，值为0时表示关闭所有的视图,此时应该最少保持有一个窗口在显示
//        if (0 == m_topMutipTabBarWidget->count()) {
//            qDebug() << "window has none view,create new view at least one";
//            // [0] 判断是否已经打开此文件,已经打开则显示此文件
//            QString nextTabBarName = m_topMutipTabBarWidget->getNextTabBarDefaultName();
//            if (m_topMutipTabBarWidget->tabBarNameIsExist(nextTabBarName)) {
//                qDebug() << "create same name Scence,deepin-draw will not create.";
//                return;
//            }
//            createNewScense(nextTabBarName);
//            m_topMutipTabBarWidget->addTabBarItem();
//        }
        return;
    }

    // [1] 鼠标选择工具回到默认状态
    m_leftToolbar->slotShortCutSelect();

    // [2] 替换最新的视图到当前显示界上
    CManageViewSigleton::GetInstance()->setCurView(view);
    m_stackedLayout->setCurrentWidget(view);
    //initSceneRect();

    // [3] 鼠标选择工具回到默认状态
    m_leftToolbar->slotShortCutSelect();

    // [4] 还原比例显示
    slotSetScale(view->getScale());

    // [5] 更新主题
    switchTheme(systemTheme);

    // [6] 标签显示或者隐藏判断
    if (m_topMutipTabBarWidget->count() == 1) {
        m_topMutipTabBarWidget->hide();
        emit signalScenceViewChanged(viewName);
    } else {
        m_topMutipTabBarWidget->show();
        emit signalScenceViewChanged("");
    }
}

void CCentralwidget::tabItemCloseRequested(QString viewName, const QString &uuid)
{
    bool modify = CManageViewSigleton::GetInstance()->getViewByUUID(uuid)->getDrawParam()->getModify();
    qDebug() << "tabItemCloseRequested:" << viewName << "modify:" << modify;
    // 判断当前关闭项是否已经被修改
    if (!modify) {
        closeCurrentScenseView();
        return;
    } else {
        emit signalCloseModifyScence();
    }
}

void CCentralwidget::slotLoadDragOrPasteFile(QString path)
{
    // 此函数主要是截断是否已经有打开过的ddf文件，避免重复打开操作
    QStringList tempfilePathList = path.split("\n");
    slotLoadDragOrPasteFile(tempfilePathList);
}

void CCentralwidget::slotLoadDragOrPasteFile(QStringList files)
{
    qDebug() << "slotLoadDragOrPasteFile:" << files;
    QStringList filterList;
    QString ddfPath = "";
    for (int i = 0; i < files.size(); i++) {
        if (QFileInfo(files[i]).suffix().toLower() == ("ddf")) {
            ddfPath = files[i].replace("file://", "");
            QString fileName = ddfPath;
            fileName = fileName.split('/').last();
            fileName = fileName.replace(".ddf", "");
            if (m_topMutipTabBarWidget->IsFileOpened(files[i])) {
                emit signalDDFFileOpened(fileName);
                continue;
            }
            filterList.append(files[i]);


//            // 如果ddf打开则自动跳转到打开的标签，不存在则打开文件
//            if (m_topMutipTabBarWidget->tabBarNameIsExist(fileName)) {
//                emit signalDDFFileOpened(fileName);
//                return;
//            }
        }
    }
    qDebug() << "slotLoadDragOrPasteFile --- filter:" << filterList;
    emit signalTransmitLoadDragOrPasteFile(/*files*/filterList);
}

void CCentralwidget::slotShowExportDialog()
{
    static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->clearSelection();
    m_exportImageDialog->showMe();
}

void CCentralwidget::slotSetScale(const qreal scale)
{
    emit signalSetScale(scale);
}

QImage CCentralwidget::getSceneImage(int type)
{
    QImage image(static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->sceneRect().width(), static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->sceneRect().height(), QImage::Format_ARGB32);
    CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->setRenderImage(type);
    if (type == 2) {
        image.fill(Qt::transparent);
        static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->setBackgroundBrush(Qt::transparent);
    }
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    static_cast<CDrawScene *>(CManageViewSigleton::GetInstance()->getCurView()->scene())->render(&painter, QRect(), QRect(), Qt::IgnoreAspectRatio);
    if (type == 2) {
        resetSceneBackgroundBrush();
    }
    CManageViewSigleton::GetInstance()->getCurView()->getDrawParam()->setRenderImage(0);

    return  image;
}

void CCentralwidget::initConnect()
{
    //导入图片信号槽
    connect(m_leftToolbar, SIGNAL(importPic()), this, SLOT(importPicture()));
    connect(m_leftToolbar, SIGNAL(signalBegainCut()), this, SLOT(slotShowCutItem()));

    connect(m_exportImageDialog, SIGNAL(signalDoSave(QString)), this, SLOT(slotDoSaveImage(QString)));

    // 连接顶部菜单添加、标签改变、删除信号
    connect(m_topMutipTabBarWidget, &CMultipTabBarWidget::signalNewAddItem, this, &CCentralwidget::addView);
    connect(m_topMutipTabBarWidget, &CMultipTabBarWidget::signalItemChanged, this, &CCentralwidget::viewChanged);
    connect(m_topMutipTabBarWidget, &CMultipTabBarWidget::signalTabItemCloseRequested, this, &CCentralwidget::tabItemCloseRequested);
    connect(m_topMutipTabBarWidget, &CMultipTabBarWidget::signalTabItemsCloseRequested, this, &CCentralwidget::signalTabItemsCloseRequested);

    connect(m_leftToolbar, &CLeftToolBar::singalDoCutFromLeftToolBar, this, [ = ]() {
        CGraphicsView *newview = CManageViewSigleton::GetInstance()->getCurView();
        newview->slotDoCutScene();
    });
}


