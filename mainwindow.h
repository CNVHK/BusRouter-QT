#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QString>
#include <QTime>
#include <QQueue>
#include <QSet>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QWidget;
class QLabel;
class QTextEdit;
class QListWidget;
class QComboBox;
class QListWidgetItem;
QT_END_NAMESPACE

struct Route {
    QString id;
    QString name;
    QVector<QString> stops;
    QList<int> travelTimes;   // 相邻站点间时间（分钟），长度 = stops.size() - 1
    QTime firstBus;           // 首班车
    QTime lastBus;            // 末班车

    // 构造函数
    Route(QString i = "", QString n = "", QVector<QString> s = {})
        : id(i), name(n), stops(s)
    {
        // 默认：每段 3 分钟（仅当 stops 有效时）
        for (int i = 0; i < stops.size() - 1; ++i) {
            travelTimes.append(3);
        }
        // 默认首末班
        firstBus = QTime(6, 0);   // 06:00
        lastBus = QTime(22, 30);  // 22:30
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    static constexpr auto SAVE_FILE = "bus_routes.json";
    void loadRoutesFromFile();
    void saveRoutesToFile();

private slots:
    void loginAsUser();
    void loginAsAdmin();
    void logout();
    void exportRoutes();
    void importRoutes();
    void updateBtnState(bool state);
    void switchToSearch();
    void switchToRoute();
    void switchToManage();
    void searchLines();
    void searchRouteById();
    void editRoute(const QString& id);
    void deleteRoute(const QString& id);
    void updateStartSuggestions(const QString& text);
    void updateEndSuggestions(const QString& text);
    void fillStartFromSuggestion(int idx);
    void fillEndFromSuggestion(int idx);
    void onRouteItemDoubleClicked(QListWidgetItem *item);
    void onRouteContextMenu(const QPoint &pos);
    void onRouteIdListItemClicked(QListWidgetItem* item);
    void refreshRouteIdList();
    void openRouteEditDialog(const Route* route = nullptr);
    int calculateTravelTime(const Route& route, const QString& from, const QString& to);


private:
    void setupUI();
    void loadMockData();
    void refreshAllStops();
    void showCurrentTab();
    QVector<Route> findDirectRoutes(const QString& start, const QString& end);
    QVector<QString> getStopPath(const Route& r, const QString& start, const QString& end);
    QVector<Route> routes;
    QVector<QString> allStops;
    QString currentUserRole;

    // Main layout
    QStackedWidget* stackedWidget;
    QWidget* loginPage;
    QWidget* searchPage;
    QWidget* routePage;
    QWidget* managePage;

    // Login
    QLabel* currentUserLabel;
    QPushButton* backToManage;
    QPushButton* backToSearch;
    QPushButton* backToRoute;
    QPushButton* logoutBtn;

    // Search Page
    QLineEdit* startEdit;
    QLineEdit* endEdit;
    QComboBox* startSuggest;
    QComboBox* endSuggest;
    QPushButton* searchLineBtn;
    QTextEdit* resultDisplay;
    // Route Page
    QLineEdit* routeIdEdit;
    QPushButton* searchRouteBtn;
    QTextEdit* routeDetailDisplay;
    QListWidget* routeIdList;

    // Manage Page
    QListWidget* routeList;
    QPushButton* addRouteBtn;

    // Dialogs
    QWidget* newRouteDialog = nullptr;
    QLineEdit *newIdEdit, *newNameEdit;
    QTextEdit *newStopsEdit;
    QString editingRouteId;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // MAINWINDOW_H
