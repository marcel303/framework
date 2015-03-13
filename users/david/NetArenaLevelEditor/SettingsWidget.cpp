#include "SettingsWidget.h"

#include "includeseditor.h"

#include "EditorView.h"

#define view view2 //hack

SettingsWidget::SettingsWidget()
{
}

SettingsWidget::~SettingsWidget()
{
}

void SettingsWidget::Initialize() //CreateSettingsWidget()
{
    QGridLayout* grid = new QGridLayout();

    QLabel* label = new QLabel("Edit Layer");
    grid->addWidget(label, 0, 1);

    label = new QLabel("Mech");
    grid->addWidget(label, 1, 0);
    label = new QLabel("Art");
    grid->addWidget(label, 2, 0);
    label = new QLabel("Coll");
    grid->addWidget(label, 3, 0);
    label = new QLabel("Obj");
    grid->addWidget(label, 4, 0);
    label = new QLabel("Tmpl");
    grid->addWidget(label, 5, 0);


    QButtonGroup* bgroup = new QButtonGroup();
    mech = new QCheckBox();
    QObject::connect(mech, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToMech()));
    grid->addWidget(mech, 1, 1);
    bgroup->addButton(mech);

    art = new QCheckBox();
    QObject::connect(art, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToArt()));
    grid->addWidget(art, 2, 1);
    bgroup->addButton(art);

    coll= new QCheckBox();
    QObject::connect(coll, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToCollission()));
    grid->addWidget(coll, 3, 1);
    bgroup->addButton(coll);

    object = new QCheckBox();
    QObject::connect(object, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToObject()));
    grid->addWidget(object, 4, 1);
    bgroup->addButton(object);

    tmpl = new QCheckBox();
    QObject::connect(tmpl, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToTemplates()));
    grid->addWidget(tmpl, 5, 1);
    bgroup->addButton(tmpl);

    bgroup->setExclusive(true);

    label = new QLabel("Transparency");
    grid->addWidget(label, 0, 2);

    view->sliderOpacMech = new QSlider(Qt::Horizontal);
    view->sliderOpacMech->setMinimum(0);
    view->sliderOpacMech->setMaximum(100);
    view->sliderOpacMech->setTickInterval(1);
    view->sliderOpacMech->setValue(100);

    view->sliderOpacArt = new QSlider(view->sliderOpacMech);
    view->sliderOpacArt->setOrientation(Qt::Horizontal);
    view->sliderOpacColl = new QSlider(view->sliderOpacMech);
    view->sliderOpacColl->setOrientation(Qt::Horizontal);
    view->sliderOpacObject = new QSlider(view->sliderOpacMech);
    view->sliderOpacObject->setOrientation(Qt::Horizontal);

    QObject::connect(view->sliderOpacMech, SIGNAL(valueChanged(int)),
            view, SLOT(SetOpacityMech(int)));
    QObject::connect(view->sliderOpacArt, SIGNAL(valueChanged(int)),
            view, SLOT(SetOpacityArt(int)));
    QObject::connect(view->sliderOpacColl, SIGNAL(valueChanged(int)),
            view, SLOT(SetOpacityCollission(int)));
    QObject::connect(view->sliderOpacObject, SIGNAL(valueChanged(int)),
            view, SLOT(SetOpacityObject(int)));

    grid->addWidget(view->sliderOpacMech, 1, 2);
    grid->addWidget(view->sliderOpacArt, 2, 2);
    grid->addWidget(view->sliderOpacColl, 3, 2);
    grid->addWidget(view->sliderOpacObject, 4, 2);

    setLayout(grid);
}

