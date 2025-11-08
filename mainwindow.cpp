#include "mainwindow.h"
#include <QStackedWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QFile>
#include <QDialog>
#include <QTimeEdit>
#include <QScrollArea>
#include <QIntValidator>

struct TripOption {
    enum Type { Direct, Transfer1, Transfer2 };
    Type type;
    int totalTime = 0;
    QString summary;
    QString html;
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    loadMockData();
    setupUI();
    stackedWidget->setCurrentWidget(loginPage);
}

void MainWindow::loadMockData()
{
    if (QFile::exists(SAVE_FILE)) {
        loadRoutesFromFile();
        return;
    }
    routes = {
        Route("D1",  "D1路",  {"火车站", "人民广场", "市政府", "图书馆", "大学城", "科技园"}),
        Route("101", "101路", {"动物园", "人民广场", "商业街", "市政府", "体育中心"}),
        Route("202", "202路", {"火车东站", "图书馆", "大学城", "科技园", "软件园"}),
        Route("303", "303路", {"机场", "火车站", "动物园", "体育中心", "软件园", "姜宜君家"})
    };
    refreshAllStops();
}

void MainWindow::saveRoutesToFile()
{
    QJsonArray root;
    for (const auto &r : std::as_const(routes)) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["name"] = r.name;
        QJsonArray stops;
        for (const auto &s : r.stops) stops.append(s);
        obj["stops"] = stops;

        QJsonArray times;
        for (int t : r.travelTimes) times.append(t);
        obj["travelTimes"] = times;

        obj["firstBus"] = r.firstBus.toString("HH:mm");
        obj["lastBus"] = r.lastBus.toString("HH:mm");

        root.append(obj);
    }
    QJsonDocument doc(root);
    QFile file(SAVE_FILE);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadRoutesFromFile()
{
    QFile file(SAVE_FILE);
    if (!file.open(QIODevice::ReadOnly)) {
        // 如果文件不存在，可以保留空 routes
        refreshAllStops();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) {
        refreshAllStops();
        return;
    }

    routes.clear();
    QJsonArray root = doc.array();
    for (const auto v : std::as_const(root)) {
        QJsonObject obj = v.toObject();
        Route r;
        r.id   = obj["id"].toString();
        r.name = obj["name"].toString();

        // 加载 stops
        for (const auto& s : obj["stops"].toArray()) {
            r.stops.append(s.toString());
        }

        // === 新增字段：travelTimes ===
        if (obj.contains("travelTimes") && obj["travelTimes"].isArray()) {
            QJsonArray timesArr = obj["travelTimes"].toArray();
            for (const auto& t : std::as_const(timesArr)) {
                r.travelTimes.append(t.toInt());
            }
        } else {
            // 兼容旧数据：默认每段 3 分钟
            for (int i = 0; i < r.stops.size() - 1; ++i) {
                r.travelTimes.append(3);
            }
        }

        // === 新增字段：首末班车 ===
        if (obj.contains("firstBus")) {
            r.firstBus = QTime::fromString(obj["firstBus"].toString(), "HH:mm");
        }
        if (obj.contains("lastBus")) {
            r.lastBus = QTime::fromString(obj["lastBus"].toString(), "HH:mm");
        }

        // 设置默认首末班（防止无效时间）
        if (!r.firstBus.isValid()) r.firstBus = QTime(6, 0);   // 06:00
        if (!r.lastBus.isValid())  r.lastBus = QTime(22, 30);  // 22:30

        routes.append(r);
    }

    refreshAllStops();
}

void MainWindow::refreshAllStops() {
    QSet<QString> stops;
    for (const auto& r : std::as_const(routes)) {
        for (const auto& s : r.stops) stops.insert(s);
    }
    allStops = stops.values();
    std::sort(allStops.begin(), allStops.end());
}

 void MainWindow::setupUI() {
    qApp->setStyle("Fusion");

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor(42,42,42));
    darkPalette.setColor(QPalette::WindowText,      QColor(212,212,212));
    darkPalette.setColor(QPalette::Base,            QColor(30,30,30));
    darkPalette.setColor(QPalette::AlternateBase,   QColor(45,45,45));
    darkPalette.setColor(QPalette::PlaceholderText, QColor(120,120,120));
    darkPalette.setColor(QPalette::Text,            QColor(212,212,212));
    darkPalette.setColor(QPalette::Button,          QColor(50,50,50));
    darkPalette.setColor(QPalette::ButtonText,      QColor(212,212,212));
    darkPalette.setColor(QPalette::Highlight,       QColor(0, 120, 215));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    darkPalette.setColor(QPalette::Link,            QColor(0, 120, 215));
    darkPalette.setColor(QPalette::LinkVisited,     QColor(164, 0, 164));

    qApp->setPalette(darkPalette);

    setStyleSheet(
        "QMainWindow{ background: #2A2A2A; }"
        "QMenuBar{ background: #2A2A2A; color: #DCDCDC; }"
        "QStatusBar{ background: #2A2A2A; color: #DCDCDC; }"
        "QTextEdit, QLineEdit, QPlainTextEdit{"
        "  background: #252526; color: #CCCCCC; border: 1px solid #454545;"
        "  selection-background-color: #264F78; }"
        "QLineEdit {"
        "   padding: 6px 8px;"
        "   border: 1px solid #454545;"
        "   border-radius: 4px;"
        "   background: #252526;"
        "   color: #CCCCCC;"
        "   selection-background-color: #264F78;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid #007ACC;"
        "   background: #1E1E1E;"
        "}"
        "QTextEdit {"
        "   padding: 8px;"
        "   border: 1px solid #454545;"
        "   border-radius: 6px;"
        "   background: #1E1E1E;"
        "   color: #D4D4D4;"
        "   selection-background-color: #264F78;"
        "}"
        "QTextEdit:focus {"
        "   border: 1px solid #007ACC;"
        "}"
        "QComboBox {"
        "   padding: 6px 8px;"
        "   border: 1px solid #454545;"
        "   border-radius: 4px;"
        "   background: #252526;"
        "   color: #CCCCCC;"
        "}"
        "QComboBox:focus {"
        "   border: 1px solid #007ACC;"
        "}"
        "QComboBox QAbstractItemView {"
        "   padding: 4px;"
        "   border: 1px solid #454545;"
        "   background: #252526;"
        "   color: #CCCCCC;"
        "   selection-background-color: #264F78;"
        "   outline: 0;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "   height: 32px;"
        "   padding: 6px 10px;"
        "   font-size: 14px;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "   background: #264F78;"
        "   color: white;"
        "}"
        "QLabel {"
        "   color: #CCCCCC;"
        "   font-size: 14px;"
        "}"
        "QLabel[title=\"true\"] {"
        "   font-size: 24px;"
        "   font-weight: bold;"
        "   color: #4F46E5;"
        "}"
        "QFormLayout QLabel {"
        "   color: #AAAAAA;"
        "   font-weight: 500;"
        "   min-width: 80px;"
        "}"
        "QScrollBar:vertical {"
        "   width: 10px;"
        "   background: #2A2A2A;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #555555;"
        "   border-radius: 4px;"
        "   min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "   background: #777777;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "   height: 0px;"
        "}");

    setWindowTitle("🚌 公交管理系统");
    setFixedSize(800, 600);

    // 创建主容器（含 stackedWidget + 底部按钮）
    auto centralWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // === Stacked Widget ===
    stackedWidget = new QStackedWidget;
    mainLayout->addWidget(stackedWidget, 1); // 1: 占满剩余空间

    // === Login Page ===
    loginPage = new QWidget;
    {
        auto centerWidget = new QWidget;
        auto centerLayout = new QVBoxLayout(centerWidget);
        centerLayout->setAlignment(Qt::AlignCenter);
        centerLayout->setSpacing(20);

        auto title = new QLabel("🚌 公交管理系统");
        title->setProperty("title", true);
        title->setAlignment(Qt::AlignCenter);

        auto userBtn = new QPushButton("👤 用户登录");
        auto adminBtn = new QPushButton("👑 管理员登录");
        for (auto* btn : {userBtn, adminBtn}) {
            btn->setMinimumSize(180, 50);
            btn->setStyleSheet(
                "QPushButton {"
                "   font-size: 16px;"
                "   border-radius: 6px;"
                "   padding: 8px 16px;"
                "   background: #4F46E5;"
                "   color: white;"
                "}"
                "QPushButton:hover { background: #4338CA; }"
                );
        }
        adminBtn->setStyleSheet(adminBtn->styleSheet().replace("#4F46E5", "#7C3AED")
                                    .replace("#4338CA", "#6D28D9"));

        auto btnLayout = new QHBoxLayout;
        btnLayout->setAlignment(Qt::AlignCenter);
        btnLayout->setSpacing(30);
        btnLayout->addWidget(userBtn);
        btnLayout->addWidget(adminBtn);

        centerLayout->addWidget(title);
        centerLayout->addLayout(btnLayout);

        auto pageLayout = new QVBoxLayout(loginPage);
        pageLayout->addWidget(centerWidget, 1, Qt::AlignCenter);
        pageLayout->setContentsMargins(50, 50, 50, 50);

        connect(userBtn, &QPushButton::clicked, this, &MainWindow::loginAsUser);
        connect(adminBtn, &QPushButton::clicked, this, &MainWindow::loginAsAdmin);
    }

    // === Search Page ===
    searchPage = new QWidget;
    {
        auto layout = new QFormLayout(searchPage);
        layout->setFormAlignment(Qt::AlignCenter);
        layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        layout->setSpacing(15);
        layout->setContentsMargins(60, 40, 60, 20);

        startEdit = new QLineEdit;
        endEdit = new QLineEdit;
        startSuggest = new QComboBox;
        endSuggest = new QComboBox;
        startSuggest->setEditable(true);
        endSuggest->setEditable(true);
        startSuggest->hide(); endSuggest->hide();

        searchLineBtn = new QPushButton("🔍 查询线路");
        searchLineBtn->setMinimumHeight(40);
        searchLineBtn->setStyleSheet("font-size: 16px; background: #3B82F6; color: white; border-radius: 6px;");
        resultDisplay = new QTextEdit;
        resultDisplay->setReadOnly(true);
        resultDisplay->setMinimumHeight(200);
        resultDisplay->document()->setDefaultStyleSheet(
            "body { font-family: 'Segoe UI', 'Microsoft YaHei', sans-serif; font-size: 14px; color: #CCCCCC; background: transparent; }"
            "h3 { margin: 8px 0; color: #4F46E5; }"
            ".route-card { margin: 10px 0; padding: 12px; border-radius: 8px; background: #1E1E1E; border-left: 4px solid #10B981; }"
            ".transfer-card { margin: 10px 0; padding: 12px; border-radius: 8px; background: #1E1E1E; border-left: 4px solid #3B82F6; }"
            ".station-list { margin: 6px 0 0 0; padding: 6px 10px; background: #252526; border-radius: 4px; }"
            ".highlight { color: #4F46E5; font-weight: bold; }"
            ".note { color: #A0A0A0; font-size: 13px; margin-top: 4px; }"
            );

        layout->addRow("起点站:", startEdit);
        layout->addRow("", startSuggest);
        layout->addRow("终点站:", endEdit);
        layout->addRow("", endSuggest);
        layout->addRow(searchLineBtn);
        layout->addRow(resultDisplay);

        connect(startEdit, &QLineEdit::textEdited, this, &MainWindow::updateStartSuggestions);
        connect(endEdit, &QLineEdit::textEdited, this, &MainWindow::updateEndSuggestions);
        connect(searchLineBtn, &QPushButton::clicked, this, &MainWindow::searchLines);
    }

    // === Route Page ===
    routePage = new QWidget;
    {
        auto layout = new QHBoxLayout(routePage);
        layout->setContentsMargins(60, 40, 60, 40);
        layout->setSpacing(20);

        auto left = new QVBoxLayout;

        // 新增：所有线路列表
        routeIdList = new QListWidget;
        routeIdList->setMinimumHeight(300);
        routeIdList->setMaximumHeight(300);
        routeIdList->setStyleSheet(
            "QListWidget {"
            "   border: 1px solid #454545;"
            "   border-radius: 4px;"
            "   background: #1E1E1E;"
            "   color: #CCCCCC;"
            "   outline: 0;"
            "}"
            "QListWidget::item {"
            "   padding: 4px;"
            "}"
            "QListWidget::item:selected {"
            "   background: #264F78;"
            "   color: white;"
            "}"
            );
        connect(routeIdList, &QListWidget::itemClicked, this, &MainWindow::onRouteIdListItemClicked);

        left->addWidget(new QLabel(" 所有线路："));
        left->addWidget(routeIdList);

        routeIdEdit = new QLineEdit;
        routeIdEdit->setPlaceholderText("输入线路号，如 D1");
        routeIdEdit->setMinimumHeight(40);
        searchRouteBtn = new QPushButton("🔍 查询");
        searchRouteBtn->setMinimumHeight(40);
        searchRouteBtn->setStyleSheet("background: #10B981; color: white; border-radius: 6px;");

        left->addWidget(routeIdEdit);
        left->addWidget(searchRouteBtn);
        left->addStretch();

        routeDetailDisplay = new QTextEdit;
        routeDetailDisplay->setReadOnly(true);
        routeDetailDisplay->document()->setDefaultStyleSheet(
            "body { font-family: 'Segoe UI', sans-serif; font-size: 14px; color: #CCCCCC; background: transparent; }"
            "h3 { color: #4F46E5; margin-bottom: 8px; }"
            "ul { margin-top: 4px; }"
            "li { margin: 4px 0; }"
            );


        layout->addLayout(left, 1);
        layout->addWidget(routeDetailDisplay, 2);

        connect(searchRouteBtn, &QPushButton::clicked, this, &MainWindow::searchRouteById);
    }

    // === Manage Page ===
    managePage = new QWidget;
    {
        auto layout = new QVBoxLayout(managePage);
        layout->setContentsMargins(60, 40, 60, 40);
        layout->setSpacing(15);

        auto manageBar = new QHBoxLayout;
        auto saveNowBtn = new QPushButton("💾 立即保存");
        auto exportBtn = new QPushButton("📤 导出路线");
        auto importBtn = new QPushButton("📥 导入路线");
        for (auto* btn : {saveNowBtn, exportBtn, importBtn}) {
            btn->setStyleSheet(
                "QPushButton {"
                "   padding: 6px 12px;"
                "   border-radius: 4px;"
                "   background: #252526;"        // VS-Code 深灰
                "   color: #CCCCCC;"
                "   border: 1px solid #454545;"
                "}"
                "QPushButton:hover {"
                "   background: #2E2E30;"
                "   border: 1px solid #007ACC;"  // 主题色边框
                "}"
                "QPushButton:pressed {"
                "   background: #1E1E1E;"
                "}");
        }
        manageBar->addWidget(saveNowBtn);
        manageBar->addWidget(exportBtn);
        manageBar->addWidget(importBtn);
        manageBar->addStretch();
        layout->insertLayout(0, manageBar);

        connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportRoutes);
        connect(importBtn, &QPushButton::clicked, this, &MainWindow::importRoutes);

        addRouteBtn = new QPushButton("➕ 添加新路线");
        addRouteBtn->setStyleSheet("background: #0EA5E9; color: white; border-radius: 6px; padding: 8px;");
        routeList = new QListWidget;
        routeList->setStyleSheet(
            "QListWidget {"
            "   border: 1px solid #454545;"
            "   border-radius: 6px;"
            "   background: #1E1E1E;"
            "   color: #CCCCCC;"
            "   outline: 0;"
            "}"
            "QListWidget::item {"
            "   padding: 6px;"
            "   border-bottom: 1px solid #333333;"
            "}"
            "QListWidget::item:selected {"
            "   background: #264F78;"
            "   color: white;"
            "}"
            "QListWidget::item:hover {"
            "   background: #2A2A2A;"
            "}"
            );

        layout->addWidget(addRouteBtn);
        layout->addWidget(routeList);

        connect(saveNowBtn, &QPushButton::clicked, this, &MainWindow::saveRoutesToFile);
        //connect(addRouteBtn, &QPushButton::clicked, this, &MainWindow::addNewRoute); // 旧调用先保留
        connect(addRouteBtn, &QPushButton::clicked, this, [this]() {
            openRouteEditDialog();
        });

        addRouteBtn->setEnabled(true);
    }

    // 添加页面到 stackedWidget
    stackedWidget->addWidget(loginPage);
    stackedWidget->addWidget(searchPage);
    stackedWidget->addWidget(routePage);
    stackedWidget->addWidget(managePage);

    // === Bottom Navigation ===
    auto footer = new QHBoxLayout;
    footer->setContentsMargins(20, 10, 20, 10);
    footer->setSpacing(10);

    backToSearch = new QPushButton("线路查询");
    backToRoute = new QPushButton("车次查询");
    backToManage = new QPushButton("路线管理");
    logoutBtn = new QPushButton("退出登录");

    for (auto* btn : {backToSearch, backToRoute, backToManage}) {
        btn->setStyleSheet(
            "QPushButton {"
            "   padding: 6px 12px;"
            "   border-radius: 4px;"
            "   background: #252526;"        // VS-Code 深灰
            "   color: #CCCCCC;"
            "   border: 1px solid #454545;"
            "}"
            "QPushButton:hover {"
            "   background: #2E2E30;"
            "   border: 1px solid #007ACC;"  // 主题色边框
            "}"
            "QPushButton:pressed {"
            "   background: #1E1E1E;"
            "}");
    }

    logoutBtn->setStyleSheet(
        "QPushButton {"
        "   padding: 6px 12px;"
        "   border-radius: 4px;"
        "   background: #B00020;"          // 深红
        "   color: #FFFFFF;"
        "   border: 1px solid #C62828;"
        "}"
        "QPushButton:hover {"
        "   background: #C62828;"
        "}");

    footer->addWidget(backToSearch);
    footer->addWidget(backToRoute);
    footer->addWidget(backToManage);
    footer->addStretch();
    footer->addWidget(logoutBtn);

    mainLayout->addLayout(footer);

    // 设置中央 widget
    setCentralWidget(centralWidget);

    // 初始隐藏管理按钮
    backToManage->setVisible(false);

    updateBtnState(false);

    connect(backToSearch, &QPushButton::clicked, this, &MainWindow::switchToSearch);
    connect(backToRoute, &QPushButton::clicked, this, &MainWindow::switchToRoute);
    connect(backToManage, &QPushButton::clicked, this, &MainWindow::switchToManage);
    connect(logoutBtn, &QPushButton::clicked, this, &MainWindow::logout);

    routeList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(routeList, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onRouteContextMenu);
    connect(routeList, &QListWidget::itemDoubleClicked,
            this, &MainWindow::onRouteItemDoubleClicked);

    connect(startSuggest, QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::fillStartFromSuggestion);
    connect(endSuggest,   QOverload<int>::of(&QComboBox::activated),
            this, &MainWindow::fillEndFromSuggestion);

    startSuggest->installEventFilter(this);
    endSuggest->installEventFilter(this);
}

void MainWindow::loginAsUser() {
    currentUserRole = "user";
    stackedWidget->setCurrentWidget(searchPage);
    backToManage->setVisible(false);
    updateBtnState(true);
    refreshRouteIdList();
}

void MainWindow::loginAsAdmin() {
    currentUserRole = "admin";
    stackedWidget->setCurrentWidget(searchPage);
    backToManage->setVisible(true);
    updateBtnState(true);
    refreshRouteIdList();
}

void MainWindow::updateBtnState(bool state)
{
    backToSearch->setEnabled(state);
    backToRoute->setEnabled(state);
    backToManage->setEnabled(state);
    logoutBtn->setEnabled(state);
}

void MainWindow::logout() {
    currentUserRole = "";
    stackedWidget->setCurrentWidget(loginPage);
    backToManage->setVisible(false);
    updateBtnState(false);
}

void MainWindow::switchToSearch() { stackedWidget->setCurrentWidget(searchPage); }
void MainWindow::switchToRoute() { stackedWidget->setCurrentWidget(routePage); }
void MainWindow::switchToManage() {
    if (currentUserRole != "admin") return;
    routeList->clear();
    for (const auto& r : std::as_const(routes)) {
        QString itemText = QString("%1 (%2)").arg(r.name, r.id);
        auto item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, r.id);
        routeList->addItem(item);
    }
    stackedWidget->setCurrentWidget(managePage);
    refreshRouteIdList();
}

void MainWindow::updateStartSuggestions(const QString& text) {
    startSuggest->clear();
    if (text.isEmpty()) {
        startSuggest->hide();
        return;
    }
    for (const auto& stop : std::as_const(allStops)) {
        if (stop.contains(text, Qt::CaseInsensitive)) {
            startSuggest->addItem(stop);
            if (startSuggest->count() >= 50) break;
        }
    }
    startSuggest->show();
    //startSuggest->showPopup();
}

void MainWindow::updateEndSuggestions(const QString& text) {
    endSuggest->clear();
    if (text.isEmpty()) {
        endSuggest->hide();
        return;
    }
    for (const auto& stop : std::as_const(allStops)) {
        if (stop.contains(text, Qt::CaseInsensitive)) {
            endSuggest->addItem(stop);
            if (endSuggest->count() >= 50) break;
        }
    }
    endSuggest->show();
    //endSuggest->showPopup();
}

void MainWindow::fillStartFromSuggestion(int idx)
{
    startEdit->setFocus();
    startSuggest->hide();
    if (idx == -1) {
        return;
    }
    startEdit->setText(startSuggest->itemText(idx));
    startEdit->setFocus();
}

void MainWindow::fillEndFromSuggestion(int idx)
{
    endEdit->setFocus();
    endSuggest->hide();
    if (idx == -1) {
        return;
    }
    endEdit->setText(endSuggest->itemText(idx));
    endEdit->setFocus();
}

QVector<Route> MainWindow::findDirectRoutes(const QString& start, const QString& end) {
    QVector<Route> found;
    for (const auto& r : std::as_const(routes)) {
        if (r.stops.contains(start) && r.stops.contains(end))
            found.append(r);
    }
    return found;
}

QVector<QString> MainWindow::getStopPath(const Route& r, const QString& start, const QString& end) {
    int i1 = r.stops.indexOf(start);
    int i2 = r.stops.indexOf(end);
    if (i1 == -1 || i2 == -1) return {};
    if (i1 < i2)
        return r.stops.mid(i1, i2 - i1 + 1);
    else {
        auto rev = r.stops.mid(i2, i1 - i2 + 1);
        std::reverse(rev.begin(), rev.end());
        return rev;
    }
}

void MainWindow::searchLines()
{
    QString start = startEdit->text().trimmed();
    QString end = endEdit->text().trimmed();
    if (start.isEmpty() || end.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入起点和终点");
        return;
    }
    if (start == end) {
        resultDisplay->setHtml("<p style=\"color: #A0A0A0;\">起点与终点相同，无需乘车。</p>");
        return;
    }

    struct Plan {
        int totalTime = 0;
        QString html;
    };
    QVector<Plan> allPlans;

    // === 1. 直达方案 ===
    for (const auto& r : std::as_const(routes)) {
        if (!r.stops.contains(start) || !r.stops.contains(end)) continue;
        auto path = getStopPath(r, start, end);
        if (path.isEmpty()) continue;
        int minutes = calculateTravelTime(r, start, end);
        if (minutes <= 0) continue;
        QString timeStr = QString("（约 %1 分钟）").arg(minutes);
        QString busInfo = QString("🕒 首班 %1 &nbsp; 末班 %2")
                              .arg(r.firstBus.toString("HH:mm"), r.lastBus.toString("HH:mm"));
        // 构建带每段耗时的路径
        QString detailedPath = "";
        for (int i = 0; i < path.size(); ++i) {
            detailedPath += path[i];
            if (i < path.size() - 1) {
                int segTime = calculateTravelTime(r, path[i], path[i+1]);
                detailedPath += QString(" <span style=\"color:#666;\">↓ %1分钟</span> → ").arg(segTime);
            }
        }
        QString html = QString(
                           "<div style=\"margin: 16px 0; padding: 16px; border-radius: 8px; background: #1E1E1E; border-bottom: 1px solid #333333; box-shadow: 0 2px 4px rgba(0,0,0,0.3);\">"
                           "<div style=\"display: inline-block; width: 4px; height: 100%; background: #10B981; margin-left: -12px; margin-right: 12px; vertical-align: top;\"></div>"
                           "<div style=\"display: inline-block; width: calc(100% - 16px);\">"
                           "<h3 style=\"margin: 0 0 8px 0; color: #00A65B;\">🚌 直达 <span style=\"color: #00A65B; font-weight: bold;\">%1</span></h3>"
                           "<p>从 <b>%2</b> 到 <b>%3</b>（共 <b>%4</b> 站）%5</p>"
                           "<div style=\"margin: 6px 0 0 0; padding: 6px 10px; background: #252526; border-radius: 4px;\">%6</div>"
                           "<div style=\"margin-top: 6px; color: #A0A0A0; font-size: 13px;\">%7</div>"
                           "</div></div>")
                           .arg(r.name, start, end, QString::number(path.size()), timeStr, detailedPath, busInfo);
        allPlans.append({minutes, html});
    }

    // === 2. 一次换乘 ===
    for (const auto& r1 : std::as_const(routes)) {
        if (!r1.stops.contains(start)) continue;
        for (const auto& r2 : std::as_const(routes)) {
            if (r1.id == r2.id || !r2.stops.contains(end)) continue;
            for (const QString& mid : r1.stops) {
                if (!r2.stops.contains(mid)) continue;
                auto p1 = getStopPath(r1, start, mid);
                auto p2 = getStopPath(r2, mid, end);
                if (p1.isEmpty() || p2.isEmpty()) continue;
                int t1 = calculateTravelTime(r1, start, mid);
                int t2 = calculateTravelTime(r2, mid, end);
                if (t1 <= 0 || t2 <= 0) continue;
                int total = t1 + t2 + 3;
                // 构建详细路径（含每段耗时）
                QString path1Detail = "";
                for (int i = 0; i < p1.size(); ++i) {
                    path1Detail += p1[i];
                    if (i < p1.size() - 1) {
                        int seg = calculateTravelTime(r1, p1[i], p1[i+1]);
                        path1Detail += QString(" <span style=\"color:#666;\">↓ %1分钟</span> → ").arg(seg);
                    }
                }
                QString path2Detail = "";
                for (int i = 0; i < p2.size(); ++i) {
                    path2Detail += p2[i];
                    if (i < p2.size() - 1) {
                        int seg = calculateTravelTime(r2, p2[i], p2[i+1]);
                        path2Detail += QString(" <span style=\"color:#666;\">↓ %1分钟</span> → ").arg(seg);
                    }
                }
                QString totalStr = QString("（约 %1 分钟，含换乘）").arg(total);
                QString busInfo = QString(
                                      "<div style=\"font-size: 13px; color: #A0A0A0; margin-top: 6px;\">"
                                      "第1段（%1）：首班 %2 末班 %3<br>"
                                      "第2段（%4）：首班 %5 末班 %6"
                                      "</div>")
                                      .arg(r1.name, r1.firstBus.toString("HH:mm"), r1.lastBus.toString("HH:mm"),
                                           r2.name, r2.firstBus.toString("HH:mm"), r2.lastBus.toString("HH:mm"));
                QString html = QString(
                                   "<div style=\"margin: 16px 0; padding: 16px; border-radius: 8px; background: #1E1E1E; border-bottom: 1px solid #333333; box-shadow: 0 2px 4px rgba(0,0,0,0.3);\">"
                                   "<div style=\"display: inline-block; width: 4px; height: 100%; background: #3B82F6; margin-left: -12px; margin-right: 12px; vertical-align: top;\"></div>"
                                   "<div style=\"display: inline-block; width: calc(100% - 16px);\">"
                                   "<h3 style=\"margin: 0 0 8px 0; color: #4F46E5;\">🔄 一次换乘 ------------------------------------------------------------------------------------------</h3>"
                                   "<p>从 <b>%1</b> 到 <b>%2</b>%3</p>"
                                   "<div style=\"margin: 8px 0; padding: 6px 10px; background: #252526; border-radius: 4px;\">"
                                   "<b>第1段</b>：乘坐 <span style=\"color: #4F46E5; font-weight: bold;\">%4</span>（%5 站）<br>%6"
                                   "</div>"
                                   "<div style=\"text-align: center; margin: 4px 0; color: #3B82F6;\">↓ 在 <b>%7</b> 换乘（步行约3分钟）↓</div>"
                                   "<div style=\"margin: 8px 0; padding: 6px 10px; background: #252526; border-radius: 4px;\">"
                                   "<b>第2段</b>：乘坐 <span style=\"color: #4F46E5; font-weight: bold;\">%8</span>（%9 站）<br>%10"
                                   "</div>"
                                   "%11"
                                   "</div></div>")
                                   .arg(start, end, totalStr,
                                        r1.name, QString::number(p1.size()), path1Detail,
                                        mid,
                                        r2.name, QString::number(p2.size()), path2Detail,
                                        busInfo);
                allPlans.append({total, html});
            }
        }
    }

    // === 3. 两次换乘 ===
    for (const auto& r1 : std::as_const(routes)) {
        if (!r1.stops.contains(start)) continue;
        for (const auto& r2 : std::as_const(routes)) {
            if (r1.id == r2.id) continue;
            for (const QString& mid1 : r1.stops) {
                if (!r2.stops.contains(mid1)) continue;
                for (const auto& r3 : std::as_const(routes)) {
                    if (r3.id == r1.id || r3.id == r2.id || !r3.stops.contains(end)) continue;
                    for (const QString& mid2 : r2.stops) {
                        if (!r3.stops.contains(mid2)) continue;
                        auto p1 = getStopPath(r1, start, mid1);
                        auto p2 = getStopPath(r2, mid1, mid2);
                        auto p3 = getStopPath(r3, mid2, end);
                        if (p1.isEmpty() || p2.isEmpty() || p3.isEmpty()) continue;
                        int t1 = calculateTravelTime(r1, start, mid1);
                        int t2 = calculateTravelTime(r2, mid1, mid2);
                        int t3 = calculateTravelTime(r3, mid2, end);
                        if (t1 <= 0 || t2 <= 0 || t3 <= 0) continue;
                        int total = t1 + t2 + t3 + 6;
                        // 构建详细路径
                        auto buildDetail = [&](const Route& r, const QVector<QString>& p) -> QString {
                            QString s;
                            for (int i = 0; i < p.size(); ++i) {
                                s += p[i];
                                if (i < p.size() - 1) {
                                    int seg = calculateTravelTime(r, p[i], p[i+1]);
                                    s += QString(" <span style=\"color:#666;\">↓ %1分钟</span> → ").arg(seg);
                                }
                            }
                            return s;
                        };
                        QString d1 = buildDetail(r1, p1);
                        QString d2 = buildDetail(r2, p2);
                        QString d3 = buildDetail(r3, p3);
                        QString totalStr = QString("（约 %1 分钟，含换乘）").arg(total);
                        QString busInfo = QString(
                                              "<div style=\"font-size: 13px; color: #A0A0A0; margin-top: 6px;\">"
                                              "第1段（%1）：首班 %2 末班 %3<br>"
                                              "第2段（%4）：首班 %5 末班 %6<br>"
                                              "第3段（%7）：首班 %8 末班 %9"
                                              "</div>")
                                              .arg(r1.name, r1.firstBus.toString("HH:mm"), r1.lastBus.toString("HH:mm"),
                                                   r2.name, r2.firstBus.toString("HH:mm"), r2.lastBus.toString("HH:mm"),
                                                   r3.name, r3.firstBus.toString("HH:mm"), r3.lastBus.toString("HH:mm"));
                        QString html = QString(
                                           "<div style=\"margin: 16px 0; padding: 16px; border-radius: 8px; background: #1E1E1E; border-bottom: 1px solid #333333; box-shadow: 0 2px 4px rgba(0,0,0,0.3);\">"
                                           "<div style=\"display: inline-block; width: 4px; height: 100%; background: #8B5CF6; margin-left: -12px; margin-right: 12px; vertical-align: top;\"></div>"
                                           "<div style=\"display: inline-block; width: calc(100% - 16px);\">"
                                           "<h3 style=\"margin: 0 0 8px 0; color: #E56C4F;\">🔄 两次换乘 ------------------------------------------------------------------------------------------</h3>"
                                           "<p>从 <b>%1</b> 到 <b>%2</b>%3</p>"
                                           "<div style=\"margin: 6px 0; padding: 6px 10px; background: #252526; border-radius: 4px;\">"
                                           "<b>第1段</b>：乘坐 <span style=\"color: #E56C4F; font-weight: bold;\">%4</span>（%5 站）<br>%6"
                                           "</div>"
                                           "<div style=\"text-align: center; margin: 4px 0; color: #3B82F6;\">↓ 在 <b>%7</b> 换乘（步行约3分钟）↓</div>"
                                           "<div style=\"margin: 6px 0; padding: 6px 10px; background: #252526; border-radius: 4px;\">"
                                           "<b>第2段</b>：乘坐 <span style=\"color: #E56C4F; font-weight: bold;\">%8</span>（%9 站）<br>%10"
                                           "</div>"
                                           "<div style=\"text-align: center; margin: 4px 0; color: #3B82F6;\">↓ 在 <b>%11</b> 换乘（步行约3分钟）↓</div>"
                                           "<div style=\"margin: 6px 0; padding: 6px 10px; background: #252526; border-radius: 4px;\">"
                                           "<b>第3段</b>：乘坐 <span style=\"color: #E56C4F; font-weight: bold;\">%12</span>（%13 站）<br>%14"
                                           "</div>"
                                           "%15"
                                           "</div></div>")
                                           .arg(start, end, totalStr,
                                                r1.name, QString::number(p1.size()), d1, mid1,
                                                r2.name, QString::number(p2.size()), d2, mid2,
                                                r3.name, QString::number(p3.size()), d3,
                                                busInfo);
                        allPlans.append({total, html});
                    }
                }
            }
        }
    }

    // === 4. 按总时间排序并显示 ===
    std::sort(allPlans.begin(), allPlans.end(), [](const Plan& a, const Plan& b) {
        return a.totalTime < b.totalTime;
    });

    QString htmlOutput;
    if (allPlans.isEmpty()) {
        htmlOutput = "<p style=\"color: #EF4444; font-style: italic;\">⚠️ 未找到从 <b>" + start + "</b> 到 <b>" + end + "</b> 的可行路线（最多两次换乘）。</p>";
    } else {
        int shown = 0;
        for (const auto& p : std::as_const(allPlans)) {
            if (shown >= 5) break;
            htmlOutput += p.html;
            shown++;
        }
    }
    resultDisplay->setHtml(htmlOutput);
}

void MainWindow::searchRouteById() {
    QString id = routeIdEdit->text().trimmed();
    for (const auto& r : std::as_const(routes)) {
        if (r.id == id) {
            // 计算全程时间
            int totalTime = 0;
            for (int t : r.travelTimes) totalTime += t;
            QString timeInfo = QString("（全程约 <b>%1 分钟</b>）").arg(totalTime);
            QString busInfo = QString("🕒 首班 <b>%1</b> &nbsp; 末班 <b>%2</b>")
                                  .arg(r.firstBus.toString("HH:mm"), r.lastBus.toString("HH:mm"));

            QString stationsHtml = "<ul style=\"padding-left: 20px; margin: 8px 0;\">";
            for (int i = 0; i < r.stops.size(); ++i) {
                stationsHtml += "<li>" + r.stops[i];
                if (i < r.travelTimes.size()) {
                    stationsHtml += QString(" <span style=\"color:#666;\">↓ %1分钟</span>").arg(r.travelTimes[i]);
                }
                stationsHtml += "</li>";
            }
            stationsHtml += "</ul>";

            QString html = QString(
                               "<h3>🚍 %1 (%2)</h3>"
                               "<p style=\"color: #A0A0A0;\">全程共 %3 站 %4</p>"
                               "<p style=\"color: #A0A0A0; margin-top: 4px;\">%5</p>"
                               "%6")
                               .arg(r.name, r.id)
                               .arg(r.stops.size())
                               .arg(timeInfo)
                               .arg(busInfo)
                               .arg(stationsHtml);
            routeDetailDisplay->setHtml(html);
            return;
        }
    }
    routeDetailDisplay->setHtml("<p style=\"color: #EF4444;\">❌ 未找到线路：" + id + "</p>");
}

void MainWindow::editRoute(const QString& id)
{
    editingRouteId = id;

    // 找到要编辑的路线
    Route* target = nullptr;
    for (auto& r : routes)
        if (r.id == id) { target = &r; break; }
    if (!target) return;

    openRouteEditDialog(target);
}

void MainWindow::deleteRoute(const QString& id) {
    if (QMessageBox::question(this, "确认", "确定删除？") == QMessageBox::Yes) {
        routes.erase(std::remove_if(routes.begin(), routes.end(),
                                    [&](const Route& r) { return r.id == id; }), routes.end());
        refreshAllStops();
        saveRoutesToFile();
        QMessageBox::information(this, "成功", "路线已删除");
    }
}

void MainWindow::onRouteItemDoubleClicked(QListWidgetItem *item)
{
    QString id = item->data(Qt::UserRole).toString();
    editRoute(id);
}

void MainWindow::onRouteContextMenu(const QPoint &pos)
{
    auto item = routeList->itemAt(pos);
    if (!item) return;

    QString id = item->data(Qt::UserRole).toString();
    QString name = item->text();

    QMenu menu(this);
    auto editAction = menu.addAction("✏️ 编辑路线");
    auto deleteAction = menu.addAction("🗑️ 删除路线");

    auto selected = menu.exec(routeList->mapToGlobal(pos));
    if (selected == editAction) {
        editRoute(id);
    } else if (selected == deleteAction) {
        deleteRoute(id);
        switchToManage();
    }
}

void MainWindow::onRouteIdListItemClicked(QListWidgetItem* item)
{
    if (!item) return;
    QString routeId = item->data(Qt::UserRole).toString();
    if (routeId.isEmpty()) {
        routeId = item->text().split(' ').first(); // 如果没存 UserRole，就从文本提取
    }
    routeIdEdit->setText(routeId);
    searchRouteById(); // 自动查询
}

void MainWindow::exportRoutes()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "导出路线", QDir::home().filePath("bus_routes.json"),
        "JSON 文件 (*.json)");
    if (fileName.isEmpty()) return;

    QJsonArray root;
    for (const auto &r : std::as_const(routes)) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["name"] = r.name;

        // 导出站点
        QJsonArray stops;
        for (const auto &s : r.stops) {
            stops.append(s);
        }
        obj["stops"] = stops;

        // ✅ 导出站点间时间（travelTimes）
        QJsonArray travelTimes;
        for (int t : r.travelTimes) {
            travelTimes.append(t);
        }
        obj["travelTimes"] = travelTimes;

        // ✅ 导出首末班车时间
        obj["firstBus"] = r.firstBus.toString("HH:mm");
        obj["lastBus"] = r.lastBus.toString("HH:mm");

        root.append(obj);
    }

    QJsonDocument doc(root);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        QMessageBox::information(this, "成功", "路线已导出至\n" + fileName);
    } else {
        QMessageBox::warning(this, "错误", "无法写入文件");
    }
}

void MainWindow::importRoutes()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "导入路线", QDir::homePath(),
        "JSON 文件 (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) {
        QMessageBox::warning(this, "错误", "JSON 格式不正确（顶层应为数组）");
        return;
    }

    QJsonArray root = doc.array();
    QVector<Route> imported;
    for (const auto v : std::as_const(root)) {
        QJsonObject obj = v.toObject();
        Route r;
        r.id   = obj["id"].toString();
        r.name = obj["name"].toString();
        for (const auto& s : obj["stops"].toArray()) {
            r.stops.append(s.toString());
        }

        // ✅ 导入新字段
        if (obj.contains("travelTimes")) {
            for (const auto& t : obj["travelTimes"].toArray()) {
                r.travelTimes.append(t.toInt());
            }
        }
        while (r.travelTimes.size() < r.stops.size() - 1) {
            r.travelTimes.append(3);
        }

        if (obj.contains("firstBus")) {
            r.firstBus = QTime::fromString(obj["firstBus"].toString(), "HH:mm");
        }
        if (obj.contains("lastBus")) {
            r.lastBus = QTime::fromString(obj["lastBus"].toString(), "HH:mm");
        }
        if (!r.firstBus.isValid()) r.firstBus = QTime(6, 0);
        if (!r.lastBus.isValid())  r.lastBus = QTime(22, 30);

        imported.append(r);
    }

    if (imported.isEmpty()) return;

    auto ans = QMessageBox::question(
        this, "导入方式",
        "是否清空现有路线后再导入？\n选“否”则追加",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if (ans == QMessageBox::Cancel) return;
    if (ans == QMessageBox::Yes) routes.clear();

    routes.append(imported);
    refreshAllStops();
    switchToManage();
    saveRoutesToFile();
    QMessageBox::information(this, "成功", QString("已导入 %1 条路线").arg(imported.size()));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == startSuggest) {
        if (event->type() == QEvent::Enter) {
            // 鼠标进入：弹出下拉
            startSuggest->showPopup();
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开：聚焦回输入框
            startEdit->setFocus();
        }
        return false;
    }
    if (obj == endSuggest) {
        if (event->type() == QEvent::Enter) {
            endSuggest->showPopup();
        } else if (event->type() == QEvent::Leave) {
            endEdit->setFocus();
        }
        return false;
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::refreshRouteIdList()
{
    routeIdList->clear();
    for (const auto& r : std::as_const(routes)) {
        auto item = new QListWidgetItem(QString("%1 - %2").arg(r.id, r.name));
        item->setData(Qt::UserRole, r.id);
        routeIdList->addItem(item);
    }
}

void MainWindow::openRouteEditDialog(const Route* routeToEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(routeToEdit ? "编辑路线" : "添加新路线");
    dialog.resize(520, 620);

    auto mainLayout = new QVBoxLayout(&dialog);

    // 基本信息
    auto formLayout = new QFormLayout;
    auto idEdit = new QLineEdit;
    auto nameEdit = new QLineEdit;
    if (routeToEdit) {
        idEdit->setText(routeToEdit->id);
        nameEdit->setText(routeToEdit->name);
        idEdit->setEnabled(false); // 编辑时不允许改 id
    }
    formLayout->addRow("路线编号:", idEdit);
    formLayout->addRow("路线名称:", nameEdit);
    mainLayout->addLayout(formLayout);

    // === 站点与时间动态区域 ===
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    auto scrollContent = new QWidget;
    auto stopsLayout = new QVBoxLayout(scrollContent);
    stopsLayout->setSpacing(6);
    scrollContent->setLayout(stopsLayout);
    scroll->setWidget(scrollContent);
    mainLayout->addWidget(new QLabel("站点与行驶时间:"));
    mainLayout->addWidget(scroll);

    QList<QLineEdit*> stopEdits;
    QList<QLineEdit*> timeEdits;

    // 初始化站点和时间（编辑模式）
    QVector<QString> initialStops;
    QList<int> initialTimes;
    if (routeToEdit) {
        initialStops = routeToEdit->stops;
        initialTimes = routeToEdit->travelTimes;
        // 兜底：如果旧数据没有 times
        while (initialTimes.size() < initialStops.size() - 1)
            initialTimes.append(3);
    } else {
        initialStops = {"起点站"};
        initialTimes = {};
    }

    // 添加所有站点
    for (int i = 0; i < initialStops.size(); ++i) {
        auto stopEdit = new QLineEdit(initialStops[i]);
        stopEdit->setPlaceholderText("输入站点名称");
        stopEdits.append(stopEdit);
        stopsLayout->addWidget(stopEdit);

        // 添加时间（除了最后一个站）
        if (i < initialTimes.size()) {
            auto timeEdit = new QLineEdit(QString::number(initialTimes[i]));
            timeEdit->setFixedWidth(60);
            timeEdit->setValidator(new QIntValidator(1, 60));
            timeEdit->setPlaceholderText("分钟");
            timeEdits.append(timeEdit);

            auto timeHBox = new QHBoxLayout;
            timeHBox->addStretch();
            timeHBox->addWidget(new QLabel("↓ 行驶时间:"));
            timeHBox->addWidget(timeEdit);
            timeHBox->addWidget(new QLabel("分钟"));
            timeHBox->addStretch();
            stopsLayout->addLayout(timeHBox);
        }
    }
    stopsLayout->addStretch();

    // 添加站点按钮
    auto addStopBtn = new QPushButton("✚ 添加下一站点");
    mainLayout->addWidget(addStopBtn);

    QObject::connect(addStopBtn, &QPushButton::clicked, [&]() {
        if (stopsLayout->count() > 0) {
            QLayoutItem* last = stopsLayout->takeAt(stopsLayout->count() - 1);
            delete last;
        }

        auto timeEdit = new QLineEdit("3");
        timeEdit->setFixedWidth(60);
        timeEdit->setValidator(new QIntValidator(1, 60));
        timeEdit->setPlaceholderText("分钟");
        timeEdits.append(timeEdit);

        auto timeHBox = new QHBoxLayout;
        timeHBox->addStretch();
        timeHBox->addWidget(new QLabel("↓ 行驶时间:"));
        timeHBox->addWidget(timeEdit);
        timeHBox->addWidget(new QLabel("分钟"));
        timeHBox->addStretch();
        stopsLayout->addLayout(timeHBox);

        auto stopEdit = new QLineEdit;
        stopEdit->setPlaceholderText("输入站点名称");
        stopEdits.append(stopEdit);
        stopsLayout->addWidget(stopEdit);

        stopsLayout->addStretch();
    });

    // 首末班车
    auto timeLayout = new QHBoxLayout;
    auto firstBusEdit = new QTimeEdit;
    auto lastBusEdit = new QTimeEdit;
    if (routeToEdit) {
        firstBusEdit->setTime(routeToEdit->firstBus);
        lastBusEdit->setTime(routeToEdit->lastBus);
    } else {
        firstBusEdit->setTime(QTime(6, 0));
        lastBusEdit->setTime(QTime(22, 30));
    }
    timeLayout->addWidget(new QLabel("首班车:"));
    timeLayout->addWidget(firstBusEdit);
    timeLayout->addSpacing(20);
    timeLayout->addWidget(new QLabel("末班车:"));
    timeLayout->addWidget(lastBusEdit);
    mainLayout->addLayout(timeLayout);

    // 按钮
    auto btnBox = new QHBoxLayout;
    auto okBtn = new QPushButton(routeToEdit ? "更新" : "确定");
    auto cancelBtn = new QPushButton("取消");
    btnBox->addStretch();
    btnBox->addWidget(okBtn);
    btnBox->addWidget(cancelBtn);
    mainLayout->addLayout(btnBox);
    for (auto* btn : {okBtn, cancelBtn, addStopBtn}) {
        btn->setStyleSheet(
            "QPushButton {"
            "   padding: 6px 12px;"
            "   border-radius: 4px;"
            "   background: #252526;"        // VS-Code 深灰
            "   color: #CCCCCC;"
            "   border: 1px solid #454545;"
            "}"
            "QPushButton:hover {"
            "   background: #2E2E30;"
            "   border: 1px solid #007ACC;"  // 主题色边框
            "}"
            "QPushButton:pressed {"
            "   background: #1E1E1E;"
            "}");
    }

    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(okBtn, &QPushButton::clicked, [&]() {
        QString id = idEdit->text().trimmed();
        QString name = nameEdit->text().trimmed();
        if (id.isEmpty() || name.isEmpty()) {
            QMessageBox::warning(&dialog, "输入错误", "请填写路线编号和名称");
            return;
        }

        QVector<QString> stops;
        for (auto* edit : stopEdits) {
            QString s = edit->text().trimmed();
            if (!s.isEmpty()) stops.append(s);
        }
        if (stops.size() < 2) {
            QMessageBox::warning(&dialog, "输入错误", "至少需要两个站点");
            return;
        }

        QList<int> times;
        for (auto* edit : timeEdits) {
            int t = edit->text().toInt();
            if (t <= 0) t = 3;
            times.append(t);
        }
        while (times.size() < stops.size() - 1) times.append(3);
        if (times.size() > stops.size() - 1) times.resize(stops.size() - 1);

        Route r;
        r.id = id;
        r.name = name;
        r.stops = stops;
        r.travelTimes = times;
        r.firstBus = firstBusEdit->time();
        r.lastBus = lastBusEdit->time();

        if (routeToEdit) {
            // 更新
            for (auto& existing : routes) {
                if (existing.id == id) {
                    existing = r;
                    break;
                }
            }
        } else {
            // 新增
            routes.append(r);
        }

        dialog.accept();
        if (routeIdList) refreshRouteIdList();
        saveRoutesToFile();
        switchToManage();
    });

    dialog.exec();
}

int MainWindow::calculateTravelTime(const Route& route, const QString& from, const QString& to)
{
    int i1 = route.stops.indexOf(from);
    int i2 = route.stops.indexOf(to);
    if (i1 == -1 || i2 == -1) return -1;
    if (route.travelTimes.size() != route.stops.size() - 1) return -1;

    int total = 0;
    if (i1 < i2) {
        for (int i = i1; i < i2; ++i) {
            total += route.travelTimes[i];
        }
    } else {
        for (int i = i1 - 1; i >= i2; --i) {
            total += route.travelTimes[i];
        }
    }
    return total;
}
