#pragma once

#include "main.h"

class QTextEdit;
class QColorDialog;
class GameObject;
class ObjectPropertyWindow : public QObject
{
	Q_OBJECT
public:
	ObjectPropertyWindow(){}
	virtual ~ObjectPropertyWindow(){}

	void CreateObjectPropertyWindow();

	void SetCurrentGameObject(GameObject* object);

	void UpdateObjectText();


	GameObject* GetCurrentGameObject();

	QWidget* m_w;
	QTextEdit* text;

	GameObject* currentObject;
	QColorDialog* picker;


public slots:
	void SaveToGameObject();
	void ShowColourPicker();
	void SetColor();

};

class GameObject : public QGraphicsPixmapItem
{
public:
	GameObject();
	virtual ~GameObject();

	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );

	void Load(QString data);
	void Load(QMap<QString, QString>& data);

	virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

	void SetPos(QPointF pos);

	int x;
	int y;

	QColor q;

	int speed;

	int pivotx;
	int pivoty;


	QString type;
	QString texture;
	QString toText();

	QList<QPair<int, int> > path;

	bool palletteTile; //is this meant for ingame or used for pallette
};
