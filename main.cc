#include <iostream>
#include "arwidget.hh"

using std::cout;
using std::endl;
#define PATH "/home/wayne/opengl/arrow/"

int main(/*int argc, char *argv[]*/)
{
    cv::Mat src  = cv::imread(PATH"o.png", CV_LOAD_IMAGE_UNCHANGED);
    int w = src.cols;
    int h = src.rows;
    ARWidget widget;
    widget.initGL(w,h);
//#define PATH "/opt/meta/AIlib/data/model/"

    widget.loadModel(PATH"1214-a.obj");

    for (int i = 0; i < 36; ++i) {
        cv::Mat tmp = src.clone();
        widget.render(tmp,cv::Point(w/2-widget.width()/2,h-widget.height()/2),i*10,0);
        cv::putText(tmp,"hello",cv::Point(w/2-widget.width()/2,h-widget.height()/2),cv::FONT_HERSHEY_PLAIN,30 ,  cv::Scalar(200, 200, 0), 2, CV_AA);

        char filename[32];
        sprintf(filename,"test%d.png",i);
        cv::imwrite(filename, tmp);

    }

    return 0;
}
