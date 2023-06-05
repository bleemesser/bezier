#include <iostream>
#include <vector>
#include <CImg.h>
#include <cctype>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

using namespace std;

struct Point {
    double x;
    double y;
};

// equality operator for Point
bool operator==(const Point& p1, const Point& p2) {
    return p1.x == p2.x && p1.y == p2.y;
}

struct Curve {
    vector<Point> controlPoints;
    vector<Point> anchorPoints;
    int order;
};

Point lerp(const Point& p1, const Point& p2, double t) {
    Point result;
    result.x = p1.x + t * (p2.x - p1.x);
    result.y = p1.y + t * (p2.y - p1.y);
    return result;
}

Point bezier(Curve c, double t) {
    if (c.order < 1 || c.order != c.controlPoints.size() + 1) {
        cout << "Invalid order" << endl;
        return Point();
    }

    // create tempPoints vector that starts with anchor point 1 and ends with anchor point 2
    // put control points in between
    vector<Point> tempPoints;
    tempPoints.push_back(c.anchorPoints[0]);
    for (int i = 0; i < c.order - 1; i++) {
        tempPoints.push_back(c.controlPoints[i]);
    }
    tempPoints.push_back(c.anchorPoints[1]);

    vector<Point> newPoints;
    for (int k = 1; k <= c.order; k++) {
        for (int i = 0; i < c.order; ++i) {
            newPoints.push_back(lerp(tempPoints[i], tempPoints[i + 1], t));
        }
        tempPoints = newPoints;
        newPoints.clear();
    }

    Point result;
    result.x = tempPoints[0].x;
    result.y = tempPoints[0].y;

    return result;
}

void drawPointToImage(int radius, cimg_library::CImg<unsigned char>& image, const Point& point, const vector<unsigned char>& color, double displayScale, double displayOffsetX, double displayOffsetY, int screenHeight, int screenWidth) {
    int x = (int)(displayScale * point.x + displayOffsetX);
    int y = (int)(screenHeight - displayScale * point.y + displayOffsetY);

    for (int i = -radius; i <= radius; ++i) {
        for (int j = -radius; j <= radius; ++j) {
            if (x + i >= 0 && x + i < screenWidth && y + j >= 0 && y + j < screenHeight) {
                image(x + i, y + j, 0, 0) = color[0];
                image(x + i, y + j, 0, 1) = color[1];
                image(x + i, y + j, 0, 2) = color[2];
            }
        }
    }
}

void calculatePoints(vector<Curve> cs, int num_points, vector<Point>& curvePoints) {
    vector<double> t(num_points);
    double dt = 1.0 / num_points;

    // for each curve
    for (int c = 0; c < cs.size(); c++) {
        // for each point on the curve
        for (int i = 0; i < num_points; ++i) {
            t[i] = i * dt;
            curvePoints[i + c * num_points] = bezier(cs[c], t[i]);
        }
    }
}

int main() {
    int screenWidth = 1920;
    int screenHeight = 1080;

    vector<Point> controlPoints = { {0.0,1.0} };
    vector<Point> anchorPoints = { {0.0, 0.0}, {1.0,1.0} };

    vector<Curve> curves;
    curves.push_back({ controlPoints, anchorPoints, 2 });
    int num_points = 1000;

    int order = 2;

    // Create a CImg object to display the curve
    cimg_library::CImg<unsigned char> image(screenWidth, screenHeight, 1, 3, 0);
    cimg_library::CImgDisplay display(image, "Bezier Curve");

    // Create a vector to store the t values
    // vector<double> t(num_points);
    double dt = 1.0 / num_points;

    // Create a vector to store the color values
    vector<unsigned char> color = { 255, 255, 255 };

    // Create a vector to store the points on the curve
    vector<Point> curvePoints(num_points);

    // Calculate the points on the curve
    calculatePoints(curves, num_points, curvePoints);

    double displayScale = 800.0;
    // center the curve on the screen
    double displayOffsetX = screenWidth / 4.0;
    double displayOffsetY = -screenHeight / 4.0;

    int prevMouseX = -1;
    int prevMouseY = -1;
    int initialMouseX = -1;
    int initialMouseY = -1;

    bool isDraggingControlPoint = false;
    bool isDraggingAnchorPoint = false;
    bool isDraggingScreen = false;
    int anchorPointIndex = -1;
    int controlPointIndex = -1;
    bool showPoints = true;
    // Draw the curve
    while (!display.is_closed()) {
        if (display.is_keyP()) {
            // set control points and anchor points to the last item in the curves vector
            controlPoints = curves[curves.size() - 1].controlPoints;
            anchorPoints = curves[curves.size() - 1].anchorPoints;

            cout << "Control Points: " << endl;
            for (int i = 0; i < controlPoints.size(); ++i) {
                cout << "(" << controlPoints[i].x << ", " << controlPoints[i].y << ")" << endl;
            }
            cout << "Anchor Points: " << endl;
            for (int i = 0; i < anchorPoints.size(); ++i) {
                cout << "(" << anchorPoints[i].x << ", " << anchorPoints[i].y << ")" << endl;
            }

            // add a control point
            curves[curves.size() - 1].controlPoints.push_back({ 0.0, 0.0 });

            // update the order
            curves[curves.size() - 1].order = curves[curves.size() - 1].controlPoints.size() + 1;

            // update the curve
            calculatePoints(curves, num_points, curvePoints);
        }
        // add a new curve, be careful that the key cannot repeat itself by being held down
        if (display.is_keyN()) {
            // check if the last curve has the same control and anchor points as what we want to add
            vector<Point> ctrlToAdd = { {0.0,1.0} };
            vector<Point> anchorToAdd = { {0.0, 0.0}, {1.0,1.0} };

            if (curves.size() > 0) {
                if (curves[curves.size() - 1].controlPoints == ctrlToAdd && curves[curves.size() - 1].anchorPoints == anchorToAdd) {
                    cout << "Cannot add a new curve with the same control and anchor points as the last curve" << endl;
                }
                else {
                    // add a new curve
                    curves.push_back({ ctrlToAdd, anchorToAdd, 2 });
                    // update curvePoints size
                    curvePoints.resize(num_points * curves.size());

                    // update the curve
                    calculatePoints(curves, num_points, curvePoints);
                }
            }
            else {
                // add a new curve
                curves.push_back({ ctrlToAdd, anchorToAdd, 2 });
                // update curvePoints size
                curvePoints.resize(num_points * curves.size());
                // update the curve
                calculatePoints(curves, num_points, curvePoints);
            }
        }

        if (display.is_keyD()) {
            if (!isDraggingScreen) {
                // Store the initial position of the mouse
                initialMouseX = display.mouse_x();
                initialMouseY = display.mouse_y();
                isDraggingScreen = true;
            }
            else {
                // Calculate the offset from the initial position
                displayOffsetX += display.mouse_x() - initialMouseX;
                displayOffsetY += display.mouse_y() - initialMouseY;

                // Update the previous mouse position
                initialMouseX = display.mouse_x();
                initialMouseY = display.mouse_y();
            }
        }
        else {
            isDraggingScreen = false;
            initialMouseX = -1;
            initialMouseY = -1;
        }
        // Draw the curves
        for (int i = 0; i < curvePoints.size(); ++i) {
            drawPointToImage(1, image, curvePoints[i], color, displayScale, displayOffsetX, displayOffsetY, screenHeight, screenWidth);
        }
        // Draw the control points and anchor points for the last curve
        if (showPoints) {
            controlPoints = curves[curves.size() - 1].controlPoints;
            anchorPoints = curves[curves.size() - 1].anchorPoints;
            for (int i = 0; i < controlPoints.size(); ++i) {
                drawPointToImage(5, image, controlPoints[i], { 255, 0, 0 }, displayScale, displayOffsetX, displayOffsetY, screenHeight, screenWidth);
            }
                for (int i = 0; i < anchorPoints.size(); ++i) {
                    drawPointToImage(5, image, anchorPoints[i], { 0, 255, 0 }, displayScale, displayOffsetX, displayOffsetY, screenHeight, screenWidth);
            }
        }
        // if h is pressed, hide the control points and anchor points
        if (display.is_keyH()) {
            showPoints = !showPoints;
            image.fill(0);
        }

        display.display(image);
        display.wait();

        if (display.is_keyQ()) {
            return 0;
        }

        if (display.is_keyARROWUP()) {
            displayScale *= 1.1;
            num_points *= 1.1;
            curvePoints.resize(num_points * curves.size());
            calculatePoints(curves, num_points, curvePoints);
        }
        if (display.is_keyARROWDOWN()) {
            displayScale /= 1.1;
            num_points /= 1.1;
            curvePoints.resize(num_points * curves.size());
            calculatePoints(curves, num_points, curvePoints);
        }
        // Drag if space is held
        if (display.is_keySPACE() && showPoints) {
            


            // If the mouse button is pressed
            if (prevMouseX == -1 && prevMouseY == -1) {
                // set control points and anchor points to the last item in the curves vector
                controlPoints = curves[curves.size() - 1].controlPoints;
                anchorPoints = curves[curves.size() - 1].anchorPoints;

                // Store the initial position of the mouse
                prevMouseX = display.mouse_x();
                prevMouseY = display.mouse_y();

                // Check if a control point is being dragged
                for (int i = 0; i < controlPoints.size(); ++i) {
                    int x = (int)(displayScale * controlPoints[i].x + displayOffsetX);
                    int y = (int)(screenHeight - displayScale * controlPoints[i].y + displayOffsetY);
                    if (abs(prevMouseX - x) <= 5 && abs(prevMouseY - y) <= 5) {
                        isDraggingControlPoint = true;
                        break;
                    }
                }

                // Check if an anchor point is being dragged
                if (!isDraggingControlPoint) {
                    for (int i = 0; i < anchorPoints.size(); ++i) {
                        int x = (int)(displayScale * anchorPoints[i].x + displayOffsetX);
                        int y = (int)(screenHeight - displayScale * anchorPoints[i].y + displayOffsetY);
                        if (abs(prevMouseX - x) <= 5 && abs(prevMouseY - y) <= 5) {
                            isDraggingAnchorPoint = true;
                            break;
                        }
                    }
                }
            }
            else {
                // Calculate the offset from the initial position
                int dx = display.mouse_x() - prevMouseX;
                int dy = display.mouse_y() - prevMouseY;

                // Update the position of the dragged point
                if (isDraggingControlPoint) {
                    // figure out which control point is being dragged
                    for (int i = 0; i < controlPoints.size(); ++i) {
                        int x = (int)(displayScale * controlPoints[i].x + displayOffsetX);
                        int y = (int)(screenHeight - displayScale * controlPoints[i].y + displayOffsetY);
                        if (abs(prevMouseX - x) <= 5 && abs(prevMouseY - y) <= 5) {
                            controlPointIndex = i;
                            break;
                        }
                    }

                    // update the control point
                    curves[curves.size() - 1].controlPoints[controlPointIndex].x += dx / displayScale;
                    curves[curves.size() - 1].controlPoints[controlPointIndex].y -= dy / displayScale;
                }
                else if (isDraggingAnchorPoint) {
                    // figure out which anchor point is being dragged
                    for (int i = 0; i < anchorPoints.size(); ++i) {
                        int x = (int)(displayScale * anchorPoints[i].x + displayOffsetX);
                        int y = (int)(screenHeight - displayScale * anchorPoints[i].y + displayOffsetY);
                        if (abs(prevMouseX - x) <= 5 && abs(prevMouseY - y) <= 5) {
                            anchorPointIndex = i;
                            break;
                        }
                    }

                    // update the anchor point
                    curves[curves.size() - 1].anchorPoints[anchorPointIndex].x += dx / displayScale;
                    curves[curves.size() - 1].anchorPoints[anchorPointIndex].y -= dy / displayScale;

                }

                // Calculate the new points on the curve
                calculatePoints(curves, num_points, curvePoints);

                // Update the previous mouse position
                prevMouseX = display.mouse_x();
                prevMouseY = display.mouse_y();
            }
        }
        else {
            // Reset the previous mouse position
            prevMouseX = -1;
            prevMouseY = -1;

            isDraggingControlPoint = false;
            isDraggingAnchorPoint = false;
        }

        // Clear the image
        if (display.is_keyC()) {
            image.fill(0);
            curves.clear();
            curvePoints.clear();

        }

        
        image.fill(0);
    }

    return 0;
}
