//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef CURVE_WIDGET_H
#define CURVE_WIDGET_H

#include <set>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtOpenGL/QGLWidget>
#include <QMetaType>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Global/GlobalDefines.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Engine/OverlaySupport.h"

class QFont;
class Variant;
class TimeLine;
class KnobGui;
class CurveWidget;
class Button;
class LineEdit;
class SpinBox;
class Gui;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class CurveGui
    : public QObject
{
    Q_OBJECT

public:

    enum SelectedDerivative
    {
        LEFT_TANGENT = 0,RIGHT_TANGENT = 1
    };

    CurveGui(const CurveWidget *curveWidget,
             boost::shared_ptr<Curve>  curve,
             KnobGui* knob,
             int dimension,
             const QString & name,
             const QColor & color,
             int thickness = 1);

    virtual ~CurveGui() OVERRIDE;

    const QString & getName() const
    {
        return _name;
    }

    void setName(const QString & name)
    {
        _name = name;
    }

    const QColor & getColor() const
    {
        return _color;
    }

    void setColor(const QColor & color)
    {
        _color = color;
    }

    int getThickness() const
    {
        return _thickness;
    }

    void setThickness(int t)
    {
        _thickness = t;
    }

    bool isVisible() const
    {
        return _visible;
    }

    void setVisibleAndRefresh(bool visible);

    /**
     * @brief same as setVisibleAndRefresh() but doesn't emit any signal for a repaint
     **/
    void setVisible(bool visible);

    bool isSelected() const
    {
        return _selected;
    }

    void setSelected(bool s)
    {
        _selected = s;
    }

    KnobGui* getKnob() const
    {
        return _knob;
    }

    int getDimension() const
    {
        return _dimension;
    }

    /**
     * @brief Evaluates the curve and returns the y position corresponding to the given x.
     * The coordinates are those of the curve, not of the widget.
     **/
    double evaluate(double x) const;
    boost::shared_ptr<Curve>  getInternalCurve() const
    {
        return _internalCurve;
    }

    void drawCurve(int curveIndex,int curvesCount);

signals:

    void curveChanged();

private:

    std::pair<KeyFrame,bool> nextPointForSegment(double x1,double* x2,const KeyFrameSet & keyframes);
    boost::shared_ptr<Curve> _internalCurve; ///ptr to the internal curve
    QString _name; /// the name of the curve
    QColor _color; /// the color that must be used to draw the curve
    int _thickness; /// its thickness
    bool _visible; /// should we draw this curve ?
    bool _selected; /// is this curve selected
    const CurveWidget* _curveWidget;
    KnobGui* _knob; //< ptr to the knob holding this curve
    int _dimension; //< which dimension is this curve representing
};


class QMenu;
class CurveWidgetPrivate;

class CurveWidget
    : public QGLWidget, public OverlaySupport
{
    friend class CurveGui;
    friend class CurveWidgetPrivate;

    Q_OBJECT

public:

    /*Pass a null timeline ptr if you don't want interaction with the global timeline. */
    CurveWidget(Gui* gui,
                boost::shared_ptr<TimeLine> timeline = boost::shared_ptr<TimeLine>(),
                QWidget* parent = NULL,
                const QGLWidget* shareWidget = NULL);

    virtual ~CurveWidget() OVERRIDE;

    const QFont & getTextFont() const;

    void centerOn(double xmin,double xmax,double ymin,double ymax);

    CurveGui * createCurve(boost::shared_ptr<Curve> curve,KnobGui* knob,int dimension, const QString &name);

    void removeCurve(CurveGui* curve);

    void centerOn(const std::vector<CurveGui*> & curves);

    void showCurvesAndHideOthers(const std::vector<CurveGui*> & curves);

    void getVisibleCurves(std::vector<CurveGui*>* curves) const;

    void addKeyFrame(CurveGui* curve, const KeyFrame & key);

    void setSelectedKeys(const SelectedKeys & keys);

    void refreshSelectedKeys();

    double getZoomFactor() const;

    void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomPAR) const;

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomPAR);

    /**
     * @brief Swap the OpenGL buffers.
     **/
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;

    /**
     * @brief Repaint
     **/
    virtual void redraw() OVERRIDE FINAL;

    /**
     * @brief Returns the width and height of the viewport in window coordinates.
     **/
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;

    /**
     * @brief Returns the pixel scale of the viewport.
     **/
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;

    /**
     * @brief Returns the colour of the background (i.e: clear color) of the viewport.
     **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;

public slots:

    void refreshDisplayedTangents();

    void exportCurveToAscii();

    void importCurveFromAscii();

    void deleteSelectedKeyFrames();

    void copySelectedKeyFrames();

    void pasteKeyFramesFromClipBoardToSelectedCurve();

    void selectAllKeyFrames();

    void constantInterpForSelectedKeyFrames();

    void linearInterpForSelectedKeyFrames();

    void smoothForSelectedKeyFrames();

    void catmullromInterpForSelectedKeyFrames();

    void cubicInterpForSelectedKeyFrames();

    void horizontalInterpForSelectedKeyFrames();

    void breakDerivativesForSelectedKeyFrames();

    void frameSelectedCurve();

    void refreshSelectedKeysBbox();

    void onTimeLineFrameChanged(SequenceTime time, int reason);

    void onTimeLineBoundariesChanged(SequenceTime left, SequenceTime right, int);

    void onCurveChanged();

    void onUpdateOnPenUpActionTriggered();

private:

    virtual void initializeGL() OVERRIDE FINAL;
    virtual void resizeGL(int width,int height) OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;

    void renderText(double x,double y,const QString & text,const QColor & color,const QFont & font) const;

    /**
     *@brief See toZoomCoordinates in ViewerGL.h
     **/
    QPointF toZoomCoordinates(double x, double y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y) const;

    const QColor & getSelectedCurveColor() const;
    const QFont & getFont() const;
    const SelectedKeys & getSelectedKeyFrames() const;

    bool isSupportingOpenGLVAO() const;

private:

    bool isTabVisible() const;

    boost::scoped_ptr<CurveWidgetPrivate> _imp;
};

Q_DECLARE_METATYPE(CurveWidget*)


class ImportExportCurveDialog
    : public QDialog
{
    Q_OBJECT

public:

    ImportExportCurveDialog(bool isExportDialog,
                            const std::vector<CurveGui*> & curves,
                            QWidget* parent = 0);

    virtual ~ImportExportCurveDialog()
    {
    }

    QString getFilePath();

    double getXStart() const;

    double getXIncrement() const;

    double getXEnd() const;

    void getCurveColumns(std::map<int,CurveGui*>* columns) const;

public slots:

    void open_file();

private:

    bool _isExportDialog;
    QVBoxLayout* _mainLayout;

    //////File
    QWidget* _fileContainer;
    QHBoxLayout* _fileLayout;
    QLabel* _fileLabel;
    LineEdit* _fileLineEdit;
    Button* _fileBrowseButton;

    //////x start value
    QWidget* _startContainer;
    QHBoxLayout* _startLayout;
    QLabel* _startLabel;
    SpinBox* _startSpinBox;

    //////x increment
    QWidget* _incrContainer;
    QHBoxLayout* _incrLayout;
    QLabel* _incrLabel;
    SpinBox* _incrSpinBox;

    //////x end value
    QWidget* _endContainer;
    QHBoxLayout* _endLayout;
    QLabel* _endLabel;
    SpinBox* _endSpinBox;


    /////Columns
    struct CurveColumn
    {
        CurveGui* _curve;
        QWidget* _curveContainer;
        QHBoxLayout* _curveLayout;
        QLabel* _curveLabel;
        SpinBox* _curveSpinBox;
    };

    std::vector< CurveColumn > _curveColumns;

    ////buttons
    QWidget* _buttonsContainer;
    QHBoxLayout* _buttonsLayout;
    Button* _okButton;
    Button* _cancelButton;
};

#endif // CURVE_WIDGET_H
