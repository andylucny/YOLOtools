#include "opencv2/opencv.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <time.h>

using namespace std;
using namespace cv;

int main() {
    cout << "wait for image from camera, then:" << endl;
    cout << "s     .. save image" << endl;
    cout << "v     .. start saving of video" << endl;
    cout << "z     .. stop saving of video" << endl;
    cout << "<Esc> .. exit" << endl;
    // camera
    VideoCapture camera(0);
    if (!camera.isOpened()) {
        cout << "Camera not connected" << endl;
        return -1;
    }
    VideoWriter out;
    bool recording = false;
    for(;;) {
        Mat image;
        camera >> image;
        if (image.empty()) break;
        if (recording) out << image;
        imshow("camera", image);
        int key = waitKey(1);
        if (key == 27) break;
        else if (key == 's') {
            imwrite(to_string(time(NULL))+".png",image);
        }
        else if (key == 'v') {
            out.open(to_string(time(NULL))+".avi",VideoWriter::fourcc('M','J','P','G'),25.0,Size(image.cols,image.rows));
            recording = true;
        }
        else if (key == 'z') {
            out.release();
            recording = false;
        }
    }   
}
