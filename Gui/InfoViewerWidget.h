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

#ifndef NATRON_GUI_INFOVIEWERWIDGET_H_
#define NATRON_GUI_INFOVIEWERWIDGET_H_

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
#include <QtCore/QPoint>
#include <QtGui/QVector4D>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Format.h"

class ViewerGL;
class QLabel;
class QHBoxLayout;

class InfoViewerWidget
    : public QWidget
{
    Q_OBJECT

public:
    explicit InfoViewerWidget(ViewerGL* v,
                              const QString & description,
                              QWidget* parent = 0);
    virtual ~InfoViewerWidget() OVERRIDE;


    bool colorAndMouseVisible();

    void setResolution(const Format & f);

    void setDataWindow(const RectD & r); // in canonical coordinates

    void setImageFormat(Natron::ImageComponentsEnum comp,Natron::ImageBitDepthEnum depth);

    void setColor(float r,float g,float b,float a);

    void setMousePos(QPoint p);

public slots:

    void hideColorAndMouseInfo();
    void showColorAndMouseInfo();
    void setFps(double actualFps,double desiredFps);
    void hideFps();

private:
    
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;
    virtual void paintEvent(QPaintEvent* e) OVERRIDE FINAL;
    
private:
    

    QHBoxLayout* layout;
    QLabel* descriptionLabel;
    QLabel* imageFormat;
    QLabel* resolution;
    Format format;
    QLabel* coordDispWindow;
    QLabel* coordMouse;
    QLabel* rectUserLabel;
    QLabel* rgbaValues;
    QLabel* color;
    QLabel* hvl_lastOption;
    QLabel* _fpsLabel;
    ViewerGL* viewer;
    Natron::ImageComponentsEnum _comp;
};

#endif /* defined(NATRON_GUI_INFOVIEWERWIDGET_H_) */
