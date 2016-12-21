#ifndef ARWIDGET_HH
#define ARWIDGET_HH

#include <opencv2/opencv.hpp>
#include <cv.h>
#include <string>

struct ARWidgetPrivate;
class ARWidget
{
public:
    ARWidget();
    ~ARWidget();

    int initGL(int width,int height);
    int loadModel(const std::string &filename);
    int render(cv::Mat &mat,const cv::Point& pos,double yaw,double pitch);

    int width();
    int height();
    void setValue(int x, int y, int z,int xx, int yy, int zz);
    void overlayImage(cv::Mat& src, const cv::Mat &overlay, const cv::Point& location);
private:
    ARWidgetPrivate *p;
};

#endif // ARWIDGET_HH
