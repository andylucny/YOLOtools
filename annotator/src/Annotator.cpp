#include "listdir.h"

#include "opencv2/opencv.hpp"
#include "screen.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace cv;

string dir = "data";

struct Annotation {
    int classId;
    double centerX;
    double centerY;
    double width;
    double height;
};

std::vector <Annotation> load_annotation(string filename)
{
    std::vector<Annotation> annotations;
    ifstream in(dir+"/"+filename);
    if (!in) return annotations;
    while (in.peek() != EOF) {
        Annotation annotation;
        annotation.classId = 0;
        try {
            in >> annotation.classId;
            if (in.peek() == EOF) break;
            in >> annotation.centerX;
            if (in.peek() == EOF) break;
            in >> annotation.centerY;
            if (in.peek() == EOF) break;
            in >> annotation.width;
            if (in.peek() == EOF) break;
            in >> annotation.height;
            if (in.peek() == EOF) break;
            char dot;
            in >> dot;
            if (dot != '.') break;
            annotations.push_back(annotation);
        }
        catch (...) {
        }
    }
    //cout << annotations.size() << " annotations loaded" << endl;
    return annotations;
}

void save_annotation(string filename, std::vector<Annotation> annotations) 
{
    if (annotations.size() == 0) return;
    ofstream out(dir+"/"+filename);
    for (auto &annotation : annotations) {
        out << annotation.classId;
        out << " " << annotation.centerX;
        out << " " << annotation.centerY;
        out << " " << annotation.width;
        out << " " << annotation.height << "." << endl;
    }
}

void erase_annotation(string filename) 
{
    std::remove((dir+"/"+filename).c_str());
}

Mat img, disp;
vector<Rect> selections;
int selectionIndex = 0;
vector<int> labels;
int labelIndex = 0;

void set_selection(vector<Annotation> &annotations, vector<int> &labels)
{
    selections.clear();
    for (size_t i = 0; i < annotations.size(); i++) {
        Size dims = disp.size();
        Point center(static_cast<int>(dims.width*annotations[i].centerX),static_cast<int>(dims.height*annotations[i].centerY));
        Size sz(static_cast<int>(dims.width*annotations[i].width),static_cast<int>(dims.height*annotations[i].height));
        selections.push_back(Rect(center.x-sz.width/2,center.y-sz.height/2,sz.width,sz.height));
    }
    labels.clear(); 
    for (auto annotation : annotations) labels.push_back(annotation.classId);
}

bool get_selection(vector<Annotation> &annotations, vector<int> &labels)
{
    annotations.clear();
    if (selections.size() == 0) return false;
    for (size_t i = 0; i < selections.size(); i++) {
        Size dims = disp.size();
        Annotation annotation;
        annotation.width = static_cast<double>(selections[i].width) / dims.width;
        annotation.height = static_cast<double>(selections[i].height) / dims.height;
        annotation.centerX = (selections[i].x + selections[i].width / 2.0) / dims.width;
        annotation.centerY = (selections[i].y + selections[i].height / 2.0) / dims.height;
        annotation.classId = labels[i];
        annotations.push_back(annotation);
    }
    return true;
}

void draw_selection()
{
    for (size_t i = 0; i < selections.size(); i++) {
        if (i == selectionIndex) continue;
        rectangle(disp, selections[i], Scalar(0, 255, 255), 1);
    }
    if (selections.size() > 0) {
        rectangle(disp, selections[selectionIndex], Scalar(0, 0, 255), 1);
        putText(disp, to_string(labels[selectionIndex]), Point(20, 50), 0, 1.6, Scalar(0, 0, 255), 2);
    }
}

enum SELECTING {
    NOPE,
    TL,
    BR
} selecting;

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    if  (event == EVENT_LBUTTONDOWN) {
        //cout << "Left button of the mouse is down - position (" << x << ", " << y << ")" << endl;
        if (selections.size() == 0) {
            Rect selection;
            selection.x = x;
            selection.y = y;
            selection.width = selection.height = 1;
            selections.push_back(selection);
            labels.push_back(0);
            selectionIndex = 0;
            selecting = BR;
        }
        else {
            int best = -1;
            double dist;
            SELECTING bestCorner = NOPE;
            for (size_t i = 0; i < selections.size(); i++) {
                double dist_tl = pow(pow(selections[i].x - x, 2.0) + pow(selections[i].y - y, 2.0), 0.5);
                double dist_br = pow(pow(selections[i].x + selections[i].width - x, 2.0) + pow(selections[i].y + selections[i].height - y, 2.0), 0.5);
                if (dist_tl < 10) {
                    if (best == -1 || dist_tl < dist) {
                        best = (int) i;
                        dist = dist_tl;
                        bestCorner = TL;
                    }
                }
                if (dist_br < 10) {
                    if (best == -1 || dist_br < dist) {
                        best = (int) i;
                        dist = dist_br;
                        bestCorner = BR;
                    }
                }
            }
            if (best != -1) {
                selectionIndex = best;
                selecting = bestCorner;
            }
            else {
                Rect selection;
                selection.x = x;
                selection.y = y;
                selection.width = selection.height = 1;
                selectionIndex = (int) selections.size();
                selections.push_back(selection);
                labels.push_back(0);
                selecting = BR;
            }
        }
        //cout << "selecting up " << selecting << endl;
    }
    else if (event == EVENT_LBUTTONUP) {
        //cout << "Left button of the mouse is up - position (" << x << ", " << y << ")" << endl;
        selecting = NOPE;
        if (selections[selectionIndex].area() < 50) {
            selections.erase(selections.begin() + selectionIndex);
            labels.erase(labels.begin() + selectionIndex);
            if (selectionIndex >= selections.size())
                if (--selectionIndex < 0) 
                    selectionIndex = 0;
        }
        //cout << "selecting dn " << selecting << endl;
    }
    else if ( event == EVENT_MOUSEMOVE ) {
        //cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;
        if (selecting == TL) {
            //cout << "tl x="  << x << " y=" << y << endl;
            if (x < selections[selectionIndex].x + selections[selectionIndex].width - 1 && y < selections[selectionIndex].y + selections[selectionIndex].height - 1) {
                selections[selectionIndex].width = selections[selectionIndex].x + selections[selectionIndex].width - x;
                selections[selectionIndex].height = selections[selectionIndex].y + selections[selectionIndex].height - y;
                selections[selectionIndex].x = x;
                selections[selectionIndex].y = y;
            }
        }
        else if (selecting == BR) {
            //cout << "br x="  << x << " y=" << y << endl;
            if (x > selections[selectionIndex].x && y > selections[selectionIndex].y) {
                selections[selectionIndex].width = x - selections[selectionIndex].x + 1;
                selections[selectionIndex].height = y - selections[selectionIndex].y + 1;
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
        //cout << "image resolution: " << img.cols << "x" << img.rows << endl;
        double ratioX = (0.9*screen.width) / img.cols;
        double ratioY = (0.9*screen.height) / img.rows;
        double ratio = min(ratioX,ratioY);
        //cout << "ratio " << ratio << endl;
        if (ratio < 1.0) resize(img,img,Size(),ratio,ratio);
        string txtName = getAnnotationFilename(fileNames[i]);
        std::vector<Annotation> annotations = load_annotation(txtName);
        disp = img.clone();
        set_selection(annotations,labels);
        selecting = NOPE;

        int key = 0;
        for (;;) {
            //show the image
            disp = img.clone();
            draw_selection();
            imshow("Annotation",disp);

            // Wait until user press some key
            key = waitKeyEx(0);
            //cout << key << endl;
            if (key == 13 || key == 2162688 || key == 65435) {
                i = (i >= static_cast<int>(fileNames.size()) - 1) ? 0 : i + 1;
                selectionIndex = 0;
            }
            else if (key == 2228224 || key == 65434) {
                i = (i > 0) ? i - 1 : i = static_cast<int>(fileNames.size()) - 1;
                selectionIndex = 0;
            }
            if (key == 13 || key == 2162688 || key == 2228224 || key == 65435 || key == 65434 || key == 27) break;
            if (key == 9) {
                if (++selectionIndex == selections.size())
                    selectionIndex = 0;
            }
            else if (key == 43) {
                if (labels.size() > 0) 
                    ++labels[selectionIndex];
            }
            else if (key == 45) {
                if (labels.size() > 0)
                    labels[selectionIndex] = (labels[selectionIndex] > 0) ? --labels[selectionIndex] : 0;
            }
        }
        if (get_selection(annotations,labels)) save_annotation(txtName,annotations);
        else erase_annotation(txtName);

        if (key == 27) break;
    }

    return 0;
}
