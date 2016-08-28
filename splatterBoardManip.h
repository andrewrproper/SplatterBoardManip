/*---------------------.
| splatterBoardManip.h  \______________________________
|                                                      \
| A graphical painting program using OpenGL,           |
| with a QT interface and a number of image            |
| manipulation filters, including convolution          |
| matrices.                                            |
|                                                      |
| modified Professor Harry Plantinga's GLPaint.h       |
| begun by Andrew R. Proper,         March 20, 2002    |
| last modified by Andrew R. Proper, March 22, 2002    |
| for cpsc 300 graphics, Calvin College, program 3     |
\______________________________________________________/
/                                                      \
| miscellaneous notes:                                 |
|   angles are measured in radians                     |
|   quadrants:                                         |
|                                                      |
|            PI/2                                      |
|                                                      |
|             |                                        |
|           2 | 1                                      |
|     PI  ----+----  0, 2*PI                           |
|           3 | 4                                      |
|             |                                        |
|                                                      |
|          3*PI/2                                      |
\_____________________________________________________*/


#ifndef SPLATTERBOARDMANIP_H
#define SPLATTERBOARDMANIP_H


#include <qgl.h>
#include <qmainwindow.h>
#include <qpen.h>
#include <qpoint.h>
#include <qimage.h>
#include <qwidget.h>
#include <qstring.h>
#include <qpointarray.h>
#include <qmenubar.h>
#include <qslider.h>
#include <qlabel.h>
#include <qbuttongroup.h>

#include <math.h>   //for drawing triangles and circles using trigonometry, etc.

class QMouseEvent;
class QResizeEvent;
class QPaintEvent;
class QToolButton;


 /*
 | a Canvas is a QGLwidget that you can draw on
*/

 //definitions used by Canvas : mathematical
#define PI        3.14159265358979323846
#define twoPI     PI * 2
#define halfPI    PI / 2
#define quarterPI PI / 4
#define atanPI  atan(PI)
#define toolCirclePointsPerPI    24    //tool configuration

 //definitions used by Canvas : color data structures
typedef GLfloat color3[3];
typedef float color3f255[3];

 //definitions used by Canvas : convolutions
typedef float     float3[3];
typedef float3    kernel3x3[3];
typedef kernel3x3 convolutionKernels[10];
const convolutionKernels convolutionMatrix =
 { { { 0.0, 0.0, 0.0 },                      //fade
     { 0.0, 1.0, 0.0 },
     { 0.0, 0.0, 0.0 } },
   { { 0.0, 0.0, 0.0 },                      //intensify
     { 0.0, 1.0, 0.0 },
     { 0.0, 0.0, 0.0 } },
   { { 0.1111111, 0.1111111, 0.1111111 },    //blur
     { 0.1111111, 0.1111111, 0.1111111 },
     { 0.1111111, 0.1111111, 0.1111111 } },
   { { -0.117647, -0.117647, -0.117647 },    //sharpen
     { -0.117647, 1.941176 , -0.117647 },
     { -0.117647, -0.117647, -0.117647 } },
   { { -0.05, -0.07, -0.05  },               //Laplacian-of-Gaussian variant
     { -0.07, 1.58,  -0.07 },
     { -0.05, -0.07, -0.05  } },
   { { 0.0  , 0.0, 0.0  },                   //edgeDetectX
     { -0.25, 1.5, -0.25 },
     { 0.0  , 0.0, 0.0  } },
   { { 0.0, -0.25, 0.0 },                    //edgeDetectY
     { 0.0, 1.5  , 0.0 },
     { 0.0, -0.25 , 0.0 } },
   { {  0.5,  1.5,  0.5  },                  //sobel
     {  0.0,  1.0,  0.0 },
     { -0.5, -1.5, -0.5  } },
   { { 0.75,  -2.25, 0.75  },                //Laplacian
     { -2.25, 4.0,   -2.25 },
     { 0.75,  -2.25, 0.75  } },
   { { 0.25,  -0.75, 0.25  },                //Laplacian2
     { -0.75, 1.0,   -0.75 },
     { 0.25,  -0.75, 0.25  } } };
enum convolutionType { fade, intensify, blur, sharpen, lapOfGauss,
                       edgeDetectX, edgeDetectY,
                       sobel, laplacian, laplacian2 };

//list of the tools supported by Canvas
enum CanvasTool { none, pen, line, rectangle, rectangleFilled, circle, 
                  circleFilled, triangle, triangleFilled };

class Canvas : public QGLWidget {
 Q_OBJECT

 public:
  Canvas( QWidget *parent = 0, const char *name = 0 );

   // Save|open images files to|from disk.
  void save( const QString &filename, const QString &format );
  void open( const QString &filename );

   // Image Manipulation functions.
  void fade();
  void intensify();
  void invert();
  void convolute(const convolutionType type);
  void clear();

   // Return the given integer value limited within the range 0 to 255.
  int limit0_255(const int & val) {
    if      (val < 0  ) return 0;
    else if (val > 255) return 255;
    else return val; 
  }

   // Activate the given tool.
  void activateTool(CanvasTool toolNum) { myActiveTool = toolNum; }
  void makeCheckImage(void);

   // Accessor functions.
  QColor penColor()        { return *myPenColor; }
  QColor fillColor()       { return *myFillColor; }
  QColor backgroundColor() { return *myBackgroundColor; }
  int    brushSize()       { return myBrushSize; }
  int    gradientDegree()  { return myGradientDegree; }

   // Modifier functions.
  void setPenColor (QColor newColor)       { *myPenColor        = newColor; }
  void setFillColor(QColor newColor)       { *myFillColor       = newColor; }
  void setBackgroundColor(QColor newColor) { *myBackgroundColor = newColor; }
  void setBrushSize(int newSize)           { myBrushSize       = newSize;  }
  void setGradientDegree(int newVal)       { myGradientDegree  = newVal; }

 protected:
  void    drawWithActiveTool();
  void    resizeGL (int w, int h);
  void    initializeGL();
  void    paintGL();


  QImage buffer, workingBuffer;
  QColor *myPenColor, *myFillColor, *myBackgroundColor;
  int    myBrushSize, myActiveTool, myGradientDegree, myFadeDegree;
  int    x1, y1, x2, y2;
  bool   mousePressed, openPic;

   // Overloaded QT functions.
  virtual void mousePressEvent  ( QMouseEvent* event);
  virtual void mouseReleaseEvent( QMouseEvent* event);
  virtual void mouseMoveEvent   ( QMouseEvent* event);

};



 /*
 | The splatterBoardManip object contains a canvas, and the graphical interface
 | elements, such as buttons, toolbars, and menus.
*/
class splatterBoardManip : public QMainWindow {
 Q_OBJECT

 public:
  splatterBoardManip( QWidget *parent = 0, const char *name = 0 );

 protected:
  Canvas	*canvas;
  QButtonGroup  *bgDrawingTools;
  QToolButton	*bClear, 
                *bInvert, *bFade, *bIntensify, 
                *bBlur, *bSharpen, 
                *bEdgeDetectX, *bEdgeDetectY, 
                *bSobel, *bLaplacian, *bLaplacian2, *bLapOfGauss,
                *bPen, *bLine, *bRectangle, *bRectangleFilled, 
                *bCircle, *bCircleFilled, *bTriangle, *bTriangleFilled,
                *bPenColor, *bFillColor, *bBackgroundColor;
  QSlider       *sBrushSize, *sGradientDegree;
  QLabel        *lBrushSize, *lGradientDegree;
  QPopupMenu	*file;
  QMenuBar	*menubar;
  QString       myWorkingPath;   // Path in which to look for files.
  QString       myAuthorText;

 protected slots:
  void slotSave();
  void slotOpen();
  void slotExit();

   // Image Manipulation Slots.
  void slotInvert();
  void slotFade();
  void slotIntensify();
  void slotBlur();
  void slotSharpen();
  void slotEdgeDetectX();
  void slotEdgeDetectY();
  void slotSobel();
  void slotLaplacian();
  void slotLaplacian2();
  void slotLapOfGauss();
  void slotClear();

   // Tool slots.
  void slotPen();
  void slotLine();
  void slotRectangle();
  void slotRectangleFilled();
  void slotCircle();
  void slotCircleFilled();
  void slotTriangle();
  void slotTriangleFilled();

   // Tool configuration slots.
  void slotPenColor();
  void slotFillColor();
  void slotBackgroundColor();
  void slotBrushSize(int value);
  void slotGradientDegree(int value);

};


#endif
