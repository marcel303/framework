#pragma once

#include <QWidget>

class QCheckBox;
class SettingsWidget : public QWidget
{
public:
    SettingsWidget();
    ~SettingsWidget();


    void Initialize();









    QCheckBox* mech;
    QCheckBox* art;
    QCheckBox* coll;
    QCheckBox* object;
    QCheckBox* tmpl;

};
