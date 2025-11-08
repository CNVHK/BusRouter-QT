#include "utils.h"
#include <algorithm>

QVector<QString> getStopPath(const Route& route, const QString& start, const QString& end) {
    int startIndex = route.stops.indexOf(start);
    int endIndex = route.stops.indexOf(end);
    if (startIndex == -1 || endIndex == -1) return {};

    if (startIndex < endIndex) {
        return route.stops.mid(startIndex, endIndex - startIndex + 1);
    } else {
        auto reversed = route.stops.mid(endIndex, startIndex - endIndex + 1);
        std::reverse(reversed.begin(), reversed.end());
        return reversed;
    }
}

QVector<TransferPlan> calculateTransfers(const QVector<Route>& routes,
                                         const QString& startStop,
                                         const QString& endStop) {
    QVector<TransferPlan> results;
    QVector<Route> startRoutes, endRoutes;

    for (const auto& r : routes) {
        if (r.stops.contains(startStop)) startRoutes.append(r);
        if (r.stops.contains(endStop))   endRoutes.append(r);
    }

    for (const auto& r1 : startRoutes) {
        for (const auto& r2 : endRoutes) {
            QVector<QString> common;
            for (const auto& s : r1.stops) {
                if (r2.stops.contains(s)) {
                    common.append(s);
                    break; // 取第一个换乘点
                }
            }
            if (!common.isEmpty()) {
                QString trans = common.first();
                TransferPlan plan;
                plan.route1 = r1;
                plan.route2 = r2;
                plan.transferStop = trans;
                plan.path1 = getStopPath(r1, startStop, trans);
                plan.path2 = getStopPath(r2, trans, endStop);
                results.append(plan);
            }
        }
    }
    return results;
}
