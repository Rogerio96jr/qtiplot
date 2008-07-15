/***************************************************************************
    File                 : MultiLayer.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief,
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
                           knut.franke*gmx.de
    Description          : Multi layer widget

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef MULTILAYER_H
#define MULTILAYER_H

#include "../MdiSubWindow.h"
#include "Graph.h"
#include <QPushButton>
#include <QLayout>
#include <QPointer>

class QLabel;
class LayerButton;
class SelectionMoveResizer;
class LegendWidget;

/**
 * \brief An MDI window (MdiSubWindow) managing one or more Graph objects.
 *
 * %Note that several parts of the code, as well as the user interface, refer to MultiLayer as "graph" or "plot",
 * practically guaranteeing confusion with the classes Graph and Plot.
 *
 * \section future Future Plans
 * Manage any QWidget instead of only Graph.
 * This would allow 3D graphs to be added as well, so you could produce mixed 2D/3D arrangements.
 * It would also allow text labels to be added directly instead of having to complicate things by wrapping them
 * up in a Graph (see documentation of ImageMarker for details) (see documentation of ImageMarker for details).
 *
 * The main problem to be figured out for this is how Graph would interface with the rest of the project.
 * A possible solution is outlined in the documentation of ApplicationWindow:
 * If MultiLayer exposes its parent Project to the widgets it manages, they could handle things like creating
 * tables by calling methods of Project instead of sending signals.
 */
class MultiLayer: public MdiSubWindow
{
	Q_OBJECT

public:
    MultiLayer (ApplicationWindow* parent = 0, int layers = 1, int rows = 1, int cols = 1, const QString& label = "", const char* name=0, Qt::WFlags f=0);
	~MultiLayer();

	QList<Graph *> layersList(){return graphsList;};
	Graph *layer(int num);
	int layerIndex(Graph *g){return graphsList.indexOf(g);};

    int numLayers(){return graphsList.size();};
    void setNumLayers(int n);

	void copy(MultiLayer* ml);

	enum HorAlignement{HCenter, Left, Right};
	enum VertAlignement{VCenter, Top, Bottom};

	bool scaleLayersOnPrint(){return d_scale_on_print;};
	void setScaleLayersOnPrint(bool on){d_scale_on_print = on;};

	bool printCropmarksEnabled(){return d_print_cropmarks;};
	void printCropmarks(bool on){d_print_cropmarks = on;};

	bool scaleLayersOnResize(){return d_scale_layers;};
	void setScaleLayersOnResize(bool ok){d_scale_layers = ok;};

	QWidget *canvas(){return d_canvas;};
	QRect canvasRect(){return d_canvas->rect();};
	QRect canvasChildrenRect();
	virtual QString sizeToString();

public slots:
	Graph* addLayer(int x = 0, int y = 0, int width = 0, int height = 0);

	bool isEmpty();
    bool removeLayer(Graph *g);
    bool removeActiveLayer();
	void confirmRemoveLayer();

	Graph* activeLayer(){return active_graph;};
	void setActiveLayer(Graph* g);
	void activateGraph(LayerButton* button);

    //! Returns the layer at the given position; returns 0 if there is no such layer.
	Graph* layerAt(const QPoint& pos);
	void setGraphGeometry(int x, int y, int w, int h);

	void findBestLayout(int &rows, int &cols);

	QSize arrangeLayers(bool userSize);
	void arrangeLayers(bool fit, bool userSize);
	bool swapLayers(int src, int dest);
    void adjustSize();

	int getRows(){return d_rows;};
	void setRows(int r);

	int getCols(){return d_cols;};
	void setCols(int c);

	int colsSpacing(){return colsSpace;};
	int rowsSpacing(){return rowsSpace;};
	void setSpacing (int rgap, int cgap);

	int leftMargin(){return left_margin;};
	int rightMargin(){return right_margin;};
	int topMargin(){return top_margin;};
	int bottomMargin(){return bottom_margin;};
	void setMargins (int lm, int rm, int tm, int bm);

	QSize layerCanvasSize(){return QSize(l_canvas_width, l_canvas_height);};
	void setLayerCanvasSize (int w, int h);

	int horizontalAlignement(){return hor_align;};
	int verticalAlignement(){return vert_align;};
	void setAlignement (int ha, int va);

	//! \name Print and Export
	//@{
	QPixmap canvasPixmap();
	void exportToFile(const QString& fileName);
	void exportImage(const QString& fileName, int quality = 100, bool transparent = false);
	void exportSVG(const QString& fname);
    void exportPDF(const QString& fname);
	void exportVector(const QString& fileName, int res = 0, bool color = true);

	void copyAllLayers();
	void print();
	void printAllLayers(QPainter *painter);
	void printActiveLayer();
	//@}

	void setFonts(const QFont& titleFnt, const QFont& scaleFnt,
							const QFont& numbersFnt, const QFont& legendFnt);

	void connectLayer(Graph *g);

	void save(const QString& fn, const QString& geometry, bool = false);

    bool hasSelectedLayers();

signals:
	void showEnrichementDialog();
	void showPlotDialog(int);
	void showAxisDialog(int);
	void showScaleDialog(int);
	void showGraphContextMenu();
	void showCurveContextMenu(QwtPlotCurve *);
	void showCurvesDialog();
	void drawLineEnded(bool);
	void showAxisTitleDialog();
	void showMarkerPopupMenu();
	void modifiedPlot();
	void cursorInfo(const QString&);
	void showLineDialog();
	void viewTitleDialog();
	void createTable(const QString&,int,int,const QString&);
	void pasteMarker();
	void setPointerCursor();
	void currentFontChanged(const QFont&);
    void enableTextEditor(Graph *);

private:
	//! \name Event Handlers
	//@{
	void wheelEvent(QWheelEvent *);
	void keyPressEvent(QKeyEvent *);
	bool eventFilter(QObject *object, QEvent *);
	void releaseLayer();
	void resizeLayers(QResizeEvent *);
	//@}

	LayerButton* addLayerButton();

	Graph* active_graph;
	//! Used for resizing of layers.
	int d_cols, d_rows, graph_width, graph_height, colsSpace, rowsSpace;
	int left_margin, right_margin, top_margin, bottom_margin;
	int l_canvas_width, l_canvas_height, hor_align, vert_align;
	bool d_scale_on_print, d_print_cropmarks;
    //! Flag telling if layers should be rescaled on the plot window is resized by the user.
	bool d_scale_layers;

    QList<LayerButton *> buttonsList;
    QList<Graph *> graphsList;
	QHBoxLayout *layerButtonsBox;
    QWidget *d_canvas;

	QPointer<SelectionMoveResizer> d_layers_selector;
};

//! Button with layer number
class LayerButton: public QPushButton
{
	Q_OBJECT

public:
    LayerButton (const QString& text = QString::null, QWidget* parent = 0);
	static int btnSize(){return 20;};

protected:
	void mousePressEvent( QMouseEvent * );
	void mouseDoubleClickEvent ( QMouseEvent * );

signals:
	void showCurvesDialog();
	void clicked(LayerButton*);
};

#endif
