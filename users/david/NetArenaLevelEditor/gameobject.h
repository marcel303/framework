#pragma once

#include "QGraphicsRectItem"

#include <QComboBox>
#include <QSlider>
#include <QLineEdit>
#include <QGroupBox>
#include <QRadioButton>
#include <QGridLayout>
#include <QPushButton>

class ObjectWidget : public QWidget
{
	Q_OBJECT

public:
	ObjectWidget();
	~ObjectWidget();

	QGroupBox* m_tps;

	QComboBox* m_type;
	QLineEdit* m_x;
	QLineEdit* m_y;
	QSlider* m_scaleSlider;

	//QGroupBox* m_transBox;
	//QGroupBox* m_templateControls;

	//QRadioButton *m_mech;
	//QRadioButton *m_coll;

	QGridLayout* m_grid;

	//QPushButton* m_editModeButton;
	//QPushButton* m_saveTemplateButton;

public slots:


};

class EditorObject : public QGraphicsRectItem //resizable super rectangle deluxe
{

	EditorObject();
	virtual ~EditorObject();


	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
	virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * e );
	virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e );

	virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);




	int m_type;
	int m_state;

	int GetX();
	int GetY();



};
