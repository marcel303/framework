#pragma once

#include <QWidget>

class QCheckBox;
class QRadioButton;
class QGroupBox;
class QSlider;
class QTextEdit;
class QGridLayout;
class QPushButton;
class SettingsWidget : public QWidget
{
	Q_OBJECT

public:
	SettingsWidget(QWidget *parent = 0);
    ~SettingsWidget();

	void Create();




	QGroupBox* m_layerBox;
	QGroupBox* m_transBox;
	QGroupBox* m_templateControls;

	QRadioButton *m_mech;
	QRadioButton *m_coll;
	QRadioButton *m_temp;
	QRadioButton *m_obj;

	QSlider* m_mechSlider;
	QSlider* m_collSlider;
	QSlider* m_foreSlider;
	QSlider* m_middleSlider;
	QSlider* m_backSlider;
	QSlider* m_objSlider;

	QTextEdit* m_objectText;
	QGridLayout* m_grid;

	QPushButton* m_editModeButton;
	QPushButton* m_saveTemplateButton;

	bool m_editT;

public slots:
	void UpdatePallettes(bool s);
	void UpdateTransparancy(int s);
	void NewTemplate();
	void EditTemplate();
	void SaveTemplate();
};
