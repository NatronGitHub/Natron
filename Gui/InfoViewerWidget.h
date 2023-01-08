/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef NATRON_GUI_INFOVIEWERWIDGET_H
#define NATRON_GUI_INFOVIEWERWIDGET_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QWidget>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtGui/QVector4D>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Format.h"
#include "Engine/ImagePlaneDesc.h"

#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

class InfoViewerWidget
    : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    explicit InfoViewerWidget(const QString & description,
                              QWidget* parent = 0);
    virtual ~InfoViewerWidget() OVERRIDE;


    bool colorVisible();
    bool mouseVisible();

    void setResolution(const QString& name);

    void setDataWindow(const RectI & r); // in pixel coordinates

    void setImageFormat(const ImagePlaneDesc& comp, ImageBitDepthEnum depth);

    void setColor(float r, float g, float b, float a);

    void setMousePos(QPoint p);


public Q_SLOTS:


    void setColorValid(bool valid);

    void setColorApproximated(bool approx);

    void hideColorInfo();
    void showColorInfo();
    void hideMouseInfo();
    void showMouseInfo();
    void setFps(double actualFps, double desiredFps);
    void hideFps();

private:

    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual QSize minimumSizeHint() const OVERRIDE FINAL;

private:


    QHBoxLayout* layout;
    Label* descriptionLabel;
    Label* imageFormat;
    Label* resolution;
    Label* coordDispWindow;
    Label* coordMouse;
    Label* rgbaValues;
    Label* color;
    Label* hvl_lastOption;
    Label* _fpsLabel;
    ImagePlaneDesc _comp;
    bool _colorValid;
    bool _colorApprox;
    double currentColor[4];
};

NATRON_NAMESPACE_EXIT

#endif /* defined(NATRON_GUI_INFOVIEWERWIDGET_H_) */
