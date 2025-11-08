#ifndef UTILS_H
#define UTILS_H

#include "mainwindow.h"
#include <QVector>

struct TransferPlan {
    Route route1;
    Route route2;
    QString transferStop;
    QVector<QString> path1;
    QVector<QString> path2;
};

QVector<QString> getStopPath(const Route& route, const QString& start, const QString& end);
QVector<TransferPlan> calculateTransfers(const QVector<Route>& routes,
                                         const QString& startStop,
                                         const QString& endStop);

#endif // UTILS_H
