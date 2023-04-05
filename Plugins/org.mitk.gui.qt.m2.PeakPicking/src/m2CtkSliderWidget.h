/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes.

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or https://www.github.com/jtfcordes/m2aia for details.

===================================================================*/

#pragma once

#include <QString>
#include <QWidget>

#include <ctkSliderWidget.h>
#include <ctkDoubleSlider.h>

class m2CtkSliderWidget : public ctkSliderWidget{
Q_OBJECT
public:
    
    explicit m2CtkSliderWidget(QWidget* parent = 0):ctkSliderWidget(parent){};
    void setOrientation(Qt::Orientation o){
        slider()->setOrientation(o);
    }
};