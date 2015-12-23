#include "SettingsWidget.h"
#include "includeseditor.h"

#include "QGroupBox"
#include "QRadioButton"
#include "QSlider"
#include "QVBoxLayout"
#include "QTextEdit"

SettingsWidget::SettingsWidget(QWidget *parent) : QWidget(parent)
{
	m_editT = false;
}

SettingsWidget::~SettingsWidget()
{
}

#include "templates.h"
#include <QListView>
void SettingsWidget::Create()
{
	m_layerBox = new QGroupBox();
	m_transBox = new QGroupBox();
	m_templateControls = new QGroupBox();

	m_mech = new QRadioButton(tr("&Mechanical"));
	m_coll = new QRadioButton(tr("&Collission"));
	m_temp = new QRadioButton(tr("&Template"));
	m_obj = new QRadioButton(tr("&Object"));

	m_mech->setChecked(true);


	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(m_mech);
	vbox->addWidget(m_coll);
	vbox->addWidget(m_temp);
	vbox->addWidget(m_obj);
	vbox->addStretch(1);

	m_layerBox->setLayout(vbox);


	m_mechSlider = new QSlider(Qt::Horizontal);
	m_collSlider = new QSlider(Qt::Horizontal);
	m_foreSlider = new QSlider(Qt::Horizontal);
	m_middleSlider = new QSlider(Qt::Horizontal);
	m_backSlider = new QSlider(Qt::Horizontal);
	m_objSlider = new QSlider(Qt::Horizontal);

	QGridLayout* gbox = new QGridLayout;
	QLabel* lbl = new QLabel("mechanics");
	gbox->addWidget(lbl, 0, 0);
	lbl = new QLabel("collission");
	gbox->addWidget(lbl, 1, 0);
	lbl = new QLabel("foreground");
	gbox->addWidget(lbl, 2, 0);
	lbl = new QLabel("middle");
	gbox->addWidget(lbl, 3, 0);
	lbl = new QLabel("background");
	gbox->addWidget(lbl, 4, 0);
	lbl = new QLabel("objects");

	gbox->addWidget(lbl, 5, 0);
	gbox->addWidget(m_mechSlider, 0, 1);
	gbox->addWidget(m_collSlider, 1, 1);
	gbox->addWidget(m_foreSlider, 2, 1);
	gbox->addWidget(m_middleSlider, 3, 1);
	gbox->addWidget(m_backSlider, 4, 1);
	gbox->addWidget(m_objSlider, 5, 1);
	m_transBox->setLayout(gbox);


	vbox = new QVBoxLayout;
	QPushButton* button = new QPushButton("New Template");
    connect(button, SIGNAL(clicked()), this, SLOT(NewTemplate()));
	vbox->addWidget(button);

	m_editModeButton = new QPushButton("Edit Level");
	connect(m_editModeButton, SIGNAL(clicked()), this, SLOT(EditTemplate()));
	vbox->addWidget(m_editModeButton);

	m_saveTemplateButton = new QPushButton("Save Template");
	connect(m_saveTemplateButton, SIGNAL(clicked()), this, SLOT(SaveTemplate()));
	vbox->addWidget(m_saveTemplateButton);

	m_templateControls->setLayout(vbox);

	m_objectText = new QTextEdit();

	m_grid = new QGridLayout;
	m_grid->addWidget(m_layerBox, 0, 0);
	m_grid->addWidget(m_transBox, 0, 1);
	m_grid->addWidget(m_templateControls, 1, 0);
	m_grid->addWidget(m_objectText, 1, 1);



	setLayout(m_grid);

	connect(m_mech, SIGNAL(toggled(bool)), this, SLOT(UpdatePallettes(bool)));
	connect(m_coll, SIGNAL(toggled(bool)), this, SLOT(UpdatePallettes(bool)));
	connect(m_temp, SIGNAL(toggled(bool)), this, SLOT(UpdatePallettes(bool)));
	connect(m_obj, SIGNAL(toggled(bool)), this, SLOT(UpdatePallettes(bool)));

	connect(m_mechSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateTransparancy(int)));
	connect(m_collSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateTransparancy(int)));
	connect(m_foreSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateTransparancy(int)));
	connect(m_middleSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateTransparancy(int)));
	connect(m_backSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateTransparancy(int)));
	connect(m_objSlider, SIGNAL(valueChanged(int)), this, SLOT(UpdateTransparancy(int)));

	m_editT = true;
	EditTemplate();
}

void SettingsWidget::UpdatePallettes(bool s)
{
	Q_UNUSED(s);

	ed.SetCurrentPallette();
}

void SettingsWidget::UpdateTransparancy(int s)
{
	Q_UNUSED(s);

	ed.UpdateTransparancy();
}

#include "templates.h"
void SettingsWidget::NewTemplate()
{
    Template* t = new Template();

	if(t->CreateNewTemplate())
	{
		ed.m_templateScene->m_currentFolder->AddTemplate(t);
		ed.m_templateScene->m_currentFolder->SetCurrentTemplate(t->m_name);

		m_editT = false;
		EditTemplate();
	}
}

#include "grid.h"
void SettingsWidget::EditTemplate()
{
	QPalette pal = m_editModeButton->palette();

	if(m_editT)
    {
		m_editModeButton->setText("Edit Template");
		pal.setColor(QPalette::Button, QColor(Qt::blue));

		m_saveTemplateButton->hide();

		ed.ReturnToLevel();
    }
    else
    {
		m_editModeButton->setText("Edit Level");
		pal.setColor(QPalette::Button, QColor(Qt::green));

		m_saveTemplateButton->show();

		ed.SetCurrentTemplate(ed.m_templateScene->m_currentFolder->m_currentTemplate);
    }
	m_editT = !m_editT;

	m_editModeButton->setFlat(true);
	m_editModeButton->setAutoFillBackground(true);
	m_editModeButton->setPalette(pal);
	m_editModeButton->update();

	ed.m_grid->update();
}

void SettingsWidget::SaveTemplate()
{
	ed.m_currentTarget->Save();
}


