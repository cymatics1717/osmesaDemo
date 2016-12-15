#include <iostream>
#include "arwidget.hh"

using std::cout;
using std::endl;
int main(/*int argc, char *argv[]*/)
{
    cv::Mat src  = cv::imread("/home/wayne/model/o.png", CV_LOAD_IMAGE_UNCHANGED);
    int w = src.cols;
    int h = src.rows;
    ARWidget widget;
    widget.initGL(w/3,h/3);
#define PATH "/opt/meta/AIlib/data/model/"

    widget.loadModel(PATH"arrow1.obj");

    for (int i = 0; i < 36; ++i) {
        cv::Mat tmp = src.clone();
        widget.render(tmp,cv::Point(w/2-widget.width()/2,h-widget.height()),i*10,0);

        char filename[32];
        sprintf(filename,"test%d.png",i);
        cv::imwrite(filename, tmp);

    }

    return 0;
}
