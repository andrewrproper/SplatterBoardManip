/*---------------------.
| splatterBoardManip.c  \______________________________
|                                                      \
| See the heador of splatterBoardManip.h for details.  |
|                                                      |
| modified Professor Harry Plantinga's GLPaint.h       |
| begun by Andrew R. Proper,         March 20, 2002    |
| last modified by Andrew R. Proper, March 22, 2002    |
\_____________________________________________________*/

#include "splatterBoardManip.h"

#include <GL/glu.h>
#include <GL/glut.h>
#include <qapplication.h>
#include <qevent.h>
#include <qpainter.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qtooltip.h>
#include <qbuttongroup.h>
#include <qpoint.h>
#include <qfiledialog.h>
#include <qimage.h>
#include <qcolordialog.h> 

 /*
 | Construct a canvas, initializing its name and member values.
*/
Canvas::Canvas( QWidget *parent, const char *name ) : QGLWidget(parent, name) {
  setCaption("SplatterBoardManip");
  //myPenColor   = new QColor(100,137,155);
  myPenColor   = new QColor(116,102,83);
  myFillColor  = new QColor(100,100,155);
  myBackgroundColor = new QColor(214,236,233);
  myBrushSize = 6;
  myGradientDegree = 95;
  myFadeDegree = 128;
  myActiveTool = none;

  buffer = grabFrameBuffer(true);
  workingBuffer = buffer;
}

 /*
 | Save the buffer into the specified image.
*/
void Canvas::save( const QString &filename, const QString &format )
 { buffer.save( filename, format.upper() ); }

 /*
 | Open the specified image into buffer and call paintGL to display it.
*/
void Canvas::open( const QString &filename ) {
  buffer.load( filename );
  openPic=true;
  updateGL();
}

 /*
 | Clear the canvas with the background color
*/
void Canvas::clear() {
  buffer.fill( myBackgroundColor->pixel() );
  openPic=true;
  updateGL();
}


 /*
 | Apply the given convolution matrix to the pixel at the given location, and
 | set the rgb color to the color for the pixel at that location.
*/
void Canvas::convolute(const convolutionType type) {
  color3f255 rgb;
  unsigned int pix;

  buffer = grabFrameBuffer(true);
  workingBuffer = buffer;

  for (int x=1; x<(buffer.width()-2); x++)
    for (int y=1; y<(buffer.height()-2); y++) {

      rgb[0] = rgb[1] = rgb[2] = 0.0;
      for( int row = -1;  row <= 1;  row++ ) {
        for( int col = -1;  col <= 1;  col++ ) {
          pix = buffer.pixel( x+row, y+col );
          rgb[0] += (double)qRed(pix)   * convolutionMatrix[type][col+1][row+1];
          rgb[1] += (double)qGreen(pix) * convolutionMatrix[type][col+1][row+1];
          rgb[2] += (double)qBlue(pix)  * convolutionMatrix[type][col+1][row+1];
        }
      }

      workingBuffer.setPixel( x, y,
       qRgb(limit0_255((int)rgb[0]),
            limit0_255((int)rgb[1]),
            limit0_255((int)rgb[2])) );

    }

  buffer = workingBuffer;
  openPic=true;
  updateGL();
}


 /*
 | Invert the colors in the image.
*/
void Canvas::invert() {
  buffer.invertPixels();
  openPic=true;
  updateGL();
}


 /*
 | Fade the colors in the image towards white.
 | Approximately the opposite of intensify().
*/
void Canvas::fade() {
  unsigned int pix;

  buffer = grabFrameBuffer(true);

  for (int y=0; y<buffer.height(); y++)
    for (int x=0; x<buffer.width(); x++) {
      pix = buffer.pixel(x,y);
      buffer.setPixel(x, y, 
       qRgb(limit0_255( qRed(pix)/2   + myFadeDegree ),
            limit0_255( qGreen(pix)/2 + myFadeDegree ),
            limit0_255( qBlue(pix)/2  + myFadeDegree )) );
    }

  openPic=true;
  updateGL();
}


 /*
 | Intensify the colors in the image towards black.
 | Approximately the opposite of fade().
*/
void Canvas::intensify() {
  unsigned int pix;

  buffer = grabFrameBuffer(true);

  for (int y=0; y<buffer.height(); y++)
    for (int x=0; x<buffer.width(); x++) {
      pix = buffer.pixel(x,y);
      buffer.setPixel(x, y, 
       qRgb(limit0_255( (qRed(pix)   - myFadeDegree) * 2),
            limit0_255( (qGreen(pix) - myFadeDegree) * 2),
            limit0_255( (qBlue(pix)  - myFadeDegree) * 2)) );
    }

  openPic=true;
  updateGL();
}


 /*
 | Receive a mouse press event.
 | Store the location at which the mouse was pressed for drawing purposes.
*/
void Canvas::mousePressEvent( QMouseEvent *e ) {
  mousePressed = true;
  x1 = e->x();
  y1 = height() - e->y();
  x2 = x1;
  y2 = y1;
}

 /* 
 | Receive a mouse release event.
 | With the active tool, clear any XOR drawings and make a regular drawing.
*/
void Canvas::mouseReleaseEvent( QMouseEvent * ) {

   // rubber-banding tools only:
  if ( myActiveTool != pen ) {
     //draw over previous XOR drawing, cancelling it out
    glLogicOp(GL_XOR);              // draw in XOR mode
    drawWithActiveTool();
     // draw the final drawing, for the current mouse location
    glLogicOp(GL_COPY);  // switch back to drawing in COPY mode, the default
    drawWithActiveTool();
    updateGL();
  }

  mousePressed = false;
  buffer=grabFrameBuffer();	// save image for resizing

}

 /*
 | Receive a mouse motion event
 | Store the new mouse location as the destination drawing point.
 | With the active tool, remove any previous XOR drawing and draw a new one.
*/
void Canvas::mouseMoveEvent( QMouseEvent *e ) {

  if ( mousePressed ) {  // drawing is currently occurring
    if ( myActiveTool == pen ) {
      x1 = x2;
      y1 = y2;
      x2 = e->x();
      y2 = height() - e->y();
      drawWithActiveTool();
    } else {
      glLogicOp(GL_XOR);      // draw in XOR mode
       //draw over previous XOR drawing, cancelling it out
      drawWithActiveTool();
       //draw new XOR drawing, for the current mouse location
      x2 = e->x();
      y2 = height() - e->y();
      drawWithActiveTool();
      glLogicOp(GL_COPY);    // switch back to drawing in COPY mode, the default
    }
    updateGL();
  }

}


 /* 
 | Initialize OpenGL settings.
*/
void Canvas::initializeGL() {
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPointSize(myBrushSize);

  glEnable(GL_POINT_SMOOTH);       //use smooth, rounded points
  glShadeModel(GL_SMOOTH);         //set default shading model
  glEnable(GL_COLOR_LOGIC_OP);     //enable color logic operations (ex: XOR)
  glPolygonMode(GL_FRONT,GL_FILL); //draw filled polygons, front only

  glClearColor( myBackgroundColor->red() / 255.0,
                myBackgroundColor->green() / 255.0,
                myBackgroundColor->blue() / 255.0, 1.0 );
  glClear(GL_COLOR_BUFFER_BIT);
  buffer.fill( myBackgroundColor->pixel() );
  openPic=true;
}

 /*
 | Resize this canvas to fit the current window size.
*/
void Canvas::resizeGL( int w, int h ) {
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, (GLint) w, (GLint) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, 0, h, -2, 2);

  buffer = buffer.scale(w, h);	//stretch or shrink image
  openPic = true;
  updateGL();
}

 /*
 | If the buffer has been changed, by opening a picture from a file or by 
 | a manipulation function, draw the pixels from the buffer into the screen.
*/
void Canvas::paintGL( ) {
  if (openPic) {
    QImage gl_buffer = convertToGLFormat(buffer);
    glRasterPos2i(0,0);
    glDrawPixels(buffer.width(), buffer.height(), GL_RGBA, 
      GL_UNSIGNED_BYTE, gl_buffer.bits());
    glFlush();
    openPic = false;
  }
}


 /*
 | Draw with the active tool using this canvas's two points, x1,y1 and x2,y2.
*/
void Canvas::drawWithActiveTool() {
  GLfloat drawingAngle;
  GLfloat angle, angleL, angleM, xDist, yDist, xLen, yLen, hypLen, longHypLen, smallHypLen, smallXlen, smallYlen, radius;
//          minVC
  GLint   minPointSize = 1;
//  int     height, width;
  bool    halfwayColorSet;
  color3 penColor,  penColorDark,  penColorLight, 
         fillColor, fillColorDark, fillColorLight,
         vertexColor;
//         firstColor, secondColor

  //switch from QT colors to openGL colors, 0...255  to  0.0...1.0
  GLfloat gradientDegree = myGradientDegree / 255.0;
  penColor[0] = myPenColor->red()   / 255.0;
  penColor[1] = myPenColor->green() / 255.0;
  penColor[2] = myPenColor->blue()  / 255.0;
  for ( int i = 0; i < 3; i++ ) {
    penColorLight[i] = penColor[i];
    penColorDark[i]  = penColor[i];
  }
  for (int i = 0; i < 3; i++) {
    if  (penColorLight[i]  + gradientDegree > 1) penColorLight[i] = 1; //max: white
    else penColorLight[i] += gradientDegree;
    if  (penColorDark[i]   - gradientDegree < 0) penColorDark[i]  = 0; //min: black
    else penColorDark[i]  -= gradientDegree;
  }
  fillColor[0] = myFillColor->red()   / 255.0;
  fillColor[1] = myFillColor->green() / 255.0;
  fillColor[2] = myFillColor->blue()  / 255.0;
  for ( int i = 0; i < 3; i++ ) {
    fillColorLight[i] = fillColor[i];
    fillColorDark[i]  = fillColor[i];
  }
  for (int i = 0; i < 3; i++) {
    if  (fillColorLight[i]  + gradientDegree > 1) fillColorLight[i] = 1; //max: white
    else fillColorLight[i] += gradientDegree;
    if  (fillColorDark[i]   - gradientDegree < 0) fillColorDark[i]  = 0; //min: black
    else fillColorDark[i]  -= gradientDegree;
  } 

  // use my brush size for points and lines
  glPointSize(myBrushSize);
  glLineWidth(myBrushSize);

  switch(myActiveTool) {
    case none    :
      break;
    case pen     :
     // A point on point1 and point2, with a line between, without any gradient.
      glPointSize(myBrushSize*0.5);
      glColor3fv(penColor);
      if (myBrushSize > minPointSize) {
        glBegin(GL_POINTS);
          glVertex2f(x1,y1);
          glVertex2f(x2,y2);
        glEnd();
      }
      glBegin(GL_LINES);
        glVertex2f(x1,y1);
        glVertex2f(x2,y2);
      glEnd();
      glPointSize(myBrushSize);
      break;
    case line      :
     // A point on point1 and point2, with a line between.
      glPointSize(myBrushSize*0.6);
      if (myBrushSize > minPointSize) {
        glBegin(GL_POINTS);
          glColor3fv(penColorLight);
          glVertex2f(x1,y1);
        glEnd();
      }
      glBegin(GL_LINES);
        glVertex2f(x1,y1);
        glColor3fv(penColorDark);
        glVertex2f(x2,y2);
      glEnd();
      if (myBrushSize > minPointSize) {
        glBegin(GL_POINTS);
          glVertex2f(x2,y2);
        glEnd();
      }
      glPointSize(myBrushSize);
      break;
    case rectangle :
     // A pen rectangle from point1 to point2, 
     // with points on its corners to smooth them out.
      glDisable(GL_POINT_SMOOTH);       //use square points
      if (myBrushSize > minPointSize) {
        glBegin(GL_POINTS);
          glColor3fv(penColorLight);
          glVertex2f(x1,y1);
          glColor3fv(penColor);
          glVertex2f(x2,y1);
          glColor3fv(penColorDark);
          glVertex2f(x2,y2);
          glColor3fv(penColor);
          glVertex2f(x1,y2);
        glEnd();
      }
      glBegin(GL_LINE_STRIP);
        glColor3fv(penColorLight);
        glVertex2f(x1,y1);
        glColor3fv(penColor);
        glVertex2f(x2,y1);
        glColor3fv(penColorDark);
        glVertex2f(x2,y2);
        glColor3fv(penColor);
        glVertex2f(x1,y2);
        glColor3fv(penColorLight);
        glVertex2f(x1,y1);
      glEnd();
      glEnable(GL_POINT_SMOOTH);       //use smooth, rounded points
      break;
    case rectangleFilled :
     // A filled rectangle from point1 to point2, 
     // a pen rectangle from point1 to point2, 
     // and points on its corners to smooth them out.
      glDisable(GL_POINT_SMOOTH);       //use square points
      glBegin(GL_POLYGON);
/*
        width  = x1 - x2;
        height = y1 - y2;
        if ( abs( width ) < abs( height ) ) {
          if ( width > 0 ) {
            for (int i = 0; i < 3; i++) {
              firstColor[i]  = fillColorLight[i];
              secondColor[i] = fillColorDark[i];
            }
          } else {
            for (int i = 0; i < 3; i++) {
              firstColor[i]  = fillColorDark[i];
              secondColor[i] = fillColorLight[i];
            }
          }
          glColor3fv(firstColor);
          glVertex2f(x1,y1);
          glVertex2f(x2,y1);
          glColor3fv(secondColor);
          glVertex2f(x2,y2);
          glVertex2f(x1,y2);
        } else {
          if ( height > 0 ) {
            for (int i = 0; i < 3; i++) {
              firstColor[i]  = fillColorLight[i];
              secondColor[i] = fillColorDark[i];
            }
          } else {
            for (int i = 0; i < 3; i++) {
              firstColor[i]  = fillColorDark[i];
              secondColor[i] = fillColorLight[i];
            }
          }
          glColor3fv(secondColor);
          glVertex2f(x1,y1);
          glColor3fv(firstColor);
          glVertex2f(x2,y1);
          glVertex2f(x2,y2);
          glColor3fv(secondColor);
          glVertex2f(x1,y2);
        }
*/
        glColor3fv(fillColorLight);
        glVertex2f(x1,y1);
        glColor3fv(fillColor);
        glVertex2f(x2,y1);
        glColor3fv(fillColorDark);
        glVertex2f(x2,y2);
        glColor3fv(fillColor);
        glVertex2f(x1,y2);
      glEnd();
      glColor3fv(penColor);
      if (myBrushSize > minPointSize) {
        glBegin(GL_POINTS);
          glColor3fv(penColorLight);
          glVertex2f(x1,y1);
          glColor3fv(penColor);
          glVertex2f(x2,y1);
          glColor3fv(penColorDark);
          glVertex2f(x2,y2);
          glColor3fv(penColor);
          glVertex2f(x1,y2);
        glEnd();
      }
      glBegin(GL_LINE_STRIP);
        glColor3fv(penColorLight);
        glVertex2f(x1,y1);
        glColor3fv(penColor);
        glVertex2f(x2,y1);
        glColor3fv(penColorDark);
        glVertex2f(x2,y2);
        glColor3fv(penColor);
        glVertex2f(x1,y2);
        glColor3fv(penColorLight);
        glVertex2f(x1,y1);
      glEnd();
      glEnable(GL_POINT_SMOOTH);       //use smooth, rounded points
      break;
    case circle :
     // A circle with its centre on point1, and its radius out to point2
      xLen = fabs(fabs(x1)-fabs(x2));
      yLen = fabs(fabs(y1)-fabs(y2));
      radius = sqrt( xLen*xLen + yLen*yLen );
      if (xLen == 0) xLen = 0.0001;          // avoid dividing by zero
      drawingAngle = atan ( yLen / xLen );  // angle from point1 to point2
      if ( y2 > y1 ) {
        if ( x2 < x1 ) drawingAngle  = PI - drawingAngle;    //flip   into quad2
      } else {
        if ( x2 > x1 ) drawingAngle  = twoPI - drawingAngle; //flip   into quad4
        else           drawingAngle += PI;                   //rotate into quad3
      }
      glBegin(GL_LINE_STRIP);
        for ( angle  = 0; angle  < twoPI + PI / toolCirclePointsPerPI; 
              angle += PI / toolCirclePointsPerPI ) {
          glColor3f( penColor[0] - gradientDegree * cos(angle),
                     penColor[1] - gradientDegree * cos(angle),
                     penColor[2] - gradientDegree * cos(angle) );
          glVertex2f( x1 + radius * cos(angle+drawingAngle),
                      y1 + radius * sin(angle+drawingAngle));
        }
      glEnd();
      break;
    case circleFilled :
     // A filled circle with its centre on point1, and its radius out to point2,
     // and a pen circle with its centre on point1 and its radius out to point2.
      xLen = fabs(fabs(x1)-fabs(x2));
      yLen = fabs(fabs(y1)-fabs(y2));
      radius = sqrt( xLen*xLen + yLen*yLen );
      if (xLen == 0) xLen = 0.0001;          // avoid dividing by zero
      drawingAngle = atan ( yLen / xLen );  // angle from point1 to point2
      if ( y2 > y1 ) {
        if ( x2 < x1 ) drawingAngle  = PI - drawingAngle;    //flip   into quad2
      } else {
        if ( x2 > x1 ) drawingAngle  = twoPI - drawingAngle; //flip   into quad4
        else           drawingAngle += PI;                   //rotate into quad3
      }
      glBegin(GL_POLYGON);
        halfwayColorSet = false;
//        glColor3f( fillColorLight[0], fillColorLight[1], fillColorLight[2] );
        for ( angle  = 0; angle < twoPI + PI / toolCirclePointsPerPI; 
              angle += PI / toolCirclePointsPerPI ) {
          for (int i = 0; i < 3; i++) {
            vertexColor[i] = fillColor[i] + gradientDegree * cos(angle);
          }
/*
          minVC = 0.0;
          for (int i = 0; i < 3; i++) {
            if ( vertexColor[i] < minVC ) minVC = vertexColor[i];
          }
          if ( minVC < 0.0 ) {
            for (int i = 0; i < 3; i++) {
              vertexColor[i] += ( minVC + 0.1 );
            }
          }
*/
          //if ( angle == 0 ) {
          //} else 
//          if ( ! halfwayColorSet && angle >= PI ) {
//            glColor3f( fillColorDark[0],  fillColorDark[1],  fillColorDark[2]  );
//            halfwayColorSet = true;
//          }
          //glColor3f( fillColor[0] + 0.5 - gradientDegree * cos(angle),
          //           fillColor[1] + 0.5 - gradientDegree * cos(angle),
          //           fillColor[2] + 0.5 - gradientDegree * cos(angle) );
          glColor3f( vertexColor[0], vertexColor[1], vertexColor[2] );
          glVertex2f( x1 + radius * cos(angle+drawingAngle), 
                      y1 + radius * sin(angle+drawingAngle)); 
        }
 //       glColor3f( fillColorDark[0],  fillColorDark[1],  fillColorDark[2]  );
      glEnd();
      glBegin(GL_LINE_STRIP);
        for ( angle  = 0; angle < twoPI + PI / toolCirclePointsPerPI; 
              angle += PI / toolCirclePointsPerPI ) {
          for (int i = 0; i < 3; i++) {
            vertexColor[i] = penColor[i] - gradientDegree * cos(angle);
          }
          //minVC = 0.0;
          //for (int i = 0; i < 3; i++) {
          //  if ( vertexColor[i] < minVC ) minVC = vertexColor[i];
          //}
          //if ( minVC < 0.0 ) {
          //  for (int i = 0; i < 3; i++) {
          //    vertexColor[i] += ( minVC + 0.1 );
          //  }
          //}
          glColor3f( vertexColor[0], vertexColor[1], vertexColor[2] );
                    // penColor[0] - gradientDegree * cos(angle),
                    // penColor[1] - gradientDegree * cos(angle),
                    // penColor[2] - gradientDegree * cos(angle) );
          glVertex2f( x1 + radius * cos(angle+drawingAngle),
                      y1 + radius * sin(angle+drawingAngle));
        }
      glEnd();
      break;
    case triangle :
     // A pen triangle from point1 to point2, attempting to be equilateral,
     // with a point on each of its corners.
      xDist = fabs( x2 - x1 ); // adjacent length
      yDist = fabs( y2 - y1 ); // opposite length
      hypLen= sqrt( xDist * xDist + yDist * yDist ); // hypotenuse length, pythagorean theorem: C^2 = A^2 + B^2
      angle = acos( xDist / hypLen ); // get center angle based on hypotenuse and opposite side
      angleL = atan( 0.5 );
      longHypLen = hypLen / cos( angleL );
      if (longHypLen != 0) {                  // be careful not to divide by zero
        smallHypLen = longHypLen / 2.0;
        angleM = ( PI / 2.0 ) - angle;
        smallXlen  = smallHypLen * cos( angleM );  // adjacentLen
        smallYlen  = smallHypLen * sin( angleM );  // oppositeLen

        glColor3fv(penColor);
        glBegin(GL_LINE_STRIP);
          glColor3fv(penColorDark);
          glVertex2f( x2, y2 );
          glColor3fv(penColorLight);
          if        ( y2 <= y1 && x2 >= x1) {          // upper right
            glVertex2f( x1-smallXlen, y1-smallYlen  );
            glVertex2f( x1+smallXlen, y1+smallYlen );
          } else if ( y2 <= y1 && x2 <  x1) {          // upper left
            glVertex2f( x1-smallXlen, y1+smallYlen  );
            glVertex2f( x1+smallXlen, y1-smallYlen );
          } else if ( y2 >  y1 && x2 >= x1) {          // lower right
            glVertex2f( x1-smallXlen, y1+smallYlen  );
            glVertex2f( x1+smallXlen, y1-smallYlen );
          } else                       {               // lower left
            glVertex2f( x1-smallXlen, y1-smallYlen  );
            glVertex2f( x1+smallXlen, y1+smallYlen );
          }
          glColor3fv(penColorDark);
          glVertex2f( x2, y2 );
        glEnd();
      }
      break;
    case triangleFilled :
     // A filled triangle from point1 to point2, attempting to be equilateral,
     // a pen triangle from point1 to point2, attempting to be equilateral,
     // and a point on each of the corners.
      xDist = fabs( x2 - x1 ); // adjacent length
      yDist = fabs( y2 - y1 ); // opposite length
      hypLen= sqrt( xDist * xDist + yDist * yDist ); // hypotenuse length, pythagorean theorem: C^2 = A^2 + B^2
      angle = acos( xDist / hypLen ); // get center angle based on hypotenuse and opposite side
      angleL = atan( 0.5 );
      longHypLen = hypLen / cos( angleL );
      if (longHypLen != 0) {                  // be careful not to divide by zero
        smallHypLen = longHypLen / 2.0;
        angleM = ( PI / 2.0 ) - angle;
        smallXlen  = smallHypLen * cos( angleM );  // adjacentLen
        smallYlen  = smallHypLen * sin( angleM );  // oppositeLen

        glColor3fv(fillColor);
        glBegin(GL_POLYGON);
          glColor3fv(fillColorLight);
          glVertex2f( x2, y2 );
          glColor3fv(fillColorDark);
          if        ( y2 <= y1 && x2 >= x1) {          // upper right
            glVertex2f( x1-smallXlen, y1-smallYlen  );
            glVertex2f( x1+smallXlen, y1+smallYlen );
          } else if ( y2 <= y1 && x2 <  x1) {          // upper left
            glVertex2f( x1-smallXlen, y1+smallYlen  );
            glVertex2f( x1+smallXlen, y1-smallYlen );
          } else if ( y2 >  y1 && x2 >= x1) {          // lower right
            glVertex2f( x1-smallXlen, y1+smallYlen  );
            glVertex2f( x1+smallXlen, y1-smallYlen );
          } else                       {               // lower left
            glVertex2f( x1-smallXlen, y1-smallYlen  );
            glVertex2f( x1+smallXlen, y1+smallYlen );
          }
        glEnd();
        glColor3fv(penColor);
        glBegin(GL_LINE_STRIP);
          glColor3fv(penColorDark);
          glVertex2f( x2, y2 );
          glColor3fv(penColorLight);
          if        ( y2 <= y1 && x2 >= x1) {          // upper right
            glVertex2f( x1-smallXlen, y1-smallYlen  );
            glVertex2f( x1+smallXlen, y1+smallYlen );
          } else if ( y2 <= y1 && x2 <  x1) {          // upper left
            glVertex2f( x1-smallXlen, y1+smallYlen  );
            glVertex2f( x1+smallXlen, y1-smallYlen );
          } else if ( y2 >  y1 && x2 >= x1) {          // lower right
            glVertex2f( x1-smallXlen, y1+smallYlen  );
            glVertex2f( x1+smallXlen, y1-smallYlen );
          } else                       {               // lower left
            glVertex2f( x1-smallXlen, y1-smallYlen  );
            glVertex2f( x1+smallXlen, y1+smallYlen );
          }
          glColor3fv(penColorDark);
          glVertex2f( x2, y2 );
        glEnd();
      }
      break;
  }
}



/*============================================\
|    General (non-OpenGL) widget stuff        |
\============================================*/

 /*
 | Construct a splatterBoardManip, initializing its member functions and 
 | creating its interface widgets.
*/
splatterBoardManip::splatterBoardManip( QWidget *parent, const char *name ) 
 : QMainWindow( parent, name ) {

  myAuthorText  = " (c) Andrew R. Proper ";
  //myAuthorText  = ",;(Andrew R. Proper, 2002,03);,";
  myWorkingPath = "images/";

  canvas = new Canvas( this );
  setCentralWidget( canvas );


  // make some toolbars

  QToolBar *tools = new QToolBar( this );

  bBackgroundColor = new QToolButton(QPixmap(),"Background Color", 
    "Background Color", this, SLOT( slotBackgroundColor() ), tools);
  bBackgroundColor->setPaletteBackgroundColor( canvas->backgroundColor() );

  tools->addSeparator();
  tools->addSeparator();

  bPenColor = new QToolButton(QPixmap(),"Pen Color", "Pen Color", this,
    SLOT( slotPenColor() ), tools);
  bPenColor->setPaletteBackgroundColor( canvas->penColor() );

  bFillColor = new QToolButton(QPixmap(),"Fill Color", "Fill Color", this,
    SLOT( slotFillColor() ), tools);
  bFillColor->setPaletteBackgroundColor( canvas->fillColor() );


  tools->addSeparator();

  bgDrawingTools = new QButtonGroup(0, Qt::Horizontal, this,"Drawing Tools");
  bgDrawingTools->setExclusive(true);
  bgDrawingTools->hide();

  bPen = new QToolButton(QPixmap(QImage("icons/buttonPen.png","png")), "Pen", 
    "Pen", this, SLOT( slotPen() ), tools);
  bLine = new QToolButton(QPixmap(QImage("icons/buttonLine.png","png")), 
    "Line", "Line", this, 
    SLOT( slotLine() ), tools);
  bRectangle=new QToolButton(QPixmap(QImage("icons/buttonRectangle.png","png")),
    "Rectangle", "Rectangle", this, 
    SLOT( slotRectangle() ), tools);
  bRectangleFilled=new QToolButton(
    QPixmap(QImage("icons/buttonRectangleFilled.png","png")),
    "Filled Rectangle", "Filled Rectangle", this, 
    SLOT( slotRectangleFilled () ), tools);
  bCircle=new QToolButton(QPixmap(QImage("icons/buttonCircle.png","png")),
    "Circle", "Circle", this, 
    SLOT( slotCircle() ), tools);
  bCircleFilled=new QToolButton(
    QPixmap(QImage("icons/buttonCircleFilled.png","png")),
    "Filled Circle", "Filled Circle", this, 
    SLOT( slotCircleFilled () ), tools);
  bTriangle=new QToolButton(QPixmap(QImage("icons/buttonTriangle.png","png")),
    "Triangle", "Triangle", this, 
    SLOT( slotTriangle() ), tools);
  bTriangleFilled=new QToolButton(
    QPixmap(QImage("icons/buttonTriangleFilled.png","png")),
    "Filled Triangle", "Filled Triangle", this, 
    SLOT( slotTriangleFilled () ), tools);

  bPen->setToggleButton(true);
  bLine->setToggleButton(true);
  bRectangle->setToggleButton(true);
  bRectangleFilled->setToggleButton(true);
  bCircle->setToggleButton(true);
  bCircleFilled->setToggleButton(true);
  bTriangle->setToggleButton(true);
  bTriangleFilled->setToggleButton(true);

  bgDrawingTools->insert(bPen,pen);
  bgDrawingTools->insert(bLine,line);
  bgDrawingTools->insert(bRectangle,rectangle);
  bgDrawingTools->insert(bRectangleFilled,rectangleFilled);
  bgDrawingTools->insert(bCircle,circle);
  bgDrawingTools->insert(bCircleFilled,circleFilled);
  bgDrawingTools->insert(bTriangle,triangle);
  bgDrawingTools->insert(bTriangleFilled,triangleFilled);


  QToolBar *tools2 = new QToolBar( this );

  lBrushSize = new QLabel("Brush Size: ",tools2,"Brush Size: "); 
  sBrushSize = new QSlider(1, 11, 1, canvas->brushSize(), Qt::Horizontal, 
    tools2, "Brush Size");
  sBrushSize->setTickInterval(2);
  sBrushSize->setTickmarks(QSlider::Below);
  connect(sBrushSize,SIGNAL(valueChanged(int)),this,SLOT(slotBrushSize(int)));

  tools2->addSeparator();

  lGradientDegree = new QLabel("Gradient Degree: ",tools2,"Gradient Degree: "); 
  sGradientDegree = new QSlider(-255, 255, 1, canvas->gradientDegree(), 
    Qt::Horizontal, tools2, "Gradient Degree");
  sGradientDegree->setTickInterval(25);
  sGradientDegree->setTickmarks(QSlider::Below);
  connect(sGradientDegree,SIGNAL(valueChanged(int)),this,
    SLOT(slotGradientDegree(int)));

  tools2->addSeparator();

  bClear = new QToolButton(QPixmap(), "Clear the screen", "Clear", this, 
    SLOT( slotClear() ), tools2 );
  bClear->setText( "Clear" );


  QToolBar *manipulationTools = new QToolBar( this );

  bInvert = new QToolButton(QPixmap(), "Invert", "Invert", this, 
    SLOT( slotInvert() ), manipulationTools);
  bInvert->setText( "Invert" );
  manipulationTools->addSeparator();

  bFade = new QToolButton(QPixmap(), "Fade", "Fade", this, 
    SLOT( slotFade() ), manipulationTools);
  bFade->setText( "Fade" );
  manipulationTools->addSeparator();

  bIntensify = new QToolButton(QPixmap(), "Intensify", "Intensify", this, 
    SLOT( slotIntensify() ), manipulationTools);
  bIntensify->setText( "Intensify" );
  manipulationTools->addSeparator();

  bBlur = new QToolButton(QPixmap(), "Blur", "Blur", this, 
    SLOT( slotBlur() ), manipulationTools);
  bBlur->setText( "Blur" );
  manipulationTools->addSeparator();

  bSharpen = new QToolButton(QPixmap(), "Sharpen", "Sharpen", this, 
    SLOT( slotSharpen() ), manipulationTools);
  bSharpen->setText( "Sharpen" );
  manipulationTools->addSeparator();

  bLapOfGauss = new QToolButton(QPixmap(), "Lap-Of-Gauss", "Lap-OfG-auss", this,
    SLOT( slotLapOfGauss() ), manipulationTools);
  bLapOfGauss->setText( "Lap-Of-Gauss" );


  QToolBar *manipulationTools2 = new QToolBar( this );

  bEdgeDetectX = new QToolButton(QPixmap(), "Edge-detect X", "Edge-detect X", 
    this, SLOT( slotEdgeDetectX() ), manipulationTools2);
  bEdgeDetectX->setText( "Edge-detect X" );
  manipulationTools2->addSeparator();

  bEdgeDetectY = new QToolButton(QPixmap(), "Edge-detect Y", "Edge-detect Y", 
    this, SLOT( slotEdgeDetectY() ), manipulationTools2);
  bEdgeDetectY->setText( "Edge-detect Y" );
  manipulationTools2->addSeparator();

  bSobel = new QToolButton(QPixmap(), "Sobel", "Sobel", this, 
    SLOT( slotSobel() ), manipulationTools2);
  bSobel->setText( "Sobel" );
  manipulationTools2->addSeparator();

  bLaplacian = new QToolButton(QPixmap(), "Laplacian", "Laplacian", this, 
    SLOT( slotLaplacian() ), manipulationTools2);
  bLaplacian->setText( "Laplacian" );
  manipulationTools2->addSeparator();

  bLaplacian2 = new QToolButton(QPixmap(), "Laplacian2", "Laplacian2", this, 
    SLOT( slotLaplacian2() ), manipulationTools2);
  bLaplacian2->setText( "Laplacian2" );


  // make a menubar

  file = new QPopupMenu();
  file->insertItem ("&Save", this, SLOT( slotSave() ) );
  file->insertItem ("&Open", this, SLOT( slotOpen() ) );
  file->insertItem ("E&xit", this, SLOT( slotExit() ), CTRL+Key_Q );
  menubar = new QMenuBar( this );
  menubar->insertItem( "&File", file);
  menubar->insertSeparator();
  menubar->insertItem( myAuthorText );


}


/*---------------------\
|    Slot functions    |
\---------------------*/

void splatterBoardManip::slotSave() {
  QString filename = 
   QFileDialog::getSaveFileName( myWorkingPath, "Images (*.png *.bmp *.xpm)", 
    this, "save image dialog" "Choose a destination image file.");
  if ( !filename.isEmpty() ) {
    canvas->save( filename, filename.right(3) );
    myWorkingPath = filename.left( filename.findRev('/')+1 );
  }
}

void splatterBoardManip::slotOpen() {
  QString filename = 
   QFileDialog::getOpenFileName( myWorkingPath, "Images (*.png *.bmp *.xpm)", 
    this, "open file dialog", "Choose an image file to open.");
  if ( !filename.isEmpty() ) {
    canvas->open( filename );
    myWorkingPath = filename.left( filename.findRev('/')+1 );
  }
}

void splatterBoardManip::slotInvert()       { canvas->invert(); }
void splatterBoardManip::slotFade()         { canvas->fade(); }
void splatterBoardManip::slotIntensify()    { canvas->intensify(); }
void splatterBoardManip::slotBlur()         { canvas->convolute(blur); }
void splatterBoardManip::slotSharpen()      { canvas->convolute(sharpen); }
void splatterBoardManip::slotEdgeDetectX()  { canvas->convolute(edgeDetectX); }
void splatterBoardManip::slotEdgeDetectY()  { canvas->convolute(edgeDetectY); }
void splatterBoardManip::slotSobel()        { canvas->convolute(sobel); }
void splatterBoardManip::slotLaplacian()    { canvas->convolute(laplacian); }
void splatterBoardManip::slotLaplacian2()   { canvas->convolute(laplacian2); }
void splatterBoardManip::slotLapOfGauss()   { canvas->convolute(lapOfGauss); }
void splatterBoardManip::slotClear()        { canvas->clear(); }

void splatterBoardManip::slotPen()          { canvas->activateTool(pen); }
void splatterBoardManip::slotLine()         { canvas->activateTool(line); }
void splatterBoardManip::slotRectangle()    { canvas->activateTool(rectangle); }
void splatterBoardManip::slotRectangleFilled() 
 { canvas->activateTool(rectangleFilled); }
void splatterBoardManip::slotCircle()       { canvas->activateTool(circle); }
void splatterBoardManip::slotCircleFilled() 
 { canvas->activateTool(circleFilled); }
void splatterBoardManip::slotTriangle()     { canvas->activateTool(triangle); }
void splatterBoardManip::slotTriangleFilled()  
 { canvas->activateTool(triangleFilled); }
void splatterBoardManip::slotExit()         { close(); }

void splatterBoardManip::slotPenColor()  { 
  QColor pickColor = QColorDialog::getColor( canvas->penColor(), this );
  if ( pickColor.isValid() ) {
    canvas->setPenColor( pickColor );
    bPenColor->setPaletteBackgroundColor( pickColor );
  }
}

void splatterBoardManip::slotFillColor() { 
  QColor pickColor = QColorDialog::getColor( canvas->fillColor(), this );
  if ( pickColor.isValid() ) {
    canvas->setFillColor( pickColor );
    bFillColor->setPaletteBackgroundColor( pickColor );
  }
}

void splatterBoardManip::slotBackgroundColor() {
  QColor pickColor = QColorDialog::getColor( canvas->backgroundColor(), this );
  if ( pickColor.isValid() ) {
    canvas->setBackgroundColor( pickColor );
    bBackgroundColor->setPaletteBackgroundColor( pickColor );
  }
}

void splatterBoardManip::slotBrushSize(int value) 
 { canvas->setBrushSize(value); }

void splatterBoardManip::slotGradientDegree(int value)
 {  canvas->setGradientDegree(value); }


