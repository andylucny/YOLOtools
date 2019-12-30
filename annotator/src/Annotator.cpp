#include "listdir.h"

#include "opencv2/opencv.hpp"
#include "screen.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;

string dir = "data";

struct Annotation {
    bool loaded;
    int classId;
    double centerX;
    double centerY;
    double width;
    double height;
};

Annotation load_annotation(string filename) 
{
    Annotation annotation;
    annotation.loaded = false;
    annotation.classId = 0;
    ifstream in(dir+"/"+filename);
    if (!in) return annotation;
    try {
        in >> annotation.classId;
        in >> annotation.centerX;
        in >> annotation.centerY;
        in >> annotation.width;
        in >> annotation.height;
        annotation.loaded = true;
    }
    catch (...) {
    }
    return annotation;
}

void save_annotation(string filename, Annotation annotation) 
{
    if (!annotation.loaded) return;
    ofstream out(dir+"/"+filename);
    out << annotation.classId;
    out << " " << annotation.centerX;
    out << " " << annotation.centerY;
    out << " " << annotation.width;
    out << " " << annotation.height << "." << endl;
}

void erase_annotation(string filename) 
{
    std::remove((dir+"/"+filename).c_str());
}

Mat img, disp;
Rect selection;
int label;

void set_selection(Annotation &annotation)
{
    if (!annotation.loaded) selection = Rect();
    else {
        Size dims = disp.size();
        Point center(static_cast<int>(dims.width*annotation.centerX),static_cast<int>(dims.height*annotation.centerY));
        Size sz(static_cast<int>(dims.width*annotation.width),static_cast<int>(dims.height*annotation.height));
        selection = Rect(center.x-sz.width/2,center.y-sz.height/2,sz.width,sz.height);
    }
}

bool get_selection(Annotation &annotation)
{
    if (selection.area() == 0) return false;
    annotation.loaded = true;
    Size dims = disp.size();
    annotation.width = static_cast<double>(selection.width) / dims.width;
    annotation.height = static_cast<double>(selection.height) / dims.height;
    annotation.centerX = (selection.x + selection.width / 2.0) / dims.width;
    annotation.centerY = (selection.y + selection.height / 2.0) / dims.height;
    return true;
}

void draw_selection()
{
    rectangle(disp,selection,Scalar(0,0,255),1);
    putText(disp,to_string(label),Point(20,50),0,1.6,Scalar(0,0,255),2);
}

enum {
    NOPE,
    TL,
    BR
} selecting;

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    if  (event == EVENT_LBUTTONDOWN) {
        //cout << "Left button of the mouse is down - position (" << x << ", " << y << ")" << endl;
        if (selection.area() == 0) {
            selection.x = x;
            selection.y = y;
            selection.width = selection.height = 1;
            selecting = BR;
        }
        else {
            double dist_tl = pow(pow(selection.x-x,2.0)+pow(selection.y-y,2.0),0.5);
            double dist_br = pow(pow(selection.x+selection.width-x,2.0)+pow(selection.y+selection.height-y,2.0),0.5);
            if (dist_tl < dist_br) {
                if (dist_tl < 10) selecting = TL;
            }
            else {
                if (dist_br < 10) selecting = BR;
            }
        }
        //cout << "selecting up " << selecting << endl;
    }
    else if (event == EVENT_LBUTTONUP) {
        //cout << "Left button of the mouse is up - position (" << x << ", " << y << ")" << endl;
        selecting = NOPE;
        if (selection.area() < 25) selection = Rect();
        //cout << "selecting dn " << selecting << endl;
    }
    else if ( event == EVENT_MOUSEMOVE ) {
        //cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;
        if (selecting == TL) {
            //cout << "tl x="  << x << " y=" << y << endl;
            if (x < selection.x + selection.width - 1 && y < selection.y + selection.height - 1) {
                selection.width = selection.x + selection.width - x;
                selection.height = selection.y + selection.height - y;
                selection.x = x;
                selection.y = y;
            }
        }
        else if (selecting == BR) {
            //cout << "br x="  << x << " y=" << y << endl;
            if (x > selection.x && y > selection.y) {
                selection.width = x - selection.x + 1;
                selection.height = y - selection.y + 1;
            }
        }
    }
    //show the image
    disp = img.clone();
    draw_selection();
    imshow("Annotation",disp);
}

int main(int argc, char** argv)
{
    std::vector<string> fileNames;
    listfiles(dir,fileNames,false);
    if (fileNames.size() <= 0) return 0;
    printdir(fileNames);

    Size screen;
    getScreenResolution(screen.width,screen.height);
    cout << "screen resolution: " << screen.width << "x" << screen.height << endl;
    cout << endl;
    cout << "use + - to increase/decrease class id" << endl;
    cout << "use (alt-)PgUp (alt-)PgDn Enter to navigate through samples" << endl;
    cout << "use mouse to define detecting rectangle, drag the top left or bottom right corner" << endl;
    cout << "(when OpenCV is compiled with Qt, alt keys as PgUp have to be pressed with Alt key)" << endl;
    cout << endl;

    namedWindow("Annotation",1);
    //set the callback function for any mouse event
    setMouseCallback("Annotation", CallBackFunc, NULL);

    for (int i=0;;) {
        cout << fileNames[i] << endl;
        img = imread(dir+"/"+fileNames[i],1);
        if (img.empty()) return 0;
        cout << "image resolution: " << img.cols << "x" << img.rows << endl;
        double ratioX = (0.9*screen.width) / img.cols;
        double ratioY = (0.9*screen.height) / img.rows;
        double ratio = min(ratioX,ratioY);
        cout << "ratio " << ratio << endl;
        if (ratio < 1.0) resize(img,img,Size(),ratio,ratio);
        string txtName = getAnnotationFilename(fileNames[i]);
        Annotation annotation = load_annotation(txtName);
        disp = img.clone();
        set_selection(annotation);
        label = annotation.classId;
        selecting = NOPE;

        int key = 0;
        for (;;) {
            //show the image
            disp = img.clone();
            draw_selection();
            imshow("Annotation",disp);

            // Wait until user press some key
            key = waitKeyEx(0);
            // cout << key << endl;
            if (key == 13 || key == 2162688 || key == 65435) i = (i >= static_cast<int>(fileNames.size())-1) ? 0 : i+1;
            else if (key == 2228224 || key == 65434) i = (i > 0) ? i-1 : i = static_cast<int>(fileNames.size())-1;
            if (key == 13 || key == 2162688 || key == 2228224 || key == 65435 || key == 65434 || key == 27) break;
            if (key == 43) annotation.classId = ++label;
            else if (key == 45) annotation.classId = (label > 0) ? --label : label;
        }
        if (get_selection(annotation)) save_annotation(txtName,annotation);
        else erase_annotation(txtName);

        if (key == 27) break;
    }

    return 0;
}
