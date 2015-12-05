#include "includeseditor.h"
#include "ed.h"
#include "EditorView.h"
#include "SettingsWidget.h"
#include "grid.h"
#include "layers.h"
#include "pallettes.h"
#include"templates.h"

#include <QGridLayout>
#include <QImage>

QSplitter* hlayout;
QVBoxLayout* vlayout;
QGridLayout* grid;

QString ArtFolderPath;

bool placeTemplate = false;


#include <QSplashScreen>
int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    QPixmap pixmap("splash.jpg");
    QSplashScreen splash(pixmap);
    splash.show();

    a.processEvents();

	ed.Initialize();
	ed.LoadPallettes();
	ed.m_grid->CreateGrid(BASEX, BASEY);


    ed.NewLevel();

	ed.m_grid->SetCurrentLevel(ed.m_level);

	grid = new QGridLayout();
	vlayout = new QVBoxLayout();
	hlayout = new QSplitter();

	QWidget rightside;

	view2 = new EditorView();
	view2->setMinimumWidth(1200);
	view2->setScene(ed.m_grid);


	viewPallette = new EditorViewBasic;
	viewPallette->setMinimumWidth(320);
	viewPallette->setDragMode(QGraphicsView::ScrollHandDrag);
	viewPallette->setScene(ed.GetCurrentPallette());

	viewPallette->resize(430, 600);

	rightside.setLayout(vlayout);
	hlayout->addWidget(view2);
	hlayout->addWidget(&rightside);
	vlayout->addWidget(viewPallette);
	vlayout->addWidget(ed.GetSettingsWidget());



    view2->showMaximized();
    viewPallette->showNormal();
    hlayout->showMaximized();


    splash.close();

	return a.exec();
}
