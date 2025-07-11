#ifndef COMMON_STRUCT_H
#define COMMON_STRUCT_H

#include <QString>

#define IMG_PAGE   0
#define CHART_PAGE 1
#define LOGIN_PAGE 2

struct std_farminfo {
    int num;
    QString name;
    QString address;
    int area_cnt;
};

struct std_image{
    QByteArray before;
    QByteArray after;
    int total_cnt;
    int red_cnt;
    int yellow_cnt;
    int green_cnt;
};

struct std_tomato{
    int farm_total_cnt;
    int total_red_cnt;
    int total_yellow_cnt;
    int total_green_cnt;
    QString harv_date;
    QString date;
    int growthrate;
    };

#endif // COMMON_STRUCT_H
