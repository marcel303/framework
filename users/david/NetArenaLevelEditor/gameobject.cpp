#include "gameobject.h"

#include <QVBoxLayout>


ObjectWidget::ObjectWidget()
{
	QGroupBox* m_tps;

	QComboBox* m_type;
	QLineEdit* m_x;
	QLineEdit* m_y;
	QSlider* m_scaleSlider;

	QGridLayout* m_grid;


	m_tps = new QGroupBox();

	//m_mech = new QRadioButton(tr("&Mechanical"));
	//m_coll = new QRadioButton(tr("&Collission"));
	//m_mech->setChecked(true);


	/QVBoxLayout *vbox = new QVBoxLayout;
	//vbox->addWidget(m_mech);
	//vbox->addWidget(m_coll);
	//vbox->addStretch(1);

	m_tps->setLayout(vbox);


	m_scaleSlider = new QSlider(Qt::Horizontal);

	QGridLayout* gbox = new QGridLayout;

}





ObjectWidget::~ObjectWidget()
{
}
















EditorObject::EditorObject()
{
}


EditorObject::~EditorObject()
{
}



void EditorObject::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{


}

void EditorObject::mouseReleaseEvent ( QGraphicsSceneMouseEvent * e )
{

}

void EditorObject::hoverEnterEvent ( QGraphicsSceneHoverEvent * e )
{

}


QRectF EditorObject::boundingRect() const
{
	return QRectF();
}


void EditorObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
}


