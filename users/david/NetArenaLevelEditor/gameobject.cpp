#include "gameobject.h"

#include "includeseditor.h"

#include "ed.h"


void ObjectPropertyWindow::CreateObjectPropertyWindow()
{
	currentObject = 0;

	m_w = new QWidget();

	QGridLayout* grid = new QGridLayout();

	text = new QTextEdit();
	grid->addWidget(text, 0, 0);

	QPushButton* b = new QPushButton;
	b->setText("Save");
	connect(b, SIGNAL(pressed()), this, SLOT(SaveToGameObject()));

	grid->addWidget(b, 1, 0);


	picker = new QColorDialog();
	picker->setOption(QColorDialog::ShowAlphaChannel);
	connect(picker, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(SetColor()));

	QPushButton* b2 = new QPushButton;
	b2->setText("Colour");
	connect(b2, SIGNAL(pressed()), this, SLOT(ShowColourPicker()));

	grid->addWidget(b2, 2, 0);

	m_w->setLayout(grid);

	m_w->setWindowTitle("Object Properties");
	m_w->show();
}

void ObjectPropertyWindow::SetColor()
{
	if(currentObject)
		currentObject->q = picker->currentColor();
	UpdateObjectText();
}

void ObjectPropertyWindow::SetCurrentGameObject(GameObject* object)
{
	text->setText(object->toText());
	currentObject = object;
}

void ObjectPropertyWindow::UpdateObjectText()
{
	if(currentObject)
		text->setText(currentObject->toText());
}

GameObject* ObjectPropertyWindow::GetCurrentGameObject()
{
	return currentObject;
}

void ObjectPropertyWindow::SaveToGameObject()
{
	if(currentObject)
	{
		currentObject->Load(text->toPlainText());
		text->setText(currentObject->toText());
	}
}

void ObjectPropertyWindow::ShowColourPicker()
{
	picker->show();
}


GameObject::GameObject()
{
	x = -1;
	y = -1;

	type = "none";
	q = Qt::black;

	texture = "";

	speed = -1;

	palletteTile = false;

	setFlags(ItemIsMovable | ItemSendsGeometryChanges);

	pivotx = 0;
	pivoty = 0;
}

GameObject::~GameObject()
{
}

void GameObject::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
	Ed::I().objectPropWindow->SetCurrentGameObject(this);

	e->accept();
}

void GameObject::SetPos(QPointF p)
{
	setPos(p);
}

typedef QPair<int, int> qp; //hack around compiler for foreach not being nice
void GameObject::Load(QString data)
{
	QList<QString> lines;

	QTextStream stream(&data);

	QList<QString> list;
	while(!stream.atEnd())
	{
		QString line = stream.readLine();
		list.push_back(line);
	}

	QMap<QString, QString> map;
	foreach(QString line, list)
	{
		QStringList list = line.split(":");
		map[*list.begin()] = list.back();
	}

	Load(map);
}


void GameObject::Load(QMap<QString, QString>& map)
{
	if(map["object"] != type) //prevent needless reloading of pivots
	{
		type = map["object"];

		QList<QString> list = GetLinesFromConfigFile(Ed::I().ObjectPath + type + ".txt");
		QStringList l2 = list.first().split(" ");
		if(l2.front() == "sheet")
		{
			l2.pop_front();
			pivotx = (l2.front().split(":").last()).toInt();
			pivoty = (l2.last().split(":").last()).toInt();
		}
	}

	if(map.contains("texture"))
		texture = map["texture"];

	setPixmap(*GetObjectPixmap(texture));

	if(map.contains("x"))
		x = map["x"].toInt() - pivotx;
	if(map.contains("y"))
		y = map["y"].toInt() - pivoty;

	setPos(x, y);

	if(map.contains("speed"))
		speed = map["speed"].toInt();


	if(map.contains("color"))
	{
		map["color"].push_front(map["color"][map["color"].size()-1]);
		map["color"].push_front(map["color"][map["color"].size()-2]);
		map["color"].push_front("#");
		map["color"].chop(2);

		q.setNamedColor(map["color"]);
	}
}

QString GameObject::toText()
{
	x = pos().x() + pivotx;
	y = pos().y() + pivoty;

	QString ret = "";
	if(type != "none")
	{ret += "object:" + type;						ret += "\n";}
	if(x >= 0)
	{ret += "x:" + QString::number(x);               ret += "\n";}
	if(y >= 0)
	{ret += "y:" + QString::number(y);               ret += "\n";}

	if(texture != "")
	{ret += "texture:" + texture; ret += "\n";}

	if(q != Qt::black)
	{
		ret += "color:";
		ret +=  QString::number(q.red(), 16);
		ret +=  QString::number(q.green(), 16);
		ret +=  QString::number(q.blue(), 16);
		ret +=  QString::number(q.alpha(), 16);		ret += "\n";
	}

	int i = 1;
	foreach(qp coord, path)
	{
		ret += "x" + QString::number(i) + ":" + QString::number(coord.first); ret += "\n";
		ret += "y" + QString::number(i) + ":" + QString::number(coord.second); ret += "\n";
		i++;
	}

	if(speed >= 0)
		ret += "speed:" + QString::number(speed);   ret += "\n";

	return ret;
}

QVariant GameObject::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if(change == ItemPositionChange)
		if(Ed::I().objectPropWindow && Ed::I().objectPropWindow->currentObject == this)
			Ed::I().objectPropWindow->UpdateObjectText();

	return QGraphicsPixmapItem::itemChange(change, value);
}
