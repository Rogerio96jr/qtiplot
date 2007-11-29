/***************************************************************************
	File                 : ApplicationWindow.cpp
	Project              : QtiPlot
--------------------------------------------------------------------
	Copyright            : (C) 2006 by Ion Vasilief,
	                       Tilman Hoener zu Siederdissen,
                          Knut Franke
	Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net,
	                       knut.franke*gmx.de
	Description          : QtiPlot's main window

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
#include "globals.h"
#include "ApplicationWindow.h"
#include "pixmaps.h"
#include "CurvesDialog.h"
#include "PlotDialog.h"
#include "AxesDialog.h"
#include "LineDialog.h"
#include "TextDialog.h"
#include "ExportDialog.h"
#include "TableDialog.h"
#include "SetColValuesDialog.h"
#include "ErrDialog.h"
#include "LegendWidget.h"
#include "ArrowMarker.h"
#include "ImageMarker.h"
#include "Graph.h"
#include "Plot.h"
#include "Grid.h"
#include "PlotWizard.h"
#include "PolynomFitDialog.h"
#include "ExpDecayDialog.h"
#include "FunctionDialog.h"
#include "FitDialog.h"
#include "SurfaceDialog.h"
#include "Graph3D.h"
#include "Plot3DDialog.h"
#include "ImageDialog.h"
#include "MultiLayer.h"
#include "LayerDialog.h"
#include "DataSetDialog.h"
#include "IntDialog.h"
#include "ConfigDialog.h"
#include "MatrixDialog.h"
#include "MatrixSizeDialog.h"
#include "MatrixValuesDialog.h"
#include "importOPJ.h"
#include "AssociationsDialog.h"
#include "RenameWindowDialog.h"
#include "QwtErrorPlotCurve.h"
#include "InterpolationDialog.h"
#include "ImportASCIIDialog.h"
#include "ImageExportDialog.h"
#include "SmoothCurveDialog.h"
#include "FilterDialog.h"
#include "FFTDialog.h"
#include "Note.h"
#include "Folder.h"
#include "FindDialog.h"
#include "ScaleDraw.h"
#include "ScriptingLangDialog.h"
#include "ScriptWindow.h"
#include "TableStatistics.h"
#include "Fit.h"
#include "MultiPeakFit.h"
#include "PolynomialFit.h"
#include "SigmoidalFit.h"
#include "LogisticFit.h"
#include "NonLinearFit.h"
#include "FunctionCurve.h"
#include "QwtPieCurve.h"
#include "Spectrogram.h"
#include "Integration.h"
#include "Differentiation.h"
#include "SmoothFilter.h"
#include "FFTFilter.h"
#include "Convolution.h"
#include "Correlation.h"
#include "CurveRangeDialog.h"
#include "ColorBox.h"
#include "QwtHistogram.h"
#include "OpenProjectDialog.h"
#include "ColorMapDialog.h"
#include "TextEditor.h"
#include "SymbolDialog.h"
#include "CustomActionDialog.h"

// TODO: move tool-specific code to an extension manager
#include "ScreenPickerTool.h"
#include "DataPickerTool.h"
#include "TranslateCurveTool.h"
#include "MultiPeakFitTool.h"
#include "LineProfileTool.h"
#include "RangeSelectorTool.h"
#include "PlotToolInterface.h"

#include <stdio.h>
#include <stdlib.h>

#include <qwt_scale_engine.h>
#include <q3listview.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QProgressDialog>
#include <QPrintDialog>
#include <QPixmapCache>
#include <QMenuBar>
#include <QClipboard>
#include <QWorkspace>
#include <QTranslator>
#include <QSplitter>
#include <QSettings>
#include <QApplication>
#include <QMessageBox>
#include <QPrinter>
#include <QActionGroup>
#include <QAction>
#include <QToolBar>
#include <QKeySequence>
#include <QImageReader>
#include <QImageWriter>
#include <QDateTime>
#include <QShortcut>
#include <QDockWidget>
#include <QTextStream>
#include <QVarLengthArray>
#include <QList>
#include <QUrl>
#include <QAssistantClient>
#include <QFontComboBox>
#include <QSpinBox>

#include <zlib.h>

#include <iostream>

using namespace Qwt3D;

extern "C"
{
void file_compress(char  *file, char  *mode);
void file_uncompress(char  *file);
}

ApplicationWindow::ApplicationWindow(bool factorySettings)
: QMainWindow(), scripted(ScriptingLangManager::newEnv(this))
{
	setAttribute(Qt::WA_DeleteOnClose);
	init(factorySettings);
}

void ApplicationWindow::init(bool factorySettings)
{
	setWindowTitle(tr("QtiPlot - untitled"));
	initGlobalConstants();
	QPixmapCache::setCacheLimit(20*QPixmapCache::cacheLimit ());

	tablesDepend = new QMenu(this);

	explorerWindow = new QDockWidget( this );
	explorerWindow->setWindowTitle(tr("Project Explorer"));
	explorerWindow->setObjectName("explorerWindow"); // this is needed for QMainWindow::restoreState()
	explorerWindow->setMinimumHeight(150);
	addDockWidget( Qt::BottomDockWidgetArea, explorerWindow );

	folders = new FolderListView();
	folders->header()->setClickEnabled( false );
	folders->addColumn( tr("Folder") );
	folders->setRootIsDecorated( true );
	folders->setResizeMode(Q3ListView::LastColumn);
	folders->header()->hide();
	folders->setSelectionMode(Q3ListView::Single);

	connect(folders, SIGNAL(currentChanged(Q3ListViewItem *)),
			this, SLOT(folderItemChanged(Q3ListViewItem *)));
	connect(folders, SIGNAL(itemRenamed(Q3ListViewItem *, int, const QString &)),
			this, SLOT(renameFolder(Q3ListViewItem *, int, const QString &)));
	connect(folders, SIGNAL(contextMenuRequested(Q3ListViewItem *, const QPoint &, int)),
			this, SLOT(showFolderPopupMenu(Q3ListViewItem *, const QPoint &, int)));
	connect(folders, SIGNAL(dragItems(QList<Q3ListViewItem *>)),
			this, SLOT(dragFolderItems(QList<Q3ListViewItem *>)));
	connect(folders, SIGNAL(dropItems(Q3ListViewItem *)),
			this, SLOT(dropFolderItems(Q3ListViewItem *)));
	connect(folders, SIGNAL(renameItem(Q3ListViewItem *)),
			this, SLOT(startRenameFolder(Q3ListViewItem *)));
	connect(folders, SIGNAL(addFolderItem()), this, SLOT(addFolder()));
	connect(folders, SIGNAL(deleteSelection()), this, SLOT(deleteSelectedItems()));

	current_folder = new Folder( 0, tr("UNTITLED"));
	FolderListItem *fli = new FolderListItem(folders, current_folder);
	current_folder->setFolderListItem(fli);
	fli->setOpen( true );

	lv = new FolderListView();
	lv->addColumn (tr("Name"),-1 );
	lv->addColumn (tr("Type"),-1 );
	lv->addColumn (tr("View"),-1 );
	lv->addColumn (tr("Size"),-1 );
	lv->addColumn (tr("Created"),-1);
	lv->addColumn (tr("Label"),-1);
	lv->setResizeMode(Q3ListView::LastColumn);
	lv->setMinimumHeight(80);
	lv->setSelectionMode(Q3ListView::Extended);
	lv->setDefaultRenameAction (Q3ListView::Accept);

	explorerSplitter = new QSplitter(Qt::Horizontal, explorerWindow);
	explorerSplitter->addWidget(folders);
	explorerSplitter->addWidget(lv);
	explorerWindow->setWidget(explorerSplitter);

	QList<int> splitterSizes;
	explorerSplitter->setSizes( splitterSizes << 45 << 45);
	explorerWindow->hide();

	logWindow = new QDockWidget(this);
	logWindow->setObjectName("logWindow"); // this is needed for QMainWindow::restoreState()
	logWindow->setWindowTitle(tr("Results Log"));
	addDockWidget( Qt::TopDockWidgetArea, logWindow );

	results=new QTextEdit(logWindow);
	results->setReadOnly (true);

	logWindow->setWidget(results);
	logWindow->hide();

#ifdef SCRIPTING_CONSOLE
	consoleWindow = new QDockWidget(this);
	consoleWindow->setObjectName("consoleWindow"); // this is needed for QMainWindow::restoreState()
	consoleWindow->setWindowTitle(tr("Scripting Console"));
	addDockWidget( Qt::TopDockWidgetArea, consoleWindow );
	console = new QTextEdit(consoleWindow);
	console->setReadOnly(true);
	consoleWindow->setWidget(console);
	consoleWindow->hide();
#endif

	// Needs to be done after initialization of dock windows,
	// because we now use QDockWidget::toggleViewAction()
	createActions();
	initToolBars();
	initMainMenu();

	ws = new QWorkspace( this );
	ws->setScrollBarsEnabled (true);
	setCentralWidget( ws );
	setAcceptDrops(true);

	hiddenWindows = new QList<QWidget*>();
	outWindows = new QList<QWidget*>();

	scriptWindow = 0;
    d_text_editor = NULL;

	renamedTables = QStringList();
	if (!factorySettings)
		readSettings();
	createLanguagesList();
	insertTranslatedStrings();
	disableToolbars();

	assistant = new QAssistantClient( QString(), this );

	actionNextWindow = new QAction(QIcon(QPixmap(next_xpm)), tr("&Next","next window"), this);
	actionNextWindow->setShortcut( tr("F5","next window shortcut") );
	connect(actionNextWindow, SIGNAL(activated()), ws, SLOT(activateNextWindow()));

	actionPrevWindow = new QAction(QIcon(QPixmap(prev_xpm)), tr("&Previous","previous window"), this);
	actionPrevWindow->setShortcut( tr("F6","previous window shortcut") );
	connect(actionPrevWindow, SIGNAL(activated()), ws, SLOT(activatePreviousWindow()));

	connect(tablesDepend, SIGNAL(activated(int)), this, SLOT(showTable(int)));

	connect(this, SIGNAL(modified()),this, SLOT(modifiedProject()));
	connect(ws, SIGNAL(windowActivated (QWidget*)),this, SLOT(windowActivated(QWidget*)));
	connect(lv, SIGNAL(doubleClicked(Q3ListViewItem *)),
			this, SLOT(maximizeWindow(Q3ListViewItem *)));
	connect(lv, SIGNAL(doubleClicked(Q3ListViewItem *)),
			this, SLOT(folderItemDoubleClicked(Q3ListViewItem *)));
	connect(lv, SIGNAL(contextMenuRequested(Q3ListViewItem *, const QPoint &, int)),
			this, SLOT(showWindowPopupMenu(Q3ListViewItem *, const QPoint &, int)));
	connect(lv, SIGNAL(dragItems(QList<Q3ListViewItem *>)),
			this, SLOT(dragFolderItems(QList<Q3ListViewItem *>)));
	connect(lv, SIGNAL(dropItems(Q3ListViewItem *)),
			this, SLOT(dropFolderItems(Q3ListViewItem *)));
	connect(lv, SIGNAL(renameItem(Q3ListViewItem *)),
			this, SLOT(startRenameFolder(Q3ListViewItem *)));
	connect(lv, SIGNAL(addFolderItem()), this, SLOT(addFolder()));
	connect(lv, SIGNAL(deleteSelection()), this, SLOT(deleteSelectedItems()));
	connect(lv, SIGNAL(itemRenamed(Q3ListViewItem *, int, const QString &)),
			this, SLOT(renameWindow(Q3ListViewItem *, int, const QString &)));

	connect(scriptEnv, SIGNAL(error(const QString&,const QString&,int)),
			this, SLOT(scriptError(const QString&,const QString&,int)));
	connect(scriptEnv, SIGNAL(print(const QString&)), this, SLOT(scriptPrint(const QString&)));

	connect(recent, SIGNAL(activated(int)), this, SLOT(openRecentProject(int)));
	connect(&http, SIGNAL(done(bool)), this, SLOT(receivedVersionFile(bool)));

	// this has to be done after connecting scriptEnv
	scriptEnv->initialize();
    loadCustomActions();
}

void ApplicationWindow::initWindow()
{
	switch(d_init_window_type){
		case TableWindow:
			newTable();
		break;
		case MatrixWindow:
			newMatrix();
		break;
		case MultiLayerWindow:
			newGraph();
		break;
		case NoteWindow:
			newNote();
		break;
		default:
			break;
	}
}

void ApplicationWindow::initGlobalConstants()
{
	d_matrix_tool_bar = true;
	d_file_tool_bar = true;
	d_table_tool_bar = true;
	d_column_tool_bar = true;
	d_edit_tool_bar = true;
	d_plot_tool_bar = true;
	d_plot3D_tool_bar = true;
	d_display_tool_bar = false;
	d_format_tool_bar = true;

	appStyle = qApp->style()->objectName();
	d_app_rect = QRect();
	projectname = "untitled";
	lastModified=0;
	lastCopiedLayer = 0;
	d_text_copy = NULL;
	d_arrow_copy = NULL;
	d_image_copy = NULL;

	logInfo = QString();
	savingTimerId=0;

	#ifdef QTIPLOT_DEMO
        demoCloseTimerId = startTimer(10*60000);
    #endif

	autoSearchUpdatesRequest = false;

	show_windows_policy = ActiveFolder;
	d_script_win_on_top = true;
	d_script_win_rect = QRect(0, 0, 500, 300);
	d_init_window_type = TableWindow;

	QString aux = qApp->applicationDirPath();
    workingDir = aux;
	helpFilePath = aux + "/manual/index.html";
	fitPluginsPath = aux + "fitPlugins";
	fitModelsPath = QString::null;
	templatesDir = aux;
	asciiDirPath = aux;
	imagesDirPath = aux;
	scriptsDirPath = aux;
	customActionsDirPath = QString::null;

	appFont = QFont();
	QString family = appFont.family();
	int pointSize = appFont.pointSize();
	tableTextFont = appFont;
	tableHeaderFont = appFont;
	plotAxesFont = QFont(family, pointSize, QFont::Bold, false);
	plotNumbersFont = QFont(family, pointSize );
	plotLegendFont = appFont;
	plotTitleFont = QFont(family, pointSize + 2, QFont::Bold,false);

	plot3DAxesFont = QFont(family, pointSize, QFont::Bold, false );
	plot3DNumbersFont = QFont(family, pointSize);
	plot3DTitleFont = QFont(family, pointSize + 2, QFont::Bold,false);

	autoSearchUpdates = false;
	askForSupport = false;
	appLanguage = QLocale::system().name().section('_',0,0);
	show_windows_policy = ApplicationWindow::ActiveFolder;

	workspaceColor = QColor("darkGray");
	panelsColor = QColor("#ffffff");
	panelsTextColor = QColor("#000000");
	tableBkgdColor = QColor("#ffffff");
	tableTextColor = QColor("#000000");
	tableHeaderColor = QColor("#000000");

	plot3DColors = QStringList();
	plot3DColors << "blue";
	plot3DColors << "#000000";
	plot3DColors << "#000000";
	plot3DColors << "#000000";
	plot3DColors << "red";
	plot3DColors << "#000000";
	plot3DColors << "#000000";
	plot3DColors << "#ffffff";

	autoSave = true;
	autoSaveTime = 15;
	d_backup_files = true;
	defaultScriptingLang = "muParser";
	d_thousands_sep = true;
	d_locale = QLocale::system().name();
	if (!d_thousands_sep)
        d_locale.setNumberOptions(QLocale::OmitGroupSeparator);

	d_decimal_digits = 13;

	d_extended_open_dialog = true;
	d_extended_export_dialog = true;
	d_extended_import_ASCII_dialog = true;
	d_extended_plot_dialog = true;

	d_add_curves_dialog_size = QSize(700, 400);
	d_show_current_folder = false;

	confirmCloseFolder = true;
	confirmCloseTable = true;
	confirmCloseMatrix = true;
	confirmClosePlot2D = true;
	confirmClosePlot3D = true;
	confirmCloseNotes = true;
	d_inform_rename_table = true;

	d_show_table_comments = false;

	titleOn = true;
	allAxesOn = false;
	canvasFrameWidth = 0;
	defaultPlotMargin = 0;
	drawBackbones = true;
	axesLineWidth = 1;
	autoscale2DPlots = true;
	autoScaleFonts = true;
	autoResizeLayers = true;
	antialiasing2DPlots = true;
	d_scale_plots_on_print = false;
	d_print_cropmarks = false;

	defaultCurveStyle = int(Graph::LineSymbols);
	defaultCurveLineWidth = 1;
	defaultSymbolSize = 7;

	majTicksStyle = int(ScaleDraw::Out);
	minTicksStyle = int(ScaleDraw::Out);
	minTicksLength = 5;
	majTicksLength = 9;

	legendFrameStyle = int(LegendWidget::Line);
	legendTextColor = Qt::black;
	legendBackground = Qt::white;
	legendBackground.setAlpha(0); // transparent by default;

	defaultArrowLineWidth = 1;
	defaultArrowColor = Qt::black;
	defaultArrowHeadLength = 4;
	defaultArrowHeadAngle = 45;
	defaultArrowHeadFill = true;
	defaultArrowLineStyle = Graph::getPenStyle("SolidLine");

	showPlot3DLegend = true;
	showPlot3DProjection = false;
	smooth3DMesh = true;
	plot3DResolution = 1;
	orthogonal3DPlots = false;
	autoscale3DPlots = true;

	fit_output_precision = 13;
	pasteFitResultsToPlot = false;
	writeFitResultsToLog = true;
	generateUniformFitPoints = true;
	fitPoints = 100;
	generatePeakCurves = true;
	peakCurvesColor = 2;
	fit_scale_errors = true;
	d_2_linear_fit_points = true;

	columnSeparator = "\t";
	ignoredLines = 0;
	renameColumns = true;
	strip_spaces = false;
	simplify_spaces = false;
	d_ASCII_file_filter = "*";
	d_ASCII_import_locale = QLocale::system().name();
	d_import_dec_separators = true;
	d_ASCII_import_mode = int(ImportASCIIDialog::NewTables);
	d_ASCII_comment_string = "#";
	d_ASCII_import_comments = false;
	d_ASCII_import_read_only = false;
	d_ASCII_import_preview = true;
	d_preview_lines = 100;

	d_export_col_separator = "\t";
	d_export_col_names = false;
    d_export_col_comment = false;
	d_export_table_selection = false;

	d_image_export_filter = ".png";
	d_export_transparency = false;
	d_export_quality = 100;
	d_export_resolution = QPrinter().resolution();
	d_export_color = true;
	d_export_vector_size = int(QPrinter::Custom);
	d_keep_plot_aspect = true;
}

void ApplicationWindow::applyUserSettings()
{
	updateAppFonts();
	setScriptingLanguage(defaultScriptingLang);

	ws->setPaletteBackgroundColor (workspaceColor);

	QColorGroup cg;
	cg.setColor(QColorGroup::Base, QColor(panelsColor) );
	qApp->setPalette(QPalette(cg, cg, cg));

	cg.setColor(QColorGroup::Text, QColor(panelsTextColor) );
	cg.setColor(QColorGroup::WindowText, QColor(panelsTextColor) );
	cg.setColor(QColorGroup::HighlightedText, QColor(panelsTextColor) );
	lv->setPalette(QPalette(cg, cg, cg));
	results->setPalette(QPalette(cg, cg, cg));

	cg.setColor(QColorGroup::Text, QColor(Qt::green) );
	cg.setColor(QColorGroup::HighlightedText, QColor(Qt::darkGreen) );
	cg.setColor(QColorGroup::Base, QColor(Qt::black) );
	info->setPalette(QPalette(cg, cg, cg));
}

void ApplicationWindow::initToolBars()
{
	initPlot3DToolBar();

	setWindowIcon(QIcon(QPixmap(logo_xpm)));
	QPixmap openIcon, saveIcon;

	fileTools = new QToolBar(tr( "File" ), this);
	fileTools->setObjectName("fileTools"); // this is needed for QMainWindow::restoreState()
	fileTools->setIconSize( QSize(18,20) );
	addToolBar( Qt::TopToolBarArea, fileTools );

	fileTools->addAction(actionNewProject);
	fileTools->addAction(actionNewFolder);
	fileTools->addAction(actionNewTable);
	fileTools->addAction(actionNewMatrix);
	fileTools->addAction(actionNewNote);
	fileTools->addAction(actionNewGraph);
	fileTools->addAction(actionNewFunctionPlot);
	fileTools->addAction(actionNewSurfacePlot);
	fileTools->addSeparator ();
	fileTools->addAction(actionOpen);
	fileTools->addAction(actionOpenTemplate);
	fileTools->addAction(actionSaveProject);
	fileTools->addAction(actionSaveTemplate);
	fileTools->addSeparator ();
	fileTools->addAction(actionLoad);
	fileTools->addSeparator ();
	fileTools->addAction(actionCopyWindow);
	fileTools->addAction(actionPrint);
	fileTools->addAction(actionExportPDF);
	fileTools->addSeparator();
	fileTools->addAction(actionShowExplorer);
	fileTools->addAction(actionShowLog);
#ifdef SCRIPTING_PYTHON
	fileTools->addAction(actionShowScriptWindow);
#endif

	editTools = new QToolBar(tr("Edit"), this);
	editTools->setObjectName("editTools"); // this is needed for QMainWindow::restoreState()
	editTools->setIconSize( QSize(18,20) );
	addToolBar( editTools );

	editTools->addAction(actionUndo);
	editTools->addAction(actionRedo);
	editTools->addAction(actionCutSelection);
	editTools->addAction(actionCopySelection);
	editTools->addAction(actionPasteSelection);
	editTools->addAction(actionClearSelection);

	plotTools = new QToolBar(tr("Plot"), this);
	plotTools->setObjectName("plotTools"); // this is needed for QMainWindow::restoreState()
	plotTools->setIconSize( QSize(16,20) );
	addToolBar( plotTools );

	plotTools->addAction(actionAddLayer);
	plotTools->addAction(actionShowLayerDialog);
	plotTools->addAction(actionAutomaticLayout);
	plotTools->addSeparator();
	plotTools->addAction(actionAddErrorBars);
	plotTools->addAction(actionShowCurvesDialog);
	plotTools->addAction(actionAddFunctionCurve);
	plotTools->addAction(actionNewLegend);
	plotTools->addSeparator ();
	plotTools->addAction(actionUnzoom);

	dataTools = new QActionGroup( this );
	dataTools->setExclusive( true );

	btnPointer = new QAction(tr("Disable &Tools"), this);
	btnPointer->setActionGroup(dataTools);
	btnPointer->setCheckable( true );
	btnPointer->setIcon(QIcon(QPixmap(pointer_xpm)) );
	btnPointer->setChecked(true);
	plotTools->addAction(btnPointer);

	btnZoomIn = new QAction(tr("&Zoom In"), this);
	btnZoomIn->setShortcut( tr("Ctrl++") );
	btnZoomIn->setActionGroup(dataTools);
	btnZoomIn->setCheckable( true );
	btnZoomIn->setIcon(QIcon(QPixmap(zoom_xpm)) );
	plotTools->addAction(btnZoomIn);

	btnZoomOut = new QAction(tr("&Zoom Out"), this);
	btnZoomOut->setShortcut( tr("Ctrl+-") );
	btnZoomOut->setActionGroup(dataTools);
	btnZoomOut->setCheckable( true );
	btnZoomOut->setIcon(QIcon(QPixmap(zoomOut_xpm)) );
	plotTools->addAction(btnZoomOut);

	btnCursor = new QAction(tr("&Data Reader"), this);
	btnCursor->setShortcut( tr("CTRL+D") );
	btnCursor->setActionGroup(dataTools);
	btnCursor->setCheckable( true );
	btnCursor->setIcon(QIcon(QPixmap(select_xpm)) );
	plotTools->addAction(btnCursor);

	btnSelect = new QAction(tr("&Select Data Range"), this);
	btnSelect->setShortcut( tr("ALT+S") );
	btnSelect->setActionGroup(dataTools);
	btnSelect->setCheckable( true );
	btnSelect->setIcon(QIcon(QPixmap(cursors_xpm)) );
	plotTools->addAction(btnSelect);

	btnPicker = new QAction(tr("S&creen Reader"), this);
	btnPicker->setActionGroup(dataTools);
	btnPicker->setCheckable( true );
	btnPicker->setIcon(QIcon(QPixmap(cursor_16)) );
	plotTools->addAction(btnPicker);

	actionDrawPoints = new QAction(tr("&Draw Data Points"), this);
	actionDrawPoints->setActionGroup(dataTools);
	actionDrawPoints->setCheckable( true );
	actionDrawPoints->setIcon(QIcon(QPixmap(draw_points_xpm)) );
	plotTools->addAction(actionDrawPoints);

	btnMovePoints = new QAction(tr("&Move Data Points..."), this);
	btnMovePoints->setShortcut( tr("Ctrl+ALT+M") );
	btnMovePoints->setActionGroup(dataTools);
	btnMovePoints->setCheckable( true );
	btnMovePoints->setIcon(QIcon(QPixmap(hand_xpm)) );
	plotTools->addAction(btnMovePoints);

	btnRemovePoints = new QAction(tr("Remove &Bad Data Points..."), this);
	btnRemovePoints->setShortcut( tr("Alt+B") );
	btnRemovePoints->setActionGroup(dataTools);
	btnRemovePoints->setCheckable( true );
	btnRemovePoints->setIcon(QIcon(QPixmap(gomme_xpm)));
	plotTools->addAction(btnRemovePoints);

	connect( dataTools, SIGNAL( triggered( QAction* ) ), this, SLOT( pickDataTool( QAction* ) ) );
	plotTools->addSeparator ();

	actionAddText = new QAction(tr("Add &Text"), this);
	actionAddText->setShortcut( tr("ALT+T") );
	actionAddText->setIcon(QIcon(QPixmap(text_xpm)));
	actionAddText->setCheckable(true);
	connect(actionAddText, SIGNAL(activated()), this, SLOT(addText()));
	plotTools->addAction(actionAddText);

	btnArrow = new QAction(tr("Draw &Arrow"), this);
	btnArrow->setShortcut( tr("CTRL+ALT+A") );
	btnArrow->setActionGroup(dataTools);
	btnArrow->setCheckable( true );
	btnArrow->setIcon(QIcon(QPixmap(arrow_xpm)) );
	plotTools->addAction(btnArrow);

	btnLine = new QAction(tr("Draw &Line"), this);
	btnLine->setShortcut( tr("CTRL+ALT+L") );
	btnLine->setActionGroup(dataTools);
	btnLine->setCheckable( true );
	btnLine->setIcon(QIcon(QPixmap(lPlot_xpm)) );
	plotTools->addAction(btnLine);

	plotTools->addAction(actionTimeStamp);
	plotTools->addAction(actionAddImage);
	plotTools->hide();

	tableTools = new QToolBar(tr("Table"), this);
	tableTools->setObjectName("tableTools"); // this is needed for QMainWindow::restoreState()
	tableTools->setIconSize( QSize(16, 20));
	addToolBar(Qt::TopToolBarArea, tableTools);

	tableTools->addAction(actionPlotL);
	tableTools->addAction(actionPlotP);
	tableTools->addAction(actionPlotLP);
	tableTools->addAction(actionPlotVerticalBars);
	tableTools->addAction(actionPlotHorizontalBars);
	tableTools->addAction(actionPlotArea);
	tableTools->addAction(actionPlotPie);
	tableTools->addAction(actionPlotHistogram);
	tableTools->addAction(actionBoxPlot);
	tableTools->addAction(actionPlotVectXYXY);
	tableTools->addAction(actionPlotVectXYAM);
	tableTools->addSeparator ();
	tableTools->addAction(actionPlot3DRibbon);
	tableTools->addAction(actionPlot3DBars);
	tableTools->addAction(actionPlot3DScatter);
	tableTools->addAction(actionPlot3DTrajectory);
	tableTools->addSeparator();
	tableTools->addAction(actionAddColToTable);
	tableTools->addAction(actionShowColStatistics);
	tableTools->addAction(actionShowRowStatistics);
	tableTools->setEnabled(false);
    tableTools->hide();

	columnTools = new QToolBar(tr( "Column"), this);
	columnTools->setObjectName("columnTools"); // this is needed for QMainWindow::restoreState()
	columnTools->setIconSize(QSize(16, 20));
	addToolBar(Qt::TopToolBarArea, columnTools);

	columnTools->addAction(actionSetXCol);
	columnTools->addAction(actionSetYCol);
	columnTools->addAction(actionSetZCol);
	columnTools->addAction(actionSetYErrCol);
	columnTools->addAction(actionDisregardCol);
	columnTools->addSeparator();
	columnTools->addAction(actionMoveColFirst);
	columnTools->addAction(actionMoveColLeft);
	columnTools->addAction(actionMoveColRight);
	columnTools->addAction(actionMoveColLast);
	columnTools->addAction(actionSwapColumns);
    columnTools->setEnabled(false);
	columnTools->hide();

	displayBar = new QToolBar( tr( "Data Display" ), this );
    displayBar->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
	displayBar->setObjectName("displayBar"); // this is needed for QMainWindow::restoreState()
	info = new QLineEdit( this );
	displayBar->addWidget( info );
	info->setReadOnly(true);

	addToolBar( Qt::TopToolBarArea, displayBar );
	displayBar->hide();

    insertToolBarBreak (displayBar);

	plotMatrixBar = new QToolBar( tr( "Matrix Plot" ), this);
	plotMatrixBar->setObjectName("plotMatrixBar");
	addToolBar(Qt::BottomToolBarArea, plotMatrixBar);

	actionPlot3DWireFrame->addTo(plotMatrixBar);
	actionPlot3DHiddenLine->addTo(plotMatrixBar);

	actionPlot3DPolygons->addTo(plotMatrixBar);
	actionPlot3DWireSurface->addTo(plotMatrixBar);

	plotMatrixBar->addSeparator();

	actionPlot3DBars->addTo(plotMatrixBar);
	actionPlot3DScatter->addTo(plotMatrixBar);

	plotMatrixBar->addSeparator();
	actionColorMap->addTo(plotMatrixBar);
	actionContourMap->addTo(plotMatrixBar);
	actionGrayMap->addTo(plotMatrixBar);
	actionImagePlot->addTo(plotMatrixBar);
	plotMatrixBar->addSeparator();
	actionPlotHistogram->addTo(plotMatrixBar);

	plotMatrixBar->hide();

	formatToolBar = new QToolBar(tr( "Format" ), this);
	formatToolBar->setObjectName("formatToolBar");
	addToolBar(Qt::TopToolBarArea, formatToolBar);

	QFontComboBox *fb = new QFontComboBox();
	connect(fb, SIGNAL(currentFontChanged(const QFont &)), this, SLOT(setFontFamily(const QFont &)));
	actionFontBox = formatToolBar->addWidget(fb);

	QSpinBox *sb = new QSpinBox();
	connect(sb, SIGNAL(valueChanged(int)), this, SLOT(setFontSize(int)));
	actionFontSize = formatToolBar->addWidget(sb);

	actionFontBold->addTo(formatToolBar);
	actionFontItalic->addTo(formatToolBar);

	actionUnderline->addTo(formatToolBar);
	actionSuperscript->addTo(formatToolBar);
	actionSubscript->addTo(formatToolBar);
	actionGreekSymbol->addTo(formatToolBar);

	formatToolBar->setEnabled(false);
	formatToolBar->hide();

	QList<QToolBar *> toolBars = toolBarsList();
	foreach (QToolBar *t, toolBars)
		connect(t, SIGNAL(actionTriggered(QAction *)), this, SLOT(performCustomAction(QAction *)));
}

void ApplicationWindow::insertTranslatedStrings()
{
	if (projectname == "untitled")
		setWindowTitle(tr("QtiPlot - untitled"));

	lv->setColumnText (0, tr("Name"));
	lv->setColumnText (1, tr("Type"));
	lv->setColumnText (2, tr("View"));
	lv->setColumnText (3, tr("Size"));
	lv->setColumnText (4, tr("Created"));
	lv->setColumnText (5, tr("Label"));

	if (scriptWindow)
		scriptWindow->setWindowTitle(tr("QtiPlot - Script Window"));
	explorerWindow->setWindowTitle(tr("Project Explorer"));
	logWindow->setWindowTitle(tr("Results Log"));
#ifdef SCRIPTING_CONSOLE
	consoleWindow->setWindowTitle(tr("Scripting Console"));
#endif
	displayBar->setWindowTitle(tr("Data Display"));
	tableTools->setWindowTitle(tr("Table"));
	columnTools->setWindowTitle(tr("Column"));
	plotTools->setWindowTitle(tr("Plot"));
	fileTools->setWindowTitle(tr("File"));
	editTools->setWindowTitle(tr("Edit"));
	plotMatrixBar->setWindowTitle(tr("Matrix Plot"));
	plot3DTools->setWindowTitle(tr("3D Surface"));
	formatToolBar->setWindowTitle(tr("Format"));

	fileMenu->changeItem(recentMenuID, tr("&Recent Projects"));

	translateActionsStrings();
	customMenu(ws->activeWindow());
}

void ApplicationWindow::initMainMenu()
{
	fileMenu = new QMenu(this);
	fileMenu->setObjectName("fileMenu");
	connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(fileMenuAboutToShow()));

	recent = new QMenu(this);
	newMenu = new QMenu(this);
	newMenu->setObjectName("newMenu");
	exportPlotMenu = new QMenu(this);
	exportPlotMenu->setObjectName("exportPlotMenu");

	edit = new QMenu(this);
	edit->setObjectName("editMenu");

	edit->addAction(actionUndo);
	edit->addAction(actionRedo);
	edit->insertSeparator();
	edit->addAction(actionCopySelection);
	edit->addAction(actionPasteSelection);
	edit->addAction(actionClearSelection);
	edit->insertSeparator();
	edit->addAction(actionDeleteFitTables);
	edit->addAction(actionClearLogInfo);

	view = new QMenu(this);
	view->setObjectName("viewMenu");

	view->setCheckable(true);
	view->addAction(actionToolBars);
	view->addAction(actionShowPlotWizard);
	view->addAction(actionShowExplorer);
	view->addAction(actionShowLog);
#ifdef SCRIPTING_CONSOLE
	view->addAction(actionShowConsole);
#endif
	view->addAction(actionShowConfigureDialog);

	graph = new QMenu(this);
	graph->setObjectName("graphMenu");
	graph->setCheckable(true);
	graph->addAction(actionAddErrorBars);
	graph->addAction(actionShowCurvesDialog);
	graph->addAction(actionAddFunctionCurve);
	graph->addAction(actionNewLegend);
	graph->insertSeparator();
	graph->addAction(actionAddText);
	graph->addAction(btnArrow);
	graph->addAction(btnLine);
	graph->addAction(actionTimeStamp);
	graph->addAction(actionAddImage);
	graph->insertSeparator();//layers section
	graph->addAction(actionAddLayer);
	graph->addAction(actionDeleteLayer);
	graph->addAction(actionShowLayerDialog);

	plot3DMenu = new QMenu(this);
	plot3DMenu->setObjectName("plot3DMenu");
	plot3DMenu->addAction(actionPlot3DWireFrame);
	plot3DMenu->addAction(actionPlot3DHiddenLine);
	plot3DMenu->addAction(actionPlot3DPolygons);
	plot3DMenu->addAction(actionPlot3DWireSurface);
	plot3DMenu->insertSeparator();
	plot3DMenu->addAction(actionPlot3DBars);
	plot3DMenu->addAction(actionPlot3DScatter);
	plot3DMenu->insertSeparator();
    plot3DMenu->addAction(actionImagePlot);
	plot3DMenu->addAction(actionColorMap);
	plot3DMenu->addAction(actionContourMap);
	plot3DMenu->addAction(actionGrayMap);
	plot3DMenu->insertSeparator();
	plot3DMenu->addAction(actionPlotHistogram);

	matrixMenu = new QMenu(this);
	matrixMenu->setObjectName("matrixMenu");
	connect(matrixMenu, SIGNAL(aboutToShow()), this, SLOT(matrixMenuAboutToShow()));

    plot2DMenu = new QMenu(this);
	plot2DMenu->setObjectName("plot2DMenu");
    connect(plot2DMenu, SIGNAL(aboutToShow()), this, SLOT(plotMenuAboutToShow()));

    plotDataMenu = new QMenu(this);
	plotDataMenu->setObjectName("plotDataMenu");
	plotDataMenu->setCheckable(true);
    connect(plotDataMenu, SIGNAL(aboutToShow()), this, SLOT(plotDataMenuAboutToShow()));

	normMenu = new QMenu(this);
	normMenu->setObjectName("normMenu");

	fillMenu = new QMenu(this);
	fillMenu->setObjectName("fillMenu");

	tableMenu = new QMenu(this);
	tableMenu->setObjectName("tableMenu");
    connect(tableMenu, SIGNAL(aboutToShow()), this, SLOT(tableMenuAboutToShow()));

	smoothMenu = new QMenu(this);
	smoothMenu->setObjectName("smoothMenu");

	filterMenu = new QMenu(this);
	filterMenu->setObjectName("filterMenu");

	decayMenu = new QMenu(this);
	decayMenu->setObjectName("decayMenu");

	multiPeakMenu = new QMenu(this);
	multiPeakMenu->setObjectName("multiPeakMenu");

	analysisMenu = new QMenu(this);
	analysisMenu->setObjectName("analysisMenu");
    connect(analysisMenu, SIGNAL(aboutToShow()), this, SLOT(analysisMenuAboutToShow()));

	format = new QMenu(this);
	format->setObjectName("formatMenu");

	scriptingMenu = new QMenu(this);
	scriptingMenu->setObjectName("scriptingMenu");

	windowsMenu = new QMenu(this);
	windowsMenu->setObjectName("windowsMenu");
	windowsMenu->setCheckable(true);
	connect(windowsMenu, SIGNAL(aboutToShow()), this, SLOT(windowsMenuAboutToShow()));

	foldersMenu = new QMenu(this);
	foldersMenu->setCheckable(true);

	help = new QMenu(this);
	help->setObjectName("helpMenu");
	help->addAction(actionShowHelp);
	help->addAction(actionChooseHelpFolder);
	help->insertSeparator();
	help->addAction(actionHomePage);
	help->addAction(actionCheckUpdates);
	help->addAction(actionDownloadManual);
	help->addAction(actionTranslations);
	help->insertSeparator();
	help->addAction(actionTechnicalSupport);
	help->addAction(actionDonate);
	help->addAction(actionHelpForums);
	help->addAction(actionHelpBugReports);
	help->insertSeparator();
	help->addAction(actionAbout);

	QList<QMenu *> menus = customizableMenusList();
	foreach (QMenu *m, menus)
    	connect(m, SIGNAL(triggered(QAction *)), this, SLOT(performCustomAction(QAction *)));

	disableActions();
}

void ApplicationWindow::tableMenuAboutToShow()
{
    tableMenu->clear();
	fillMenu->clear();

    QMenu *setAsMenu = tableMenu->addMenu(tr("Set Columns &As"));
	setAsMenu->addAction(actionSetXCol);
	setAsMenu->addAction(actionSetYCol);
	setAsMenu->addAction(actionSetZCol);
	setAsMenu->insertSeparator();
	setAsMenu->addAction(actionSetYErrCol);
	setAsMenu->addAction(actionSetXErrCol);
	setAsMenu->insertSeparator();
	setAsMenu->addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
	setAsMenu->addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));
	setAsMenu->insertSeparator();
	setAsMenu->addAction(actionDisregardCol);

	tableMenu->addAction(actionShowColumnOptionsDialog);
	tableMenu->insertSeparator();

	tableMenu->addAction(actionShowColumnValuesDialog);
	tableMenu->addAction(actionTableRecalculate);

    fillMenu = tableMenu->addMenu (tr("&Fill Columns With"));
	fillMenu->addAction(actionSetAscValues);
	fillMenu->addAction(actionSetRandomValues);

	tableMenu->addAction(actionClearTable);
	tableMenu->insertSeparator();
	tableMenu->addAction(actionAddColToTable);
	tableMenu->addAction(actionShowColsDialog);
	tableMenu->insertSeparator();
	tableMenu->addAction(actionMoveColFirst);
	tableMenu->addAction(actionMoveColLeft);
	tableMenu->addAction(actionMoveColRight);
	tableMenu->addAction(actionMoveColLast);
	tableMenu->addAction(actionSwapColumns);
	tableMenu->insertSeparator();
	tableMenu->addAction(actionShowRowsDialog);
	tableMenu->addAction(actionDeleteRows);
	tableMenu->addAction(actionGoToRow);
	tableMenu->insertSeparator();
	tableMenu->addAction(actionConvertTable);

    reloadCustomActions();
}

void ApplicationWindow::plotDataMenuAboutToShow()
{
    plotDataMenu->clear();
	plotDataMenu->addAction(btnPointer);
	plotDataMenu->addAction(btnZoomIn);
	plotDataMenu->addAction(btnZoomOut);
	plotDataMenu->addAction(actionUnzoom);
	plotDataMenu->insertSeparator();
	plotDataMenu->addAction(btnCursor);
	plotDataMenu->addAction(btnSelect);
	plotDataMenu->addAction(btnPicker);
	plotDataMenu->insertSeparator();
	plotDataMenu->addAction(actionDrawPoints);
	plotDataMenu->addAction(btnMovePoints);
	plotDataMenu->addAction(btnRemovePoints);

    reloadCustomActions();
}

void ApplicationWindow::plotMenuAboutToShow()
{
	plot2DMenu->clear();

	plot2DMenu->addAction(actionPlotL);
	plot2DMenu->addAction(actionPlotP);
	plot2DMenu->addAction(actionPlotLP);

    QMenu *specialPlotMenu = plot2DMenu->addMenu (tr("Special Line/Symb&ol"));
	specialPlotMenu->addAction(actionPlotVerticalDropLines);
	specialPlotMenu->addAction(actionPlotSpline);
	specialPlotMenu->addAction(actionPlotVertSteps);
	specialPlotMenu->addAction(actionPlotHorSteps);
	plot2DMenu->insertSeparator();
	plot2DMenu->addAction(actionPlotVerticalBars);
	plot2DMenu->addAction(actionPlotHorizontalBars);
	plot2DMenu->addAction(actionPlotArea);
	plot2DMenu->addAction(actionPlotPie);
	plot2DMenu->addAction(actionPlotVectXYXY);
	plot2DMenu->addAction(actionPlotVectXYAM);
	plot2DMenu->insertSeparator();

	QMenu *statMenu = plot2DMenu->addMenu (tr("Statistical &Graphs"));
	statMenu->addAction(actionBoxPlot);
	statMenu->addAction(actionPlotHistogram);
	statMenu->addAction(actionPlotStackedHistograms);

    QMenu *panelsMenu = plot2DMenu->addMenu (tr("Pa&nel"));
	panelsMenu->addAction(actionPlot2VerticalLayers);
	panelsMenu->addAction(actionPlot2HorizontalLayers);
	panelsMenu->addAction(actionPlot4Layers);
	panelsMenu->addAction(actionPlotStackedLayers);

	QMenu *plot3D = plot2DMenu->addMenu (tr("3&D Plot"));
	plot3D->addAction(actionPlot3DRibbon);
	plot3D->addAction(actionPlot3DBars);
	plot3D->addAction(actionPlot3DScatter);
	plot3D->addAction(actionPlot3DTrajectory);

    reloadCustomActions();
}

void ApplicationWindow::customMenu(QWidget* w)
{
	menuBar()->clear();
	menuBar()->insertItem(tr("&File"), fileMenu);
	fileMenuAboutToShow();
	menuBar()->insertItem(tr("&Edit"), edit);
	menuBar()->insertItem(tr("&View"), view);
	menuBar()->insertItem(tr("Scripting"), scriptingMenu);

	scriptingMenu->clear();
#ifdef SCRIPTING_DIALOG
	scriptingMenu->addAction(actionScriptingLang);
#endif
	scriptingMenu->addAction(actionRestartScripting);
    scriptingMenu->addAction(actionCustomActionDialog);

	// these use the same keyboard shortcut (Ctrl+Return) and should not be enabled at the same time
	actionNoteEvaluate->setEnabled(false);
	actionTableRecalculate->setEnabled(false);

	if(w){
		actionPrintAllPlots->setEnabled(projectHas2DPlots());
		actionPrint->setEnabled(true);
		actionCutSelection->setEnabled(true);
		actionCopySelection->setEnabled(true);
		actionPasteSelection->setEnabled(true);
		actionClearSelection->setEnabled(true);
		actionSaveTemplate->setEnabled(true);
		if (tableWindows.count() > 0)
			actionShowExportASCIIDialog->setEnabled(true);
		else
			actionShowExportASCIIDialog->setEnabled(false);

		if (w->isA("MultiLayer")) {
			menuBar()->insertItem(tr("&Graph"), graph);
			menuBar()->insertItem(tr("&Data"), plotDataMenu);
			plotDataMenuAboutToShow();
			menuBar()->insertItem(tr("&Analysis"), analysisMenu);
			analysisMenuAboutToShow();
			menuBar()->insertItem(tr("For&mat"), format);

			format->clear();
			format->addAction(actionShowPlotDialog);
			format->insertSeparator();
            format->addAction(actionShowScaleDialog);
            format->addAction(actionShowAxisDialog);
            actionShowAxisDialog->setEnabled(true);
            format->insertSeparator();
            format->addAction(actionShowGridDialog);
			format->addAction(actionShowTitleDialog);
		} else if (w->isA("Graph3D")) {
			disableActions();

			menuBar()->insertItem(tr("For&mat"), format);

			actionPrint->setEnabled(true);
			actionSaveTemplate->setEnabled(true);

			format->clear();
			format->addAction(actionShowPlotDialog);
			format->addAction(actionShowScaleDialog);
			format->addAction(actionShowAxisDialog);
			format->addAction(actionShowTitleDialog);
			if (((Graph3D*)w)->coordStyle() == Qwt3D::NOCOORD)
				actionShowAxisDialog->setEnabled(false);
		} else if (w->inherits("Table")) {
			menuBar()->insertItem(tr("&Plot"), plot2DMenu);
            menuBar()->insertItem(tr("&Analysis"), analysisMenu);
			analysisMenuAboutToShow();
            menuBar()->insertItem(tr("&Table"), tableMenu);
			tableMenuAboutToShow();
			actionTableRecalculate->setEnabled(true);
		} else if (w->isA("Matrix")){
			actionTableRecalculate->setEnabled(true);
			menuBar()->insertItem(tr("3D &Plot"), plot3DMenu);
			menuBar()->insertItem(tr("&Matrix"), matrixMenu);
			matrixMenuAboutToShow();
			menuBar()->insertItem(tr("&Analysis"), analysisMenu);
			analysisMenuAboutToShow();
		} else if (w->isA("Note")) {
			actionSaveTemplate->setEnabled(false);
			actionNoteEvaluate->setEnabled(true);
			scriptingMenu->insertSeparator();
			scriptingMenu->addAction(actionNoteExecute);
			scriptingMenu->addAction(actionNoteExecuteAll);
			scriptingMenu->addAction(actionNoteEvaluate);

			actionNoteExecute->disconnect(SIGNAL(activated()));
			actionNoteExecuteAll->disconnect(SIGNAL(activated()));
			actionNoteEvaluate->disconnect(SIGNAL(activated()));
			connect(actionNoteExecute, SIGNAL(activated()), w, SLOT(execute()));
			connect(actionNoteExecuteAll, SIGNAL(activated()), w, SLOT(executeAll()));
			connect(actionNoteEvaluate, SIGNAL(activated()), w, SLOT(evaluate()));
		} else
			disableActions();
	} else
		disableActions();

    menuBar()->insertItem(tr("&Windows"), windowsMenu);
	windowsMenuAboutToShow();
	menuBar()->insertItem(tr("&Help"), help );

	reloadCustomActions();
}

void ApplicationWindow::disableActions()
{
	actionSaveTemplate->setEnabled(false);
	actionPrintAllPlots->setEnabled(false);
	actionPrint->setEnabled(false);

	actionUndo->setEnabled(false);
	actionRedo->setEnabled(false);

	actionCutSelection->setEnabled(false);
	actionCopySelection->setEnabled(false);
	actionPasteSelection->setEnabled(false);
	actionClearSelection->setEnabled(false);
}

void ApplicationWindow::customColumnActions()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

    actionMoveColFirst->setEnabled(false);
    actionMoveColLeft->setEnabled(false);
    actionMoveColRight->setEnabled(false);
    actionMoveColLast->setEnabled(false);
    actionSetXCol->setEnabled(false);
    actionSetYCol->setEnabled(false);
    actionSetZCol->setEnabled(false);
    actionSetYErrCol->setEnabled(false);
    actionDisregardCol->setEnabled(false);
    actionSwapColumns->setEnabled(false);

    Table *t = static_cast<Table*>(ws->activeWindow());
    int selectedCols = t->selectedColsNumber();
    if (selectedCols == 1){
        int col = t->selectedColumn();
        if (col > 0){
            actionMoveColFirst->setEnabled(true);
            actionMoveColLeft->setEnabled(true);
        }

        if (col < t->numCols() - 1){
            actionMoveColRight->setEnabled(true);
            actionMoveColLast->setEnabled(true);
        }
	}

	if (selectedCols >= 1){
        actionSetXCol->setEnabled(true);
        actionSetYCol->setEnabled(true);
        actionSetZCol->setEnabled(true);
        actionSetYErrCol->setEnabled(true);
        actionDisregardCol->setEnabled(true);
	}

	if (selectedCols == 2)
	    actionSwapColumns->setEnabled(true);
}

void ApplicationWindow::customToolBars(QWidget* w)
{
    disableToolbars();
	if (!w)
        return;

    if (w->isA("MultiLayer") && d_plot_tool_bar){
        if(!plotTools->isVisible())
            plotTools->show();
        plotTools->setEnabled (true);

		if(d_format_tool_bar && !formatToolBar->isVisible()){
			formatToolBar->setEnabled (true);
            formatToolBar->show();
		}
    } else if (w->inherits("Table")){
        if(d_table_tool_bar){
            if(!tableTools->isVisible())
                tableTools->show();
            tableTools->setEnabled (true);
        }
        if (d_column_tool_bar){
            if(!columnTools->isVisible())
                columnTools->show();
            columnTools->setEnabled (true);
            customColumnActions();
        }
    } else if (w->isA("Matrix") && d_matrix_tool_bar){
         if(!plotMatrixBar->isVisible())
            plotMatrixBar->show();
        plotMatrixBar->setEnabled (true);
    } else if (w->isA("Graph3D") && d_plot3D_tool_bar){
        if(!plot3DTools->isVisible())
            plot3DTools->show();

        if (((Graph3D*)w)->plotStyle() == Qwt3D::NOPLOT)
            plot3DTools->setEnabled(false);
        else
            plot3DTools->setEnabled(true);
        custom3DActions(w);
    }
}

void ApplicationWindow::disableToolbars()
{
	plotTools->setEnabled(false);
	tableTools->setEnabled(false);
	columnTools->setEnabled(false);
	plot3DTools->setEnabled(false);
	plotMatrixBar->setEnabled(false);
}

void ApplicationWindow::plot3DRibbon()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *table = static_cast<Table*>(ws->activeWindow());
	if(table->selectedColumns().count() == 1){
		if (!validFor3DPlot(table))
			return;
		plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Ribbon);
	} else
		QMessageBox::warning(this, tr("QtiPLot - Plot error"), tr("You must select exactly one column for plotting!"));
}

void ApplicationWindow::plot3DWireframe()
{
	plot3DMatrix (0, Qwt3D::WIREFRAME);
}

void ApplicationWindow::plot3DHiddenLine()
{
	plot3DMatrix (0, Qwt3D::HIDDENLINE);
}

void ApplicationWindow::plot3DPolygons()
{
	plot3DMatrix (0, Qwt3D::FILLED);
}

void ApplicationWindow::plot3DWireSurface()
{
	plot3DMatrix (0, Qwt3D::FILLEDMESH);
}

void ApplicationWindow::plot3DBars()
{
	QWidget* w = ws->activeWindow();
	if (!w)
		return;

	if (w->inherits("Table")){
		Table *table = static_cast<Table *>(w);
		if (!validFor3DPlot(table))
			return;

		if(table->selectedColumns().count() == 1)
			plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Bars);
		else
			QMessageBox::warning(this, tr("QtiPlot - Plot error"),tr("You must select exactly one column for plotting!"));
	}
	else if(w->inherits("Matrix"))
		plot3DMatrix(0, Qwt3D::USER);
}

void ApplicationWindow::plot3DScatter()
{
	QWidget* w = ws->activeWindow();
	if (!w)
		return;

	if (w->inherits("Table"))
	{
		Table *table = static_cast<Table *>(w);
		if (!validFor3DPlot(table))
			return;

		if(table->selectedColumns().count() == 1)
			plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Scatter);
		else
			QMessageBox::warning(this, tr("QtiPlot - Plot error"),tr("You must select exactly one column for plotting!"));
	}
	else if(w->inherits("Matrix"))
		plot3DMatrix (0, Qwt3D::POINTS);
}

void ApplicationWindow::plot3DTrajectory()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *table = static_cast<Table *>(ws->activeWindow());
    if (!validFor3DPlot(table))
        return;

    if(table->selectedColumns().count() == 1)
        plotXYZ(table, table->colName(table->selectedColumn()), Graph3D::Trajectory);
    else
        QMessageBox::warning(this, tr("QtiPlot - Plot error"), tr("You must select exactly one column for plotting!"));
}

void ApplicationWindow::plotBoxDiagram()
{
    generate2DGraph(Graph::Box);
}

void ApplicationWindow::plotVerticalBars()
{
	generate2DGraph(Graph::VerticalBars);
}

void ApplicationWindow::plotHorizontalBars()
{
	generate2DGraph(Graph::HorizontalBars);
}

MultiLayer* ApplicationWindow::plotHistogram()
{
    return generate2DGraph(Graph::Histogram);
}

MultiLayer* ApplicationWindow::plotHistogram(Matrix *m)
{
	if (!m) {
		m = (Matrix*)ws->activeWindow();
		if (!m || !m->isA("Matrix"))
			return 0;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MultiLayer* g = new MultiLayer("", ws, 0);
	g->setAttribute(Qt::WA_DeleteOnClose);

	Graph* plot = g->addLayer();
	setPreferences(plot);

	plot->addHistogram(m);
	initMultilayerPlot(g, generateUniqueName(tr("Graph")));
	g->setMargins(5, 5, 5, 5);
	g->arrangeLayers(true, false);

	emit modified();
	QApplication::restoreOverrideCursor();
	return g;
}

void ApplicationWindow::plotArea()
{
	generate2DGraph(Graph::Area);
}

void ApplicationWindow::plotPie()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *table = static_cast<Table *>(ws->activeWindow());

	if(table->selectedColumns().count() != 1){
		QMessageBox::warning(this, tr("QtiPlot - Plot error"),
				tr("You must select exactly one column for plotting!"));
		return;
	}

	QStringList s = table->selectedColumns();
	if (s.count()>0){
		Q3TableSelection sel = table->getSelection();
		multilayerPlot(table, s, Graph::Pie, sel.topRow(), sel.bottomRow());
	} else
		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select a column to plot!"));
}

void ApplicationWindow::plotL()
{
	generate2DGraph(Graph::Line);
}

void ApplicationWindow::plotP()
{
	generate2DGraph(Graph::Scatter);
}

void ApplicationWindow::plotLP()
{
	generate2DGraph(Graph::LineSymbols);
}

void ApplicationWindow::plotVerticalDropLines()
{
	generate2DGraph(Graph::VerticalDropLines);
}

void ApplicationWindow::plotSpline()
{
	generate2DGraph(Graph::Spline);
}

void ApplicationWindow::plotVertSteps()
{
	generate2DGraph(Graph::VerticalSteps);
}

void ApplicationWindow::plotHorSteps()
{
	generate2DGraph(Graph::HorizontalSteps);
}

void ApplicationWindow::plotVectXYXY()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table * table = static_cast<Table *>(ws->activeWindow());
	if (!validFor2DPlot(table))
		return;

	QStringList s = table->selectedColumns();
	if (s.count() == 4) {
		Q3TableSelection sel = table->getSelection();
		multilayerPlot(table, s, Graph::VectXYXY, sel.topRow(), sel.bottomRow());
	} else
		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select four columns for this operation!"));
}

void ApplicationWindow::plotVectXYAM()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table * table = static_cast<Table *>(ws->activeWindow());
	if (!validFor2DPlot(table))
		return;

	QStringList s = table->selectedColumns();
	if (s.count() == 4){
		Q3TableSelection sel = table->getSelection();
		multilayerPlot(table, s, Graph::VectXYAM, sel.topRow(), sel.bottomRow());
	} else
		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select four columns for this operation!"));
}

void ApplicationWindow::renameListViewItem(const QString& oldName,const QString& newName)
{
	Q3ListViewItem *it=lv->findItem (oldName,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		it->setText(0,newName);
}

void ApplicationWindow::setListViewLabel(const QString& caption,const QString& label)
{
	Q3ListViewItem *it=lv->findItem ( caption, 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		it->setText(5,label);
}

void ApplicationWindow::setListViewDate(const QString& caption,const QString& date)
{
	Q3ListViewItem *it=lv->findItem ( caption, 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		it->setText(4,date);
}

void ApplicationWindow::setListView(const QString& caption,const QString& view)
{
	Q3ListViewItem *it=lv->findItem ( caption,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		it->setText(2,view);
}

void ApplicationWindow::setListViewSize(const QString& caption,const QString& size)
{
	Q3ListViewItem *it=lv->findItem ( caption,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		it->setText(3,size);
}

QString ApplicationWindow::listViewDate(const QString& caption)
{
	Q3ListViewItem *it=lv->findItem (caption,0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		return it->text(4);
	else
		return "";
}

void ApplicationWindow::updateTableNames(const QString& oldName, const QString& newName)
{
	QWidgetList *windows = windowsList();
	foreach (QWidget *w, *windows) {
		if (w->isA("MultiLayer")) {
			QWidgetList gr_lst = ((MultiLayer*)w)->graphPtrs();
			foreach(QWidget *widget, gr_lst)
				((Graph *)widget)->updateCurveNames(oldName, newName);
		} else if (w->isA("Graph3D")) {
			QString name = ((Graph3D*)w)->formula();
			if (name.contains(oldName, true)) {
				name.replace(oldName,newName);
				((Graph3D*)w)->setPlotAssociation(name);
			}
		}
	}
	delete windows;
}

void ApplicationWindow::updateColNames(const QString& oldName, const QString& newName)
{
	QWidgetList *windows = windowsList();
	foreach (QWidget *w, *windows){
		if (w->isA("MultiLayer")){
			QWidgetList gr_lst = ((MultiLayer*)w)->graphPtrs();
			foreach (QWidget *widget, gr_lst)
                ((Graph *)widget)->updateCurveNames(oldName, newName, false);
		}
		else if (w->isA("Graph3D")){
			QString name = ((Graph3D*)w)->formula();
			if (name.contains(oldName)){
				name.replace(oldName,newName);
				((Graph3D*)w)->setPlotAssociation(name);
			}
		}
	}
	delete windows;
}

void ApplicationWindow::changeMatrixName(const QString& oldName, const QString& newName)
{
	QWidgetList *lst = windowsList();
	foreach(QWidget *w, *lst)
	{
		if (w->isA("Graph3D"))
		{
			QString s = ((Graph3D*)w)->formula();
			if (s.contains(oldName))
			{
				s.replace(oldName, newName);
				((Graph3D*)w)->setPlotAssociation(s);
			}
		}
		else if (w->isA("MultiLayer"))
		{
			QWidgetList graphsList = ((MultiLayer*)w)->graphPtrs();
			foreach (QWidget *gr_widget, graphsList)
			{
				Graph* g = (Graph*)gr_widget;
				for (int i=0; i<g->curves(); i++)
				{
					QwtPlotItem *sp = (QwtPlotItem *)g->plotItem(i);
					if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->title().text() == oldName)
						sp->setTitle(newName);
				}
			}
		}
	}
	delete lst;
}

void ApplicationWindow::remove3DMatrixPlots(Matrix *m)
{
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows){
		if (w->isA("Graph3D") && ((Graph3D*)w)->matrix() == m)
			((Graph3D*)w)->clearData();
		else if (w->isA("MultiLayer")){
			QWidgetList graphsList = ((MultiLayer*)w)->graphPtrs();
			for (int j=0; j<(int)graphsList.count(); j++){
				Graph* g = (Graph*)graphsList.at(j);
				for (int i=0; i<g->curves(); i++){
				    if (g->curveType(i) == Graph::Histogram){
                        QwtHistogram *h = (QwtHistogram *)g->plotItem(i);
                        if (h && h->matrix() == m)
                            g->removeCurve(i);
				    } else {
                        Spectrogram *sp = (Spectrogram *)g->plotItem(i);
                        if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->matrix() == m)
                            g->removeCurve(i);
				    }
				}
			}
		}
	}
	delete windows;
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateMatrixPlots(QWidget *window)
{
	Matrix *m = (Matrix *)window;
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows){
		if (w->isA("Graph3D") && ((Graph3D*)w)->matrix() == m)
			((Graph3D*)w)->updateMatrixData(m);
		else if (w->isA("MultiLayer")){
			QWidgetList graphsList = ((MultiLayer*)w)->graphPtrs();
			for (int j=0; j<(int)graphsList.count(); j++){
				Graph* g = (Graph*)graphsList.at(j);
				for (int i=0; i<g->curves(); i++){
				    if (g->curveType(i) == Graph::Histogram){
                        QwtHistogram *h = (QwtHistogram *)g->plotItem(i);
                        if (h && h->matrix() == m)
                            h->loadData();
				    } else {
                        Spectrogram *sp = (Spectrogram *)g->plotItem(i);
                        if (sp && sp->rtti() == QwtPlotItem::Rtti_PlotSpectrogram && sp->matrix() == m)
                            sp->updateData(m);
				    }
				}
                g->updatePlot();
			}
		}
	}

	delete windows;
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::add3DData()
{
	if (tableWindows.count() <= 0)
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no tables available in this project.</h4>"
					"<p><h4>Please create a table and try again!</h4>"));
		return;
	}

	QStringList zColumns = columnsList(Table::Z);
	if ((int)zColumns.count() <= 0)
	{
		QMessageBox::critical(this,tr("QtiPlot - Warning"),
				tr("There are no available columns with plot designation set to Z!"));
		return;
	}

	DataSetDialog *ad = new DataSetDialog(tr("Column") + " : ", this);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	connect (ad,SIGNAL(options(const QString&)), this, SLOT(insertNew3DData(const QString&)));
	ad->setWindowTitle(tr("QtiPlot - Choose data set"));
	ad->setCurveNames(zColumns);
	ad->exec();
}

void ApplicationWindow::change3DData()
{
	DataSetDialog *ad = new DataSetDialog(tr("Column") + " : ", this);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	connect (ad,SIGNAL(options(const QString&)), this, SLOT(change3DData(const QString&)));

	ad->setWindowTitle(tr("QtiPlot - Choose data set"));
	ad->setCurveNames(columnsList(Table::Z));
	ad->exec();
}

void ApplicationWindow::change3DMatrix()
{
	DataSetDialog *ad = new DataSetDialog(tr("Matrix") + " : ", this);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	connect (ad, SIGNAL(options(const QString&)), this, SLOT(change3DMatrix(const QString&)));

	ad->setWindowTitle(tr("QtiPlot - Choose matrix to plot"));
	ad->setCurveNames(matrixNames());

	Graph3D* g = (Graph3D*)ws->activeWindow();
	if (g && g->matrix())
		ad->setCurentDataSet(g->matrix()->objectName());
	ad->exec();
}

void ApplicationWindow::change3DMatrix(const QString& matrix_name)
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
	{
		Graph3D* g = (Graph3D*)ws->activeWindow();
		Matrix *m = matrix(matrix_name);
		if (m && g)
			g->addMatrixData(m);

		emit modified();
	}
}

void ApplicationWindow::add3DMatrixPlot()
{
	QStringList matrices = matrixNames();
	if ((int)matrices.count() <= 0)
	{
		QMessageBox::warning(this, tr("QtiPlot - Warning"),
				tr("<h4>There are no matrices available in this project.</h4>"
					"<p><h4>Please create a matrix and try again!</h4>"));
		return;
	}

	DataSetDialog *ad = new DataSetDialog(tr("Matrix") + " :", this);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	connect (ad,SIGNAL(options(const QString&)), this, SLOT(insert3DMatrixPlot(const QString&)));

	ad->setWindowTitle(tr("QtiPlot - Choose matrix to plot"));
	ad->setCurveNames(matrices);
	ad->exec();
}

void ApplicationWindow::insert3DMatrixPlot(const QString& matrix_name)
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
	{
		((Graph3D*)ws->activeWindow())->addMatrixData(matrix(matrix_name));
		emit modified();
	}
}

void ApplicationWindow::insertNew3DData(const QString& colName)
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
	{
		((Graph3D*)ws->activeWindow())->insertNewData(table(colName),colName);
		emit modified();
	}
}

void ApplicationWindow::change3DData(const QString& colName)
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Graph3D")) {
		((Graph3D*)ws->activeWindow())->changeDataColumn(table(colName), colName);
		emit modified();
	}
}

void ApplicationWindow::editSurfacePlot()
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Graph3D")){
		Graph3D* g = (Graph3D*)ws->activeWindow();
		SurfaceDialog* sd = new SurfaceDialog(this);
		sd->setAttribute(Qt::WA_DeleteOnClose);

		if (g->hasData() && g->userFunction())
			sd->setFunction(g);
		else if (g->hasData() && g->parametricSurface())
			sd->setParametricSurface(g);
		sd->exec();
	}
}

void ApplicationWindow::newSurfacePlot()
{
	SurfaceDialog* sd = new SurfaceDialog(this);
	sd->setAttribute(Qt::WA_DeleteOnClose);
	sd->exec();
}

Graph3D* ApplicationWindow::plotSurface(const QString& formula, double xl, double xr,
		double yl, double yr, double zl, double zr, int columns, int rows)
{
	QString label = generateUniqueName(tr("Graph"));

	Graph3D *plot = new Graph3D("", ws);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	plot->resize(500,400);
	plot->setWindowTitle(label);
	plot->setName(label);
	customPlot3D(plot);
	plot->addFunction(formula, xl, xr, yl, yr, zl, zr, columns, rows);

	initPlot3D(plot);

	emit modified();
	return plot;
}

Graph3D* ApplicationWindow::plotParametricSurface(const QString& xFormula, const QString& yFormula,
		const QString& zFormula, double ul, double ur, double vl, double vr,
		int columns, int rows, bool uPeriodic, bool vPeriodic)
{
	QString label = generateUniqueName(tr("Graph"));

	Graph3D *plot = new Graph3D("", ws);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	plot->resize(500, 400);
	plot->setWindowTitle(label);
	plot->setName(label);
	customPlot3D(plot);
	plot->addParametricSurface(xFormula, yFormula, zFormula, ul, ur, vl, vr,
								columns, rows, uPeriodic, vPeriodic);
	initPlot3D(plot);
	emit modified();
	return plot;
}

void ApplicationWindow::updateSurfaceFuncList(const QString& s)
{
	surfaceFunc.remove(s);
	surfaceFunc.push_front(s);
	while ((int)surfaceFunc.size() > 10)
		surfaceFunc.pop_back();
}

Graph3D* ApplicationWindow::dataPlot3D(const QString& caption,const QString& formula,
		double xl, double xr, double yl, double yr, double zl, double zr)
{
	int pos=formula.find("_",0);
	QString wCaption=formula.left(pos);

	Table* w=table(wCaption);
	if (!w)
		return 0;

	int posX=formula.find("(",pos);
	QString xCol=formula.mid(pos+1,posX-pos-1);

	pos=formula.find(",",posX);
	posX=formula.find("(",pos);
	QString yCol=formula.mid(pos+1,posX-pos-1);

	Graph3D *plot = new Graph3D("", ws, 0);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	plot->addData(w, xCol, yCol, xl, xr, yl, yr, zl, zr);
	plot->update();

	QString label=caption;
	while(alreadyUsedName(label))
		label = generateUniqueName(tr("Graph"));

	plot->setWindowTitle(label);
	plot->setName(label);
	initPlot3D(plot);

	return plot;
}

Graph3D* ApplicationWindow::newPlot3D()
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QString label = generateUniqueName(tr("Graph"));

	Graph3D *plot = new Graph3D("", ws, 0);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	plot->resize(500, 400);
	plot->setWindowTitle(label);
	plot->setName(label);

	customPlot3D(plot);
	initPlot3D(plot);

	emit modified();
	QApplication::restoreOverrideCursor();
	return plot;
}

Graph3D* ApplicationWindow::plotXYZ(Table* table, const QString& zColName, int type)
{
	int zCol = table->colIndex(zColName);
	if (zCol < 0)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	Graph3D *plot = new Graph3D("", ws, 0);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	QString label = generateUniqueName(tr("Graph"));
	plot->setWindowTitle(label);
	plot->setName(label);

	customPlot3D(plot);
	if (type == Graph3D::Ribbon) {
		int ycol = table->colIndex(zColName);
		plot->addData(table, table->colName(table->colX(ycol)), zColName);
	} else
		plot->addData(table, table->colX(zCol), table->colY(zCol), zCol, type);
	initPlot3D(plot);

	emit modified();
	QApplication::restoreOverrideCursor();
	return plot;
}

Graph3D* ApplicationWindow::openPlotXYZ(const QString& caption,const QString& formula,
		double xl, double xr, double yl, double yr, double zl, double zr)
{
	int pos=formula.find("_",0);
	QString wCaption=formula.left(pos);

	Table* w=table(wCaption);
	if (!w)
		return 0;

	int posX=formula.find("(X)",pos);
	QString xColName=formula.mid(pos+1,posX-pos-1);

	pos=formula.find(",",posX);

	posX=formula.find("(Y)",pos);
	QString yColName=formula.mid(pos+1,posX-pos-1);

	pos=formula.find(",",posX);
	posX=formula.find("(Z)",pos);
	QString zColName=formula.mid(pos+1,posX-pos-1);

	int xCol=w->colIndex(xColName);
	int yCol=w->colIndex(yColName);
	int zCol=w->colIndex(zColName);

	Graph3D *plot=new Graph3D("", ws, 0);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	plot->loadData(w, xCol, yCol, zCol, xl, xr, yl, yr, zl, zr);

	QString label = caption;
	if (alreadyUsedName(label))
		label = generateUniqueName(tr("Graph"));

	plot->setWindowTitle(label);
	plot->setName(label);
	initPlot3D(plot);
	return plot;
}

void ApplicationWindow::customPlot3D(Graph3D *plot)
{
	plot->setDataColors(QColor(plot3DColors[4]), QColor(plot3DColors[0]));
	plot->setMeshColor(QColor(plot3DColors[2]));
	plot->setAxesColor(QColor(plot3DColors[6]));
	plot->setNumbersColor(QColor(plot3DColors[5]));
	plot->setLabelsColor(QColor(plot3DColors[1]));
	plot->setBackgroundColor(QColor(plot3DColors[7]));
	plot->setGridColor(QColor(plot3DColors[3]));
	plot->setResolution(plot3DResolution);
	plot->showColorLegend(showPlot3DLegend);
	plot->setAntialiasing(smooth3DMesh);
	plot->setOrthogonal(orthogonal3DPlots);
	if (showPlot3DProjection)
		plot->setFloorData();
	plot->setNumbersFont(plot3DNumbersFont);
	plot->setXAxisLabelFont(plot3DAxesFont);
	plot->setYAxisLabelFont(plot3DAxesFont);
	plot->setZAxisLabelFont(plot3DAxesFont);
	plot->setTitleFont(plot3DTitleFont);
}

void ApplicationWindow::initPlot3D(Graph3D *plot)
{
	current_folder->addWindow(plot);
	plot->setFolder(current_folder);
	ws->addWindow(plot);
	connectSurfacePlot(plot);

	plot->setIcon(QPixmap(trajectory_xpm));
	plot->show();
	plot->setFocus();

	addListViewItem(plot);

	if (!plot3DTools->isVisible())
		plot3DTools->show();

	if (!plot3DTools->isEnabled())
		plot3DTools->setEnabled(true);

	customMenu(plot);
	customToolBars(plot);
}

void ApplicationWindow::exportMatrix()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	ImageExportDialog *ied = new ImageExportDialog(this, m!=NULL, d_extended_export_dialog);
	ied->setDir(workingDir);
    ied->selectFilter(d_image_export_filter);
	if ( ied->exec() != QDialog::Accepted )
		return;
	workingDir = ied->directory().path();
	if (ied->selectedFiles().isEmpty())
		return;

	QString selected_filter = ied->selectedFilter();
	QString file_name = ied->selectedFiles()[0];
	QFileInfo file_info(file_name);
	if (!file_info.fileName().contains("."))
		file_name.append(selected_filter.remove("*"));

	QFile file(file_name);
	if (!file.open( QIODevice::WriteOnly )){
		QMessageBox::critical(this, tr("QtiPlot - Export error"),
        tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
		return;
	}

	if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") || selected_filter.contains(".ps"))
		m->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
	else {
		QList<QByteArray> list = QImageWriter::supportedImageFormats();
		for (int i=0; i<(int)list.count(); i++){
			if (selected_filter.contains("." + (list[i]).lower()))
				m->image().save(file_name, list[i], ied->quality());
		}
	}
}

Matrix* ApplicationWindow::importImage(const QString& fileName)
{
	QString fn = fileName;
	if (fn.isEmpty()){
		QList<QByteArray> list = QImageReader::supportedImageFormats();
		QString filter = tr("Images") + " (", aux1, aux2;
		for (int i=0; i<(int)list.count(); i++){
			aux1 = " *."+list[i]+" ";
			aux2 += " *."+list[i]+";;";
			filter += aux1;
		}
		filter+=");;" + aux2;

		fn = QFileDialog::getOpenFileName(this, tr("QtiPlot - Import image from file"), imagesDirPath, filter);
		if ( !fn.isEmpty() ){
			QFileInfo fi(fn);
			imagesDirPath = fi.dirPath(true);
		}
	}

    QImage image(fn);
    if (image.isNull())
        return 0;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QWidget *w = ws->activeWindow();
    Matrix* m = NULL;
    if (w && w->isA("Matrix")){
        m = (Matrix *)w;
        m->setImage(image);
    } else {
        m = new Matrix(scriptEnv, image, "", ws);
        m->setAttribute(Qt::WA_DeleteOnClose);
        initMatrix(m, generateUniqueName(tr("Matrix")));
        m->show();
        m->setWindowLabel(fn);
        m->setCaptionPolicy(MyWidget::Both);
        setListViewLabel(m->objectName(), fn);
    }

    QApplication::restoreOverrideCursor();
    return m;
}

void ApplicationWindow::loadImage()
{
	QList<QByteArray> list = QImageReader::supportedImageFormats();
	QString filter = tr("Images") + " (", aux1, aux2;
	for (int i=0; i<(int)list.count(); i++){
		aux1 = " *."+list[i]+" ";
		aux2 += " *."+list[i]+";;";
		filter += aux1;
	}
	filter+=");;" + aux2;

	QString fn = QFileDialog::getOpenFileName(this, tr("QtiPlot - Load image from file"), imagesDirPath, filter);
	if ( !fn.isEmpty() ){
		loadImage(fn);
		QFileInfo fi(fn);
		imagesDirPath = fi.dirPath(true);
	}
}

void ApplicationWindow::loadImage(const QString& fn)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MultiLayer *plot = multilayerPlot(generateUniqueName(tr("Graph")));
	plot->setWindowLabel(fn);
	plot->setCaptionPolicy(MyWidget::Both);
	setListViewLabel(plot->objectName(), fn);

	plot->showNormal();
	Graph *g = plot->addLayer(0, 0, plot->width(), plot->height()-20);

	g->setTitle("");
	for (int i=0; i<4; i++)
		g->enableAxis(i, false);
	g->removeLegend();
	g->addImage(fn);
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::polishGraph(Graph *g, int style)
{
	if (style == Graph::VerticalBars || style == Graph::HorizontalBars ||style == Graph::Histogram)
	{
		QList<int> ticksList;
		int ticksStyle = ScaleDraw::Out;
		ticksList<<ticksStyle<<ticksStyle<<ticksStyle<<ticksStyle;
		g->setMajorTicksType(ticksList);
		g->setMinorTicksType(ticksList);
	}
	if (style == Graph::HorizontalBars){
		g->setAxisTitle(QwtPlot::xBottom, tr("X Axis Title"));
		g->setAxisTitle(QwtPlot::yLeft, tr("Y Axis Title"));
	}
}

MultiLayer* ApplicationWindow::multilayerPlot(const QString& caption)
{
	MultiLayer* ml = new MultiLayer("", ws, 0);
	ml->setAttribute(Qt::WA_DeleteOnClose);
	QString label = caption;
	initMultilayerPlot(ml, label.replace(QRegExp("_"),"-"));
	return ml;
}

MultiLayer* ApplicationWindow::newGraph(const QString& caption)
{
	MultiLayer *ml = multilayerPlot(generateUniqueName(caption));
	if (ml) {
        Graph *g = ml->addLayer();
		setPreferences(g);
        g->newLegend();
        g->setAutoscaleFonts(false);
        g->setIgnoreResizeEvents(false);
        ml->arrangeLayers(false, false);
        ml->adjustSize();
        g->setAutoscaleFonts(autoScaleFonts);//restore user defined fonts behaviour
        g->setIgnoreResizeEvents(!autoResizeLayers);
        customMenu(ml);
    }
	return ml;
}

MultiLayer* ApplicationWindow::multilayerPlot(Table* w, const QStringList& colList, int style, int startRow, int endRow)
{//used when plotting selected columns
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MultiLayer* g = new MultiLayer("", ws, 0);
	g->setAttribute(Qt::WA_DeleteOnClose);

	Graph *ag = g->addLayer();
	if (!ag)
		return 0;

    setPreferences(ag);
	ag->addCurves(w, colList, style, defaultCurveLineWidth, defaultSymbolSize, startRow, endRow);

	initMultilayerPlot(g, generateUniqueName(tr("Graph")));

	polishGraph(ag, style);
	ag->newLegend();
	g->arrangeLayers(false, false);
	customMenu(g);

	emit modified();
	QApplication::restoreOverrideCursor();
	return g;
}

MultiLayer* ApplicationWindow::multilayerPlot(int c, int r, int style)
{//used when plotting from the panel menu
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return 0;

	Table *w = (Table*)ws->activeWindow();
	if (!validFor2DPlot(w))
		return 0;

	QStringList list = w->selectedYColumns();
	if((int)list.count() < 1) {
		QMessageBox::warning(this, tr("QtiPlot - Plot error"), tr("Please select a Y column to plot!"));
		return 0;
	}

	int curves = (int)list.count();
	if (r<0)
		r = curves;

	MultiLayer* g = new MultiLayer("", ws, 0);
	g->setAttribute(Qt::WA_DeleteOnClose);
	initMultilayerPlot(g, generateUniqueName(tr("Graph")));
	int layers = c*r;
	if (curves<layers) {
		for (int i=0; i<curves; i++) {
			Graph *ag = g->addLayer();
			if (ag){
                setPreferences(ag);
				ag->addCurves(w, QStringList(list[i]), style, defaultCurveLineWidth, defaultSymbolSize);
				ag->newLegend();
				ag->setAutoscaleFonts(false);//in order to avoid to small fonts
                ag->setIgnoreResizeEvents(false);
				polishGraph(ag, style);
			}
		}
	} else {
		for (int i=0; i<layers; i++) {
			Graph *ag = g->addLayer();
			if (ag) {
				QStringList lst;
				lst << list[i];
                setPreferences(ag);
				ag->addCurves(w, lst, style, defaultCurveLineWidth, defaultSymbolSize);
				ag->newLegend();
				ag->setAutoscaleFonts(false);//in order to avoid to small fonts
                ag->setIgnoreResizeEvents(false);
				polishGraph(ag, style);
			}
		}
	}
	g->setRows(r);
	g->setCols(c);
	g->arrangeLayers(false, false);
    g->adjustSize();
    QWidgetList lst = g->graphPtrs();
	foreach(QWidget *widget, lst) {
        Graph *ag = (Graph *)widget;
        ag->setAutoscaleFonts(autoScaleFonts);//restore user defined fonts behaviour
        ag->setIgnoreResizeEvents(!autoResizeLayers);
    }
	customMenu(g);
	emit modified();
	return g;
}

MultiLayer* ApplicationWindow::multilayerPlot(const QStringList& colList)
{//used when plotting from wizard
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	MultiLayer* g = new MultiLayer("", ws, 0);
	g->setAttribute(Qt::WA_DeleteOnClose);
	Graph *ag = g->addLayer();
	setPreferences(ag);
	polishGraph(ag, defaultCurveStyle);
	int curves = (int)colList.count();
	int errorBars = 0;
	for (int i=0; i<curves; i++) {
		if (colList[i].contains("(yErr)") || colList[i].contains("(xErr)"))
			errorBars++;
	}

	for (int i=0; i<curves; i++)
	{
		QString s = colList[i];
		int pos = s.find(":", 0);
		QString caption = s.left(pos) + "_";
		Table *w = (Table *)table(caption);

		int posX = s.find("(X)", pos);
		QString xColName = caption+s.mid(pos+2, posX-pos-2);
		int xCol=w->colIndex(xColName);

		posX = s.find(",", posX);
		int posY = s.find("(Y)", posX);
		QString yColName = caption+s.mid(posX+2, posY-posX-2);

		if (s.contains("(yErr)") || s.contains("(xErr)"))
		{
			posY = s.find(",", posY);
			int posErr, errType;
			if (s.contains("(yErr)"))
			{
				errType = QwtErrorPlotCurve::Vertical;
				posErr = s.find("(yErr)", posY);
			}
			else
			{
				errType = QwtErrorPlotCurve::Horizontal;
				posErr = s.find("(xErr)",posY);
			}

			QString errColName = caption+s.mid(posY+2, posErr-posY-2);
			ag->addErrorBars(xColName, yColName, w, errColName, errType);
		}
		else
            ag->insertCurve(w, xCol, yColName, defaultCurveStyle);

        CurveLayout cl = ag->initCurveLayout(defaultCurveStyle, curves - errorBars);
        cl.lWidth = defaultCurveLineWidth;
        cl.sSize = defaultSymbolSize;
        ag->updateCurveLayout(i, &cl);
	}
	ag->newLegend();
	ag->updatePlot();
    initMultilayerPlot(g, generateUniqueName(tr("Graph")));
    g->arrangeLayers(true, false);
	customMenu(g);
	emit modified();
	QApplication::restoreOverrideCursor();
	return g;
}

void ApplicationWindow::initMultilayerPlot(MultiLayer* g, const QString& name)
{
	QString label = name;
	while(alreadyUsedName(label))
		label = generateUniqueName(tr("Graph"));

	current_folder->addWindow(g);
	g->setFolder(current_folder);

	ws->addWindow(g);
	connectMultilayerPlot(g);

	g->setWindowTitle(label);
	g->setName(label);
	g->setIcon(QPixmap(graph_xpm));
	g->setScaleLayersOnPrint(d_scale_plots_on_print);
	g->printCropmarks(d_print_cropmarks);
	g->setLocale(d_locale);
	g->show();
	g->setFocus();

	addListViewItem(g);
}

void ApplicationWindow::customizeTables(const QColor& bgColor,const QColor& textColor,
		const QColor& headerColor,const QFont& textFont, const QFont& headerFont, bool showComments)
{
	tableBkgdColor = bgColor;
	tableTextColor = textColor;
	tableHeaderColor = headerColor;
	tableTextFont = textFont;
	tableHeaderFont = headerFont;
	d_show_table_comments = showComments;

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows){
		if (w->inherits("Table"))
			customTable((Table*)w);
	}
	delete windows;
}

void ApplicationWindow::customTable(Table* w)
{
	QColorGroup cg;
	cg.setColor(QColorGroup::Base, QColor(tableBkgdColor));
	cg.setColor(QColorGroup::Text, QColor(tableTextColor));
	w->setPalette(QPalette(cg, cg, cg));

	w->setHeaderColor (tableHeaderColor);
	w->setTextFont(tableTextFont);
	w->setHeaderFont(tableHeaderFont);
	w->showComments(d_show_table_comments);
	w->setNumericPrecision(d_decimal_digits);
	w->setLocale(d_locale);
}

void ApplicationWindow::setPreferences(Graph* g)
{
	if (!g->isPiePlot()){
		if (allAxesOn){
			for (int i=0; i<4; i++)
				g->enableAxis(i);
			g->updateSecondaryAxis(QwtPlot::xTop);
			g->updateSecondaryAxis(QwtPlot::yRight);
		}

		QList<int> ticksList;
		ticksList<<majTicksStyle<<majTicksStyle<<majTicksStyle<<majTicksStyle;
		g->setMajorTicksType(ticksList);
		ticksList.clear();
		ticksList<<minTicksStyle<<minTicksStyle<<minTicksStyle<<minTicksStyle;
		g->setMinorTicksType(ticksList);

		g->setTicksLength (minTicksLength, majTicksLength);
		g->setAxesLinewidth(axesLineWidth);
		g->drawAxesBackbones(drawBackbones);
	}

	g->initFonts(plotAxesFont, plotNumbersFont);
	g->setTextMarkerDefaults(legendFrameStyle, plotLegendFont, legendTextColor, legendBackground);
	g->setArrowDefaults(defaultArrowLineWidth, defaultArrowColor, defaultArrowLineStyle,
			defaultArrowHeadLength, defaultArrowHeadAngle, defaultArrowHeadFill);
	g->initTitle(titleOn, plotTitleFont);
	g->setCanvasFrame(canvasFrameWidth);
	g->plotWidget()->setMargin(defaultPlotMargin);
	g->enableAutoscaling(autoscale2DPlots);
	g->setAutoscaleFonts(autoScaleFonts);
    g->setIgnoreResizeEvents(!autoResizeLayers);
	g->setAntialiasing(antialiasing2DPlots);
	g->plotWidget()->setLocale(d_locale);
}

/*
 *used when importing an ASCII file
 */
Table* ApplicationWindow::newTable(const QString& fname, const QString &sep,
		int lines, bool renameCols, bool stripSpaces, bool simplifySpaces,
		bool importComments, const QString &commentString, bool readOnly)
{
	Table* w = new Table(scriptEnv, fname, sep, lines, renameCols, stripSpaces,
			simplifySpaces, importComments, commentString, readOnly, fname, ws, 0, 0);
	if (w){
		w->setAttribute(Qt::WA_DeleteOnClose);
		QFileInfo fi(fname);
		QString tableName = fi.baseName().remove(QRegExp("\\s")).replace("_", "-");
		initTable(w, generateUniqueName(tableName, false));
		w->show();
	}
	return w;
}

/*
 *creates a new empty table
 */
Table* ApplicationWindow::newTable()
{
	Table* w = new Table(scriptEnv, 30, 2, "", ws, 0);
	w->setAttribute(Qt::WA_DeleteOnClose);
	initTable(w, generateUniqueName(tr("Table")));
	w->showNormal();
	return w;
}

/*
 *used when opening a project file
 */
Table* ApplicationWindow::newTable(const QString& caption, int r, int c)
{
	Table* w = new Table(scriptEnv, r, c, "", ws, 0);
	w->setAttribute(Qt::WA_DeleteOnClose);
	initTable(w, caption);
	if (w->objectName() != caption){//the table was renamed
		renamedTables << caption << w->objectName();
		if (d_inform_rename_table){
			QMessageBox:: warning(this, tr("QtiPlot - Renamed Window"),
			tr("The table '%1' already exists. It has been renamed '%2'.").arg(caption).arg(w->objectName()));
		}
	}
	w->showNormal();
	return w;
}

Table* ApplicationWindow::newTable(int r, int c, const QString& name, const QString& legend)
{
	Table* w = new Table(scriptEnv, r, c, legend, ws, 0);
	w->setAttribute(Qt::WA_DeleteOnClose);
	initTable(w, name);
	return w;
}

Table* ApplicationWindow::newTable(const QString& caption, int r, int c, const QString& text)
{
	QStringList lst = caption.split("\t", QString::SkipEmptyParts);
    QString legend = QString();
    if (lst.count() == 2)
        legend = lst[1];

	Table* w = new Table(scriptEnv, r, c, legend, ws, 0);
	w->setAttribute(Qt::WA_DeleteOnClose);

	QStringList rows = text.split("\n", QString::SkipEmptyParts);
	QString rlist = rows[0];
	QStringList list = rlist.split("\t");
	w->setHeader(list);

	for (int i=0; i<r; i++)
	{
		rlist=rows[i+1];
		list=rlist.split("\t");
		for (int j=0; j<c; j++)
			w->setText(i, j, list[j]);
	}

	initTable(w, lst[0]);
	w->showNormal();
	return w;
}

Table* ApplicationWindow::newHiddenTable(const QString& name, const QString& label, int r, int c, const QString& text)
{
	Table* w = new Table(scriptEnv, r, c, label, 0, 0);
	w->setAttribute(Qt::WA_DeleteOnClose);

	if (!text.isEmpty()) {
		QStringList rows = text.split("\n", QString::SkipEmptyParts);
		QStringList list = rows[0].split("\t");
		w->setHeader(list);

		QString rlist;
		for (int i=0; i<r; i++){
			rlist=rows[i+1];
			list = rlist.split("\t");
			for (int j=0; j<c; j++)
				w->setText(i, j, list[j]);
		}
	}

	initTable(w, name);
	hideWindow(w);
	return w;
}

void ApplicationWindow::initTable(Table* w, const QString& caption)
{
	QString name = caption;
	name = name.replace ("_","-");

	while(name.isEmpty() || alreadyUsedName(name))
		name = generateUniqueName(tr("Table"));

	current_folder->addWindow(w);
	w->setFolder(current_folder);
	ws->addWindow(w);

	connectTable(w);
	customTable(w);

	tableWindows << name;
	w->setName(name);
	w->setIcon( QPixmap(worksheet_xpm) );
	w->setSpecifications(w->saveToString(windowGeometryInfo(w)));

	addListViewItem(w);
	emit modified();
}

/*
 * !creates a new table with type statistics on target columns/rows of table base
 */
TableStatistics *ApplicationWindow::newTableStatistics(Table *base, int type, QList<int> target, const QString &caption)
{
	TableStatistics* s = new TableStatistics(scriptEnv, ws, base, (TableStatistics::Type) type, target);
	if (caption.isEmpty())
		initTable(s, s->objectName());
	else
		initTable(s, caption);
	connect(base, SIGNAL(modifiedData(Table*,const QString&)), s, SLOT(update(Table*,const QString&)));
	connect(base, SIGNAL(changedColHeader(const QString&, const QString&)), s, SLOT(renameCol(const QString&, const QString&)));
	connect(base, SIGNAL(removedCol(const QString&)), s, SLOT(removeCol(const QString&)));
	s->showNormal();
	return s;
}

/*
 *creates a new empty note window
 */
Note* ApplicationWindow::newNote(const QString& caption)
{
	Note* m = new Note(scriptEnv, "", ws);
	if (caption.isEmpty())
		initNote(m, generateUniqueName(tr("Notes")));
	else
		initNote(m, caption);
	m->showNormal();
	return m;
}

void ApplicationWindow::initNote(Note* m, const QString& caption)
{
	QString name=caption;
	while(name.isEmpty() || alreadyUsedName(name))
		name = generateUniqueName(tr("Notes"));

	m->setName(name);
	m->setIcon( QPixmap(note_xpm) );
	m->askOnCloseEvent(confirmCloseNotes);
	m->setFolder(current_folder);
	m->setDirPath(scriptsDirPath);

	current_folder->addWindow(m);
	ws->addWindow(m);
	addListViewItem(m);

	connect(m->textWidget(), SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
	connect(m->textWidget(), SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));
	connect(m, SIGNAL(modifiedWindow(QWidget*)), this, SLOT(modifiedProject(QWidget*)));
	connect(m, SIGNAL(resizedWindow(QWidget*)),this,SLOT(modifiedProject(QWidget*)));
	connect(m, SIGNAL(closedWindow(MyWidget*)), this, SLOT(closeWindow(MyWidget*)));
	connect(m, SIGNAL(hiddenWindow(MyWidget*)), this, SLOT(hideWindow(MyWidget*)));
	connect(m, SIGNAL(statusChanged(MyWidget*)), this, SLOT(updateWindowStatus(MyWidget*)));
	connect(m, SIGNAL(showTitleBarMenu()), this, SLOT(showWindowTitleBarMenu()));
	connect(m, SIGNAL(dirPathChanged(const QString&)), this, SLOT(scriptsDirPathChanged(const QString&)));
	connect(m, SIGNAL(moved()), this, SLOT(modifiedProject()));

	emit modified();
}

Matrix* ApplicationWindow::newMatrix(int rows, int columns)
{
	Matrix* m = new Matrix(scriptEnv, rows, columns, "", ws, 0);
	m->setAttribute(Qt::WA_DeleteOnClose);
	QString caption = generateUniqueName(tr("Matrix"));
	initMatrix(m, caption);
	m->showNormal();
	return m;
}

Matrix* ApplicationWindow::newMatrix(const QString& caption, int r, int c)
{
	Matrix* w = new Matrix(scriptEnv, r, c, "", ws, 0);
	w->setAttribute(Qt::WA_DeleteOnClose);
	initMatrix(w, caption);
	if (w->objectName() != caption)//the matrix was renamed
		renamedTables << caption << w->objectName();

	w->showNormal();
	return w;
}

void ApplicationWindow::viewMatrixImage()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->setViewType(Matrix::ImageView);
	modifiedProject();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixTable()
{
	Matrix* m = static_cast<Matrix*>(ws->activeWindow());
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->setViewType(Matrix::TableView);
	modifiedProject();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixXY()
{
    Matrix* m = static_cast<Matrix*>(ws->activeWindow());
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->setHeaderViewType(Matrix::XY);
	modifiedProject();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::viewMatrixColumnRow()
{
    Matrix* m = static_cast<Matrix*>(ws->activeWindow());
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->setHeaderViewType(Matrix::ColumnRow);
	modifiedProject();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::setMatrixGrayScale()
{
	Matrix* m = static_cast<Matrix*>(ws->activeWindow());
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->setGrayScale();
	QApplication::restoreOverrideCursor();

	modifiedProject();
}

void ApplicationWindow::setMatrixRainbowScale()
{
	Matrix* m = static_cast<Matrix*>(ws->activeWindow());
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->setRainbowColorMap();
	QApplication::restoreOverrideCursor();

	modifiedProject();
}

void ApplicationWindow::showColorMapDialog()
{
	Matrix* m = static_cast<Matrix*>(ws->activeWindow());
	if (!m)
		return;

	ColorMapDialog *cmd = new ColorMapDialog(this);
	cmd->setAttribute(Qt::WA_DeleteOnClose);
	cmd->setMatrix(m);
	cmd->exec();
}

void ApplicationWindow::transposeMatrix()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->transpose();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::flipMatrixVertically()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->flipVertically();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::flipMatrixHorizontally()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->flipHorizontally();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::rotateMatrix90()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->rotate90();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::rotateMatrixMinus90()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->rotate90(false);
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::matrixDeterminant()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QDateTime dt = QDateTime::currentDateTime ();
	QString info=dt.toString(Qt::LocalDate);
	info+= "\n" + tr("Determinant of ") + QString(m->objectName()) + ":\t";
	info+= "det = " + QString::number(m->determinant()) + "\n";
	info+="-------------------------------------------------------------\n";

	logInfo+=info;

	showResults(true);
}

void ApplicationWindow::invertMatrix()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	m->invert();
}

Table* ApplicationWindow::convertMatrixToTable()
{
	Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return 0;

	return matrixToTable(m);
}

Table* ApplicationWindow::matrixToTable(Matrix* m)
{
	if (!m)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int rows = m->numRows();
	int cols = m->numCols();

	Table* w = new Table(scriptEnv, rows, cols, "", ws, 0);
	w->setAttribute(Qt::WA_DeleteOnClose);
	for (int i = 0; i<rows; i++){
		for (int j = 0; j<cols; j++)
			w->setCell(i, j, m->cell(i,j));
	}

	initTable(w, generateUniqueName(tr("Table")));
	w->setWindowLabel(m->windowLabel());
	w->setCaptionPolicy(m->captionPolicy());
	w->resize(m->size());
	w->showNormal();

	QApplication::restoreOverrideCursor();
	return w;
}

void ApplicationWindow::initMatrix(Matrix* m, const QString& caption)
{
	QString name = caption;
	while(alreadyUsedName(name)){name = generateUniqueName(tr("Matrix"));}

	m->setWindowTitle(name);
	m->setName(name);
	m->setIcon( QPixmap(matrix_xpm) );
	m->askOnCloseEvent(confirmCloseMatrix);
	m->setNumericPrecision(d_decimal_digits);
	m->setFolder(current_folder);
    m->setLocale(d_locale);

	current_folder->addWindow(m);
	ws->addWindow(m);
	addListViewItem(m);

	connect(m, SIGNAL(showTitleBarMenu()),this,SLOT(showWindowTitleBarMenu()));
	connect(m, SIGNAL(modifiedWindow(QWidget*)), this, SLOT(modifiedProject(QWidget*)));
	connect(m, SIGNAL(modifiedWindow(QWidget*)), this, SLOT(updateMatrixPlots(QWidget *)));
	connect(m, SIGNAL(resizedWindow(QWidget*)),this,SLOT(modifiedProject(QWidget*)));
	connect(m, SIGNAL(closedWindow(MyWidget*)), this, SLOT(closeWindow(MyWidget*)));
	connect(m, SIGNAL(hiddenWindow(MyWidget*)), this, SLOT(hideWindow(MyWidget*)));
	connect(m, SIGNAL(statusChanged(MyWidget*)),this, SLOT(updateWindowStatus(MyWidget*)));
	connect(m, SIGNAL(showContextMenu()), this, SLOT(showWindowContextMenu()));
	connect(m, SIGNAL(moved()), this, SLOT(modifiedProject()));

	emit modified();
}

Matrix* ApplicationWindow::convertTableToMatrix()
{
	Table* t = (Table*)ws->activeWindow();
	if (!t)
		return 0;

	return tableToMatrix (t);
}

Matrix* ApplicationWindow::tableToMatrix(Table* t)
{
	if (!t)
		return 0;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	int rows = t->numRows();
	int cols = t->numCols();

	QString caption = generateUniqueName(tr("Matrix"));
	Matrix* m = new Matrix(scriptEnv, rows, cols, "", ws, 0);
	initMatrix(m, caption);

	m->setAttribute(Qt::WA_DeleteOnClose);
	for (int i = 0; i<rows; i++){
		for (int j = 0; j<cols; j++)
			m->setCell(i, j, t->cell(i, j));
	}

	m->setWindowLabel(m->windowLabel());
	m->setCaptionPolicy(m->captionPolicy());
	m->resize(m->size());
	m->showNormal();

	QApplication::restoreOverrideCursor();
	return m;
}

QWidget* ApplicationWindow::window(const QString& name)
{
	QWidget* w = 0;
	QWidgetList *windows = windowsList();
	for (int i = 0; i < int(windows->count());i++ ){
		if (windows->at(i)->objectName() == name){
			w = windows->at(i);
			break;
		}
	}
	delete windows;
	return  w;
}

Table* ApplicationWindow::table(const QString& name)
{
	int pos = name.find("_", 0);
	QString caption = name.left(pos);

	QList<QWidget*> *lst = windowsList();
	foreach(QWidget *w, *lst){
		if (w->inherits("Table") && w->objectName() == caption){
			delete lst;
			return (Table*)w;
		}
	}
	delete lst;
	return  0;
}

Matrix* ApplicationWindow::matrix(const QString& name)
{
	QString caption = name;
	if (!renamedTables.isEmpty() && renamedTables.contains(caption)){
		int index = renamedTables.findIndex(caption);
		caption = renamedTables[index+1];
	}

	QWidgetList *lst = windowsList();
	foreach(QWidget *w, *lst){
		if (w->isA("Matrix") && w->objectName() == caption){
			delete lst;
			return (Matrix*)w;
		}
	}
	delete lst;
	return  0;
}

void ApplicationWindow::windowActivated(QWidget *w)
{
	if (!w || !w->inherits("MyWidget"))
		return;

	customToolBars(w);
	customMenu(w);

	Folder *f = ((MyWidget *)w)->folder();
	if (f)
        f->setActiveWindow((MyWidget *)w);

	emit modified();
}

void ApplicationWindow::addErrorBars()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g)
        return;

    if (!g->curves()){
		QMessageBox::warning(this, tr("QtiPlot - Warning"), tr("There are no curves available on this plot!"));
		return;
	}

	if (g->isPiePlot()){
        QMessageBox::warning(this, tr("QtiPlot - Warning"), tr("This functionality is not available for pie plots!"));
        return;
	}

    ErrDialog* ed = new ErrDialog(this);
    ed->setAttribute(Qt::WA_DeleteOnClose);
    connect (ed,SIGNAL(options(const QString&,int,const QString&,int)),this,SLOT(defineErrorBars(const QString&,int,const QString&,int)));
    connect (ed,SIGNAL(options(const QString&,const QString&,int)),this,SLOT(defineErrorBars(const QString&,const QString&,int)));

    ed->setCurveNames(g->analysableCurvesList());
    ed->setSrcTables(tableList());
    ed->exec();
}

void ApplicationWindow::defineErrorBars(const QString& name, int type, const QString& percent, int direction)
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	Table *w = table(name);
	if (!w)
	{ //user defined function
		QMessageBox::critical(this,tr("QtiPlot - Error bars error"),
				tr("This feature is not available for user defined function curves!"));
		return;
	}

	DataCurve *master_curve = (DataCurve *)g->curve(name);
	QString xColName = master_curve->xColumnName();
	if (xColName.isEmpty())
		return;

	if (direction == QwtErrorPlotCurve::Horizontal)
		w->addCol(Table::xErr);
	else
		w->addCol(Table::yErr);

	int r=w->numRows();
	int c=w->numCols()-1;
	int ycol=w->colIndex(name);
	if (!direction)
		ycol=w->colIndex(xColName);

	QVarLengthArray<double> Y(r);
	Y=w->col(ycol);
	QString errColName=w->colName(c);

	double prc=percent.toDouble();
	double moyenne=0.0;
	if (type==0){
		for (int i=0;i<r;i++){
			if (!w->text(i,ycol).isEmpty())
				w->setText(i, c, QString::number(Y[i]*prc/100.0,'g',15));
		}
	} else if (type==1) {
		int i;
		double dev=0.0;
		for (i=0;i<r;i++)
			moyenne+=Y[i];
		moyenne/=r;
		for (i=0;i<r;i++)
			dev+=(Y[i]-moyenne)*(Y[i]-moyenne);
		dev=sqrt(dev/(r-1));
		for (i=0;i<r;i++){
			if (!w->table()->item(i,ycol)->text().isEmpty())
				w->setText(i, c, QString::number(dev, 'g', 15));
		}
	}
	g->addErrorBars(xColName, name, w, errColName, direction);
}

void ApplicationWindow::defineErrorBars(const QString& curveName, const QString& errColumnName, int direction)
{
	Table *w=table(curveName);
	if (!w)
	{//user defined function --> no worksheet available
		QMessageBox::critical(this,tr("QtiPlot - Error"),
				tr("This feature is not available for user defined function curves!"));
		return;
	}

	Table *errTable=table(errColumnName);
	if (w->numRows() != errTable->numRows()){
		QMessageBox::critical(this,tr("QtiPlot - Error"),
				tr("The selected columns have different numbers of rows!"));

		addErrorBars();
		return;
	}

	int errCol=errTable->colIndex(errColumnName);
	if (errTable->isEmptyColumn(errCol)){
		QMessageBox::critical(this, tr("QtiPlot - Error"),
				tr("The selected error column is empty!"));
		addErrorBars();
		return;
	}

	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	g->addErrorBars(curveName, errTable, errColumnName, direction);
	emit modified();
}

void ApplicationWindow::removeCurves(const QString& name)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows)
	{
		if (w->isA("MultiLayer"))
		{
			QWidgetList lst= ((MultiLayer*)w)->graphPtrs();
			foreach(QWidget *widget, lst)
                ((Graph *)widget)->removeCurves(name);
		}
		else if (w->isA("Graph3D"))
		{
			if ( (((Graph3D*)w)->formula()).contains(name) )
				((Graph3D*)w)->clearData();
		}
	}
	delete windows;
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::updateCurves(Table *t, const QString& name)
{
	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows){
		if (w->isA("MultiLayer")){
			QWidgetList graphsList = ((MultiLayer*)w)->graphPtrs();
			for (int k=0; k<(int)graphsList.count(); k++){
				Graph* g = (Graph*)graphsList.at(k);
                if (g)
                    g->updateCurvesData(t, name);
			}
		} else if (w->isA("Graph3D")){
			Graph3D* g = (Graph3D*)w;
			if ((g->formula()).contains(name))
				g->updateData(t);
		}
	}
	delete windows;
}

void ApplicationWindow::showPreferencesDialog()
{
	ConfigDialog* cd= new ConfigDialog(this);
	cd->setAttribute(Qt::WA_DeleteOnClose);
	cd->setColumnSeparator(columnSeparator);
	cd->exec();
}

void ApplicationWindow::setSaveSettings(bool autoSaving, int min)
{
	if (autoSave==autoSaving && autoSaveTime==min)
		return;

	autoSave=autoSaving;
	autoSaveTime=min;

	killTimer(savingTimerId);

	if (autoSave)
		savingTimerId=startTimer(autoSaveTime*60000);
	else
		savingTimerId=0;
}

void ApplicationWindow::changeAppStyle(const QString& s)
{
	// style keys are case insensitive
	if (appStyle.toLower() == s.toLower())
		return;

	qApp->setStyle(s);
	appStyle = qApp->style()->objectName();

	QPalette pal = qApp->palette();
	pal.setColor (QPalette::Active, QPalette::Base, QColor(panelsColor));
	qApp->setPalette(pal);

}

void ApplicationWindow::changeAppFont(const QFont& f)
{
	if (appFont == f)
		return;

	appFont = f;
	updateAppFonts();
}

void ApplicationWindow::updateAppFonts()
{
	qApp->setFont(appFont);
	this->setFont(appFont);
	info->setFont(QFont(appFont.family(), 2 + appFont.pointSize(), QFont::Bold,false));
}

void ApplicationWindow::updateConfirmOptions(bool askTables, bool askMatrices, bool askPlots2D,
		bool askPlots3D, bool askNotes)
{
	QList<QWidget*> *windows = windowsList();
	if (confirmCloseTable != askTables){
		confirmCloseTable=askTables;
		for (int i = 0; i < int(windows->count());i++ ){
			if (windows->at(i)->inherits("Table"))
				((MyWidget*)windows->at(i))->askOnCloseEvent(confirmCloseTable);
		}
	}

	if (confirmCloseMatrix != askMatrices){
		confirmCloseMatrix = askMatrices;
		for (int i = 0; i < int(windows->count());i++ ){
			if (windows->at(i)->isA("Matrix"))
				((MyWidget*)windows->at(i))->askOnCloseEvent(confirmCloseMatrix);
		}
	}

	if (confirmClosePlot2D != askPlots2D){
		confirmClosePlot2D=askPlots2D;
		for (int i = 0; i < int(windows->count());i++ ){
			if (windows->at(i)->isA("MultiLayer"))
				((MyWidget*)windows->at(i))->askOnCloseEvent(confirmClosePlot2D);
		}
	}

	if (confirmClosePlot3D != askPlots3D){
		confirmClosePlot3D=askPlots3D;
		for (int i = 0; i < int(windows->count());i++ ){
			if (windows->at(i)->isA("Graph3D"))
				((MyWidget*)windows->at(i))->askOnCloseEvent(confirmClosePlot3D);
		}
	}

	if (confirmCloseNotes != askNotes){
		confirmCloseNotes = askNotes;
		for (int i = 0; i < int(windows->count());i++ ){
			if (windows->at(i)->isA("Note"))
				((MyWidget*)windows->at(i))->askOnCloseEvent(confirmCloseNotes);
		}
	}

	delete windows;
}

void ApplicationWindow::setGraphDefaultSettings(bool autoscale, bool scaleFonts,
												bool resizeLayers, bool antialiasing)
{
	if (autoscale2DPlots == autoscale &&
		autoScaleFonts == scaleFonts &&
		autoResizeLayers != resizeLayers &&
		antialiasing2DPlots == antialiasing)
		return;

	autoscale2DPlots = autoscale;
	autoScaleFonts = scaleFonts;
	autoResizeLayers = !resizeLayers;
	antialiasing2DPlots = antialiasing;

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows)
	{
		if (w->isA("MultiLayer"))
		{
			QWidgetList lst = ((MultiLayer*)w)->graphPtrs();
			Graph *g;
			foreach(QWidget *widget, lst)
			{
				g = (Graph *)widget;
				g->enableAutoscaling(autoscale2DPlots);
				g->updateScale();
				g->setIgnoreResizeEvents(!autoResizeLayers);
				g->setAutoscaleFonts(autoScaleFonts);
				g->setAntialiasing(antialiasing2DPlots);
			}
		}
	}
	delete windows;
}

void ApplicationWindow::setLegendDefaultSettings(int frame, const QFont& font,
		const QColor& textCol, const QColor& backgroundCol)
{
	if (legendFrameStyle == frame &&
			legendTextColor == textCol &&
			legendBackground == backgroundCol &&
			plotLegendFont == font)
		return;

	legendFrameStyle = frame;
	legendTextColor = textCol;
	legendBackground = backgroundCol;
	plotLegendFont = font;

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows)
	{
		if (w->isA("MultiLayer"))
		{
			QWidgetList graphsList = ((MultiLayer*)w)->graphPtrs();
			foreach(QWidget *widget, graphsList)
				((Graph *)widget)->setTextMarkerDefaults(frame, font, textCol, backgroundCol);
		}
	}
	delete windows;
	saveSettings();
}

void ApplicationWindow::setArrowDefaultSettings(int lineWidth,  const QColor& c, Qt::PenStyle style,
		int headLength, int headAngle, bool fillHead)
{
	if (defaultArrowLineWidth == lineWidth &&
			defaultArrowColor == c &&
			defaultArrowLineStyle == style &&
			defaultArrowHeadLength == headLength &&
			defaultArrowHeadAngle == headAngle &&
			defaultArrowHeadFill == fillHead)
		return;

	defaultArrowLineWidth = lineWidth;
	defaultArrowColor = c;
	defaultArrowLineStyle = style;
	defaultArrowHeadLength = headLength;
	defaultArrowHeadAngle = headAngle;
	defaultArrowHeadFill = fillHead;

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows)
	{
		if (w->isA("MultiLayer"))
		{
			QWidgetList graphsList = ((MultiLayer*)w)->graphPtrs();
			foreach(QWidget *widget, graphsList)
				((Graph *)widget)->setArrowDefaults(defaultArrowLineWidth, defaultArrowColor,

					defaultArrowLineStyle, defaultArrowHeadLength,
					defaultArrowHeadAngle, defaultArrowHeadFill);
		}
	}
	delete windows;
	saveSettings();
}

ApplicationWindow * ApplicationWindow::plotFile(const QString& fn)
{
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	ApplicationWindow *app = new ApplicationWindow();
	app->applyUserSettings();
	app->restoreApplicationGeometry();

	Table* t = app->newTable(fn, app->columnSeparator, 0, true, app->strip_spaces, app->simplify_spaces,
                app->d_ASCII_import_comments, app->d_ASCII_comment_string, app->d_ASCII_import_read_only);
	t->setCaptionPolicy(MyWidget::Both);
	app->multilayerPlot(t, t->YColumns(),Graph::LineSymbols);
	QApplication::restoreOverrideCursor();
	return 0;
}

void ApplicationWindow::importASCII()
{
	ImportASCIIDialog *import_dialog = new ImportASCIIDialog(ws->activeWindow() && ws->activeWindow()->inherits("Table"), this, d_extended_import_ASCII_dialog);
	import_dialog->setDir(asciiDirPath);
	import_dialog->selectFilter(d_ASCII_file_filter);
	if (import_dialog->exec() != QDialog::Accepted)
		return;

	asciiDirPath = import_dialog->directory().path();
    d_ASCII_import_mode = import_dialog->importMode();
    columnSeparator = import_dialog->columnSeparator();
    ignoredLines = import_dialog->ignoredLines();
    renameColumns = import_dialog->renameColumns();
    strip_spaces = import_dialog->stripSpaces();
    simplify_spaces = import_dialog->simplifySpaces();
    d_ASCII_import_locale = import_dialog->decimalSeparators();
    d_import_dec_separators = import_dialog->updateDecimalSeparators();
    d_ASCII_comment_string = import_dialog->commentString();
    d_ASCII_import_comments = import_dialog->importComments();
    d_ASCII_import_read_only = import_dialog->readOnly();
    saveSettings();

	importASCII(import_dialog->selectedFiles(),
			import_dialog->importMode(),
			import_dialog->columnSeparator(),
			import_dialog->ignoredLines(),
			import_dialog->renameColumns(),
			import_dialog->stripSpaces(),
			import_dialog->simplifySpaces(),
			import_dialog->importComments(),
			import_dialog->updateDecimalSeparators(),
			import_dialog->decimalSeparators(),
			import_dialog->commentString(),
			import_dialog->readOnly());
}

void ApplicationWindow::importASCII(const QStringList& files, int import_mode, const QString& local_column_separator,
        int local_ignored_lines, bool local_rename_columns, bool local_strip_spaces, bool local_simplify_spaces,
        bool local_import_comments, bool update_dec_separators, QLocale local_separators, const QString& local_comment_string,
		bool import_read_only)
{
	if (files.isEmpty())
		return;

	switch(import_mode) {
		case ImportASCIIDialog::NewTables:
			{
				int dx, dy;
				QStringList sorted_files = files;
				sorted_files.sort();
				for (int i=0; i<sorted_files.size(); i++){
					Table *w = newTable(sorted_files[i], local_column_separator, local_ignored_lines,
                                        local_rename_columns, local_strip_spaces, local_simplify_spaces,
                                        local_import_comments, local_comment_string, import_read_only);
					if (!w) continue;
					w->setCaptionPolicy(MyWidget::Both);
					setListViewLabel(w->objectName(), sorted_files[i]);
					if (i==0) {
						dx = w->verticalHeaderWidth();
						dy = w->parentWidget()->frameGeometry().height() - w->height();
						w->parentWidget()->move(QPoint(0,0));
					} else
						w->parentWidget()->move(QPoint(i*dx,i*dy));

					if (update_dec_separators)
						w->updateDecimalSeparators(local_separators);
				}
				modifiedProject();
				break;
			}
		case ImportASCIIDialog::NewColumns:
		case ImportASCIIDialog::NewRows:
			{
				Table *t = (Table*) ws->activeWindow();
				if (t && t->inherits("Table")){
					for (int i=0; i<files.size(); i++)
                        t->importMultipleASCIIFiles(files[i], local_column_separator, local_ignored_lines, local_rename_columns,
							local_strip_spaces, local_simplify_spaces, local_import_comments,
							local_comment_string, import_read_only, import_mode);

					t->setWindowLabel(files.join("; "));
					t->setCaptionPolicy(MyWidget::Name);
					if (update_dec_separators)
						t->updateDecimalSeparators(local_separators);
					t->notifyChanges();
					emit modifiedProject(t);
				}
				break;
			}
		case ImportASCIIDialog::Overwrite:
			{
				Table *t = (Table*) ws->activeWindow();
				if ( t && t->inherits("Table")){
					t->importASCII(files[0], local_column_separator, local_ignored_lines, local_rename_columns,
                                    local_strip_spaces, local_simplify_spaces, local_import_comments, false,
                                    local_comment_string, import_read_only);
					if (update_dec_separators)
						t->updateDecimalSeparators(local_separators);
					t->setWindowLabel(files[0]);
					t->notifyChanges();
				} else {
					t = newTable(files[0], local_column_separator, local_ignored_lines, local_rename_columns,
                                 local_strip_spaces, local_simplify_spaces, local_import_comments,
                                 local_comment_string, import_read_only);
					if (update_dec_separators)
						t->updateDecimalSeparators(local_separators);
				}

				if (t){
					t->setCaptionPolicy(MyWidget::Both);
					setListViewLabel(t->objectName(), files[0]);
					modifiedProject(t);
				}
				break;
			}
	}
}

void ApplicationWindow::open()
{
	OpenProjectDialog *open_dialog = new OpenProjectDialog(this, d_extended_open_dialog);
	open_dialog->setDirectory(workingDir);
	if (open_dialog->exec() != QDialog::Accepted || open_dialog->selectedFiles().isEmpty())
		return;
	workingDir = open_dialog->directory().path();

	switch(open_dialog->openMode()) {
		case OpenProjectDialog::NewProject:
			{
				QString fn = open_dialog->selectedFiles()[0];
				QFileInfo fi(fn);

				if (projectname != "untitled"){
					QFileInfo fi(projectname);
					QString pn = fi.absFilePath();
					if (fn == pn){
						QMessageBox::warning(this, tr("QtiPlot - File openning error"),
								tr("The file: <b>%1</b> is the current file!").arg(fn));
						return;
					}
				}

				if (fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti~",Qt::CaseInsensitive) ||
					fn.endsWith(".opj",Qt::CaseInsensitive) || fn.endsWith(".ogm",Qt::CaseInsensitive) ||
					fn.endsWith(".ogw",Qt::CaseInsensitive) || fn.endsWith(".ogg",Qt::CaseInsensitive) ||
					fn.endsWith(".qti.gz",Qt::CaseInsensitive))
				{
					if (!fi.exists ()){
						QMessageBox::critical(this, tr("QtiPlot - File openning error"),
								tr("The file: <b>%1</b> doesn't exist!").arg(fn));
						return;
					}

					saveSettings();//the recent projects must be saved

					ApplicationWindow *a = open (fn);
					if (a){
						a->workingDir = workingDir;
						if (fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti~",Qt::CaseInsensitive) ||
                            fn.endsWith(".opj",Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive) ||
							fn.endsWith(".qti.gz",Qt::CaseInsensitive))
                            this->close();
					}
				} else {
					QMessageBox::critical(this,tr("QtiPlot - File openning error"),
							tr("The file: <b>%1</b> is not a QtiPlot or Origin project file!").arg(fn));
					return;
				}
				break;
			}
		case OpenProjectDialog::NewFolder:
			appendProject(open_dialog->selectedFiles()[0]);
			break;
	}
}

ApplicationWindow* ApplicationWindow::open(const QString& fn, bool factorySettings)
{
	if (fn.endsWith(".opj", Qt::CaseInsensitive) || fn.endsWith(".ogm", Qt::CaseInsensitive) ||
		fn.endsWith(".ogw", Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive))
		return importOPJ(fn);
	else if (fn.endsWith(".py", Qt::CaseInsensitive))
		return loadScript(fn);
	else if (!( fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti.gz",Qt::CaseInsensitive) ||
                fn.endsWith(".qti~",Qt::CaseInsensitive)))
		return plotFile(fn);

	QString fname = fn;
	if (fn.endsWith(".qti.gz", Qt::CaseInsensitive)){//decompress using zlib
		file_uncompress((char *)fname.ascii());
		fname = fname.left(fname.size() - 3);
	}

	QFile f(fname);
	QTextStream t( &f );
	f.open(QIODevice::ReadOnly);
	QString s = t.readLine();
    QStringList list = s.split(QRegExp("\\s"), QString::SkipEmptyParts);
    if (list.count() < 2 || list[0] != "QtiPlot"){
        f.close();
        if (QFile::exists(fname + "~")){
            int choice = QMessageBox::question(this, tr("QtiPlot - File opening error"),
					tr("The file <b>%1</b> is corrupted, but there exists a backup copy.<br>Do you want to open the backup instead?").arg(fn),
					QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape);
            if (choice == QMessageBox::Yes)
                return open(fname + "~");
            else
                QMessageBox::critical(this, tr("QtiPlot - File opening error"),  tr("The file: <b> %1 </b> was not created using QtiPlot!").arg(fn));
            return 0;
		}
    }

    QStringList vl = list[1].split(".", QString::SkipEmptyParts);
    d_file_version = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();

	ApplicationWindow* app = openProject(fname, factorySettings);

	f.close();
	return app;
}

void ApplicationWindow::openRecentProject(int index)
{
	QString fn = recent->text(index);
	int pos = fn.find(" ",0);
	fn = fn.right(fn.length()-pos-1);

	QFile f(fn);
	if (!f.exists()){
		QMessageBox::critical(this,tr("QtiPlot - File Open Error"),
				tr("The file: <b> %1 </b> <p>does not exist anymore!"
					"<p>It will be removed from the list.").arg(fn));

		recentProjects.remove(fn);
        updateRecentProjectsList();
		return;
	}

	if (projectname != "untitled"){
		QFileInfo fi(projectname);
		QString pn = fi.absFilePath();
		if (fn == pn){
			QMessageBox::warning(this, tr("QtiPlot - File openning error"),
					tr("The file: <p><b> %1 </b><p> is the current file!").arg(fn));
			return;
		}
	}

	if (!fn.isEmpty()){
		saveSettings();//the recent projects must be saved
		ApplicationWindow * a = open (fn);
		if (a && (fn.endsWith(".qti",Qt::CaseInsensitive) || fn.endsWith(".qti~",Qt::CaseInsensitive) ||
            fn.endsWith(".opj",Qt::CaseInsensitive) || fn.endsWith(".ogg", Qt::CaseInsensitive)))
			this->close();
	}
}

ApplicationWindow* ApplicationWindow::openProject(const QString& fn, bool factorySettings)
{
	ApplicationWindow *app = new ApplicationWindow(factorySettings);
	app->applyUserSettings();
	app->projectname = fn;
	app->d_file_version = d_file_version;
	app->setWindowTitle(tr("QtiPlot") + " - " + fn);

	QFile f(fn);
	QTextStream t( &f );
	t.setEncoding(QTextStream::UnicodeUTF8);
	f.open(QIODevice::ReadOnly);

	QFileInfo fi(fn);
	QString baseName = fi.fileName();

	t.readLine();
	if (d_file_version < 73)
		t.readLine();

	QString s = t.readLine();
	QStringList list=s.split("\t", QString::SkipEmptyParts);
	if (list[0] == "<scripting-lang>")
	{
		if (!app->setScriptingLanguage(list[1], true))
			QMessageBox::warning(app, tr("QtiPlot - File opening error"),
					tr("The file \"%1\" was created using \"%2\" as scripting language.\n\n"\
						"Initializing support for this language FAILED; I'm using \"%3\" instead.\n"\
						"Various parts of this file may not be displayed as expected.")\
					.arg(fn).arg(list[1]).arg(scriptEnv->name()));

		s = t.readLine();
		list=s.split("\t", QString::SkipEmptyParts);
	}
	int aux=0,widgets=list[1].toInt();

	QString titleBase = tr("Window") + ": ";
	QString title = titleBase + "1/"+QString::number(widgets)+"  ";

	QProgressDialog progress(this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setRange(0, widgets);
	progress.setMinimumWidth(app->width()/2);
	progress.setWindowTitle(tr("QtiPlot - Opening file") + ": " + baseName);
	progress.setLabelText(title);
	progress.setActiveWindow();

	Folder *cf = app->projectFolder();
	app->folders->blockSignals (true);
	app->blockSignals (true);
	//rename project folder item
	FolderListItem *item = (FolderListItem *)app->folders->firstChild();
	item->setText(0, fi.baseName());
	item->folder()->setObjectName(fi.baseName());

	//process tables and matrix information
	while ( !t.atEnd() && !progress.wasCanceled()){
		s = t.readLine();
		list.clear();
		if  (s.left(8) == "<folder>"){
			list = s.split("\t");
			Folder *f = new Folder(app->current_folder, list[1]);
			f->setBirthDate(list[2]);
			f->setModificationDate(list[3]);
			if(list.count() > 4)
				if (list[4] == "current")
					cf = f;

			FolderListItem *fli = new FolderListItem(app->current_folder->folderListItem(), f);
			f->setFolderListItem(fli);

			app->current_folder = f;
		} else if  (s == "<table>") {
			title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
			QStringList lst;
			while ( s!="</table>" ){
				s=t.readLine();
				lst<<s;
			}
			lst.pop_back();
			openTable(app,lst);
			progress.setValue(aux);
		} else if (s.left(17)=="<TableStatistics>") {
			QStringList lst;
			while ( s!="</TableStatistics>" ){
				s=t.readLine();
				lst<<s;
			}
			lst.pop_back();
			app->openTableStatistics(lst);
		} else if  (s == "<matrix>") {
			title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
			QStringList lst;
			while ( s != "</matrix>" ) {
				s=t.readLine();
				lst<<s;
			}
			lst.pop_back();
			openMatrix(app, lst);
			progress.setValue(aux);
		} else if  (s == "<note>") {
			title= titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
			for (int i=0; i<3; i++){
				s = t.readLine();
				list << s;
			}
			Note* m = openNote(app,list);
			QStringList cont;
			while ( s != "</note>" ){
				s=t.readLine();
				cont << s;
			}
			cont.pop_back();
			m->restore(cont);
			progress.setValue(aux);
		} else if  (s == "</folder>") {
			Folder *parent = (Folder *)app->current_folder->parent();
			if (!parent)
				app->current_folder = app->projectFolder();
			else
				app->current_folder = parent;
		}
	}
	f.close();

	if (progress.wasCanceled()){
		app->saved = true;
		app->close();
		return 0;
	}

	//process the rest
	f.open(QIODevice::ReadOnly);

	MultiLayer *plot=0;
	while ( !t.atEnd() && !progress.wasCanceled()){
		s=t.readLine();
		if  (s.left(8) == "<folder>"){
			list = s.split("\t");
			app->current_folder = app->current_folder->findSubfolder(list[1]);
		} else if  (s == "<multiLayer>")
		{//process multilayers information
			title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);

			s=t.readLine();
			QStringList graph=s.split("\t");
			QString caption=graph[0];
			plot = app->multilayerPlot(caption);
			plot->setCols(graph[1].toInt());
			plot->setRows(graph[2].toInt());

			app->setListViewDate(caption, graph[3]);
			plot->setBirthDate(graph[3]);

			restoreWindowGeometry(app, plot, t.readLine());
			plot->blockSignals(true);

			if (d_file_version > 71)
			{
				QStringList lst=t.readLine().split("\t");
				plot->setWindowLabel(lst[1]);
				app->setListViewLabel(plot->objectName(),lst[1]);
				plot->setCaptionPolicy((MyWidget::CaptionPolicy)lst[2].toInt());
			}
			if (d_file_version > 83)
			{
				QStringList lst=t.readLine().split("\t", QString::SkipEmptyParts);
				plot->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
				lst=t.readLine().split("\t", QString::SkipEmptyParts);
				plot->setSpacing(lst[1].toInt(),lst[2].toInt());
				lst=t.readLine().split("\t", QString::SkipEmptyParts);
				plot->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());
				lst=t.readLine().split("\t", QString::SkipEmptyParts);
				plot->setAlignement(lst[1].toInt(),lst[2].toInt());
			}

			while ( s!="</multiLayer>" )
			{//open layers
				s = t.readLine();
				if (s.left(7)=="<graph>")
				{
					list.clear();
					while ( s!="</graph>" )
					{
						s=t.readLine();
						list<<s;
					}
					openGraph(app, plot, list);
				}
			}
			plot->blockSignals(false);
			progress.setValue(aux);
		}
		else if  (s == "<SurfacePlot>")
		{//process 3D plots information
			list.clear();
			title = titleBase + QString::number(++aux)+"/"+QString::number(widgets);
			progress.setLabelText(title);
			while ( s!="</SurfacePlot>" )
			{
				s=t.readLine();
				list<<s;
			}
			openSurfacePlot(app,list);
			progress.setValue(aux);
		}
		else if  (s == "</folder>")
		{
			Folder *parent = (Folder *)app->current_folder->parent();
			if (!parent)
				app->current_folder = projectFolder();
			else
				app->current_folder = parent;
		}
		else if  (s.left(5)=="<log>")
		{//process analysis information
			s = t.readLine();
			while ( s != "</log>" )
			{
				app->logInfo+= s+"\n";
				s = t.readLine();
			}
			app->results->setText(app->logInfo);
		}
	}
	f.close();

	if (progress.wasCanceled())
	{
		app->saved = true;
		app->close();
		return 0;
	}

	app->logInfo=app->logInfo.remove ("</log>\n", false);

	QFileInfo fi2(f);
	QString fileName = fi2.absFilePath();

	app->recentProjects.remove(fileName);
	app->recentProjects.push_front(fileName);
	app->updateRecentProjectsList();

	app->folders->setCurrentItem(cf->folderListItem());
	app->folders->blockSignals (false);
	//change folder to user defined current folder
	app->changeFolder(cf, true);

	app->blockSignals (false);
	app->renamedTables.clear();

	app->restoreApplicationGeometry();
	app->executeNotes();
    app->savedProject();
	return app;
}

void ApplicationWindow::executeNotes()
{
	QList<MyWidget *> lst = projectFolder()->windowsList();
	foreach(MyWidget *widget, lst)
		if (widget->isA("Note") && ((Note*)widget)->autoexec())
			((Note*)widget)->executeAll();
}

void ApplicationWindow::scriptError(const QString &message, const QString &scriptName, int lineNumber)
{
	QMessageBox::critical(this, tr("QtiPlot") + " - "+ tr("Script Error"), message);
}

void ApplicationWindow::scriptPrint(const QString &text)
{
#ifdef SCRIPTING_CONSOLE
	if(!text.stripWhiteSpace().isEmpty()) console->append(text);
#else
	printf(text.ascii());
#endif
}

bool ApplicationWindow::setScriptingLanguage(const QString &lang, bool force)
{
	if (!force && lang == scriptEnv->name()) return true;
	if (lang.isEmpty()) return false;

	ScriptingEnv *newEnv = ScriptingLangManager::newEnv(lang, this);
	if (!newEnv)
		return false;

	connect(newEnv, SIGNAL(error(const QString&,const QString&,int)),
			this, SLOT(scriptError(const QString&,const QString&,int)));
	connect(newEnv, SIGNAL(print(const QString&)), this, SLOT(scriptPrint(const QString&)));
	if (!newEnv->initialize()){
		delete newEnv;
		return false;
	}

	// notify everyone who might be interested
	ScriptingChangeEvent *sce = new ScriptingChangeEvent(newEnv);
	QApplication::sendEvent(this, sce);
	delete sce;

	foreach(QObject *i, findChildren<QObject*>())
		QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));
	if (scriptWindow)
		foreach(QObject *i, scriptWindow->findChildren<QObject*>())
			QApplication::postEvent(i, new ScriptingChangeEvent(newEnv));

	return true;
}

void ApplicationWindow::showScriptingLangDialog()
{
	ScriptingLangDialog* d = new ScriptingLangDialog(scriptEnv, this);
	d->exec();
}

void ApplicationWindow::restartScriptingEnv()
{
	if (setScriptingLanguage(scriptEnv->name(), true))
		executeNotes();
	else
		QMessageBox::critical(this, tr("QtiPlot - Scripting Error"),
				tr("Scripting language \"%1\" failed to initialize.").arg(scriptEnv->name()));
}

void ApplicationWindow::openTemplate()
{
	QString filter = "QtiPlot 2D Graph Template (*.qpt);;";
	filter += "QtiPlot 3D Surface Template (*.qst);;";
	filter += "QtiPlot Table Template (*.qtt);;";
	filter += "QtiPlot Matrix Template (*.qmt);;";

	QString fn = QFileDialog::getOpenFileName(this, tr("QtiPlot - Open Template File"), templatesDir, filter);
	if (!fn.isEmpty()){
		QFileInfo fi(fn);
		templatesDir = fi.dirPath(true);
		if (fn.contains(".qmt") || fn.contains(".qpt") || fn.contains(".qtt") || fn.contains(".qst"))
			openTemplate(fn);
		else {
			QMessageBox::critical(this,tr("QtiPlot - File opening error"),
					tr("The file: <b>%1</b> is not a QtiPlot template file!").arg(fn));
			return;
		}
	}
}

MyWidget* ApplicationWindow::openTemplate(const QString& fn)
{
	if (fn.isEmpty() || !QFile::exists(fn)){
		QMessageBox::critical(this, tr("QtiPlot - File opening error"),
					tr("The file: <b>%1</b> doesn't exist!").arg(fn));
		return 0;
	}

	QFile f(fn);
	QTextStream t(&f);
	t.setEncoding(QTextStream::UnicodeUTF8);
	f.open(QIODevice::ReadOnly);
	QStringList l=t.readLine().split(QRegExp("\\s"), QString::SkipEmptyParts);
	QString fileType=l[0];
	if (fileType != "QtiPlot"){
		QMessageBox::critical(this,tr("QtiPlot - File opening error"),
						tr("The file: <b> %1 </b> was not created using QtiPlot!").arg(fn));
		return 0;
	}

	QStringList vl = l[1].split(".", QString::SkipEmptyParts);
	d_file_version = 100*(vl[0]).toInt()+10*(vl[1]).toInt()+(vl[2]).toInt();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	MyWidget *w = 0;
	QString templateType;
	t>>templateType;

	if (templateType == "<SurfacePlot>") {
		t.skipWhiteSpace();
		QStringList lst;
		while (!t.atEnd())
			lst << t.readLine();
		w = openSurfacePlot(this,lst);
		if (w)
			((Graph3D *)w)->clearData();
	} else {
		int rows, cols;
		t>>rows; t>>cols;
		t.skipWhiteSpace();
		QString geometry = t.readLine();

		if (templateType == "<multiLayer>"){
			w = multilayerPlot(generateUniqueName(tr("Graph")));
			if (w){
				((MultiLayer*)w)->setCols(cols);
				((MultiLayer*)w)->setRows(rows);
				restoreWindowGeometry(this, w, geometry);
				if (d_file_version > 83){
					QStringList lst=t.readLine().split("\t", QString::SkipEmptyParts);
					((MultiLayer*)w)->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
					lst=t.readLine().split("\t", QString::SkipEmptyParts);
					((MultiLayer*)w)->setSpacing(lst[1].toInt(),lst[2].toInt());
					lst=t.readLine().split("\t", QString::SkipEmptyParts);
					((MultiLayer*)w)->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());
					lst=t.readLine().split("\t", QString::SkipEmptyParts);
					((MultiLayer*)w)->setAlignement(lst[1].toInt(),lst[2].toInt());
				}
				while (!t.atEnd()){//open layers
					QString s=t.readLine();
					if (s.left(7)=="<graph>"){
						QStringList lst;
						while ( s!="</graph>" ){
							s = t.readLine();
							lst << s;
						}
					openGraph(this, (MultiLayer*)w, lst);
					}
				}
			}
		} else {
			if (templateType == "<table>")
				w = newTable(tr("Table1"), rows, cols);
			else if (templateType == "<matrix>")
				w = newMatrix(rows, cols);
			if (w){
				QStringList lst;
				while (!t.atEnd())
					lst << t.readLine();
				w->restore(lst);
				restoreWindowGeometry(this, w, geometry);
			}
		}
	}

	f.close();
	if (w){
		customMenu((QWidget*)w);
		customToolBars((QWidget*)w);
	}

	QApplication::restoreOverrideCursor();
	return w;
}

void ApplicationWindow::readSettings()
{
#ifdef Q_OS_MAC // Mac
	QSettings settings(QSettings::IniFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#else
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#endif

	/* ---------------- group General --------------- */
	settings.beginGroup("/General");
	settings.beginGroup("/ApplicationGeometry");//main window geometry
	d_app_rect = QRect(settings.value("/x", 0).toInt(), settings.value("/y", 0).toInt(),
				 settings.value("/width", 0).toInt(), settings.value("/height", 0).toInt());
	settings.endGroup();

	autoSearchUpdates = settings.value("/AutoSearchUpdates", false).toBool();
#ifdef QTIPLOT_DEMO
	// don't overdo it.. ;)
	askForSupport = settings.value("/Support", false).toBool();
#else
	askForSupport = settings.value("/Support", true).toBool();
#endif
	appLanguage = settings.value("/Language", QLocale::system().name().section('_',0,0)).toString();
	show_windows_policy = (ShowWindowsPolicy)settings.value("/ShowWindowsPolicy", ApplicationWindow::ActiveFolder).toInt();

    recentProjects = settings.value("/RecentProjects").toStringList();
    //Follows an ugly hack added by Ion in order to fix Qt4 porting issues
    //(only needed on Windows due to a Qt bug?)
#ifdef Q_OS_WIN
	if (!recentProjects.isEmpty() && recentProjects[0].contains("^e"))
        recentProjects = recentProjects[0].split("^e", QString::SkipEmptyParts);
    else if (recentProjects.count() == 1){
        QString s = recentProjects[0];
        if (s.remove(QRegExp("\\s")).isEmpty())
            recentProjects = QStringList();
    }
#endif

	updateRecentProjectsList();

	changeAppStyle(settings.value("/Style", appStyle).toString());
	autoSave = settings.value("/AutoSave",true).toBool();
	autoSaveTime = settings.value("/AutoSaveTime",15).toInt();
    d_backup_files = settings.value("/BackupProjects", true).toBool();
	d_init_window_type = (WindowType)settings.value("/InitWindow", TableWindow).toInt();
	defaultScriptingLang = settings.value("/ScriptingLang","muParser").toString();
	d_thousands_sep = settings.value("/ThousandsSeparator", true).toBool();
	d_locale = QLocale(settings.value("/Locale", QLocale::system().name()).toString());
	if (!d_thousands_sep)
        d_locale.setNumberOptions(QLocale::OmitGroupSeparator);

	d_decimal_digits = settings.value("/DecimalDigits", 13).toInt();

	//restore dock windows and tool bars
	restoreState(settings.value("/DockWindows").toByteArray());
	explorerSplitter->restoreState(settings.value("/ExplorerSplitter").toByteArray());
	QList<int> lst = explorerSplitter->sizes();
	for (int i=0; i< lst.count(); i++){
		if (lst[i] == 0){
			lst[i] = 45;
			explorerSplitter->setSizes(lst);
		}
	}

	QStringList applicationFont = settings.value("/Font").toStringList();
	if (applicationFont.size() == 4)
		appFont=QFont (applicationFont[0],applicationFont[1].toInt(),applicationFont[2].toInt(),applicationFont[3].toInt());

	settings.beginGroup("/Dialogs");
	d_extended_open_dialog = settings.value("/ExtendedOpenDialog", true).toBool();
	d_extended_export_dialog = settings.value("/ExtendedExportDialog", true).toBool();
	d_extended_import_ASCII_dialog = settings.value("/ExtendedImportAsciiDialog", true).toBool();
	d_extended_plot_dialog = settings.value("/ExtendedPlotDialog", true).toBool();//used by PlotDialog

	settings.beginGroup("/AddRemoveCurves");
	d_add_curves_dialog_size = QSize(settings.value("/Width", 700).toInt(),
									settings.value("/Height", 400).toInt());
	d_show_current_folder = settings.value("/ShowCurrentFolder", false).toBool();
	settings.endGroup(); // AddRemoveCurves Dialog
	settings.endGroup(); // Dialogs

	settings.beginGroup("/Colors");
	workspaceColor = settings.value("/Workspace","darkGray").value<QColor>();
	// see http://doc.trolltech.com/4.2/qvariant.html for instructions on qcolor <-> qvariant conversion
	panelsColor = settings.value("/Panels","#ffffff").value<QColor>();
	panelsTextColor = settings.value("/PanelsText","#000000").value<QColor>();
	settings.endGroup(); // Colors

	settings.beginGroup("/Paths");
    workingDir = settings.value("/WorkingDir", qApp->applicationDirPath()).toString();
#ifdef Q_OS_WIN
	helpFilePath = settings.value("/HelpFile", qApp->applicationDirPath()+"/manual/index.html").toString();
	fitPluginsPath = settings.value("/FitPlugins", "fitPlugins").toString();
	templatesDir = settings.value("/TemplatesDir", qApp->applicationDirPath()).toString();
	asciiDirPath = settings.value("/ASCII", qApp->applicationDirPath()).toString();
	imagesDirPath = settings.value("/Images", qApp->applicationDirPath()).toString();
#else
	QVariant help_file_setting = settings.value("/HelpFile");
	if (help_file_setting.isValid())
		helpFilePath = help_file_setting.toString();
	else {
		QString locale = QLocale().name(); // language_country according to ISO 639 and 3166, respectively
		QStringList help_dir_suffixes;
		help_dir_suffixes << locale << locale.section('_',0,0) << appLanguage << "en";
		QFileInfo help_file_info;
		foreach (QString suffix, help_dir_suffixes) {
			help_file_info.setFile(QString("/usr/share/doc/qtiplot/manual-%1/index.html").arg(suffix));
			if (help_file_info.exists())
				break;
		}
		// intentionally defaults to .../manual-en/index.html even if it doesn't exist
		helpFilePath = help_file_info.absoluteFilePath();
	}
	fitPluginsPath = settings.value("/FitPlugins", "/usr/lib/qtiplot/plugins").toString();
	templatesDir = settings.value("/TemplatesDir", QDir::homePath()).toString();
	asciiDirPath = settings.value("/ASCII", QDir::homePath()).toString();
	imagesDirPath = settings.value("/Images", QDir::homePath()).toString();
    workingDir = settings.value("/WorkingDir", QDir::homePath()).toString();
#endif
	scriptsDirPath = settings.value("/ScriptsDir", qApp->applicationDirPath()).toString();
	fitModelsPath = settings.value("/FitModelsDir", "").toString();
	customActionsDirPath = settings.value("/CustomActionsDir", "").toString();
	settings.endGroup(); // Paths
	settings.endGroup();
	/* ------------- end group General ------------------- */

	settings.beginGroup("/UserFunctions");
	if (100*maj_version + 10*min_version + patch_version == 91 &&
        settings.contains("/FitFunctions")){
        saveFitFunctions(settings.value("/FitFunctions").toStringList());
		settings.remove("/FitFunctions");
	}
	surfaceFunc = settings.value("/SurfaceFunctions").toStringList();
	xFunctions = settings.value("/xFunctions").toStringList();
	yFunctions = settings.value("/yFunctions").toStringList();
	rFunctions = settings.value("/rFunctions").toStringList();
	thetaFunctions = settings.value("/thetaFunctions").toStringList();
	d_param_surface_func = settings.value("/ParametricSurfaces").toStringList();
	settings.endGroup(); // UserFunctions

	settings.beginGroup("/Confirmations");
	confirmCloseFolder = settings.value("/Folder", true).toBool();
	confirmCloseTable = settings.value("/Table", true).toBool();
	confirmCloseMatrix = settings.value("/Matrix", true).toBool();
	confirmClosePlot2D = settings.value("/Plot2D", true).toBool();
	confirmClosePlot3D = settings.value("/Plot3D", true).toBool();
	confirmCloseNotes = settings.value("/Note", true).toBool();
	d_inform_rename_table = settings.value("/RenameTable", true).toBool();
	settings.endGroup(); // Confirmations


	/* ---------------- group Tables --------------- */
	settings.beginGroup("/Tables");
	d_show_table_comments = settings.value("/DisplayComments", false).toBool();
	QStringList tableFonts = settings.value("/Fonts").toStringList();
	if (tableFonts.size() == 8)
	{
		tableTextFont=QFont (tableFonts[0],tableFonts[1].toInt(),tableFonts[2].toInt(),tableFonts[3].toInt());
		tableHeaderFont=QFont (tableFonts[4],tableFonts[5].toInt(),tableFonts[6].toInt(),tableFonts[7].toInt());
	}

	settings.beginGroup("/Colors");
	tableBkgdColor = settings.value("/Background","#ffffff").value<QColor>();
	tableTextColor = settings.value("/Text","#000000").value<QColor>();
	tableHeaderColor = settings.value("/Header","#000000").value<QColor>();
	settings.endGroup(); // Colors
	settings.endGroup();
	/* --------------- end group Tables ------------------------ */

	/* --------------- group 2D Plots ----------------------------- */
	settings.beginGroup("/2DPlots");
	settings.beginGroup("/General");
	titleOn = settings.value("/Title", true).toBool();
	allAxesOn = settings.value("/AllAxes", false).toBool();
	canvasFrameWidth = settings.value("/CanvasFrameWidth", 0).toInt();
	defaultPlotMargin = settings.value("/Margin", 0).toInt();
	drawBackbones = settings.value("/AxesBackbones", true).toBool();
	axesLineWidth = settings.value("/AxesLineWidth", 1).toInt();
	autoscale2DPlots = settings.value("/Autoscale", true).toBool();
	autoScaleFonts = settings.value("/AutoScaleFonts", true).toBool();
	autoResizeLayers = settings.value("/AutoResizeLayers", true).toBool();
	antialiasing2DPlots = settings.value("/Antialiasing", true).toBool();
	d_scale_plots_on_print = settings.value("/ScaleLayersOnPrint", false).toBool();
	d_print_cropmarks = settings.value("/PrintCropmarks", false).toBool();

	QStringList graphFonts = settings.value("/Fonts").toStringList();
	if (graphFonts.size() == 16) {
		plotAxesFont=QFont (graphFonts[0],graphFonts[1].toInt(),graphFonts[2].toInt(),graphFonts[3].toInt());
		plotNumbersFont=QFont (graphFonts[4],graphFonts[5].toInt(),graphFonts[6].toInt(),graphFonts[7].toInt());
		plotLegendFont=QFont (graphFonts[8],graphFonts[9].toInt(),graphFonts[10].toInt(),graphFonts[11].toInt());
		plotTitleFont=QFont (graphFonts[12],graphFonts[13].toInt(),graphFonts[14].toInt(),graphFonts[15].toInt());
	}
	settings.endGroup(); // General

	settings.beginGroup("/Curves");
	defaultCurveStyle = settings.value("/Style", Graph::LineSymbols).toInt();
	defaultCurveLineWidth = settings.value("/LineWidth", 1).toInt();
	defaultSymbolSize = settings.value("/SymbolSize", 7).toInt();
	settings.endGroup(); // Curves

	settings.beginGroup("/Ticks");
	majTicksStyle = settings.value("/MajTicksStyle", ScaleDraw::Out).toInt();
	minTicksStyle = settings.value("/MinTicksStyle", ScaleDraw::Out).toInt();
	minTicksLength = settings.value("/MinTicksLength", 5).toInt();
	majTicksLength = settings.value("/MajTicksLength", 9).toInt();
	settings.endGroup(); // Ticks

	settings.beginGroup("/Legend");
	legendFrameStyle = settings.value("/FrameStyle", LegendWidget::Line).toInt();
	legendTextColor = settings.value("/TextColor", "#000000").value<QColor>(); //default color Qt::black
	legendBackground = settings.value("/BackgroundColor", "#ffffff").value<QColor>(); //default color Qt::white
	legendBackground.setAlpha(settings.value("/Transparency", 0).toInt()); // transparent by default;
	settings.endGroup(); // Legend

	settings.beginGroup("/Arrows");
	defaultArrowLineWidth = settings.value("/Width", 1).toInt();
	defaultArrowColor = settings.value("/Color", "#000000").value<QColor>();//default color Qt::black
	defaultArrowHeadLength = settings.value("/HeadLength", 4).toInt();
	defaultArrowHeadAngle = settings.value("/HeadAngle", 45).toInt();
	defaultArrowHeadFill = settings.value("/HeadFill", true).toBool();
	defaultArrowLineStyle = Graph::getPenStyle(settings.value("/LineStyle", "SolidLine").toString());
	settings.endGroup(); // Arrows
	settings.endGroup();
	/* ----------------- end group 2D Plots --------------------------- */

	/* ----------------- group 3D Plots --------------------------- */
	settings.beginGroup("/3DPlots");
	showPlot3DLegend = settings.value("/Legend",true).toBool();
	showPlot3DProjection = settings.value("/Projection", false).toBool();
	smooth3DMesh = settings.value("/Antialiasing", true).toBool();
	plot3DResolution = settings.value ("/Resolution", 1).toInt();
	orthogonal3DPlots = settings.value("/Orthogonal", false).toBool();
	autoscale3DPlots = settings.value ("/Autoscale", true).toBool();

	QStringList plot3DFonts = settings.value("/Fonts").toStringList();
	if (plot3DFonts.size() == 12){
		plot3DTitleFont=QFont (plot3DFonts[0],plot3DFonts[1].toInt(),plot3DFonts[2].toInt(),plot3DFonts[3].toInt());
		plot3DNumbersFont=QFont (plot3DFonts[4],plot3DFonts[5].toInt(),plot3DFonts[6].toInt(),plot3DFonts[7].toInt());
		plot3DAxesFont=QFont (plot3DFonts[8],plot3DFonts[9].toInt(),plot3DFonts[10].toInt(),plot3DFonts[11].toInt());
	}

	settings.beginGroup("/Colors");
	plot3DColors = QStringList();
	plot3DColors << QColor(settings.value("/MaxData", "blue").value<QColor>()).name();
	plot3DColors << QColor(settings.value("/Labels", "#000000").value<QColor>()).name();
	plot3DColors << QColor(settings.value("/Mesh", "#000000").value<QColor>()).name();
	plot3DColors << QColor(settings.value("/Grid", "#000000").value<QColor>()).name();
	plot3DColors << QColor(settings.value("/MinData", "red").value<QColor>()).name();
	plot3DColors << QColor(settings.value("/Numbers", "#000000").value<QColor>()).name();
	plot3DColors << QColor(settings.value("/Axes", "#000000").value<QColor>()).name();
	plot3DColors << QColor(settings.value("/Background", "#ffffff").value<QColor>()).name();
	settings.endGroup(); // Colors
	settings.endGroup();
	/* ----------------- end group 3D Plots --------------------------- */

	settings.beginGroup("/Fitting");
	fit_output_precision = settings.value("/OutputPrecision", 15).toInt();
	pasteFitResultsToPlot = settings.value("/PasteResultsToPlot", false).toBool();
	writeFitResultsToLog = settings.value("/WriteResultsToLog", true).toBool();
	generateUniformFitPoints = settings.value("/GenerateFunction", true).toBool();
	fitPoints = settings.value("/Points", 100).toInt();
	generatePeakCurves = settings.value("/GeneratePeakCurves", true).toBool();
	peakCurvesColor = settings.value("/PeaksColor", 2).toInt();//green color
	fit_scale_errors = settings.value("/ScaleErrors", true).toBool();
	d_2_linear_fit_points = settings.value("/TwoPointsLinearFit", true).toBool();
	settings.endGroup(); // Fitting

	settings.beginGroup("/ImportASCII");
	columnSeparator = settings.value("/ColumnSeparator", "\\t").toString();
	columnSeparator.replace("\\t", "\t").replace("\\s", " ");
	ignoredLines = settings.value("/IgnoreLines", 0).toInt();
	renameColumns = settings.value("/RenameColumns", true).toBool();
	strip_spaces = settings.value("/StripSpaces", false).toBool();
	simplify_spaces = settings.value("/SimplifySpaces", false).toBool();
	d_ASCII_file_filter = settings.value("/AsciiFileTypeFilter", "*").toString();
	d_ASCII_import_locale = settings.value("/AsciiImportLocale", QLocale::system().name()).toString();
	d_import_dec_separators = settings.value("/UpdateDecSeparators", true).toBool();
	d_ASCII_import_mode = settings.value("/ImportMode", ImportASCIIDialog::NewTables).toInt();
	d_ASCII_comment_string = settings.value("/CommentString", "#").toString();
	d_ASCII_import_comments = settings.value("/ImportComments", false).toBool();
    d_ASCII_import_read_only = settings.value("/ImportReadOnly", false).toBool();
	d_ASCII_import_preview = settings.value("/Preview", true).toBool();
	d_preview_lines = settings.value("/PreviewLines", 100).toInt();
	settings.endGroup(); // Import ASCII

	settings.beginGroup("/ExportASCII");
	d_export_col_separator = settings.value("/ColumnSeparator", "\\t").toString();
	d_export_col_separator.replace("\\t", "\t").replace("\\s", " ");
	d_export_col_names = settings.value("/ExportLabels", false).toBool();
    d_export_col_comment = settings.value("/ExportComments", false).toBool();

	d_export_table_selection = settings.value("/ExportSelection", false).toBool();
	settings.endGroup(); // ExportASCII

    settings.beginGroup("/ExportImage");
	d_image_export_filter = settings.value("/ImageFileTypeFilter", ".png").toString();
	d_export_transparency = settings.value("/ExportTransparency", false).toBool();
	d_export_quality = settings.value("/ImageQuality", 100).toInt();
	d_export_resolution = settings.value("/Resolution", QPrinter().resolution()).toInt();
	d_export_color = settings.value("/ExportColor", true).toBool();
	d_export_vector_size = settings.value("/ExportPageSize", QPrinter::Custom).toInt();
	d_keep_plot_aspect = settings.value("/KeepAspect", true).toBool();
	settings.endGroup(); // ExportImage

	settings.beginGroup("/ScriptWindow");
	d_script_win_on_top = settings.value("/AlwaysOnTop", true).toBool();
	d_script_win_rect = QRect(settings.value("/x", 0).toInt(), settings.value("/y", 0).toInt(),
							settings.value("/width", 500).toInt(), settings.value("/height", 300).toInt());
	settings.endGroup();

	settings.beginGroup("/ToolBars");
	d_file_tool_bar = settings.value("/FileToolBar", true).toBool();
    d_edit_tool_bar = settings.value("/EditToolBar", true).toBool();
	d_table_tool_bar = settings.value("/TableToolBar", true).toBool();
	d_column_tool_bar = settings.value("/ColumnToolBar", true).toBool();
    d_matrix_tool_bar = settings.value("/MatrixToolBar", true).toBool();
	d_plot_tool_bar = settings.value("/PlotToolBar", true).toBool();
	d_plot3D_tool_bar = settings.value("/Plot3DToolBar", true).toBool();
	d_display_tool_bar = settings.value("/DisplayToolBar", false).toBool();
	d_format_tool_bar = settings.value("/FormatToolBar", true).toBool();
	settings.endGroup();
}

void ApplicationWindow::saveSettings()
{

#ifdef Q_OS_MAC // Mac
	QSettings settings(QSettings::IniFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#else
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#endif

	/* ---------------- group General --------------- */
	settings.beginGroup("/General");

	settings.beginGroup("/ApplicationGeometry");
	d_app_rect = QRect(this->pos(), this->size());
	if (this->isMaximized())
		d_app_rect = QRect();

	settings.setValue("/x", d_app_rect.x());
	settings.setValue("/y", d_app_rect.y());
	settings.setValue("/width", d_app_rect.width());
	settings.setValue("/height", d_app_rect.height());
	settings.endGroup();

	settings.setValue("/AutoSearchUpdates", autoSearchUpdates);
	settings.setValue("/Support", askForSupport);
	settings.setValue("/Language", appLanguage);
	settings.setValue("/ShowWindowsPolicy", show_windows_policy);
	settings.setValue("/RecentProjects", recentProjects);
	settings.setValue("/Style", appStyle);
	settings.setValue("/AutoSave", autoSave);
	settings.setValue("/AutoSaveTime", autoSaveTime);
	settings.setValue("/BackupProjects", d_backup_files);
	settings.setValue("/InitWindow", int(d_init_window_type));

	settings.setValue("/ScriptingLang", defaultScriptingLang);
    settings.setValue("/ThousandsSeparator", d_thousands_sep);
	settings.setValue("/Locale", d_locale.name());
	settings.setValue("/DecimalDigits", d_decimal_digits);

	settings.setValue("/DockWindows", saveState());
	settings.setValue("/ExplorerSplitter", explorerSplitter->saveState());

	QStringList applicationFont;
	applicationFont<<appFont.family();
	applicationFont<<QString::number(appFont.pointSize());
	applicationFont<<QString::number(appFont.weight());
	applicationFont<<QString::number(appFont.italic());
	settings.setValue("/Font", applicationFont);

	settings.beginGroup("/Dialogs");
	settings.setValue("/ExtendedOpenDialog", d_extended_open_dialog);
	settings.setValue("/ExtendedExportDialog", d_extended_export_dialog);
	settings.setValue("/ExtendedImportAsciiDialog", d_extended_import_ASCII_dialog);
	settings.setValue("/ExtendedPlotDialog", d_extended_plot_dialog);
	settings.beginGroup("/AddRemoveCurves");
	settings.setValue("/Width", d_add_curves_dialog_size.width());
	settings.setValue("/Height", d_add_curves_dialog_size.height());
	settings.setValue("/ShowCurrentFolder", d_show_current_folder);
	settings.endGroup(); // AddRemoveCurves Dialog
	settings.endGroup(); // Dialogs

	settings.beginGroup("/Colors");
	settings.setValue("/Workspace", workspaceColor);
	settings.setValue("/Panels", panelsColor);
	settings.setValue("/PanelsText", panelsTextColor);
	settings.endGroup(); // Colors

	settings.beginGroup("/Paths");
	settings.setValue("/WorkingDir", workingDir);
	settings.setValue("/TemplatesDir", templatesDir);
	settings.setValue("/HelpFile", helpFilePath);
	settings.setValue("/FitPlugins", fitPluginsPath);
	settings.setValue("/ASCII", asciiDirPath);
	settings.setValue("/Images", imagesDirPath);
	settings.setValue("/ScriptsDir", scriptsDirPath);
    settings.setValue("/FitModelsDir", fitModelsPath);
    settings.setValue("/CustomActionsDir", customActionsDirPath);
	settings.endGroup(); // Paths
	settings.endGroup();
	/* ---------------- end group General --------------- */

	settings.beginGroup("/UserFunctions");
	settings.setValue("/SurfaceFunctions", surfaceFunc);
	settings.setValue("/xFunctions", xFunctions);
	settings.setValue("/yFunctions", yFunctions);
	settings.setValue("/rFunctions", rFunctions);
	settings.setValue("/thetaFunctions", thetaFunctions);
    settings.setValue("/ParametricSurfaces", d_param_surface_func);
	settings.endGroup(); // UserFunctions

	settings.beginGroup("/Confirmations");
	settings.setValue("/Folder", confirmCloseFolder);
	settings.setValue("/Table", confirmCloseTable);
	settings.setValue("/Matrix", confirmCloseMatrix);
	settings.setValue("/Plot2D", confirmClosePlot2D);
	settings.setValue("/Plot3D", confirmClosePlot3D);
	settings.setValue("/Note", confirmCloseNotes);
	settings.setValue("/RenameTable", d_inform_rename_table);
	settings.endGroup(); // Confirmations

	/* ----------------- group Tables -------------- */
	settings.beginGroup("/Tables");
	settings.setValue("/DisplayComments", d_show_table_comments);
	QStringList tableFonts;
	tableFonts<<tableTextFont.family();
	tableFonts<<QString::number(tableTextFont.pointSize());
	tableFonts<<QString::number(tableTextFont.weight());
	tableFonts<<QString::number(tableTextFont.italic());
	tableFonts<<tableHeaderFont.family();
	tableFonts<<QString::number(tableHeaderFont.pointSize());
	tableFonts<<QString::number(tableHeaderFont.weight());
	tableFonts<<QString::number(tableHeaderFont.italic());
	settings.setValue("/Fonts", tableFonts);

	settings.beginGroup("/Colors");
	settings.setValue("/Background", tableBkgdColor);
	settings.setValue("/Text", tableTextColor);
	settings.setValue("/Header", tableHeaderColor);
	settings.endGroup(); // Colors
	settings.endGroup();
	/* ----------------- end group Tables ---------- */

	/* ----------------- group 2D Plots ------------ */
	settings.beginGroup("/2DPlots");
	settings.beginGroup("/General");
	settings.setValue("/Title", titleOn);
	settings.setValue("/AllAxes", allAxesOn);
	settings.setValue("/CanvasFrameWidth", canvasFrameWidth);
	settings.setValue("/Margin", defaultPlotMargin);
	settings.setValue("/AxesBackbones", drawBackbones);
	settings.setValue("/AxesLineWidth", axesLineWidth);
	settings.setValue("/Autoscale", autoscale2DPlots);
	settings.setValue("/AutoScaleFonts", autoScaleFonts);
	settings.setValue("/AutoResizeLayers", autoResizeLayers);
	settings.setValue("/Antialiasing", antialiasing2DPlots);
	settings.setValue("/ScaleLayersOnPrint", d_scale_plots_on_print);
	settings.setValue("/PrintCropmarks", d_print_cropmarks);

	QStringList graphFonts;
	graphFonts<<plotAxesFont.family();
	graphFonts<<QString::number(plotAxesFont.pointSize());
	graphFonts<<QString::number(plotAxesFont.weight());
	graphFonts<<QString::number(plotAxesFont.italic());
	graphFonts<<plotNumbersFont.family();
	graphFonts<<QString::number(plotNumbersFont.pointSize());
	graphFonts<<QString::number(plotNumbersFont.weight());
	graphFonts<<QString::number(plotNumbersFont.italic());
	graphFonts<<plotLegendFont.family();
	graphFonts<<QString::number(plotLegendFont.pointSize());
	graphFonts<<QString::number(plotLegendFont.weight());
	graphFonts<<QString::number(plotLegendFont.italic());
	graphFonts<<plotTitleFont.family();
	graphFonts<<QString::number(plotTitleFont.pointSize());
	graphFonts<<QString::number(plotTitleFont.weight());
	graphFonts<<QString::number(plotTitleFont.italic());
	settings.setValue("/Fonts", graphFonts);
	settings.endGroup(); // General

	settings.beginGroup("/Curves");
	settings.setValue("/Style", defaultCurveStyle);
	settings.setValue("/LineWidth", defaultCurveLineWidth);
	settings.setValue("/SymbolSize", defaultSymbolSize);
	settings.endGroup(); // Curves

	settings.beginGroup("/Ticks");
	settings.setValue ("/MajTicksStyle", majTicksStyle);
	settings.setValue ("/MinTicksStyle", minTicksStyle);
	settings.setValue("/MinTicksLength", minTicksLength);
	settings.setValue("/MajTicksLength", majTicksLength);
	settings.endGroup(); // Ticks

	settings.beginGroup("/Legend");
	settings.setValue("/FrameStyle", legendFrameStyle);
	settings.setValue("/TextColor", legendTextColor);
	settings.setValue("/BackgroundColor", legendBackground);
	settings.setValue("/Transparency", legendBackground.alpha());
	settings.endGroup(); // Legend

	settings.beginGroup("/Arrows");
	settings.setValue("/Width", defaultArrowLineWidth);
	settings.setValue("/Color", defaultArrowColor.name());
	settings.setValue("/HeadLength", defaultArrowHeadLength);
	settings.setValue("/HeadAngle", defaultArrowHeadAngle);
	settings.setValue("/HeadFill", defaultArrowHeadFill);
	settings.setValue("/LineStyle", Graph::penStyleName(defaultArrowLineStyle));
	settings.endGroup(); // Arrows
	settings.endGroup();
	/* ----------------- end group 2D Plots -------- */

	/* ----------------- group 3D Plots ------------ */
	settings.beginGroup("/3DPlots");
	settings.setValue("/Legend", showPlot3DLegend);
	settings.setValue("/Projection", showPlot3DProjection);
	settings.setValue("/Antialiasing", smooth3DMesh);
	settings.setValue("/Resolution", plot3DResolution);
	settings.setValue("/Orthogonal", orthogonal3DPlots);
	settings.setValue("/Autoscale", autoscale3DPlots);

	QStringList plot3DFonts;
	plot3DFonts<<plot3DTitleFont.family();
	plot3DFonts<<QString::number(plot3DTitleFont.pointSize());
	plot3DFonts<<QString::number(plot3DTitleFont.weight());
	plot3DFonts<<QString::number(plot3DTitleFont.italic());
	plot3DFonts<<plot3DNumbersFont.family();
	plot3DFonts<<QString::number(plot3DNumbersFont.pointSize());
	plot3DFonts<<QString::number(plot3DNumbersFont.weight());
	plot3DFonts<<QString::number(plot3DNumbersFont.italic());
	plot3DFonts<<plot3DAxesFont.family();
	plot3DFonts<<QString::number(plot3DAxesFont.pointSize());
	plot3DFonts<<QString::number(plot3DAxesFont.weight());
	plot3DFonts<<QString::number(plot3DAxesFont.italic());
	settings.setValue("/Fonts", plot3DFonts);

	settings.beginGroup("/Colors");
	settings.setValue("/MaxData", plot3DColors[0]);
	settings.setValue("/Labels", plot3DColors[1]);
	settings.setValue("/Mesh", plot3DColors[2]);
	settings.setValue("/Grid", plot3DColors[3]);
	settings.setValue("/MinData", plot3DColors[4]);
	settings.setValue("/Numbers", plot3DColors[5]);
	settings.setValue("/Axes", plot3DColors[6]);
	settings.setValue("/Background", plot3DColors[7]);
	settings.endGroup(); // Colors
	settings.endGroup();
	/* ----------------- end group 2D Plots -------- */

	settings.beginGroup("/Fitting");
	settings.setValue("/OutputPrecision", fit_output_precision);
	settings.setValue("/PasteResultsToPlot", pasteFitResultsToPlot);
	settings.setValue("/WriteResultsToLog", writeFitResultsToLog);
	settings.setValue("/GenerateFunction", generateUniformFitPoints);
	settings.setValue("/Points", fitPoints);
	settings.setValue("/GeneratePeakCurves", generatePeakCurves);
	settings.setValue("/PeaksColor", peakCurvesColor);
	settings.setValue("/ScaleErrors", fit_scale_errors);
	settings.setValue("/TwoPointsLinearFit", d_2_linear_fit_points);
	settings.endGroup(); // Fitting

	settings.beginGroup("/ImportASCII");
	QString sep = columnSeparator;
	settings.setValue("/ColumnSeparator", sep.replace("\t", "\\t").replace(" ", "\\s"));
	settings.setValue("/IgnoreLines", ignoredLines);
	settings.setValue("/RenameColumns", renameColumns);
	settings.setValue("/StripSpaces", strip_spaces);
	settings.setValue("/SimplifySpaces", simplify_spaces);
    settings.setValue("/AsciiFileTypeFilter", d_ASCII_file_filter);
	settings.setValue("/AsciiImportLocale", d_ASCII_import_locale.name());
	settings.setValue("/UpdateDecSeparators", d_import_dec_separators);
    settings.setValue("/ImportMode", d_ASCII_import_mode);
    settings.setValue("/CommentString", d_ASCII_comment_string);
    settings.setValue("/ImportComments", d_ASCII_import_comments);
    settings.setValue("/ImportReadOnly", d_ASCII_import_read_only);
	settings.setValue("/Preview", d_ASCII_import_preview);
	settings.setValue("/PreviewLines", d_preview_lines);
	settings.endGroup(); // ImportASCII

	settings.beginGroup("/ExportASCII");
	sep = d_export_col_separator;
	settings.setValue("/ColumnSeparator", sep.replace("\t", "\\t").replace(" ", "\\s"));
	settings.setValue("/ExportLabels", d_export_col_names);
	settings.setValue("/ExportComments", d_export_col_comment);
	settings.setValue("/ExportSelection", d_export_table_selection);
	settings.endGroup(); // ExportASCII

    settings.beginGroup("/ExportImage");
	settings.setValue("/ImageFileTypeFilter", d_image_export_filter);
	settings.setValue("/ExportTransparency", d_export_transparency);
	settings.setValue("/ImageQuality", d_export_quality);
	settings.setValue("/Resolution", d_export_resolution);
	settings.setValue("/ExportColor", d_export_color);
	settings.setValue("/ExportPageSize", d_export_vector_size);
	settings.setValue("/KeepAspect", d_keep_plot_aspect);
	settings.endGroup(); // ExportImage

	settings.beginGroup("/ScriptWindow");
	settings.setValue("/AlwaysOnTop", d_script_win_on_top);
	settings.setValue("/x", d_script_win_rect.x());
	settings.setValue("/y", d_script_win_rect.y());
	settings.setValue("/width", d_script_win_rect.width());
	settings.setValue("/height", d_script_win_rect.height());
	settings.endGroup();

    settings.beginGroup("/ToolBars");
    settings.setValue("/FileToolBar", d_file_tool_bar);
    settings.setValue("/EditToolBar", d_edit_tool_bar);
    settings.setValue("/TableToolBar", d_table_tool_bar);
    settings.setValue("/ColumnToolBar", d_column_tool_bar);
    settings.setValue("/MatrixToolBar", d_matrix_tool_bar);
    settings.setValue("/PlotToolBar", d_plot_tool_bar);
    settings.setValue("/Plot3DToolBar", d_plot3D_tool_bar);
    settings.setValue("/DisplayToolBar", d_display_tool_bar);
	settings.setValue("/FormatToolBar", d_format_tool_bar);
	settings.endGroup();
}

void ApplicationWindow::exportGraph()
{
	QWidget *w = ws->activeWindow();
	if (!w)
		return;

	MultiLayer *plot2D = 0;
	Graph3D *plot3D = 0;
	if(w->isA("MultiLayer")){
		plot2D = (MultiLayer*)w;
		if (plot2D->isEmpty()){
			QMessageBox::critical(this, tr("QtiPlot - Export Error"),
					tr("<h4>There are no plot layers available in this window!</h4>"));
			return;
		}
	} else if (w->isA("Graph3D"))
		plot3D = (Graph3D*)w;
	else
		return;

	ImageExportDialog *ied = new ImageExportDialog(this, plot2D!=NULL, d_extended_export_dialog);
	ied->setDir(workingDir);
    ied->selectFilter(d_image_export_filter);
	if ( ied->exec() != QDialog::Accepted )
		return;
	workingDir = ied->directory().path();
	if (ied->selectedFiles().isEmpty())
		return;

	QString selected_filter = ied->selectedFilter();
	QString file_name = ied->selectedFiles()[0];
	QFileInfo file_info(file_name);
	if (!file_info.fileName().contains("."))
		file_name.append(selected_filter.remove("*"));

	QFile file(file_name);
	if (!file.open( QIODevice::WriteOnly )){
		QMessageBox::critical(this, tr("QtiPlot - Export error"),
				tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
		return;
	}
	file.close();

	if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") || 
		selected_filter.contains(".ps") || selected_filter.contains(".svg")) {
		if (plot3D)
			plot3D->exportVector(file_name);
		else if (plot2D){
			if (selected_filter.contains(".svg"))
				plot2D->exportSVG(file_name);	
			else
				plot2D->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
		}
	} else {
		QList<QByteArray> list = QImageWriter::supportedImageFormats();
		for (int i=0; i<(int)list.count(); i++){
			if (selected_filter.contains("." + (list[i]).lower())) {
				if (plot2D)
					plot2D->exportImage(file_name, ied->quality(), ied->transparency());
				else if (plot3D)
					plot3D->exportImage(file_name, ied->quality(), ied->transparency());
			}
		}
	}
}

void ApplicationWindow::exportLayer()
{
	QWidget *w=ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)w)->activeGraph();
	if (!g)
		return;

	ImageExportDialog *ied = new ImageExportDialog(this, g!=NULL, d_extended_export_dialog);
	ied->setDir(workingDir);
	ied->selectFilter(d_image_export_filter);
	if ( ied->exec() != QDialog::Accepted )
		return;
	workingDir = ied->directory().path();
	if (ied->selectedFiles().isEmpty())
		return;

	QString selected_filter = ied->selectedFilter();
	QString file_name = ied->selectedFiles()[0];
	QFileInfo file_info(file_name);
	if (!file_info.fileName().contains("."))
		file_name.append(selected_filter.remove("*"));

	QFile file(file_name);
	if ( !file.open( QIODevice::WriteOnly ) ){
		QMessageBox::critical(this, tr("QtiPlot - Export error"),
				tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(file_name));
		return;
	}
	file.close();

	if (selected_filter.contains(".eps") || selected_filter.contains(".pdf") || selected_filter.contains(".ps"))
		g->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
	else if (selected_filter.contains(".svg"))
		g->exportSVG(file_name);
    else if (selected_filter.contains(".emf"))
		g->exportEMF(file_name);
    else {
		QList<QByteArray> list = QImageWriter::supportedImageFormats();
		for (int i=0; i<(int)list.count(); i++)
			if (selected_filter.contains("."+(list[i]).lower()))
				g->exportImage(file_name, ied->quality(), ied->transparency());
	}
}

void ApplicationWindow::exportAllGraphs()
{
	ImageExportDialog *ied = new ImageExportDialog(this, true, d_extended_export_dialog);
	ied->setWindowTitle(tr("Choose a directory to export the graphs to"));
	QStringList tmp = ied->filters();
	ied->setFileMode(QFileDialog::Directory);
	ied->setFilters(tmp);
	ied->setLabelText(QFileDialog::FileType, tr("Output format:"));
	ied->setLabelText(QFileDialog::FileName, tr("Directory:"));

	ied->setDir(workingDir);
    ied->selectFilter(d_image_export_filter);

	if ( ied->exec() != QDialog::Accepted )
		return;
	workingDir = ied->directory().path();
	if (ied->selectedFiles().isEmpty())
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QString output_dir = ied->selectedFiles()[0];
	QString file_suffix = ied->selectedFilter();
	file_suffix.lower();
	file_suffix.remove("*");

	QWidgetList *windows = windowsList();
	bool confirm_overwrite = true;
	MultiLayer *plot2D;
	Graph3D *plot3D;

	foreach (QWidget *w, *windows)
	{
		if (w->isA("MultiLayer")) {
			plot3D = 0;
			plot2D = (MultiLayer *)w;
			if (plot2D->isEmpty()) {
				QApplication::restoreOverrideCursor();
				QMessageBox::warning(this, tr("QtiPlot - Warning"),
						tr("There are no plot layers available in window <b>%1</b>.<br>"
							"Graph window not exported!").arg(plot2D->objectName()));
				QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
				continue;
			}
		} else if (w->isA("Graph3D")) {
			plot2D = 0;
			plot3D = (Graph3D *)w;
		} else
			continue;

		QString file_name = output_dir + "/" + w->objectName() + file_suffix;
		QFile f(file_name);
		if (f.exists() && confirm_overwrite) {
			QApplication::restoreOverrideCursor();
			switch(QMessageBox::question(this, tr("QtiPlot - Overwrite file?"),
						tr("A file called: <p><b>%1</b><p>already exists. "
							"Do you want to overwrite it?") .arg(file_name), tr("&Yes"), tr("&All"), tr("&Cancel"), 0, 1)) {
				case 1:
					confirm_overwrite = false;
				case 0:
					QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
					break;
				case 2:
					delete windows;
					return;
			}
		}
		if ( !f.open( QIODevice::WriteOnly ) ) {
			QApplication::restoreOverrideCursor();
			QMessageBox::critical(this, tr("QtiPlot - Export error"),
					tr("Could not write to file: <br><h4>%1</h4><p>"
						"Please verify that you have the right to write to this location!").arg(file_name));
			delete windows;
			return;
		}
		f.close();

		if (file_suffix.contains(".eps") || file_suffix.contains(".pdf") || 
			file_suffix.contains(".ps") || file_suffix.contains(".svg")) {
			if (plot3D)
				plot3D->exportVector(file_name);
			else if (plot2D){
				if (file_suffix.contains(".svg"))
					plot2D->exportSVG(file_name);
				else
					plot2D->exportVector(file_name, ied->resolution(), ied->color(), ied->keepAspect(), ied->pageSize());
			}
		} else {
			QList<QByteArray> list = QImageWriter::supportedImageFormats();
			for (int i=0; i<(int)list.count(); i++){
				if (file_suffix.contains("." + (list[i]).lower())) {
					if (plot2D)
						plot2D->exportImage(file_name, ied->quality(), ied->transparency());
					else if (plot3D)
						plot3D->exportImage(file_name, ied->quality(), ied->transparency());
				}
			}
		}
	}

	delete windows;
	QApplication::restoreOverrideCursor();
}

QString ApplicationWindow::windowGeometryInfo(MyWidget *w)
{
	QString s = "geometry\t";
    if (w->status() == MyWidget::Maximized)
	{
		if (w == w->folder()->activeWindow())
			return s + "maximized\tactive\n";
		else
			return s + "maximized\n";
	}

	if (!w->parent())
        s+="0\t0\t500\t400\t";
    else
    {
        QPoint p = w->parentWidget()->pos();// store position
        s+=QString::number(p.x())+"\t";
        s+=QString::number(p.y())+"\t";
        s+=QString::number(w->parentWidget()->frameGeometry().width())+"\t";
        s+=QString::number(w->parentWidget()->frameGeometry().height())+"\t";
    }

    if (w->status() == MyWidget::Minimized)
        s += "minimized\t";

    bool hide = hidden(w);
    if (w == w->folder()->activeWindow() && !hide)
        s+="active\n";
    else if(hide)
        s+="hidden\n";
    else
        s+="\n";
	return s;
}

void ApplicationWindow::restoreWindowGeometry(ApplicationWindow *app, MyWidget *w, const QString s)
{
	w->blockSignals (true);
	QString caption = w->objectName();
	if (s.contains ("minimized")) {
	    QStringList lst = s.split("\t");
	    if (lst.count() > 4)
            w->parentWidget()->setGeometry(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
		w->setStatus(MyWidget::Minimized);
		app->setListView(caption, tr("Minimized"));
	} else if (s.contains ("maximized")) {
		w->setStatus(MyWidget::Maximized);
		if (w->isA("MultiLayer"))
            ((MultiLayer*)w)->setOpenMaximized();

		app->setListView(caption, tr("Maximized"));
	} else {
		QStringList lst = s.split("\t");
		w->parentWidget()->setGeometry(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
		w->setStatus(MyWidget::Normal);

        if (lst.count() > 5) {
            if (lst[5] == "hidden")
                app->hideWindow(w);
        }
	}

	if (s.contains ("active")) {
        Folder *f = w->folder();
        if (f)
            f->setActiveWindow(w);
	}

	w->blockSignals (false);
}

Folder* ApplicationWindow::projectFolder()
{
	return ((FolderListItem *)folders->firstChild())->folder();
}

bool ApplicationWindow::saveProject(bool compress)
{
	if (projectname == "untitled" || projectname.endsWith(".opj", Qt::CaseInsensitive) ||
		projectname.endsWith(".ogm", Qt::CaseInsensitive) || projectname.endsWith(".ogw", Qt::CaseInsensitive)
		|| projectname.endsWith(".ogg", Qt::CaseInsensitive))
	{
		saveProjectAs();
		return false;
	}

#ifdef QTIPLOT_DEMO
	showDemoVersionMessage();
	return false;
#endif

	saveFolder(projectFolder(), projectname, compress);

	setWindowTitle("QtiPlot - " + projectname);
	savedProject();
	actionUndo->setEnabled(false);
	actionRedo->setEnabled(false);

	if (autoSave){
		if (savingTimerId)
			killTimer(savingTimerId);
		savingTimerId=startTimer(autoSaveTime*60000);
	} else
		savingTimerId=0;

	QApplication::restoreOverrideCursor();
	return true;
}

void ApplicationWindow::saveProjectAs(const QString& fileName, bool compress)
{
#ifdef QTIPLOT_DEMO
	showDemoVersionMessage();
	return;
#endif

	QString fn = fileName;
	if (fileName.isEmpty()){
		QString filter = tr("QtiPlot project")+" (*.qti);;";
		filter += tr("Compressed QtiPlot project")+" (*.qti.gz)";

		QString selectedFilter;
		fn = QFileDialog::getSaveFileName(this, tr("Save Project As"), workingDir, filter, &selectedFilter);
		if (selectedFilter.contains(".gz"))
			compress = true;
	}

	if ( !fn.isEmpty() ){
		QFileInfo fi(fn);
		workingDir = fi.dirPath(true);
		QString baseName = fi.fileName();
		if (!baseName.contains("."))
			fn.append(".qti");

		projectname = fn;
		if (saveProject(compress)){
			recentProjects.remove(projectname);
			recentProjects.push_front(projectname);
			updateRecentProjectsList();

			QFileInfo fi(fn);
			QString baseName = fi.baseName();
			FolderListItem *item = (FolderListItem *)folders->firstChild();
			item->setText(0, baseName);
			item->folder()->setObjectName(baseName);
		}
	}
}

void ApplicationWindow::saveNoteAs()
{
	Note* w = (Note*)ws->activeWindow();
	if (!w || !w->inherits("Note"))
		return;
	w->exportASCII();
}

void ApplicationWindow::saveAsTemplate(MyWidget* w, const QString& fileName)
{
	if (!w) {
		w = (MyWidget*)ws->activeWindow();
		if (!w)
			return;
	}

	QString fn = fileName;
	if (fn.isEmpty()){
		QString filter;
		if (w->isA("Matrix"))
			filter = tr("QtiPlot Matrix Template")+" (*.qmt)";
		else if (w->isA("MultiLayer"))
			filter = tr("QtiPlot 2D Graph Template")+" (*.qpt)";
		else if (w->inherits("Table"))
			filter = tr("QtiPlot Table Template")+" (*.qtt)";
		else if (w->isA("Graph3D"))
			filter = tr("QtiPlot 3D Surface Template")+" (*.qst)";

		QString selectedFilter;
		fn = QFileDialog::getSaveFileName(this, tr("Save Window As Template"), templatesDir + "/" + w->objectName(), filter, &selectedFilter);

		if (!fn.isEmpty()){
			QFileInfo fi(fn);
			workingDir = fi.dirPath(true);
			QString baseName = fi.fileName();
			if (!baseName.contains(".")){
				selectedFilter = selectedFilter.right(5).left(4);
				fn.append(selectedFilter);
			}
		} else
			return;
	}

	QFile f(fn);
	if ( !f.open( QIODevice::WriteOnly ) ){
		QMessageBox::critical(this, tr("QtiPlot - Export error"),
		tr("Could not write to file: <br><h4> %1 </h4><p>Please verify that you have the right to write to this location!").arg(fn));
		return;
	}

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	QString text = "QtiPlot " + QString::number(maj_version)+"."+ QString::number(min_version)+"."+
				QString::number(patch_version) + " template file\n";
	text += w->saveAsTemplate(windowGeometryInfo(w));
	QTextStream t( &f );
	t.setEncoding(QTextStream::UnicodeUTF8);
	t << text;
	f.close();
	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::rename()
{
	MyWidget* m = (MyWidget*)ws->activeWindow();
	if (!m)
		return;

	RenameWindowDialog *rwd = new RenameWindowDialog(this);
	rwd->setAttribute(Qt::WA_DeleteOnClose);
	rwd->setWidget(m);
	rwd->exec();
}

void ApplicationWindow::renameWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MyWidget *w = it->window();
	if (!w)
		return;

	RenameWindowDialog *rwd = new RenameWindowDialog(this);
	rwd->setAttribute(Qt::WA_DeleteOnClose);
	rwd->setWidget(w);
	rwd->exec();
}

void ApplicationWindow::renameWindow(Q3ListViewItem *item, int, const QString &text)
{
	if (!item)
		return;

	MyWidget *w = ((WindowListItem *)item)->window();
	if (!w || text == w->objectName())
		return;

	if(!setWindowName(w, text))
        item->setText(0, w->objectName());
}

bool ApplicationWindow::setWindowName(MyWidget *w, const QString &text)
{
	if (!w)
		return false;

	QString name = w->objectName();
	if (name == text)
		return true;

	QString newName = text;
	newName.replace("-", "_");
	if (newName.isEmpty()){
		QMessageBox::critical(this, tr("QtiPlot - Error"), tr("Please enter a valid name!"));
		return false;
	} else if (newName.contains(QRegExp("\\W"))){
		QMessageBox::critical(this, tr("QtiPlot - Error"),
				tr("The name you chose is not valid: only letters and digits are allowed!")+
				"<p>" + tr("Please choose another name!"));
		return false;
	}

	newName.replace("_", "-");

	while(alreadyUsedName(newName)){
		QMessageBox::critical(this, tr("QtiPlot - Error"), tr("Name <b>%1</b> already exists!").arg(newName)+
				"<p>"+tr("Please choose another name!")+
				"<p>"+tr("Warning: for internal consistency reasons the underscore character is replaced with a minus sign."));
		return false;
	}

	if (w->inherits("Table")){
		int id = tableWindows.findIndex(name);
		tableWindows[id] = newName;
		updateTableNames(name, newName);
	} else if (w->isA("Matrix"))
		changeMatrixName(name, newName);

	w->setCaptionPolicy(w->captionPolicy());
	w->setName(newName);
	renameListViewItem(name, newName);
	return true;
}

QStringList ApplicationWindow::columnsList(Table::PlotDesignation plotType)
{
	QList<QWidget*> *windows = windowsList();
	QStringList list;
	foreach (QWidget *w, *windows)
	{
		if (!w->inherits("Table"))
			continue;

		Table *t = (Table *)w;
		for (int i=0; i < t->numCols(); i++)
		{
			if (t->colPlotDesignation(i) == plotType || plotType == Table::All)
				list << QString(t->objectName()) + "_" + t->colLabel(i);
		}
	}

	delete windows;
	return list;
}

void ApplicationWindow::showCurvesDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	if (((MultiLayer*)ws->activeWindow())->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Error"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	if (g->isPiePlot()){
		QMessageBox::warning(this,tr("QtiPlot - Error"),
				tr("This functionality is not available for pie plots!"));
	} else {
		CurvesDialog* crvDialog = new CurvesDialog(this);
		crvDialog->setAttribute(Qt::WA_DeleteOnClose);
		crvDialog->setGraph(g);
		crvDialog->resize(d_add_curves_dialog_size);
		crvDialog->setModal(true);
		crvDialog->show();
	}
}

QList<QWidget*>* ApplicationWindow::tableList()
{
	QList<QWidget*>* lst = new QList<QWidget*>();
	QList<QWidget*> *windows = windowsList();
	for (int i = 0; i < int(windows->count());i++ )
	{
		if (windows->at(i)->inherits("Table"))
			lst->append(windows->at(i));
	}
	delete windows;
	return lst;
}

void ApplicationWindow::showPlotAssociations(int curve)
{
	QWidget *w = ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	Graph *g = ((MultiLayer*)w)->activeGraph();
	if (!g)
		return;

	AssociationsDialog* ad=new AssociationsDialog(this, Qt::WindowStaysOnTopHint);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	ad->setGraph(g);
	ad->initTablesList(tableList(), curve);
	ad->exec();
}

void ApplicationWindow::showTitleDialog()
{
	QWidget *w = ws->activeWindow();
	if (!w)
		return;

	if (w->isA("MultiLayer")){
		Graph* g = ((MultiLayer*)w)->activeGraph();
		if (g){
			TextDialog* td= new TextDialog(TextDialog::AxisTitle, this,0);
			td->setAttribute(Qt::WA_DeleteOnClose);
			connect (td,SIGNAL(changeFont(const QFont &)),g,SLOT(setTitleFont(const QFont &)));
			connect (td,SIGNAL(changeText(const QString &)),g,SLOT(setTitle(const QString &)));
			connect (td,SIGNAL(changeColor(const QColor &)),g,SLOT(setTitleColor(const QColor &)));
			connect (td,SIGNAL(changeAlignment(int)),g,SLOT(setTitleAlignment(int)));

			QwtText t = g->plotWidget()->title();
			td->setText(t.text());
			td->setFont(t.font());
			td->setTextColor(t.color());
			td->setAlignment(t.renderFlags());
			td->exec();
		}
	} else if (w->isA("Graph3D")) {
		Plot3DDialog* pd = (Plot3DDialog*)showPlot3dDialog();
		if (pd)
			pd->showTitleTab();
	}
}

void ApplicationWindow::showXAxisTitleDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (g)
	{
		TextDialog* td= new TextDialog(TextDialog::AxisTitle, this,0);
		td->setAttribute(Qt::WA_DeleteOnClose);
		connect (td,SIGNAL(changeFont(const QFont &)),g,SLOT(setXAxisTitleFont(const QFont &)));
		connect (td,SIGNAL(changeText(const QString &)),g,SLOT(setXAxisTitle(const QString &)));
		connect (td,SIGNAL(changeColor(const QColor &)),g,SLOT(setXAxisTitleColor(const QColor &)));
		connect (td,SIGNAL(changeAlignment(int)),g,SLOT(setXAxisTitleAlignment(int)));

		td->setText(g->axisTitle(QwtPlot::xBottom));
		td->setFont(g->axisTitleFont(2));
		td->setTextColor(g->axisTitleColor(2));
		td->setAlignment(g->axisTitleAlignment(2));
		td->setWindowTitle(tr("QtiPlot - X Axis Title"));
		td->exec();
	}
}

void ApplicationWindow::showYAxisTitleDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (g)
	{
		TextDialog* td= new TextDialog(TextDialog::AxisTitle, this,0);
		td->setAttribute(Qt::WA_DeleteOnClose);
		connect (td,SIGNAL(changeFont(const QFont &)),g,SLOT(setYAxisTitleFont(const QFont &)));
		connect (td,SIGNAL(changeText(const QString &)),g,SLOT(setYAxisTitle(const QString &)));
		connect (td,SIGNAL(changeColor(const QColor &)),g,SLOT(setYAxisTitleColor(const QColor &)));
		connect (td,SIGNAL(changeAlignment(int)),g,SLOT(setYAxisTitleAlignment(int)));

		td->setText(g->axisTitle(QwtPlot::yLeft));
		td->setFont(g->axisTitleFont(0));
		td->setTextColor(g->axisTitleColor(0));
		td->setAlignment(g->axisTitleAlignment(0));
		td->setWindowTitle(tr("QtiPlot - Y Axis Title"));
		td->exec();
	}
}

void ApplicationWindow::showRightAxisTitleDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (g)
	{
		TextDialog* td= new TextDialog(TextDialog::AxisTitle, this, 0);
		td->setAttribute(Qt::WA_DeleteOnClose);
		connect (td,SIGNAL(changeFont(const QFont &)),g,SLOT(setRightAxisTitleFont(const QFont &)));
		connect (td,SIGNAL(changeText(const QString &)),g,SLOT(setRightAxisTitle(const QString &)));
		connect (td,SIGNAL(changeColor(const QColor &)),g,SLOT(setRightAxisTitleColor(const QColor &)));
		connect (td,SIGNAL(changeAlignment(int)),g,SLOT(setRightAxisTitleAlignment(int)));

		td->setText(g->axisTitle(QwtPlot::yRight));
		td->setFont(g->axisTitleFont(1));
		td->setTextColor(g->axisTitleColor(1));
		td->setAlignment(g->axisTitleAlignment(1));
		td->setWindowTitle(tr("QtiPlot - Right Axis Title"));
		td->exec();
	}
}

void ApplicationWindow::showTopAxisTitleDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (g){
		TextDialog* td= new TextDialog(TextDialog::AxisTitle, this, 0);
		td->setAttribute(Qt::WA_DeleteOnClose);
		connect (td,SIGNAL(changeFont(const QFont &)),g,SLOT(setTopAxisTitleFont(const QFont &)));
		connect (td,SIGNAL(changeText(const QString &)),g,SLOT(setTopAxisTitle(const QString &)));
		connect (td,SIGNAL(changeColor(const QColor &)),g,SLOT(setTopAxisTitleColor(const QColor &)));
		connect (td,SIGNAL(changeAlignment(int)),g,SLOT(setTopAxisTitleAlignment(int)));

		td->setText(g->axisTitle(QwtPlot::xTop));
		td->setFont(g->axisTitleFont(3));
		td->setTextColor(g->axisTitleColor(3));
		td->setAlignment(g->axisTitleAlignment(3));
		td->setWindowTitle(tr("QtiPLot - Top Axis Title"));
		td->exec();
	}
}

void ApplicationWindow::showExportASCIIDialog()
{
	if (tableWindows.count()){
		QString tableName;
		if (ws->activeWindow() && ws->activeWindow()->inherits("Table"))
			tableName = ws->activeWindow()->objectName();
		ExportDialog* ed = new ExportDialog(tableName, this, Qt::WindowContextHelpButtonHint);
		ed->setAttribute(Qt::WA_DeleteOnClose);
		ed->exec();
	}
}

void ApplicationWindow::exportAllTables(const QString& sep, bool colNames, bool colComments, bool expSelection)
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory to export the tables to"), workingDir, QFileDialog::ShowDirsOnly);
	if (!dir.isEmpty()){
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		QWidgetList *windows = windowsList();
		workingDir = dir;

		bool confirmOverwrite = true;
		bool success = true;
		QWidget *w;
		foreach(w, *windows){
			if (w->inherits("Table")){
				Table *t = (Table*)w;
				QString fileName = dir + "/" + w->objectName() + ".txt";
				QFile f(fileName);
				if (f.exists(fileName) && confirmOverwrite){
					QApplication::restoreOverrideCursor();
					switch(QMessageBox::question(this, tr("QtiPlot - Overwrite file?"),
								tr("A file called: <p><b>%1</b><p>already exists. "
									"Do you want to overwrite it?").arg(fileName), tr("&Yes"), tr("&All"), tr("&Cancel"), 0, 1))
					{
						case 0:
							success = t->exportASCII(fileName, sep, colNames, colComments, expSelection);
							break;

						case 1:
							confirmOverwrite = false;
							success = t->exportASCII(fileName, sep, colNames, colComments, expSelection);
							break;

						case 2:
							return;
							break;
					}
				} else
					success = t->exportASCII(fileName, sep, colNames, colComments, expSelection);

				if (!success)
					break;
			}
		}
		delete windows;
		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::exportASCII(const QString& tableName, const QString& sep,
                        bool colNames, bool colComments, bool expSelection)
{
	Table* t = table(tableName);
	if (!t)
		return;

	QString selectedFilter;
	QString fname = QFileDialog::getSaveFileName(this, tr("Choose a filename to save under"), asciiDirPath, "*.txt;;*.dat;;*.DAT", &selectedFilter);
	if (!fname.isEmpty() ){
		QFileInfo fi(fname);
		QString baseName = fi.fileName();
		if (baseName.contains(".")==0)
			fname.append(selectedFilter.remove("*"));

		asciiDirPath = fi.dirPath(true);

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		t->exportASCII(fname, sep, colNames, colComments, expSelection);
		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::showRowsDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	bool ok;
	int rows = QInputDialog::getInteger(this, tr("QtiPlot - Enter rows number"), tr("Rows"),
			((Table*)ws->activeWindow())->numRows(), 0, 1000000, 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint);
	if ( ok )
		((Table*)ws->activeWindow())->resizeRows(rows);
}

void ApplicationWindow::showDeleteRowsDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

    Table *t = (Table*)ws->activeWindow();
	bool ok;

	int start_row = QInputDialog::getInteger(this, tr("QtiPlot - Delete rows"), tr("Start row"),
                    1, 1, t->numRows(), 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint);
    if (ok){
        int end_row = QInputDialog::getInteger(this, tr("QtiPlot - Delete rows"), tr("End row"),
                        t->numRows(), 1, t->numRows(), 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint);
        if (ok)
            t->deleteRows(start_row, end_row);
	}
}

void ApplicationWindow::showColsDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	bool ok;
	int cols = QInputDialog::getInteger(this, tr("QtiPlot - Enter columns number"), tr("Columns"),
			((Table*)ws->activeWindow())->numCols(), 0, 1000000, 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint);
	if ( ok )
		((Table*)ws->activeWindow())->resizeCols(cols);
}

void ApplicationWindow::showColumnValuesDialog()
{
	Table* w = (Table*)ws->activeWindow();
	if (!w || !w->inherits("Table"))
        return;

    if (w->selectedColumns().count()>0 || w->table()->currentSelection() >= 0){
        SetColValuesDialog* vd = new SetColValuesDialog(scriptEnv, this);
        vd->setAttribute(Qt::WA_DeleteOnClose);
        vd->setTable(w);
        vd->exec();
    } else
        QMessageBox::warning(this, tr("QtiPlot - Column selection error"), tr("Please select a column first!"));
}

void ApplicationWindow::recalculateTable()
{
	QWidget* w = ws->activeWindow();
	if (!w)
		return;

	if (w->inherits("Table"))
		((Table*)w)->calculate();
	else if (w->isA("Matrix"))
		((Matrix*)w)->calculate();
}

void ApplicationWindow::sortActiveTable()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	if (int(((Table*)ws->activeWindow())->selectedColumns().count())>0)
		((Table*)ws->activeWindow())->sortTableDialog();
	else
		QMessageBox::warning(this, "QtiPlot - Column selection error","Please select a column first!");
}

void ApplicationWindow::sortSelection()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table*)ws->activeWindow())->sortColumnsDialog();
}

void ApplicationWindow::normalizeActiveTable()
{
	Table* w = (Table*)ws->activeWindow();
	if ( w && tableWindows.contains(w->objectName()))
	{
		if (int(w->selectedColumns().count())>0)
			w->normalize();
		else
			QMessageBox::warning(this, tr("QtiPlot - Column selection error"), tr("Please select a column first!"));
	}
}

void ApplicationWindow::normalizeSelection()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	if (int(((Table*)ws->activeWindow())->selectedColumns().count())>0)
		((Table*)ws->activeWindow())->normalizeSelection();
	else
		QMessageBox::warning(this, tr("QtiPlot - Column selection error"), tr("Please select a column first!"));
}

void ApplicationWindow::correlate()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table*)ws->activeWindow();
	QStringList s = t->selectedColumns();
	if ((int)s.count() != 2){
		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select two columns for this operation!"));
		return;
	}

	Correlation *cor = new Correlation(this, t, s[0], s[1]);
	cor->run();
	delete cor;
}

void ApplicationWindow::autoCorrelate()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table*)ws->activeWindow();
	QStringList s = t->selectedColumns();
	if ((int)s.count() != 1)
	{
		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select exactly one columns for this operation!"));
		return;
	}

	Correlation *cor = new Correlation(this, t, s[0], s[0]);
	cor->run();
	delete cor;
}

void ApplicationWindow::convolute()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table*)ws->activeWindow();
	QStringList s = t->selectedColumns();
	if ((int)s.count() != 2)
	{
		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select two columns for this operation:\n the first represents the signal and the second the response function!"));
		return;
	}

	Convolution *cv = new Convolution(this, t, s[0], s[1]);
	cv->run();
	delete cv;
}

void ApplicationWindow::deconvolute()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table*)ws->activeWindow();
	QStringList s = t->selectedColumns();
	if ((int)s.count() != 2)
	{
		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select two columns for this operation:\n the first represents the signal and the second the response function!"));
		return;
	}

	Deconvolution *dcv = new Deconvolution(this, t, s[0], s[1]);
	dcv->run();
	delete dcv;
}

void ApplicationWindow::showColStatistics()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;
	Table *t = (Table*)ws->activeWindow();

	if (int(t->selectedColumns().count()) > 0)
	{
		QList<int> targets;
		for (int i=0; i < t->numCols(); i++)
			if (t->isColumnSelected(i, true))
				targets << i;
		newTableStatistics(t, TableStatistics::column, targets)->showNormal();
	}
	else
		QMessageBox::warning(this, tr("QtiPlot - Column selection error"),
				tr("Please select a column first!"));
}

void ApplicationWindow::showRowStatistics()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;
	Table *t = (Table*)ws->activeWindow();

	if (t->numSelectedRows() > 0){
		QList<int> targets;
		for (int i=0; i < t->numRows(); i++)
			if (t->isRowSelected(i, true))
				targets << i;
		newTableStatistics(t, TableStatistics::row, targets)->showNormal();
	} else
		QMessageBox::warning(this, tr("QtiPlot - Row selection error"),
				tr("Please select a row first!"));
}

void ApplicationWindow::showColMenu(int c)
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table* w = (Table*)ws->activeWindow();

	QMenu contextMenu(this);
	QMenu plot(this);
	QMenu specialPlot(this);
	QMenu fill(this);
	QMenu sorting(this);
	QMenu colType(this);
	colType.setCheckable(true);
	QMenu panels(this);
	QMenu stat(this);
	QMenu norm(this);

	if ((int)w->selectedColumns().count()==1)
	{
		w->setSelectedCol(c);
		plot.addAction(QIcon(QPixmap(lPlot_xpm)),tr("&Line"), this, SLOT(plotL()));
		plot.addAction(QIcon(QPixmap(pPlot_xpm)),tr("&Scatter"), this, SLOT(plotP()));
		plot.addAction(QIcon(QPixmap(lpPlot_xpm)),tr("Line + S&ymbol"), this, SLOT(plotLP()));

		specialPlot.addAction(QIcon(QPixmap(dropLines_xpm)),tr("Vertical &Drop Lines"), this, SLOT(plotVerticalDropLines()));
		specialPlot.addAction(QIcon(QPixmap(spline_xpm)),tr("&Spline"), this,SLOT(plotSpline()));
		specialPlot.addAction(QIcon(QPixmap(vert_steps_xpm)),tr("&Vertical Steps"), this, SLOT(plotVertSteps()));
		specialPlot.addAction(QIcon(QPixmap(hor_steps_xpm)),tr("&Horizontal Steps"), this, SLOT(plotHorSteps()));
		specialPlot.setTitle(tr("Special Line/Symb&ol"));
		plot.addMenu(&specialPlot);
		plot.insertSeparator();

		plot.addAction(QIcon(QPixmap(vertBars_xpm)),tr("&Columns"), this, SLOT(plotVerticalBars()));
		plot.addAction(QIcon(QPixmap(hBars_xpm)),tr("&Rows"), this, SLOT(plotHorizontalBars()));
		plot.addAction(QIcon(QPixmap(area_xpm)),tr("&Area"), this, SLOT(plotArea()));

		plot.addAction(QIcon(QPixmap(pie_xpm)),tr("&Pie"), this, SLOT(plotPie()));
		plot.insertSeparator();

		plot.addAction(QIcon(QPixmap(ribbon_xpm)),tr("3D Ribbo&n"), this, SLOT(plot3DRibbon()));
		plot.addAction(QIcon(QPixmap(bars_xpm)),tr("3D &Bars"), this, SLOT(plot3DBars()));
		plot.addAction(QIcon(QPixmap(scatter_xpm)),tr("3&D Scatter"), this, SLOT(plot3DScatter()));
		plot.addAction(QIcon(QPixmap(trajectory_xpm)),tr("3D &Trajectory"), this, SLOT(plot3DTrajectory()));

		plot.insertSeparator();

		stat.addAction(actionBoxPlot);
		stat.addAction(QIcon(QPixmap(histogram_xpm)),tr("&Histogram"), this, SLOT(plotHistogram()));
		stat.addAction(QIcon(QPixmap(stacked_hist_xpm)),tr("&Stacked Histograms"), this, SLOT(plotStackedHistograms()));
		stat.setTitle(tr("Statistical &Graphs"));
		plot.addMenu(&stat);

		plot.setTitle(tr("&Plot"));
		contextMenu.addMenu(&plot);
		contextMenu.insertSeparator();

		contextMenu.addAction(QIcon(QPixmap(cut_xpm)),tr("Cu&t"), w, SLOT(cutSelection()));
		contextMenu.addAction(QIcon(QPixmap(copy_xpm)),tr("&Copy"), w, SLOT(copySelection()));
		contextMenu.addAction(QIcon(QPixmap(paste_xpm)),tr("Past&e"), w, SLOT(pasteSelection()));
		contextMenu.insertSeparator();

		QAction * xColID=colType.addAction(QIcon(QPixmap(x_col_xpm)), tr("&X"), this, SLOT(setXCol()));
		xColID->setCheckable(true);
        QAction * yColID=colType.addAction(QIcon(QPixmap(y_col_xpm)), tr("&Y"), this, SLOT(setYCol()));
        yColID->setCheckable(true);
        QAction * zColID=colType.addAction(QIcon(QPixmap(z_col_xpm)), tr("&Z"), this, SLOT(setZCol()));
        zColID->setCheckable(true);
        colType.insertSeparator();
        QAction * xErrColID =colType.addAction(tr("X E&rror"), this, SLOT(setXErrCol()));
        xErrColID->setCheckable(true);
        QAction * yErrColID = colType.addAction(QIcon(QPixmap(errors_xpm)), tr("Y &Error"), this, SLOT(setYErrCol()));
        yErrColID->setCheckable(true);
        colType.insertSeparator();
        QAction * noneID=colType.addAction(QIcon(QPixmap(disregard_col_xpm)), tr("&None"), this, SLOT(disregardCol()));
        noneID->setCheckable(true);

        if (w->colPlotDesignation(c) == Table::X)
            xColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::Y)
            yColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::Z)
            zColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::xErr)
            xErrColID->setChecked(true);
        else if (w->colPlotDesignation(c) == Table::yErr)
            yErrColID->setChecked(true);
        else
            noneID->setChecked(true);

        actionReadOnlyCol->addTo(&colType);
        actionReadOnlyCol->setCheckable(true);
        actionReadOnlyCol->setChecked(w->isReadOnlyColumn(c));

		colType.setTitle(tr("Set As"));
		contextMenu.addMenu(&colType);

		if (ws->activeWindow()->inherits("Table")){
			contextMenu.insertSeparator();

			contextMenu.addAction(actionShowColumnValuesDialog);
			contextMenu.addAction(actionTableRecalculate);
			fill.addAction(actionSetAscValues);
			fill.addAction(actionSetRandomValues);
			fill.setTitle(tr("&Fill Column With"));
			contextMenu.addMenu(&fill);

			norm.addAction(tr("&Column"), w, SLOT(normalizeSelection()));
			norm.addAction(actionNormalizeTable);
			norm.setTitle(tr("&Normalize"));
			contextMenu.addMenu(& norm);

			contextMenu.insertSeparator();
			contextMenu.addAction(actionShowColStatistics);

			contextMenu.insertSeparator();

			contextMenu.addAction(QIcon(QPixmap(erase_xpm)), tr("Clea&r"), w, SLOT(clearCol()));
			contextMenu.addAction(QIcon(QPixmap(close_xpm)), tr("&Delete"), w, SLOT(removeCol()));
			contextMenu.addAction(tr("&Insert"), w, SLOT(insertCol()));
			contextMenu.addAction(tr("&Add Column"),w, SLOT(addCol()));
			contextMenu.insertSeparator();

			sorting.addAction(tr("&Ascending"),w, SLOT(sortColAsc()));
			sorting.addAction(tr("&Descending"),w, SLOT(sortColDesc()));
			sorting.setTitle(tr("Sort Colu&mn"));
			contextMenu.addMenu(&sorting);

			contextMenu.addAction(actionSortTable);
		}

		contextMenu.insertSeparator();
		contextMenu.addAction(actionShowColumnOptionsDialog);
	}
	else if ((int)w->selectedColumns().count()>1)
	{
		plot.addAction(QIcon(QPixmap(lPlot_xpm)),tr("&Line"), this, SLOT(plotL()));
		plot.addAction(QIcon(QPixmap(pPlot_xpm)),tr("&Scatter"), this, SLOT(plotP()));
		plot.addAction(QIcon(QPixmap(lpPlot_xpm)),tr("Line + S&ymbol"), this,SLOT(plotLP()));

		specialPlot.addAction(QIcon(QPixmap(dropLines_xpm)),tr("Vertical &Drop Lines"), this, SLOT(plotVerticalDropLines()));
		specialPlot.addAction(QIcon(QPixmap(spline_xpm)),tr("&Spline"), this, SLOT(plotSpline()));
		specialPlot.addAction(QIcon(QPixmap(vert_steps_xpm)),tr("&Vertical Steps"), this, SLOT(plotVertSteps()));
		specialPlot.addAction(QIcon(QPixmap(hor_steps_xpm)),tr("&Vertical Steps"), this, SLOT(plotHorSteps()));
		specialPlot.setTitle(tr("Special Line/Symb&ol"));
		plot.addMenu(&specialPlot);
		plot.insertSeparator();

		plot.addAction(QIcon(QPixmap(vertBars_xpm)),tr("&Columns"), this, SLOT(plotVerticalBars()));
		plot.addAction(QIcon(QPixmap(hBars_xpm)),tr("&Rows"), this, SLOT(plotHorizontalBars()));
		plot.addAction(QIcon(QPixmap(area_xpm)),tr("&Area"), this, SLOT(plotArea()));
		plot.addAction(QIcon(QPixmap(vectXYXY_xpm)),tr("Vectors &XYXY"), this, SLOT(plotVectXYXY()));
		plot.insertSeparator();

		stat.addAction(actionBoxPlot);
		stat.addAction(QIcon(QPixmap(histogram_xpm)),tr("&Histogram"), this, SLOT(plotHistogram()));
		stat.addAction(QIcon(QPixmap(stacked_hist_xpm)),tr("&Stacked Histograms"), this, SLOT(plotStackedHistograms()));
		stat.setTitle(tr("Statistical &Graphs"));
		plot.addMenu(&stat);

		panels.addAction(QIcon(QPixmap(panel_v2_xpm)),tr("&Vertical 2 Layers"), this, SLOT(plot2VerticalLayers()));
		panels.addAction(QIcon(QPixmap(panel_h2_xpm)),tr("&Horizontal 2 Layers"), this, SLOT(plot2HorizontalLayers()));
		panels.addAction(QIcon(QPixmap(panel_4_xpm)),tr("&4 Layers"), this, SLOT(plot4Layers()));
		panels.addAction(QIcon(QPixmap(stacked_xpm)),tr("&Stacked Layers"), this, SLOT(plotStackedLayers()));
		panels.setTitle(tr("Pa&nel"));
		plot.addMenu(&panels);

		plot.setTitle(tr("&Plot"));
		contextMenu.addMenu(&plot);
		contextMenu.insertSeparator();
		contextMenu.addAction(QIcon(QPixmap(cut_xpm)),tr("Cu&t"), w, SLOT(cutSelection()));
		contextMenu.addAction(QIcon(QPixmap(copy_xpm)),tr("&Copy"), w, SLOT(copySelection()));
		contextMenu.addAction(QIcon(QPixmap(paste_xpm)),tr("Past&e"), w, SLOT(pasteSelection()));
		contextMenu.insertSeparator();

		if (ws->activeWindow()->inherits("Table")){
			contextMenu.addAction(QIcon(QPixmap(erase_xpm)),tr("Clea&r"), w, SLOT(clearSelection()));
			contextMenu.addAction(QIcon(QPixmap(close_xpm)),tr("&Delete"), w, SLOT(removeCol()));
			contextMenu.insertSeparator();
			contextMenu.addAction(tr("&Insert"), w, SLOT(insertCol()));
			contextMenu.addAction(tr("&Add Column"),w, SLOT(addCol()));
			contextMenu.insertSeparator();
		}

		colType.addAction(actionSetXCol);
		colType.addAction(actionSetYCol);
		colType.addAction(actionSetZCol);
		colType.insertSeparator();
		colType.addAction(actionSetXErrCol);
		colType.addAction(actionSetYErrCol);
		colType.insertSeparator();
		colType.addAction(tr("&Read-only"), this, SLOT(setReadOnlyColumns()));
		colType.addAction(tr("Read/&Write"), this, SLOT(setReadWriteColumns()));
		colType.insertSeparator();
		colType.addAction(actionDisregardCol);
		colType.setTitle(tr("Set As"));
		contextMenu.addMenu(&colType);

		if (ws->activeWindow()->inherits("Table"))
		{
			contextMenu.insertSeparator();

			fill.addAction(actionSetAscValues);
			fill.addAction(actionSetRandomValues);
			fill.setTitle(tr("&Fill Columns With"));
			contextMenu.addMenu(&fill);

			norm.addAction(actionNormalizeSelection);
			norm.addAction(actionNormalizeTable);
			norm.setTitle(tr("&Normalize"));
			contextMenu.addMenu(&norm);

			contextMenu.insertSeparator();
			contextMenu.addAction(actionSortSelection);
			contextMenu.addAction(actionSortTable);
			contextMenu.insertSeparator();
			contextMenu.addAction(actionShowColStatistics);
		}
	}

	QPoint posMouse=QCursor::pos();
	contextMenu.exec(posMouse);
}

void ApplicationWindow::plot2VerticalLayers()
{
	multilayerPlot(1, 2, defaultCurveStyle);
}

void ApplicationWindow::plot2HorizontalLayers()
{
	multilayerPlot(2, 1, defaultCurveStyle);
}

void ApplicationWindow::plot4Layers()
{
	multilayerPlot(2, 2, defaultCurveStyle);
}

void ApplicationWindow::plotStackedLayers()
{
	multilayerPlot(1, -1, defaultCurveStyle);
}

void ApplicationWindow::plotStackedHistograms()
{
	multilayerPlot(1, -1, Graph::Histogram);
}

void ApplicationWindow::showMatrixDialog()
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Matrix"))
	{
		Matrix* w = (Matrix*)ws->activeWindow();

		MatrixDialog* md = new MatrixDialog(this);
		md->setAttribute(Qt::WA_DeleteOnClose);
		md->setMatrix (w);
		md->exec();
	}
}

void ApplicationWindow::showMatrixSizeDialog()
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Matrix")){
		MatrixSizeDialog* md = new MatrixSizeDialog((Matrix*)ws->activeWindow(), this);
		md->setAttribute(Qt::WA_DeleteOnClose);
		md->exec();
	}
}

void ApplicationWindow::showMatrixValuesDialog()
{
	if ( ws->activeWindow() && ws->activeWindow()->isA("Matrix")){
		MatrixValuesDialog* md = new MatrixValuesDialog(scriptEnv, this);
		md->setAttribute(Qt::WA_DeleteOnClose);
		md->setMatrix((Matrix*)ws->activeWindow());
		md->exec();
	}
}

void ApplicationWindow::showColumnOptionsDialog()
{
	if ( !ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table*)ws->activeWindow();
	if(t->selectedColumns().count()>0) {
		TableDialog* td = new TableDialog(t, this);
		td->setAttribute(Qt::WA_DeleteOnClose);
		td->exec();
	} else
		QMessageBox::warning(this, tr("QtiPlot"), tr("Please select a column first!"));
}

void ApplicationWindow::showGeneralPlotDialog()
{
	QWidget* plot = ws->activeWindow();
	if (!plot)
		return;

	if (plot->isA("MultiLayer") && ((MultiLayer*)plot)->layers())
		showPlotDialog();
	else if (plot->isA("Graph3D")){
	    QDialog* gd = showScaleDialog();
		((Plot3DDialog*)gd)->showGeneralTab();
	}
}

void ApplicationWindow::showAxisDialog()
{
	QWidget* plot = (QWidget*)ws->activeWindow();
	if (!plot)
		return;

	QDialog* gd = showScaleDialog();
	if (gd && plot->isA("MultiLayer") && ((MultiLayer*)plot)->layers())
		((AxesDialog*)gd)->showAxesPage();
	else if (gd && plot->isA("Graph3D"))
		((Plot3DDialog*)gd)->showAxisTab();
}

void ApplicationWindow::showGridDialog()
{
	AxesDialog* gd = (AxesDialog*)showScaleDialog();
	if (gd)
		gd->showGridPage();
}

QDialog* ApplicationWindow::showScaleDialog()
{
	QWidget *w = ws->activeWindow();
	if (!w)
		return 0;

	if (w->isA("MultiLayer")){
		if (((MultiLayer*)w)->isEmpty())
			return 0;

		Graph* g = ((MultiLayer*)w)->activeGraph();
		AxesDialog* ad = new AxesDialog(this);
        ad->setGraph(g);
        ad->exec();
        return ad;
	} else if (w->isA("Graph3D"))
		return showPlot3dDialog();

	return 0;
}

AxesDialog* ApplicationWindow::showScalePageFromAxisDialog(int axisPos)
{
	AxesDialog* gd = (AxesDialog*)showScaleDialog();
	if (gd)
		gd->setCurrentScale(axisPos);

	return gd;
}

AxesDialog* ApplicationWindow::showAxisPageFromAxisDialog(int axisPos)
{
	AxesDialog* gd = (AxesDialog*)showScaleDialog();
	if (gd){
		gd->showAxesPage();
		gd->setCurrentScale(axisPos);
	}
	return gd;
}

QDialog* ApplicationWindow::showPlot3dDialog()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D")){
		Graph3D* g = (Graph3D*)ws->activeWindow();
		if (!g->hasData()){
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, tr("QtiPlot - Warning"),
					tr("Not available for empty 3D surface plots!"));
			return 0;
		}

		Plot3DDialog* pd = new Plot3DDialog(this);
		pd->setPlot(g);
		pd->show();
		return pd;
	}
	else return 0;
}

void ApplicationWindow::showPlotDialog(int curveKey)
{
	QWidget *w = ws->activeWindow();
	if (!w)
		return;

	if (w->isA("MultiLayer")){
		PlotDialog* pd = new PlotDialog(d_extended_plot_dialog, this);
        pd->setAttribute(Qt::WA_DeleteOnClose);
        pd->insertColumnsList(columnsList(Table::All));
        pd->setMultiLayer((MultiLayer*)w);
        if (curveKey >= 0){
			Graph *g = ((MultiLayer*)w)->activeGraph();
			if (g)
            	pd->selectCurve(g->curveIndex(curveKey));
		}
        pd->initFonts(plotTitleFont, plotAxesFont, plotNumbersFont, plotLegendFont);
		pd->showAll(d_extended_plot_dialog);
        pd->show();
	}
}

void ApplicationWindow::showCurvePlotDialog()
{
	showPlotDialog(actionShowCurvePlotDialog->data().toInt());
}

void ApplicationWindow::showCurveContextMenu(int curveKey)
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph *g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	DataCurve *c = (DataCurve *)g->curve(g->curveIndex(curveKey));
	if (!c || !c->isVisible())
		return;

	QMenu curveMenu(this);
	curveMenu.addAction(c->title().text(), this, SLOT(showCurvePlotDialog()));
	curveMenu.insertSeparator();

	curveMenu.addAction(actionHideCurve);
	actionHideCurve->setData(curveKey);

    if (g->visibleCurves() > 1 && c->type() == Graph::Function){
        curveMenu.addAction(actionHideOtherCurves);
        actionHideOtherCurves->setData(curveKey);
    } else if (c->type() != Graph::Function) {
        if ((g->visibleCurves() - c->errorBarsList().count()) > 1) {
            curveMenu.addAction(actionHideOtherCurves);
            actionHideOtherCurves->setData(curveKey);
        }
    }

	if (g->visibleCurves() != g->curves())
		curveMenu.addAction(actionShowAllCurves);
	curveMenu.insertSeparator();

    if (g->activeTool()){
        if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector ||
            g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
            curveMenu.addAction(actionCopySelection);
    }

	if (c->type() == Graph::Function){
        curveMenu.insertSeparator();
		curveMenu.addAction(actionEditFunction);
		actionEditFunction->setData(curveKey);
	} else if (c->type() != Graph::ErrorBars){
        if (g->activeTool()){
            if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector ||
                g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker){
                curveMenu.addAction(actionCutSelection);
                curveMenu.addAction(actionPasteSelection);
				curveMenu.addAction(actionClearSelection);
				curveMenu.insertSeparator();
                if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector){
                    QAction *act = new QAction(tr("Set Display Range"), this);
                    connect(act, SIGNAL(activated()), (RangeSelectorTool *)g->activeTool(), SLOT(setCurveRange()));
                    curveMenu.addAction(act);
                }
			}
        }

		curveMenu.addAction(actionEditCurveRange);
		actionEditCurveRange->setData(curveKey);

		curveMenu.addAction(actionCurveFullRange);
		if (c->isFullRange())
			actionCurveFullRange->setDisabled(true);
		else
			actionCurveFullRange->setEnabled(true);
		actionCurveFullRange->setData(curveKey);

		curveMenu.insertSeparator();
	}

	curveMenu.addAction(actionShowCurveWorksheet);
	actionShowCurveWorksheet->setData(curveKey);

	curveMenu.addAction(actionShowCurvePlotDialog);
	actionShowCurvePlotDialog->setData(curveKey);

	curveMenu.insertSeparator();

	curveMenu.addAction(actionRemoveCurve);
	actionRemoveCurve->setData(curveKey);
	curveMenu.exec(QCursor::pos());
}

void ApplicationWindow::showAllCurves()
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	for(int i=0; i< g->curves(); i++)
		g->showCurve(i);
	g->replot();
}

void ApplicationWindow::hideOtherCurves()
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	int curveKey = actionHideOtherCurves->data().toInt();
	for(int i=0; i< g->curves(); i++)
		g->showCurve(i, false);

	g->showCurve(g->curveIndex(curveKey));
	g->replot();
}

void ApplicationWindow::hideCurve()
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	int curveKey = actionHideCurve->data().toInt();
	g->showCurve(g->curveIndex(curveKey), false);
}

void ApplicationWindow::removeCurve()
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	int curveKey = actionRemoveCurve->data().toInt();
	g->removeCurve(g->curveIndex(curveKey));
	g->updatePlot();
}

void ApplicationWindow::showCurveWorksheet(Graph *g, int curveIndex)
{
	if (!g)
		return;

    const QwtPlotItem *it = g->plotItem(curveIndex);
	if (!it)
		return;

	if (it->rtti() == QwtPlotItem::Rtti_PlotSpectrogram){
		Spectrogram *sp = (Spectrogram *)it;
		if (sp->matrix())
			sp->matrix()->showMaximized();
	} else if (((PlotCurve *)it)->type() == Graph::Function)
		g->createTable((PlotCurve *)it);
    else {
		showTable(it->title().text());
		if (g->activeTool() && g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
            ((DataPickerTool *)g->activeTool())->selectTableRow();
    }
}

void ApplicationWindow::showCurveWorksheet()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	int curveKey = actionShowCurveWorksheet->data().toInt();
	showCurveWorksheet(g, g->curveIndex(curveKey));
}

void ApplicationWindow::zoomIn()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty())
	{
		QMessageBox::warning(this, tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setOn(true);
		return;
	}

	if ((Graph*)plot->activeGraph()->isPiePlot())
	{
		if (btnZoomIn->isOn())
			QMessageBox::warning(this,tr("QtiPlot - Warning"),
					tr("This functionality is not available for pie plots!"));
		btnPointer->setOn(true);
		return;
	}

	QWidgetList graphsList=plot->graphPtrs();
	foreach(QWidget *widget, graphsList)
	{
		Graph *g = (Graph *)widget;
		if (!g->isPiePlot())
			g->zoom(true);
	}
}

void ApplicationWindow::zoomOut()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty() || (Graph*)plot->activeGraph()->isPiePlot())
		return;

	((Graph*)plot->activeGraph())->zoomOut();
	btnPointer->setOn(true);
}

void ApplicationWindow::setAutoScale()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"));
		return;
	}

	Graph *g = (Graph*)plot->activeGraph();
	if (g){
		g->setAutoScale();
		emit modified();
	}
}

void ApplicationWindow::removePoints()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g || !g->validCurvesDataSize())
	{
		btnPointer->setChecked(true);
		return;
	}

	if (g->isPiePlot())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("This functionality is not available for pie plots!"));
		btnPointer->setChecked(true);
		return;
	}
	else
	{
		switch(QMessageBox::warning (this,tr("QtiPlot"),
					tr("This will modify the data in the worksheets!\nAre you sure you want to continue?"),
					tr("Continue"),tr("Cancel"),0,1))
		{
			case 0:
				g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Remove, info, SLOT(setText(const QString&))));
				displayBar->show();
				break;

			case 1:
				btnPointer->setChecked(true);
				break;
		}
	}
}

void ApplicationWindow::movePoints()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g || !g->validCurvesDataSize()){
		btnPointer->setChecked(true);
		return;
	}

	if (g->isPiePlot()){
		QMessageBox::warning(this, tr("QtiPlot - Warning"),
				tr("This functionality is not available for pie plots!"));

		btnPointer->setChecked(true);
		return;
	} else {
		switch(QMessageBox::warning (this, tr("QtiPlot"),
					tr("This will modify the data in the worksheets!\nAre you sure you want to continue?"),
					tr("Continue"), tr("Cancel"), 0, 1))
		{
			case 0:
				if (g){
					g->setActiveTool(new DataPickerTool(g, this, DataPickerTool::Move, info, SLOT(setText(const QString&))));
					displayBar->show();
				}
				break;

			case 1:
				btnPointer->setChecked(true);
				break;
		}
	}
}

void ApplicationWindow::exportPDF()
{
	QWidget* w = ws->activeWindow();
	if (!w)
		return;

	if (w->isA("MultiLayer") && ((MultiLayer*)w)->isEmpty())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"));
		return;
	}

    QString fname = QFileDialog::getSaveFileName(this, tr("Choose a filename to save under"), workingDir, "*.pdf");
	if (!fname.isEmpty() )
	{
		QFileInfo fi(fname);
		QString baseName = fi.fileName();
		if (!baseName.contains("."))
			fname.append(".pdf");

        workingDir = fi.dirPath(true);

        QFile f(fname);
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            QMessageBox::critical(this, tr("QtiPlot - Export error"),
            tr("Could not write to file: <h4>%1</h4><p>Please verify that you have the right to write to this location or that the file is not being used by another application!").arg(fname));
            return;
        }

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        ((MyWidget*)w)->exportPDF(fname);

		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::print(QWidget* w)
{
	if (w->isA("MultiLayer") && ((MultiLayer*)w)->isEmpty())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"));
		return;
	}

	((MyWidget*)w)->print();
}

//print active window
void ApplicationWindow::print()
{
	QWidget* w = ws->activeWindow();
	if (!w)
		return;

	print(w);
}

// print window from project explorer
void ApplicationWindow::printWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MyWidget *w= it->window();
	if (!w)
		return;

	print(w);
}

void ApplicationWindow::printAllPlots()
{
	QPrinter printer;
	printer.setOrientation(QPrinter::Landscape);
	printer.setColorMode (QPrinter::Color);
	printer.setFullPage(true);

	if (printer.setup())
	{
		QPainter *paint = new QPainter (&printer);
		QWidgetList *windows = windowsList();

		int plots = 0;
		QWidget *w = 0;
		foreach(w, *windows)
		{
			if (w->isA("MultiLayer"))
				plots++;
		}

		printer.setMinMax (0, plots);
		printer.setFromTo (0, plots);

		foreach(w, *windows)
		{
			if (w->isA("MultiLayer") && printer.newPage())
				((MultiLayer*)w)->printAllLayers(paint);
		}
		paint->end();
		delete paint;
		delete windows;
	}
}

void ApplicationWindow::showExpGrowthDialog()
{
	showExpDecayDialog(-1);
}

void ApplicationWindow::showExpDecayDialog()
{
	showExpDecayDialog(1);
}

void ApplicationWindow::showExpDecayDialog(int type)
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	ExpDecayDialog *edd = new ExpDecayDialog(type, this);
	edd->setAttribute(Qt::WA_DeleteOnClose);
	connect (g, SIGNAL(destroyed()), edd, SLOT(close()));

	edd->setGraph(g);
	edd->show();
}

void ApplicationWindow::showTwoExpDecayDialog()
{
	showExpDecayDialog(2);
}

void ApplicationWindow::showExpDecay3Dialog()
{
	showExpDecayDialog(3);
}

void ApplicationWindow::showFitDialog()
{
	QWidget *w = ws->activeWindow();
	if (!w)
		return;

	MultiLayer* plot = 0;
	if(w->isA("MultiLayer"))
		plot = (MultiLayer*)w;
	else if(w->inherits("Table"))
		plot = multilayerPlot((Table *)w, ((Table *)w)->drawableColumnSelection(), Graph::LineSymbols);

	if (!plot)
		return;

	Graph* g = (Graph*)plot->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	FitDialog *fd = new FitDialog(g, this);
	fd->setAttribute(Qt::WA_DeleteOnClose);
	connect (plot, SIGNAL(destroyed()), fd, SLOT(close()));

	fd->setSrcTables(tableList());
	fd->show();
	fd->resize(fd->minimumSize());
}

void ApplicationWindow::showFilterDialog(int filter)
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if ( g && g->validCurvesDataSize())
	{
		FilterDialog *fd = new FilterDialog(filter, this);
		fd->setAttribute(Qt::WA_DeleteOnClose);
		fd->setGraph(g);
		fd->exec();
	}
}

void ApplicationWindow::lowPassFilterDialog()
{
	showFilterDialog(FFTFilter::LowPass);
}

void ApplicationWindow::highPassFilterDialog()
{
	 showFilterDialog(FFTFilter::HighPass);
}

void ApplicationWindow::bandPassFilterDialog()
{
	showFilterDialog(FFTFilter::BandPass);
}

void ApplicationWindow::bandBlockFilterDialog()
{
	showFilterDialog(FFTFilter::BandBlock);
}

void ApplicationWindow::showFFTDialog()
{
	QWidget *w = ws->activeWindow();
	if (!w)
		return;

	FFTDialog *sd = 0;
	if (w->isA("MultiLayer")) {
		Graph* g = ((MultiLayer*)w)->activeGraph();
		if ( g && g->validCurvesDataSize() ){
			sd = new FFTDialog(FFTDialog::onGraph, this);
			sd->setAttribute(Qt::WA_DeleteOnClose);
			sd->setGraph(g);
		}
	} else if (w->inherits("Table")) {
		sd = new FFTDialog(FFTDialog::onTable, this);
		sd->setAttribute(Qt::WA_DeleteOnClose);
		sd->setTable((Table*)w);
	} else if (w->inherits("Matrix")) {
    #ifdef QTIPLOT_PRO
		sd = new FFTDialog(FFTDialog::onMatrix, this);
		sd->setAttribute(Qt::WA_DeleteOnClose);
		sd->setMatrix((Matrix*)w);
    #else
        QString s = tr("This feature is only available to users having subscribed for a binaries maintenance contract!");
        s += " " + tr("Please visit the following web page for more details:");
        s += "<p><a href = http://soft.proindependent.com/pricing.html>http://soft.proindependent.com/pricing.html</a></p>";
        QMessageBox::critical(this, tr("QtiPlot"), s);
        return;
    #endif
	}

	if (sd)
        sd->exec();
}

void ApplicationWindow::showSmoothDialog(int m)
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	SmoothCurveDialog *sd = new SmoothCurveDialog(m, this);
	sd->setAttribute(Qt::WA_DeleteOnClose);
	sd->setGraph(g);
	sd->exec();
}

void ApplicationWindow::showSmoothSavGolDialog()
{
    showSmoothDialog(SmoothFilter::SavitzkyGolay);
}

void ApplicationWindow::showSmoothFFTDialog()
{
	showSmoothDialog(SmoothFilter::FFT);
}

void ApplicationWindow::showSmoothAverageDialog()
{
	showSmoothDialog(SmoothFilter::Average);
}

void ApplicationWindow::showInterpolationDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	InterpolationDialog *id = new InterpolationDialog(this);
	id->setAttribute(Qt::WA_DeleteOnClose);
	connect (g, SIGNAL(destroyed()), id, SLOT(close()));
	id->setGraph(g);
	id->show();
}

void ApplicationWindow::showFitPolynomDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	PolynomFitDialog *pfd = new PolynomFitDialog(this);
	pfd->setAttribute(Qt::WA_DeleteOnClose);
	connect(g, SIGNAL(destroyed()), pfd, SLOT(close()));
	pfd->setGraph(g);
	pfd->show();
}

void ApplicationWindow::fitLinear()
{
	analysis("fitLinear");
}

void ApplicationWindow::updateLog(const QString& result)
{
	if ( !result.isEmpty() )
	{
		logInfo+=result;
		showResults(true);
		emit modified();
	}
}

void ApplicationWindow::showIntegrationDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	IntDialog *id = new IntDialog(this);
	id->setAttribute(Qt::WA_DeleteOnClose);
	connect (g, SIGNAL(destroyed()), id, SLOT(close()));
	id->setGraph(g);
	id->show();
}

void ApplicationWindow::fitSigmoidal()
{
	analysis("fitSigmoidal");
}

void ApplicationWindow::fitGauss()
{
	analysis("fitGauss");
}

void ApplicationWindow::fitLorentz()

{
	analysis("fitLorentz");
}

void ApplicationWindow::differentiate()
{
	analysis("differentiate");
}

void ApplicationWindow::showResults(bool ok)
{
	if (ok){
		if (!logInfo.isEmpty())
			results->setText(logInfo);
		else
			results->setText(tr("Sorry, there are no results to display!"));

		logWindow->show();
		QTextCursor cur = results->textCursor();
		cur.movePosition(QTextCursor::End);
		results->setTextCursor(cur);
	} else
		logWindow->hide();
}

void ApplicationWindow::showResults(const QString& s, bool ok)
{
	logInfo += s;
	if (!logInfo.isEmpty())
			results->setText(logInfo);
	showResults(ok);
}

void ApplicationWindow::showScreenReader()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	QWidgetList graphsList=plot->graphPtrs();
	foreach(QWidget *w, graphsList)
		((Graph *)w)->setActiveTool(new ScreenPickerTool((Graph*)w, info, SLOT(setText(const QString&))));

	displayBar->show();
}

void ApplicationWindow::drawPoints()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	QWidgetList graphsList=plot->graphPtrs();
	foreach(QWidget *w, graphsList)
		((Graph *)w)->setActiveTool(new DrawPointTool(this, (Graph*)w, info, SLOT(setText(const QString&))));

	displayBar->show();
}

void ApplicationWindow::showRangeSelectors()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this, tr("QtiPlot - Warning"),
				tr("There are no plot layers available in this window!"));
		btnPointer->setChecked(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g)
		return;

	if (!g->curves()){
		QMessageBox::warning(this, tr("QtiPlot - Warning"),
				tr("There are no curves available on this plot!"));
		btnPointer->setChecked(true);
		return;
	} else if (g->isPiePlot()) {
		QMessageBox::warning(this, tr("QtiPlot - Warning"),
				tr("This functionality is not available for pie plots!"));
		btnPointer->setChecked(true);
		return;
	}

	displayBar->show();
	g->enableRangeSelectors(info, SLOT(setText(const QString&)));
}

void ApplicationWindow::showCursor()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	if ((Graph*)plot->activeGraph()->isPiePlot()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("This functionality is not available for pie plots!"));

		btnPointer->setChecked(true);
		return;
	}

	QWidgetList graphsList=plot->graphPtrs();
	foreach(QWidget *w, graphsList)
		if (!((Graph *)w)->isPiePlot() && ((Graph *)w)->validCurvesDataSize())
			((Graph *)w)->setActiveTool(new DataPickerTool((Graph*)w, this, DataPickerTool::Display, info, SLOT(setText(const QString&))));

	displayBar->show();
}

void ApplicationWindow::newLegend()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if ( g )
		g->newLegend();
}

void ApplicationWindow::addTimeStamp()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if ( g )
		g->addTimeStamp();
}

void ApplicationWindow::disableAddText()
{
	actionAddText->setChecked(false);
	showTextDialog();
}

void ApplicationWindow::addText()
{
	if (!btnPointer->isOn())
		btnPointer->setOn(TRUE);

	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	Graph *g = (Graph*)plot->activeGraph();
	if (g)
		g->drawText(true);
}

void ApplicationWindow::addImage()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g)
		return;

	QList<QByteArray> list = QImageReader::supportedImageFormats();
	QString filter = tr("Images") + " (", aux1, aux2;
	for (int i=0; i<(int)list.count(); i++){
		aux1 = " *."+list[i]+" ";
		aux2 += " *."+list[i]+";;";
		filter += aux1;
	}
	filter+=");;" + aux2;

	QString fn = QFileDialog::getOpenFileName(this, tr("QtiPlot - Insert image from file"), imagesDirPath, filter);
	if ( !fn.isEmpty() ){
		QFileInfo fi(fn);
		imagesDirPath = fi.dirPath(true);

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		g->addImage(fn);
		QApplication::restoreOverrideCursor();
	}
}

void ApplicationWindow::drawLine()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));

		btnPointer->setChecked(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (g)
	{
		g->drawLine(true);
		emit modified();
	}
}

void ApplicationWindow::drawArrow()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if (plot->isEmpty())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));

		btnPointer->setOn(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (g)
	{
		g->drawLine(true, 1);
		emit modified();
	}
}

void ApplicationWindow::showImageDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (g)
	{
		ImageMarker *im = (ImageMarker *) g->selectedMarkerPtr();
		if (!im)
			return;

		ImageDialog *id = new ImageDialog(this);
		id->setAttribute(Qt::WA_DeleteOnClose);
		connect (id, SIGNAL(setGeometry(int, int, int, int)),
				g, SLOT(updateImageMarker(int, int, int, int)));
		id->setIcon(QPixmap(logo_xpm));
		id->setOrigin(im->origin());
		id->setSize(im->size());
		id->exec();
	}
}

void ApplicationWindow::showLayerDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer*)ws->activeWindow();
	if(plot->isEmpty())
	{
		QMessageBox::warning(this, tr("QtiPlot - Warning"),
				tr("There are no plot layers available in this window."));
		return;
	}

	LayerDialog *id=new LayerDialog(this);
	id->setAttribute(Qt::WA_DeleteOnClose);
	id->setMultiLayer(plot);
	id->exec();
}

void ApplicationWindow::showTextDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if ( g ){
		LegendWidget *l = (LegendWidget *) g->selectedText();
		if (!l)
			return;

		TextDialog *td = new TextDialog(TextDialog::TextMarker, this, 0);
		td->setAttribute(Qt::WA_DeleteOnClose);
		td->setLegendWidget(l);
		td->exec();
	}
}

void ApplicationWindow::showLineDialog()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (g){
		ArrowMarker *lm = (ArrowMarker *) g->selectedMarkerPtr();
		if (!lm)
			return;

		LineDialog *ld = new LineDialog(lm, this, Qt::Tool);
		ld->setAttribute(Qt::WA_DeleteOnClose);
		ld->exec();
	}
}

void ApplicationWindow::addColToTable()
{
	Table* m = (Table*)ws->activeWindow();
	if ( m )
		m->addCol();
}

void ApplicationWindow::clearSelection()
{
	if(lv->hasFocus()){
		deleteSelectedItems();
		return;
	}

	QWidget* m = (QWidget*)ws->activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->clearSelection();
	else if (m->isA("Matrix"))
		((Matrix*)m)->clearSelection();
	else if (m->isA("MultiLayer")){
		Graph* g = ((MultiLayer*)m)->activeGraph();
		if (!g)
			return;

        if (g->activeTool()){
            if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
                ((RangeSelectorTool *)g->activeTool())->clearSelection();
            else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
                ((DataPickerTool *)g->activeTool())->removePoint();
        } else if (g->titleSelected())
			g->removeTitle();
		else if (g->markerSelected())
			g->removeMarker();
	}
	else if (m->isA("Note"))
		((Note*)m)->textWidget()->clear();
	emit modified();
}

void ApplicationWindow::copySelection()
{
	if(results->hasFocus()){
		results->copy();
		return;
	} else if(info->hasFocus()) {
		info->copy();
		return;
	}

	QWidget* m = (QWidget*)ws->activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->copySelection();
	else if (m->isA("Matrix"))
		((Matrix*)m)->copySelection();
	else if (m->isA("MultiLayer")){
		MultiLayer* plot = (MultiLayer*)m;
		if (!plot || plot->layers() == 0)
			return;

		Graph* g = (Graph*)plot->activeGraph();
		if (!g)
            return;

        if (g->activeTool()){
            if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
                ((RangeSelectorTool *)g->activeTool())->copySelection();
            else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
                ((DataPickerTool *)g->activeTool())->copySelection();
		} else if (g->markerSelected()){
			copyMarker();
		} else
			copyActiveLayer();

		plot->copyAllLayers();
	}
	else if (m->isA("Note"))
		((Note*)m)->textWidget()->copy();
}

void ApplicationWindow::cutSelection()
{
	QWidget* m = (QWidget*)ws->activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->cutSelection();
	else if (m->isA("Matrix"))
		((Matrix*)m)->cutSelection();
	else if(m->isA("MultiLayer")){
		MultiLayer* plot = (MultiLayer*)m;
		if (!plot || plot->layers() == 0)
			return;

		Graph* g = (Graph*)plot->activeGraph();
		if (!g)
            return;

		if (g->activeTool()){
            if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
                ((RangeSelectorTool *)g->activeTool())->cutSelection();
            else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
                ((DataPickerTool *)g->activeTool())->cutSelection();
        } else {
            copyMarker();
            g->removeMarker();
        }
	}
	else if (m->isA("Note"))
		((Note*)m)->textWidget()->cut();

	emit modified();
}

void ApplicationWindow::copyMarker()
{
    lastCopiedLayer = NULL;

	QWidget* m = (QWidget*)ws->activeWindow();
	MultiLayer* plot = (MultiLayer*)m;
	Graph* g = (Graph*)plot->activeGraph();
	if (g && g->markerSelected()){
		if (g->selectedText()){
			d_text_copy = g->selectedText();
			d_image_copy = NULL;
			d_arrow_copy = NULL;
		} else if (g->arrowMarkerSelected()){
			d_arrow_copy = (ArrowMarker *) g->selectedMarkerPtr();
            d_image_copy = NULL;
			d_text_copy = NULL;
		} else if (g->imageMarkerSelected()){
			d_image_copy = (ImageMarker *) g->selectedMarkerPtr();
			d_text_copy = NULL;
			d_arrow_copy = NULL;
		}
	}
}

void ApplicationWindow::pasteSelection()
{
	QWidget* m = (QWidget*)ws->activeWindow();
	if (!m)
		return;

	if (m->inherits("Table"))
		((Table*)m)->pasteSelection();
	else if (m->isA("Matrix"))
		((Matrix*)m)->pasteSelection();
	else if (m->isA("Note"))
		((Note*)m)->textWidget()->paste();
	else if (m->isA("MultiLayer")){
		MultiLayer* plot = (MultiLayer*)m;
		if (!plot)
			return;

		if (lastCopiedLayer){
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

			Graph* g = plot->addLayer();
			g->copy(lastCopiedLayer);
			QPoint pos = plot->mapFromGlobal(QCursor::pos());
			plot->setGraphGeometry(pos.x(), pos.y()-20, lastCopiedLayer->width(), lastCopiedLayer->height());

			QApplication::restoreOverrideCursor();
		} else {
			if (plot->layers() == 0)
				return;

			Graph* g = (Graph*)plot->activeGraph();
			if (!g)
				return;

            if (g->activeTool()){
                if (g->activeTool()->rtti() == PlotToolInterface::Rtti_RangeSelector)
                    ((RangeSelectorTool *)g->activeTool())->pasteSelection();
                else if (g->activeTool()->rtti() == PlotToolInterface::Rtti_DataPicker)
                    ((DataPickerTool *)g->activeTool())->pasteSelection();
            } else if (d_text_copy){
				LegendWidget *t = g->insertText(d_text_copy);
				t->move(g->mapFromGlobal(QCursor::pos()));
			} else if (d_arrow_copy){
                ArrowMarker *a = g->addArrow(d_arrow_copy);
                a->setStartPoint(QPoint(d_arrow_copy->startPoint().x() + 10,
                                        d_arrow_copy->startPoint().y() + 10));
                a->setEndPoint(QPoint(d_arrow_copy->endPoint().x() + 10,
                                      d_arrow_copy->endPoint().y() + 10));
                g->replot();
                g->deselectMarker();
			} else if (d_image_copy){
                ImageMarker *i = g->addImage(d_image_copy);
				QPoint pos = g->plotWidget()->canvas()->mapFromGlobal(QCursor::pos());
				QSize size = d_image_copy->size();
				i->setRect(pos.x(), pos.y(), size.width(), size.height());
                g->replot();
                g->deselectMarker();
            }
		}
	}
	emit modified();
}

MyWidget* ApplicationWindow::clone(MyWidget* w)
{
    if (!w) {
        w = (MyWidget*)ws->activeWindow();
		if (!w){
			QMessageBox::critical(this,tr("QtiPlot - Duplicate window error"),
				tr("There are no windows available in this project!"));
			return 0;
		}
	}

	MyWidget* nw = 0;
	MyWidget::Status status = w->status();
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (w->isA("MultiLayer")){
		nw = multilayerPlot(generateUniqueName(tr("Graph")));
		((MultiLayer *)nw)->copy((MultiLayer *)w);
	} else if (w->inherits("Table")){
		Table *t = (Table *)w;
		QString caption = generateUniqueName(tr("Table"));
    	nw = newTable(caption, t->numRows(), t->numCols());
    	((Table *)nw)->copy(t);
    	QString spec = t->saveToString("geometry\n");
    	((Table *)nw)->setSpecifications(spec.replace(t->objectName(), caption));
	} else if (w->isA("Graph3D")){
		Graph3D *g = (Graph3D *)w;
		if (!g->hasData()){
        	QApplication::restoreOverrideCursor();
        	QMessageBox::warning(this, tr("QtiPlot - Duplicate error"), tr("Empty 3D surface plots cannot be duplicated!"));
        	return 0;
    	}

		QString caption = generateUniqueName(tr("Graph"));
		QString s = g->formula();
		if (g->userFunction()){
			UserFunction *f = g->userFunction();
			nw = plotSurface(f->function(), g->xStart(), g->xStop(), g->yStart(), g->yStop(),
							 g->zStart(), g->zStop(), f->columns(), f->rows());
		} else if (g->parametricSurface()){
			UserParametricSurface *s = g->parametricSurface();
			nw = plotParametricSurface(s->xFormula(), s->yFormula(), s->zFormula(), s->uStart(), s->uEnd(),
				 	s->vStart(), s->vEnd(), s->columns(), s->rows(), s->uPeriodic(), s->vPeriodic());
		} else if (s.endsWith("(Z)"))
			nw = openPlotXYZ(caption, s, g->xStart(),g->xStop(), g->yStart(),g->yStop(),g->zStart(),g->zStop());
		else if (s.endsWith("(Y)"))//Ribbon plot
			nw = dataPlot3D(caption, s, g->xStart(),g->xStop(), g->yStart(),g->yStop(),g->zStart(),g->zStop());
		else
			nw = openMatrixPlot3D(caption, s, g->xStart(), g->xStop(), g->yStart(), g->yStop(),g->zStart(),g->zStop());

		if (!nw)
			return 0;

        if (status == MyWidget::Maximized)
            nw->hide();
		((Graph3D *)nw)->copy(g);
		customToolBars((QWidget*)nw);
	} else if (w->isA("Matrix")){
		nw = newMatrix(((Matrix *)w)->numRows(), ((Matrix *)w)->numCols());
    	((Matrix *)nw)->copy((Matrix *)w);
	} else if (w->isA("Note")){
		nw = newNote();
		if (nw)
			((Note*)nw)->setText(((Note*)w)->text());
	}

	if (nw){
		if (w->isA("MultiLayer")){
			if (status == MyWidget::Maximized)
				nw->showMaximized();
		} else if (w->isA("Graph3D")){
            ((Graph3D*)nw)->setIgnoreFonts(true);
			if (status != MyWidget::Maximized){
				nw->resize(w->size());
				nw->showNormal();
			} else
                nw->showMaximized();
            ((Graph3D*)nw)->setIgnoreFonts(false);
		} else {
			nw->resize(w->size());
			nw->showNormal();
		}

		nw->setWindowLabel(w->windowLabel());
		nw->setCaptionPolicy(w->captionPolicy());
		setListViewLabel(nw->objectName(), w->windowLabel());
		setListViewSize(nw->objectName(), w->sizeToString());
	}
	QApplication::restoreOverrideCursor();
	customMenu(nw);
	return nw;
}

void ApplicationWindow::undo()
{
	if (!lastModified)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	if (lastModified->inherits("Table")){
		Table *t = (Table *)lastModified;
		t->setNewSpecifications();
		QString newCaption=t->oldCaption();
		QString name=lastModified->objectName();
		if (newCaption != name){
			int id=tableWindows.findIndex(name);
			tableWindows[id]=newCaption;
			updateTableNames(name,newCaption);
			renameListViewItem(name,newCaption);
		}
		t->restore(t->getSpecifications());
		actionUndo->setEnabled(false);
		actionRedo->setEnabled(true);
	} else if (lastModified->isA("Note")) {
		((Note*)lastModified)->textWidget()->undo();
		actionUndo->setEnabled(false);
		actionRedo->setEnabled(true);
	}

	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::redo()
{
	if (!lastModified)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	if (lastModified->inherits("Table"))
	{
		Table *t= (Table *)lastModified;
		QString newCaption=t->newCaption();
		QString name=lastModified->objectName();
		if (newCaption != name)
		{
			int id=tableWindows.findIndex(name);
			tableWindows[id]=newCaption;
			updateTableNames(name,newCaption);
			renameListViewItem(name,newCaption);
		}
		t->restore(t->getNewSpecifications());
		actionUndo->setEnabled(true);
		actionRedo->setEnabled(false);
	}
	else if (lastModified->isA("Note"))
	{
		((Note*)lastModified)->textWidget()->redo();
		actionUndo->setEnabled(true);
		actionRedo->setEnabled(false);
	}
	QApplication::restoreOverrideCursor();
}

bool ApplicationWindow::hidden(QWidget* window)
{
	if (hiddenWindows->contains(window) || outWindows->contains(window))
		return true;

	return false;
}

void ApplicationWindow::updateWindowStatus(MyWidget* w)
{
	setListView(w->objectName(), w->aspect());
	if (w->status() == MyWidget::Maximized){
		QList<MyWidget *> windows = current_folder->windowsList();
		foreach(MyWidget *oldMaxWindow, windows){
			if (oldMaxWindow != w && oldMaxWindow->status() == MyWidget::Maximized)
				oldMaxWindow->setStatus(MyWidget::Normal);
		}
	}
	modifiedProject();
}

void ApplicationWindow::hideActiveWindow()
{
	MyWidget *w=(MyWidget *)ws->activeWindow();
	if (!w)
		return;

	hideWindow(w);
}

void ApplicationWindow::hideWindow(MyWidget* w)
{
	hiddenWindows->append(w);
	w->setHidden();
	emit modified();
}

void ApplicationWindow::hideWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MyWidget *w = it->window();
	if (!w)
		return;

	hideWindow(w);
}

void ApplicationWindow::resizeActiveWindow()
{
	QWidget *w = (QWidget *)ws->activeWindow();
	if (!w)
		return;

	ImageDialog *id = new ImageDialog(this);
	id->setAttribute(Qt::WA_DeleteOnClose);
	connect (id, SIGNAL(setGeometry(int,int,int,int)), this, SLOT(setWindowGeometry(int,int,int,int)));

	id->setWindowTitle(tr("QtiPlot - Window Geometry"));
	id->setOrigin(w->parentWidget()->pos());
	id->setSize(w->parentWidget()->size());
	id->exec();
}

void ApplicationWindow::resizeWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MyWidget *w = it->window();
	if (!w)
		return;

	ws->setActiveWindow ( w );

	ImageDialog *id = new ImageDialog(this);
	id->setAttribute(Qt::WA_DeleteOnClose);
	connect (id, SIGNAL(setGeometry(int,int,int,int)), this, SLOT(setWindowGeometry(int,int,int,int)));

	id->setWindowTitle(tr("QtiPlot - Window Geometry"));
	id->setOrigin(w->parentWidget()->pos());
	id->setSize(w->parentWidget()->size());
	id->exec();
}

void ApplicationWindow::setWindowGeometry(int x, int y, int w, int h)
{
	ws->activeWindow()->parentWidget()->setGeometry(x, y, w, h);
}

void ApplicationWindow::activateWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	activateWindow(it->window());
}

void ApplicationWindow::activateWindow(MyWidget *w)
{
	if (!w)
		return;

	w->setNormal();
	ws->setActiveWindow(w);

	updateWindowLists(w);
	emit modified();
}

void ApplicationWindow::maximizeWindow(Q3ListViewItem * lbi)
{
	if (!lbi || lbi->rtti() == FolderListItem::RTTI)
		return;

	MyWidget *w = ((WindowListItem*)lbi)->window();
	if (!w)
		return;

	w->setMaximized();
	updateWindowLists(w);
	emit modified();
}

void ApplicationWindow::maximizeWindow(MyWidget *w)
{
	if (!w)
		return;

	w->setMaximized();
	updateWindowLists(w);
	emit modified();
}

void ApplicationWindow::maximizeWindow()
{
	maximizeWindow(lv->currentItem());
}

void ApplicationWindow::minimizeWindow()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MyWidget *w= it->window();
	if (!w)
		return;

	updateWindowLists(w);
	w->setMinimized();
	emit modified();
}

void ApplicationWindow::minimizeWindow(MyWidget *w)
{
	if (!w)
		return;

	updateWindowLists(w);
	w->setMinimized();
	emit modified();
}

void ApplicationWindow::updateWindowLists(MyWidget *w)
{
	if (!w)
		return;

	if (hiddenWindows->contains(w))
		hiddenWindows->takeAt(hiddenWindows->indexOf(w));
	else if (outWindows->contains(w))
	{
		outWindows->takeAt(outWindows->indexOf(w));
		ws->addWindow(w);
		w->setAttribute(Qt::WA_DeleteOnClose);
	}
}

void ApplicationWindow::closeActiveWindow()
{
	QWidget *w=(QWidget *)ws->activeWindow();
	if (w)
		w->close();
}

void ApplicationWindow::removeWindowFromLists(MyWidget* w)
{
	if (!w)
		return;

	QString caption = w->objectName();
	if (w->inherits("Table")){
		Table* m=(Table*)w;
		for (int i=0; i<m->numCols(); i++){
			QString name=m->colName(i);
			removeCurves(name);
		}
		tableWindows.remove(caption);
		if (w == lastModified){
			actionUndo->setEnabled(false);
			actionRedo->setEnabled(false);
		}
	}
	else if (w->isA("TableStatistics"))
		tableWindows.remove(caption);
	else if (w->isA("MultiLayer")){
		MultiLayer *ml =  (MultiLayer*)w;
		Graph *g = ml->activeGraph();
		if (g)
			btnPointer->setChecked(true);
	}
	else if (w->isA("Matrix"))
		remove3DMatrixPlots((Matrix*)w);

	if (hiddenWindows->contains(w))
		hiddenWindows->takeAt(hiddenWindows->indexOf(w));
	else if (outWindows->contains(w))
		outWindows->takeAt(outWindows->indexOf(w));
}

void ApplicationWindow::closeWindow(MyWidget* window)
{
	if (!window)
		return;

	removeWindowFromLists(window);
	if (window->inherits("Table")){
		if (!tableWindows.count())
			actionShowExportASCIIDialog->setEnabled(false);
	}

	Folder *f = window->folder();
	f->removeWindow(window);

	//update list view in project explorer
	Q3ListViewItem *it=lv->findItem (window->objectName(), 0, Q3ListView::ExactMatch|Q3ListView::CaseSensitive);
	if (it)
		lv->takeItem(it);

	delete window;

    if (show_windows_policy == ActiveFolder && !f->windowsList().count()){
        customMenu(0);
        customToolBars(0);
    } else if (show_windows_policy == SubFolders && !(current_folder->children()).isEmpty()){
		FolderListItem *fi = current_folder->folderListItem();
		FolderListItem *item = (FolderListItem *)fi->firstChild();
		int initial_depth = item->depth();
		bool emptyFolder = true;
		while (item && item->depth() >= initial_depth){
			QList<MyWidget *> lst = item->folder()->windowsList();
			if (lst.count() > 0){
			    emptyFolder = false;
                break;
			}
			item = (FolderListItem *)item->itemBelow();
		}
		if (emptyFolder){
            customMenu(0);
            customToolBars(0);
		}
    }
	emit modified();
}

void ApplicationWindow::about()
{
QString text = "<h2>"+ versionString() + "</h2>";
text +=	"<h3>" + QString(copyright_string).replace("\n", "<br>") + "</h3>";
text += "<h3>" + tr("Released") + ": " + QString(release_date) + "</h3>";

QMessageBox *mb = new QMessageBox();
mb->setWindowTitle (tr("About QtiPlot"));
mb->setWindowIcon (QPixmap(logo_xpm));
mb->setIconPixmap(QPixmap(logo_xpm));
mb->setText(text);
mb->exec();
}

void ApplicationWindow::analysisMenuAboutToShow()
{
    analysisMenu->clear();
    QWidget *w = ws->activeWindow();
    if (!w)
        return;

	if (w->isA("MultiLayer")){
        QMenu *translateMenu = analysisMenu->addMenu (tr("&Translate"));
        translateMenu->addAction(actionTranslateVert);
        translateMenu->addAction(actionTranslateHor);
        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionDifferentiate);
        analysisMenu->addAction(actionShowIntDialog);
        analysisMenu->insertSeparator();

		smoothMenu->clear();
        smoothMenu = analysisMenu->addMenu (tr("&Smooth"));
        smoothMenu->addAction(actionSmoothSavGol);
        smoothMenu->addAction(actionSmoothAverage);
        smoothMenu->addAction(actionSmoothFFT);

		filterMenu->clear();
		filterMenu = analysisMenu->addMenu (tr("&FFT filter"));
        filterMenu->addAction(actionLowPassFilter);
        filterMenu->addAction(actionHighPassFilter);
        filterMenu->addAction(actionBandPassFilter);
        filterMenu->addAction(actionBandBlockFilter);

        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionInterpolate);
        analysisMenu->addAction(actionFFT);
        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionFitLinear);
        analysisMenu->addAction(actionShowFitPolynomDialog);
        analysisMenu->insertSeparator();

		decayMenu->clear();
        decayMenu = analysisMenu->addMenu (tr("Fit E&xponential Decay"));
        decayMenu->addAction(actionShowExpDecayDialog);
        decayMenu->addAction(actionShowTwoExpDecayDialog);
        decayMenu->addAction(actionShowExpDecay3Dialog);

        analysisMenu->addAction(actionFitExpGrowth);
        analysisMenu->addAction(actionFitSigmoidal);
        analysisMenu->addAction(actionFitGauss);
        analysisMenu->addAction(actionFitLorentz);

		multiPeakMenu->clear();
		multiPeakMenu = analysisMenu->addMenu (tr("Fit &Multi-peak"));
        multiPeakMenu->addAction(actionMultiPeakGauss);
        multiPeakMenu->addAction(actionMultiPeakLorentz);
        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionShowFitDialog);
	} else if (w->isA("Matrix")){
        analysisMenu->addAction(actionFFT);
        analysisMenu->addAction(actionMatrixFFTDirect);
        analysisMenu->addAction(actionMatrixFFTInverse);
	} else if (w->inherits("Table")){
        analysisMenu->addAction(actionShowColStatistics);
        analysisMenu->addAction(actionShowRowStatistics);
        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionSortSelection);
        analysisMenu->addAction(actionSortTable);

		normMenu->clear();
		normMenu = analysisMenu->addMenu (tr("&Normalize"));
        normMenu->addAction(actionNormalizeSelection);
        normMenu->addAction(actionNormalizeTable);

        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionFFT);
        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionCorrelate);
        analysisMenu->addAction(actionAutoCorrelate);
        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionConvolute);
        analysisMenu->addAction(actionDeconvolute);
        analysisMenu->insertSeparator();
        analysisMenu->addAction(actionShowFitDialog);
	}
    reloadCustomActions();
}

void ApplicationWindow::matrixMenuAboutToShow()
{
	matrixMenu->clear();
	matrixMenu->addAction(actionSetMatrixProperties);
	matrixMenu->addAction(actionSetMatrixDimensions);
	matrixMenu->insertSeparator();
	matrixMenu->addAction(actionSetMatrixValues);
	matrixMenu->addAction(actionTableRecalculate);
	matrixMenu->insertSeparator();
	matrixMenu->addAction(actionRotateMatrix);
	matrixMenu->addAction(actionRotateMatrixMinus);
	matrixMenu->addAction(actionFlipMatrixVertically);
	matrixMenu->addAction(actionFlipMatrixHorizontally);
	matrixMenu->insertSeparator();
	matrixMenu->addAction(actionTransposeMatrix);
	matrixMenu->addAction(actionInvertMatrix);
	matrixMenu->addAction(actionMatrixDeterminant);
	matrixMenu->insertSeparator();
	matrixMenu->addAction(actionGoToRow);
	matrixMenu->insertSeparator();
	QMenu *matrixViewMenu = matrixMenu->addMenu (tr("Vie&w"));
	matrixViewMenu->addAction(actionViewMatrixImage);
	matrixViewMenu->addAction(actionViewMatrix);
    QMenu *matrixPaletteMenu = matrixMenu->addMenu (tr("&Palette"));
	matrixPaletteMenu->addAction(actionMatrixGrayScale);
	matrixPaletteMenu->addAction(actionMatrixRainbowScale);
	matrixPaletteMenu->addAction(actionMatrixCustomScale);
	matrixMenu->insertSeparator();
	matrixMenu->addAction(actionMatrixColumnRow);
    matrixMenu->addAction(actionMatrixXY);
	matrixMenu->insertSeparator();
	matrixMenu->addAction(actionConvertMatrix);

	if (!ws->activeWindow() || !ws->activeWindow()->isA("Matrix"))
		return;

	Matrix* m = (Matrix*)ws->activeWindow();
	actionViewMatrixImage->setChecked(m->viewType() == Matrix::ImageView);
	actionViewMatrix->setChecked(m->viewType() == Matrix::TableView);
	actionMatrixColumnRow->setChecked(m->headerViewType() == Matrix::ColumnRow);
	actionMatrixColumnRow->setEnabled(m->viewType() == Matrix::TableView);
	actionMatrixXY->setChecked(m->headerViewType() == Matrix::XY);
	actionMatrixXY->setEnabled(m->viewType() == Matrix::TableView);

    actionMatrixGrayScale->setChecked(m->colorMapType() == Matrix::GrayScale);
	actionMatrixRainbowScale->setChecked(m->colorMapType() == Matrix::Rainbow);
	actionMatrixCustomScale->setChecked(m->colorMapType() == Matrix::Custom);

    reloadCustomActions();
}

void ApplicationWindow::fileMenuAboutToShow()
{
	fileMenu->clear();
	newMenu->clear();
	exportPlotMenu->clear();

	newMenu = fileMenu->addMenu(tr("&New"));
	newMenu->addAction(actionNewProject);
    newMenu->addAction(actionNewFolder);
	newMenu->addAction(actionNewTable);
	newMenu->addAction(actionNewMatrix);
	newMenu->addAction(actionNewNote);
	newMenu->addAction(actionNewGraph);
	newMenu->addAction(actionNewFunctionPlot);
	newMenu->addAction(actionNewSurfacePlot);
	fileMenu->addAction(actionOpen);

	recentMenuID = fileMenu->insertItem(tr("&Recent Projects"), recent);

	fileMenu->insertSeparator();
	fileMenu->addAction(actionLoadImage);
	fileMenu->addAction(actionImportImage);

	QWidget *w = ws->activeWindow();
	if (w && w->isA("Matrix"))
		fileMenu->addAction(actionExportMatrix);

	fileMenu->insertSeparator();
	fileMenu->addAction(actionSaveProject);
	fileMenu->addAction(actionSaveProjectAs);
	fileMenu->insertSeparator();
	fileMenu->addAction(actionOpenTemplate);
	fileMenu->addAction(actionSaveTemplate);
	fileMenu->insertSeparator();

	if (w && (w->isA("MultiLayer") || w->isA("Graph3D"))){
		exportPlotMenu = fileMenu->addMenu (tr("&Export Graph"));
		exportPlotMenu->addAction(actionExportGraph);
		exportPlotMenu->addAction(actionExportAllGraphs);
	}

	fileMenu->addAction(actionPrint);
	fileMenu->addAction(actionPrintAllPlots);
	fileMenu->insertSeparator();
	fileMenu->addAction(actionShowExportASCIIDialog);
	fileMenu->addAction(actionLoad);
	fileMenu->insertSeparator();
	fileMenu->addAction(actionCloseAllWindows);

	reloadCustomActions();
}

void ApplicationWindow::windowsMenuAboutToShow()
{
	windowsMenu->clear();
	foldersMenu->clear();

	Folder *project_folder = projectFolder();
	FolderListItem *item = project_folder->folderListItem();
	int initial_depth = item->depth();
	int folder_param = 0;
	while (item && item->depth() >= initial_depth){
		Folder *f = item->folder();
		int id;
		if (folder_param < 9)
			id = foldersMenu->insertItem("&" + QString::number(folder_param+1) + " " + f->path(), this, SLOT(foldersMenuActivated(int)));
		else
			id = foldersMenu->insertItem(f->path(), this, SLOT(foldersMenuActivated(int)));

		foldersMenu->setItemParameter(id, folder_param);
		folder_param++;
		foldersMenu->setItemChecked(id, f == current_folder);

		item = (FolderListItem *)item->itemBelow();
	}

	windowsMenu->insertItem(tr("&Folders"), foldersMenu);
	windowsMenu->insertSeparator();

	QList<QWidget*> windows = ws->windowList();
	int n = int(windows.count());
	if (!n ){
		#ifdef SCRIPTING_PYTHON
			windowsMenu->addAction(actionShowScriptWindow);
		#endif
		return;
	}

	windowsMenu->insertItem(tr("&Cascade"), this, SLOT(cascade() ) );
	windowsMenu->insertItem(tr("&Tile"), ws, SLOT(tile() ) );
	windowsMenu->insertSeparator();
	windowsMenu->addAction(actionNextWindow);
	windowsMenu->addAction(actionPrevWindow);
	windowsMenu->insertSeparator();
	windowsMenu->addAction(actionRename);
	windowsMenu->addAction(actionCopyWindow);
	windowsMenu->insertSeparator();
#ifdef SCRIPTING_PYTHON
	windowsMenu->addAction(actionShowScriptWindow);
	windowsMenu->insertSeparator();
#endif

	windowsMenu->addAction(actionResizeActiveWindow);
	windowsMenu->insertItem(tr("&Hide Window"),
			this, SLOT(hideActiveWindow()));
	windowsMenu->insertItem(QPixmap(close_xpm), tr("Close &Window"),
			this, SLOT(closeActiveWindow()), Qt::CTRL+Qt::Key_W );

	if (n>0 && n<10){
		windowsMenu->insertSeparator();
		for (int i = 0; i<n; ++i ){
			int id = windowsMenu->insertItem(windows.at(i)->objectName(),
					this, SLOT( windowsMenuActivated( int ) ) );
			windowsMenu->setItemParameter( id, i );
			windowsMenu->setItemChecked( id, ws->activeWindow() == windows.at(i) );
		}
	} else if (n>=10) {
		windowsMenu->insertSeparator();
		for ( int i = 0; i<9; ++i ){
			int id = windowsMenu->insertItem(windows.at(i)->objectName(),
					this, SLOT( windowsMenuActivated( int ) ) );
			windowsMenu->setItemParameter( id, i );
			windowsMenu->setItemChecked( id, ws->activeWindow() == windows.at(i) );
		}
		windowsMenu->insertSeparator();
		windowsMenu->insertItem(tr("More windows..."),this, SLOT(showMoreWindows()));
	}
    reloadCustomActions();
}

void ApplicationWindow::showMarkerPopupMenu()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	QMenu markerMenu(this);

	if (g->imageMarkerSelected()){
		markerMenu.insertItem(QPixmap(pixelProfile_xpm),tr("&View Pixel Line profile"),this, SLOT(pixelLineProfile()));
		markerMenu.insertItem(tr("&Intensity Matrix"),this, SLOT(intensityTable()));
		markerMenu.insertSeparator();
	}

	markerMenu.insertItem(QPixmap(cut_xpm),tr("&Cut"),this, SLOT(cutSelection()));
	markerMenu.insertItem(QPixmap(copy_xpm), tr("&Copy"),this, SLOT(copySelection()));
	markerMenu.insertItem(QPixmap(erase_xpm), tr("&Delete"),this, SLOT(clearSelection()));
	markerMenu.insertSeparator();
	if (g->arrowMarkerSelected())
		markerMenu.insertItem(tr("&Properties..."),this, SLOT(showLineDialog()));
	else if (g->imageMarkerSelected())
		markerMenu.insertItem(tr("&Properties..."),this, SLOT(showImageDialog()));
	else
		markerMenu.insertItem(tr("&Properties..."),this, SLOT(showTextDialog()));

	markerMenu.exec(QCursor::pos());
}

void ApplicationWindow::showMoreWindows()
{
	if (explorerWindow->isVisible())
		QMessageBox::information(this, "QtiPlot",tr("Please use the project explorer to select a window!"));
	else
		explorerWindow->show();
}

void ApplicationWindow::windowsMenuActivated( int id )
{
	QList<QWidget*> windows = ws->windowList();
	QWidget* w = windows.at( id );
	if ( w ){
		w->showNormal();
		w->setFocus();
		if(hidden(w)){
			hiddenWindows->takeAt(hiddenWindows->indexOf(w));
			setListView(w->objectName(), tr("Normal"));
		}
	}
}

void ApplicationWindow::foldersMenuActivated( int id )
{
	Folder *project_folder = projectFolder();
	FolderListItem *item = project_folder->folderListItem();
	int initial_depth = item->depth();
	int folder_param = 0;
	while (item && item->depth() >= initial_depth){
		Folder *f = item->folder();
		if (folder_param == id){
			changeFolder (f);
			break;
		}

		folder_param++;
		item = (FolderListItem *)item->itemBelow();
	}
}

void ApplicationWindow::newProject()
{
	saveSettings();//the recent projects must be saved

	ApplicationWindow *ed = new ApplicationWindow();
	ed->applyUserSettings();
	ed->restoreApplicationGeometry();
	ed->initWindow();
	ed->savedProject();
	this->close();
}

void ApplicationWindow::savedProject()
{
	actionSaveProject->setEnabled(false);
	saved = true;
}

void ApplicationWindow::modifiedProject()
{
	if (saved == false)
		return;

	actionSaveProject->setEnabled(true);
	saved = false;
}

void ApplicationWindow::modifiedProject(QWidget *w)
{
	modifiedProject();

	actionUndo->setEnabled(true);
	lastModified=w;
}

void ApplicationWindow::timerEvent ( QTimerEvent *e)
{
    #ifdef QTIPLOT_DEMO
        if (e->timerId() == demoCloseTimerId){
            saved = true;
            showDemoVersionMessage();
            qApp->closeAllWindows();
        }
    #endif

	if (e->timerId() == savingTimerId)
		saveProject();
	else
		QWidget::timerEvent(e);
}

void ApplicationWindow::dropEvent( QDropEvent* e )
{
	QStringList fileNames;
	if (Q3UriDrag::decodeLocalFiles(e, fileNames)){
		QList<QByteArray> lst = QImageReader::supportedImageFormats() << "JPG";
		QStringList asciiFiles;

		for(int i = 0; i<(int)fileNames.count(); i++){
			QString fn = fileNames[i];
			QFileInfo fi (fn);
			QString ext = fi.extension().lower();
			QStringList tempList;
			QByteArray temp;
			// convert QList<QByteArray> to QStringList to be able to 'filter'
			foreach(temp,lst)
				tempList.append(QString(temp));
			QStringList l = tempList.filter(ext, Qt::CaseInsensitive);
			if (l.count()>0)
				loadImage(fn);
			else if ( ext == "opj" || ext == "qti")
				open(fn);
			else
				asciiFiles << fn;
		}

		importASCII(asciiFiles, ImportASCIIDialog::NewTables, columnSeparator, ignoredLines,
                    renameColumns, strip_spaces, simplify_spaces, d_ASCII_import_comments,
                    d_import_dec_separators, d_ASCII_import_locale, d_ASCII_comment_string,
                    d_ASCII_import_read_only);
	}
}

void ApplicationWindow::dragEnterEvent( QDragEnterEvent* e )
{
	if (e->source()){
		e->ignore();
		return;
	}

	e->accept(Q3UriDrag::canDecode(e));
}

void ApplicationWindow::closeEvent( QCloseEvent* ce )
{
	if (!saved){
		QString s = tr("Save changes to project: <p><b> %1 </b> ?").arg(projectname);
		switch( QMessageBox::information(this, tr("QtiPlot"), s, tr("Yes"), tr("No"),
					tr("Cancel"), 0, 2 ) )
		{
			case 0:
				if (!saveProject()){
					ce->ignore();
					break;
				}
				saveSettings();//the recent projects must be saved
				ce->accept();
				break;

			case 1:
			default:
				saveSettings();//the recent projects must be saved
				ce->accept();
				break;

			case 2:
				ce->ignore();
				break;
		}
	} else {
		saveSettings();//the recent projects must be saved
		ce->accept();
	}
}

void ApplicationWindow::customEvent(QEvent *e)
{
	if (e->type() == SCRIPTING_CHANGE_EVENT){
		scriptingChangeEvent((ScriptingChangeEvent*)e);
		connect(scriptEnv,SIGNAL(error(const QString&,const QString&,int)),this,SLOT(scriptError(const QString&,const QString&,int)));
	}
}

void ApplicationWindow::deleteSelectedItems()
{
	if (folders->hasFocus() && folders->currentItem() != folders->firstChild())
	{//we never allow the user to delete the project folder item
		deleteFolder();
		return;
	}

	Q3ListViewItem *item;
	QList<Q3ListViewItem *> lst;
	for (item = lv->firstChild(); item; item = item->nextSibling()){
		if (item->isSelected())
			lst.append(item);
	}

	folders->blockSignals(true);
	foreach(item, lst){
		if (item->rtti() == FolderListItem::RTTI){
			Folder *f = ((FolderListItem *)item)->folder();
			if (deleteFolder(f))
				delete item;
		} else
			((WindowListItem *)item)->window()->close();
	}
	folders->blockSignals(false);
}

void ApplicationWindow::showListViewSelectionMenu(const QPoint &p)
{
	QMenu cm(this);
	cm.insertItem(tr("&Show All Windows"), this, SLOT(showSelectedWindows()));
	cm.insertItem(tr("&Hide All Windows"), this, SLOT(hideSelectedWindows()));
	cm.insertSeparator();
	cm.insertItem(tr("&Delete Selection"), this, SLOT(deleteSelectedItems()), Qt::Key_F8);
	cm.exec(p);
}

void ApplicationWindow::showListViewPopupMenu(const QPoint &p)
{
	QMenu cm(this);
	QMenu window(this);

	window.addAction(actionNewTable);
	window.addAction(actionNewMatrix);
	window.addAction(actionNewNote);
	window.addAction(actionNewGraph);
	window.addAction(actionNewFunctionPlot);
	window.addAction(actionNewSurfacePlot);
	cm.insertItem(tr("New &Window"), &window);

	cm.insertItem(QPixmap(newfolder_xpm), tr("New F&older"), this, SLOT(addFolder()), Qt::Key_F7);
	cm.insertSeparator();
	cm.insertItem(tr("Auto &Column Width"), lv, SLOT(adjustColumns()));
	cm.exec(p);
}

void ApplicationWindow::showWindowPopupMenu(Q3ListViewItem *it, const QPoint &p, int)
{
	if (folders->isRenaming())
		return;

	if (!it){
		showListViewPopupMenu(p);
		return;
	}

	Q3ListViewItem *item;
	int selected = 0;
	for (item = lv->firstChild(); item; item = item->nextSibling()){
		if (item->isSelected())
			selected++;

		if (selected>1){
			showListViewSelectionMenu(p);
			return;
		}
	}

	if (it->rtti() == FolderListItem::RTTI){
		current_folder = ((FolderListItem *)it)->folder();
		showFolderPopupMenu(it, p, false);
		return;
	}

	MyWidget *w= ((WindowListItem *)it)->window();
	if (w){
		QMenu cm(this);
		QMenu plots(this);

		cm.addAction(actionActivateWindow);
		cm.addAction(actionMinimizeWindow);
		cm.addAction(actionMaximizeWindow);
		cm.insertSeparator();
		if (!hidden(w))
			cm.addAction(actionHideWindow);
		cm.insertItem(QPixmap(close_xpm), tr("&Delete Window"), w, SLOT(close()), Qt::Key_F8);
		cm.insertSeparator();
		cm.insertItem(tr("&Rename Window"), this, SLOT(renameWindow()), Qt::Key_F2);
		cm.addAction(actionResizeWindow);
		cm.insertSeparator();
		cm.addAction(actionPrintWindow);
		cm.insertSeparator();
		cm.insertItem(tr("&Properties..."), this, SLOT(windowProperties()));

		if (w->inherits("Table")){
			QStringList graphs = dependingPlots(w->objectName());
			if (int(graphs.count())>0){
				cm.insertSeparator();
				for (int i=0;i<int(graphs.count());i++)
					plots.insertItem(graphs[i], window(graphs[i]), SLOT(showMaximized()));

				cm.insertItem(tr("D&epending Graphs"),&plots);
			}
		} else if (w->isA("Matrix")){
			QStringList graphs = depending3DPlots((Matrix*)w);
			if (int(graphs.count())>0){
				cm.insertSeparator();
				for (int i=0;i<int(graphs.count());i++)
					plots.insertItem(graphs[i], window(graphs[i]), SLOT(showMaximized()));

				cm.insertItem(tr("D&epending 3D Graphs"),&plots);
			}
		} else if (w->isA("MultiLayer")) {
			tablesDepend->clear();
			QStringList tbls=multilayerDependencies(w);
			int n = int(tbls.count());
			if (n > 0){
				cm.insertSeparator();
				for (int i=0; i<n; i++)
					tablesDepend->insertItem(tbls[i], i, -1);

				cm.insertItem(tr("D&epends on"), tablesDepend);
			}
		} else if (w->isA("Graph3D")) {
			Graph3D *sp=(Graph3D*)w;
			Matrix *m = sp->matrix();
			QString formula = sp->formula();
			if (!formula.isEmpty()){
				cm.insertSeparator();
				if (formula.contains("_")){
					QStringList tl = formula.split("_", QString::SkipEmptyParts);
					tablesDepend->clear();
					tablesDepend->insertItem(tl[0], 0, -1);
					cm.insertItem(tr("D&epends on"), tablesDepend);
				} else if (m) {
					plots.insertItem(m->objectName(), m, SLOT(showNormal()));
					cm.insertItem(tr("D&epends on"),&plots);
				} else {
					plots.insertItem(formula, w, SLOT(showNormal()));
					cm.insertItem(tr("Function"), &plots);
				}
			}
		}
		cm.exec(p);
	}
}

void ApplicationWindow::showTable(int i)
{
	Table *t = table(tablesDepend->text(i));
	if (!t)
		return;

	updateWindowLists(t);

	t->showMaximized();
	Q3ListViewItem *it=lv->findItem (t->objectName(), 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		it->setText(2,tr("Maximized"));
}

void ApplicationWindow::showTable(const QString& curve)
{
	Table* w=table(curve);
	if (!w)
		return;

	updateWindowLists(w);
	int colIndex = w->colIndex(curve);
	w->setSelectedCol(colIndex);
	w->table()->clearSelection();
	w->table()->selectColumn(colIndex);
	w->showMaximized();
	Q3ListViewItem *it=lv->findItem (w->objectName(), 0, Q3ListView::ExactMatch | Qt::CaseSensitive );
	if (it)
		it->setText(2,tr("Maximized"));
	emit modified();
}

QStringList ApplicationWindow::depending3DPlots(Matrix *m)
{
	QWidgetList *windows = windowsList();
	QStringList plots;
	for (int i=0; i<(int)windows->count(); i++)
	{
		QWidget *w = windows->at(i);
		if (w->isA("Graph3D") && ((Graph3D *)w)->matrix() == m)
			plots << w->objectName();
	}
	delete windows;
	return plots;
}

QStringList ApplicationWindow::dependingPlots(const QString& name)
{
	QWidgetList *windows = windowsList();
	QStringList onPlot, plots;

	for (int i=0; i<(int)windows->count(); i++){
		QWidget *w = windows->at(i);
		if (w->isA("MultiLayer")){
			QWidgetList lst= ((MultiLayer*)w)->graphPtrs();
			foreach(QWidget *widget, lst){
				Graph *g = (Graph *)widget;
				onPlot = g->curvesList();
				onPlot = onPlot.grep (name,TRUE);
				if (int(onPlot.count()) && plots.contains(w->objectName())<=0)
					plots << w->objectName();
			}
		}else if (w->isA("Graph3D")){
			if ((((Graph3D*)w)->formula()).contains(name,TRUE) && plots.contains(w->objectName())<=0)
				plots << w->objectName();
		}
	}
	delete windows;
	return plots;
}

QStringList ApplicationWindow::multilayerDependencies(QWidget *w)
{
	QStringList tables;
	MultiLayer *g=(MultiLayer*)w;
	QWidgetList graphsList = g->graphPtrs();
	for (int i=0; i<graphsList.count(); i++)
	{
		Graph* ag=(Graph*)graphsList.at(i);
		QStringList onPlot=ag->curvesList();
		for (int j=0; j<onPlot.count(); j++)
		{
			QStringList tl = onPlot[j].split("_", QString::SkipEmptyParts);
			if (tables.contains(tl[0])<=0)
				tables << tl[0];
		}
	}
	return tables;
}

void ApplicationWindow::showGraphContextMenu()
{
	QWidget* w = (QWidget*)ws->activeWindow();
	if (!w)
		return;

	if (w->isA("MultiLayer")){
		MultiLayer *plot=(MultiLayer*)w;
		QMenu cm(this);
		QMenu exports(this);
		QMenu copy(this);
		QMenu prints(this);
		Graph* ag = (Graph*)plot->activeGraph();

		if (ag->isPiePlot())
			cm.insertItem(tr("Re&move Pie Curve"),ag, SLOT(removePie()));
		else {
			if (ag->visibleCurves() != ag->curves()){
				cm.addAction(actionShowAllCurves);
				cm.insertSeparator();
			}
			cm.addAction(actionShowCurvesDialog);
			cm.addAction(actionAddFunctionCurve);
			cm.insertItem(tr("Anal&yze"), analysisMenu);
		}

		if (lastCopiedLayer){
			cm.insertSeparator();
			cm.insertItem(QPixmap(paste_xpm), tr("&Paste Layer"),this, SLOT(pasteSelection()));
		} else if (d_text_copy){
            cm.insertSeparator();
            cm.insertItem(QPixmap(paste_xpm),tr("&Paste Text"),plot, SIGNAL(pasteMarker()));
        } else if (d_arrow_copy){
            cm.insertSeparator();
            cm.insertItem(QPixmap(paste_xpm),tr("&Paste Line/Arrow"),plot, SIGNAL(pasteMarker()));
        } else if (d_image_copy){
            cm.insertSeparator();
            cm.insertItem(QPixmap(paste_xpm),tr("&Paste Image"),plot, SIGNAL(pasteMarker()));
		}
		cm.insertSeparator();
		copy.insertItem(tr("&Layer"), this, SLOT(copyActiveLayer()));
		copy.insertItem(tr("&Window"),plot, SLOT(copyAllLayers()));
		cm.insertItem(QPixmap(copy_xpm),tr("&Copy"), &copy);

		exports.insertItem(tr("&Layer"), this, SLOT(exportLayer()));
		exports.insertItem(tr("&Window"), this, SLOT(exportGraph()));
		cm.insertItem(tr("E&xport"),&exports);

		prints.insertItem(tr("&Layer"), plot, SLOT(printActiveLayer()));
		prints.insertItem(tr("&Window"),plot, SLOT(print()));
		cm.insertItem(QPixmap(fileprint_xpm),tr("&Print"),&prints);
		cm.insertSeparator();
		cm.insertItem(tr("P&roperties..."), this, SLOT(showGeneralPlotDialog()));
		cm.insertSeparator();
		cm.insertItem(QPixmap(close_xpm), tr("&Delete Layer"), plot, SLOT(confirmRemoveLayer()));
		cm.exec(QCursor::pos());
	}
}

void ApplicationWindow::showWindowContextMenu()
{
	QWidget* w = (QWidget*)ws->activeWindow();
	if (!w)
		return;

	QMenu cm(this);
	QMenu plot3D(this);
	if (w->isA("MultiLayer")){
		MultiLayer *g = (MultiLayer*)w;
		if (lastCopiedLayer){
			cm.insertItem(QPixmap(paste_xpm), tr("&Paste Layer"), this, SLOT(pasteSelection()));
			cm.insertSeparator();
		}

		cm.addAction(actionAddLayer);
		if (g->layers() != 0)
			cm.addAction(actionDeleteLayer);

		cm.addAction(actionShowLayerDialog);
		cm.insertSeparator();
		cm.addAction(actionRename);
		cm.addAction(actionCopyWindow);
		cm.insertSeparator();
		cm.insertItem(QPixmap(copy_xpm),tr("&Copy Page"), g, SLOT(copyAllLayers()));
		cm.insertItem(tr("E&xport Page"), this, SLOT(exportGraph()));
		cm.addAction(actionPrint);
		cm.insertSeparator();
		cm.addAction(actionCloseWindow);
	} else if (w->isA("Graph3D")){
		Graph3D *g=(Graph3D*)w;
		if (!g->hasData()){
			cm.insertItem(tr("3D &Plot"), &plot3D);
			plot3D.addAction(actionAdd3DData);
			plot3D.insertItem(tr("&Matrix..."), this, SLOT(add3DMatrixPlot()));
			plot3D.addAction(actionEditSurfacePlot);
		} else {
			if (g->table())
				cm.insertItem(tr("Choose &Data Set..."), this, SLOT(change3DData()));
			else if (g->matrix())
				cm.insertItem(tr("Choose &Matrix..."), this, SLOT(change3DMatrix()));
			else if (g->userFunction() || g->parametricSurface())
				cm.addAction(actionEditSurfacePlot);
			cm.insertItem(QPixmap(erase_xpm), tr("C&lear"), g, SLOT(clearData()));
		}

		cm.insertSeparator();
		cm.addAction(actionRename);
		cm.addAction(actionCopyWindow);
		cm.insertSeparator();
		cm.insertItem(tr("&Copy Graph"), g, SLOT(copyImage()));
		cm.insertItem(tr("&Export"), this, SLOT(exportGraph()));
		cm.addAction(actionPrint);
		cm.insertSeparator();
		cm.addAction(actionCloseWindow);
	} else if (w->isA("Matrix")) {
		Matrix *t = (Matrix *)w;
		if (t->viewType() == Matrix::TableView){
            cm.insertItem(QPixmap(cut_xpm),tr("Cu&t"), t, SLOT(cutSelection()));
            cm.insertItem(QPixmap(copy_xpm),tr("&Copy"), t, SLOT(copySelection()));
            cm.insertItem(QPixmap(paste_xpm),tr("&Paste"), t, SLOT(pasteSelection()));
            cm.insertSeparator();
            cm.insertItem(tr("&Insert Row"), t, SLOT(insertRow()));
            cm.insertItem(tr("&Insert Column"), t, SLOT(insertColumn()));
            if (t->numSelectedRows() > 0)
                cm.insertItem(QPixmap(close_xpm), tr("&Delete Rows"), t, SLOT(deleteSelectedRows()));
            else if (t->numSelectedColumns() > 0)
                cm.insertItem(QPixmap(close_xpm), tr("&Delete Columns"), t, SLOT(deleteSelectedColumns()));

            cm.insertItem(QPixmap(erase_xpm),tr("Clea&r"), t, SLOT(clearSelection()));
		} else if (t->viewType() == Matrix::ImageView){
		    cm.addAction(actionImportImage);
            cm.addAction(actionExportMatrix);
            cm.insertSeparator();
            cm.addAction(actionSetMatrixProperties);
            cm.addAction(actionSetMatrixDimensions);
            cm.insertSeparator();
            cm.addAction(actionSetMatrixValues);
            cm.addAction(actionTableRecalculate);
            cm.insertSeparator();
            cm.addAction(actionRotateMatrix);
            cm.addAction(actionRotateMatrixMinus);
            cm.insertSeparator();
            cm.addAction(actionFlipMatrixVertically);
            cm.addAction(actionFlipMatrixHorizontally);
            cm.insertSeparator();
            cm.addAction(actionTransposeMatrix);
            cm.addAction(actionInvertMatrix);
		}
	}
	cm.exec(QCursor::pos());
}

void ApplicationWindow::showWindowTitleBarMenu()
{
	if (!ws->activeWindow())
		return;

	QMenu cm(this);

	if (ws->activeWindow()->inherits("Table")){
		cm.addAction(actionShowExportASCIIDialog);
		cm.insertSeparator();
	}

	if (ws->activeWindow()->isA("Note"))
		cm.addAction(actionSaveNote);
	else
		cm.addAction(actionSaveTemplate);
	cm.addAction(actionPrint);
	cm.insertSeparator();
	cm.addAction(actionRename);
	cm.addAction(actionCopyWindow);
	cm.insertSeparator();
	cm.addAction(actionHideActiveWindow);
	cm.addAction(actionCloseWindow);
	cm.exec(QCursor::pos());
}

void ApplicationWindow::showTableContextMenu(bool selection)
{
	Table *t = (Table*)ws->activeWindow();
	if (!t)
		return;

	QMenu cm(this);
	if (selection){
		if ((int)t->selectedColumns().count() > 0){
			showColMenu(t->firstSelectedColumn());
			return;
		} else if (t->numSelectedRows() == 1) {
			cm.addAction(actionShowColumnValuesDialog);
			cm.insertItem(QPixmap(cut_xpm),tr("Cu&t"), t, SLOT(cutSelection()));
			cm.insertItem(QPixmap(copy_xpm),tr("&Copy"), t, SLOT(copySelection()));
			cm.insertItem(QPixmap(paste_xpm),tr("&Paste"), t, SLOT(pasteSelection()));
			cm.insertSeparator();
			cm.addAction(actionTableRecalculate);
			cm.insertItem(tr("&Insert Row"), t, SLOT(insertRow()));
			cm.insertItem(QPixmap(close_xpm), tr("&Delete Row"), t, SLOT(deleteSelectedRows()));
			cm.insertItem(QPixmap(erase_xpm), tr("Clea&r Row"), t, SLOT(clearSelection()));
			cm.insertSeparator();
			cm.addAction(actionShowRowStatistics);
		} else if (t->numSelectedRows() > 1) {
			cm.addAction(actionShowColumnValuesDialog);
			cm.insertItem(QPixmap(cut_xpm),tr("Cu&t"), t, SLOT(cutSelection()));
			cm.insertItem(QPixmap(copy_xpm),tr("&Copy"), t, SLOT(copySelection()));
			cm.insertItem(QPixmap(paste_xpm),tr("&Paste"), t, SLOT(pasteSelection()));
			cm.insertSeparator();
			cm.addAction(actionTableRecalculate);
			cm.insertItem(QPixmap(close_xpm), tr("&Delete Rows"), t, SLOT(deleteSelectedRows()));
			cm.insertItem(QPixmap(erase_xpm),tr("Clea&r Rows"), t, SLOT(clearSelection()));
			cm.insertSeparator();
			cm.addAction(actionShowRowStatistics);
		} else if (t->numRows() > 0 && t->numCols() > 0){
			cm.addAction(actionShowColumnValuesDialog);
			cm.insertItem(QPixmap(cut_xpm),tr("Cu&t"), t, SLOT(cutSelection()));
			cm.insertItem(QPixmap(copy_xpm),tr("&Copy"), t, SLOT(copySelection()));
			cm.insertItem(QPixmap(paste_xpm),tr("&Paste"), t, SLOT(pasteSelection()));
			cm.insertSeparator();
			cm.addAction(actionTableRecalculate);
			cm.insertItem(QPixmap(erase_xpm),tr("Clea&r"), t, SLOT(clearSelection()));
		}
	} else {
		cm.addAction(actionShowExportASCIIDialog);
		cm.insertSeparator();
		cm.addAction(actionAddColToTable);
		cm.addAction(actionClearTable);
		cm.insertSeparator();
		cm.addAction(actionGoToRow);
	}
	cm.exec(QCursor::pos());
}

void ApplicationWindow::chooseHelpFolder()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose the location of the QtiPlot help folder!"),
			qApp->applicationDirPath());

	if (!dir.isEmpty())
	{
		helpFilePath = dir + "/index.html";

		QFile helpFile(helpFilePath);
		if (!helpFile.exists())
		{
			QMessageBox::critical(this, tr("QtiPlot - index.html File Not Found!"),
					tr("There is no file called <b>index.html</b> in this folder.<br>Please choose another folder!"));
		}
	}
}

void ApplicationWindow::showStandAloneHelp()
{
#ifdef Q_OS_MAC // Mac
	QSettings settings(QSettings::IniFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#else
	QSettings settings(QSettings::NativeFormat,QSettings::UserScope, "ProIndependent", "QtiPlot");
#endif

	settings.beginGroup("/General");
	settings.beginGroup("/Paths");
	QString helpPath = settings.value("/HelpFile", qApp->applicationDirPath()+"/manual/index.html").toString();
	settings.endGroup();
	settings.endGroup();

	QFile helpFile(helpPath);
	if (!helpPath.isEmpty() && !helpFile.exists())
	{
		QMessageBox::critical(0, tr("QtiPlot - Help Files Not Found!"),
				tr("The manual can be downloaded from the following internet address:")+
				"<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
		exit(0);
	}

	QFileInfo fi(helpPath);
	QString profilePath = QString(fi.dirPath(true)+"/qtiplot.adp");
	if (!QFile(profilePath).exists())
	{
		QMessageBox::critical(0, tr("QtiPlot - Help Profile Not Found!"),
				tr("The assistant could not start because the file <b>%1</b> was not found in the help file directory!").arg("qtiplot.adp")+"<br>"+
				tr("This file is provided with the QtiPlot manual which can be downloaded from the following internet address:")+
				"<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
		exit(0);
	}

	QStringList cmdLst = QStringList() << "-profile" << profilePath;
	QAssistantClient *assist = new QAssistantClient( QString(), 0);
	assist->setArguments( cmdLst );
	assist->showPage(helpPath);
	connect(assist, SIGNAL(assistantClosed()), qApp, SLOT(quit()) );
}

void ApplicationWindow::showHelp()
{
	QFile helpFile(helpFilePath);
	if (!helpFile.exists())
	{
		QMessageBox::critical(this,tr("QtiPlot - Help Files Not Found!"),
				tr("Please indicate the location of the help file!")+"<br>"+
				tr("The manual can be downloaded from the following internet address:")+
				"<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
		QString fn = QFileDialog::getOpenFileName(QDir::currentDirPath(), "*.html", this );
		if (!fn.isEmpty())
		{
			QFileInfo fi(fn);
			helpFilePath=fi.absFilePath();
			saveSettings();
		}
	}

	QFileInfo fi(helpFilePath);
	QString profilePath = QString(fi.dirPath(true)+"/qtiplot.adp");
	if (!QFile(profilePath).exists())
	{
		QMessageBox::critical(this,tr("QtiPlot - Help Profile Not Found!"),
				tr("The assistant could not start because the file <b>%1</b> was not found in the help file directory!").arg("qtiplot.adp")+"<br>"+
				tr("This file is provided with the QtiPlot manual which can be downloaded from the following internet address:")+
				"<p><a href = http://soft.proindependent.com/manuals.html>http://soft.proindependent.com/manuals.html</a></p>");
		return;
	}

	QStringList cmdLst = QStringList() << "-profile" << profilePath;
	assistant->setArguments( cmdLst );
	assistant->showPage(helpFilePath);
}

void ApplicationWindow::showPlotWizard()
{
	if (tableWindows.count()>0)
	{
		PlotWizard* pw = new PlotWizard(this, 0);
		pw->setAttribute(Qt::WA_DeleteOnClose);
		connect (pw,SIGNAL(plot(const QStringList&)),this,SLOT(multilayerPlot(const QStringList&)));

		pw->insertTablesList(tableWindows);
		pw->setColumnsList(columnsList(Table::All));
		pw->changeColumnsList(tableWindows[0]);
		pw->exec();
	}
	else
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no tables available in this project.</h4>"
					"<p><h4>Please create a table and try again!</h4>"));
}

void ApplicationWindow::setCurveFullRange()
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	int curveKey = actionCurveFullRange->data().toInt();
	g->setCurveFullRange(g->curveIndex(curveKey));
}

void ApplicationWindow::showCurveRangeDialog()
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	int curveKey = actionEditCurveRange->data().toInt();
	showCurveRangeDialog(g, g->curveIndex(curveKey));
}

CurveRangeDialog* ApplicationWindow::showCurveRangeDialog(Graph *g, int curve)
{
	if (!g)
		return 0;

	CurveRangeDialog* crd = new CurveRangeDialog(this);
	crd->setAttribute(Qt::WA_DeleteOnClose);
	crd->setCurveToModify(g, curve);
	crd->exec();
	return crd;
}

void ApplicationWindow::showFunctionDialog()
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	int curveKey = actionEditFunction->data().toInt();
	showFunctionDialog(g, g->curveIndex(curveKey));
}

void ApplicationWindow::showFunctionDialog(Graph *g, int curve)
{
	if ( !g )
		return;

	FunctionDialog* fd = functionDialog();
	fd->setWindowTitle(tr("QtiPlot - Edit function"));
	fd->setCurveToModify(g, curve);
}

FunctionDialog* ApplicationWindow::functionDialog()
{
	FunctionDialog* fd = new FunctionDialog(this);
	fd->setAttribute(Qt::WA_DeleteOnClose);
	connect (fd,SIGNAL(clearParamFunctionsList()),this,SLOT(clearParamFunctionsList()));
	connect (fd,SIGNAL(clearPolarFunctionsList()),this,SLOT(clearPolarFunctionsList()));

	fd->insertParamFunctionsList(xFunctions, yFunctions);
	fd->insertPolarFunctionsList(rFunctions, thetaFunctions);
	fd->show();
	fd->setActiveWindow();
	return fd;
}

void ApplicationWindow::addFunctionCurve()
{
	QWidget* w = ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	if (((MultiLayer*)w)->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		return;
	}

	Graph* g = ((MultiLayer*)w)->activeGraph();
	if ( g ) {
		FunctionDialog* fd = functionDialog();
		if (fd)
			fd->setGraph(g);
	}
}

void ApplicationWindow::updateFunctionLists(int type, QStringList &formulas)
{
	int maxListSize = 10;
	if (type == 2)
	{
		rFunctions.remove(formulas[0]);
		rFunctions.push_front(formulas[0]);

		thetaFunctions.remove(formulas[1]);
		thetaFunctions.push_front(formulas[1]);

		while ((int)rFunctions.size() > maxListSize)
			rFunctions.pop_back();
		while ((int)thetaFunctions.size() > maxListSize)
			thetaFunctions.pop_back();
	}
	else if (type == 1)
	{
		xFunctions.remove(formulas[0]);
		xFunctions.push_front(formulas[0]);

		yFunctions.remove(formulas[1]);
		yFunctions.push_front(formulas[1]);

		while ((int)xFunctions.size() > maxListSize)
			xFunctions.pop_back();
		while ((int)yFunctions.size() > maxListSize)
			yFunctions.pop_back();
	}
}

MultiLayer * ApplicationWindow::newFunctionPlot(QStringList &formulas, double start, double end, int points, const QString& var, int type)
{
    MultiLayer *ml = newGraph();
    if (ml)
        ml->activeGraph()->addFunction(formulas, start, end, points, var, type);

	updateFunctionLists(type, formulas);
	return ml;
}

void ApplicationWindow::clearLogInfo()
{
	if (!logInfo.isEmpty())
	{
		logInfo="";
		results->setText(logInfo);
		emit modified();
	}
}

void ApplicationWindow::clearParamFunctionsList()
{
	xFunctions.clear();
	yFunctions.clear();
}

void ApplicationWindow::clearPolarFunctionsList()
{
	rFunctions.clear();
	thetaFunctions.clear();
}

void ApplicationWindow::clearSurfaceFunctionsList()
{
	surfaceFunc.clear();
}

void ApplicationWindow::setFramed3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
	{
		((Graph3D*)ws->activeWindow())->setFramed();
		actionShowAxisDialog->setEnabled(TRUE);
	}
}

void ApplicationWindow::setBoxed3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
	{
		((Graph3D*)ws->activeWindow())->setBoxed();
		actionShowAxisDialog->setEnabled(TRUE);
	}
}

void ApplicationWindow::removeAxes3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
	{
		((Graph3D*)ws->activeWindow())->setNoAxes();
		actionShowAxisDialog->setEnabled(false);
	}
}

void ApplicationWindow::removeGrid3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setPolygonStyle();
}

void ApplicationWindow::setHiddenLineGrid3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setHiddenLineStyle();
}

void ApplicationWindow::setPoints3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setDotStyle();
}

void ApplicationWindow::setCones3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setConeStyle();
}

void ApplicationWindow::setCrosses3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setCrossStyle();
}

void ApplicationWindow::setBars3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setBarStyle();
}

void ApplicationWindow::setLineGrid3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setWireframeStyle();
}

void ApplicationWindow::setFilledMesh3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setFilledMeshStyle();
}

void ApplicationWindow::setFloorData3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setFloorData();
}

void ApplicationWindow::setFloorIso3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setFloorIsolines();
}

void ApplicationWindow::setEmptyFloor3DPlot()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setEmptyFloor();
}

void ApplicationWindow::setFrontGrid3DPlot(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setFrontGrid(on);
}

void ApplicationWindow::setBackGrid3DPlot(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setBackGrid(on);
}

void ApplicationWindow::setFloorGrid3DPlot(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setFloorGrid(on);
}

void ApplicationWindow::setCeilGrid3DPlot(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setCeilGrid(on);
}

void ApplicationWindow::setRightGrid3DPlot(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setRightGrid(on);
}

void ApplicationWindow::setLeftGrid3DPlot(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setLeftGrid(on);
}

void ApplicationWindow::pickPlotStyle( QAction* action )
{
	if (!action )
		return;

	if (action == polygon)
		removeGrid3DPlot();
	else if (action == filledmesh)
		setFilledMesh3DPlot();
	else if (action == wireframe)
		setLineGrid3DPlot();
	else if (action == hiddenline)
		setHiddenLineGrid3DPlot();
	else if (action == pointstyle)
		setPoints3DPlot();
	else if (action == conestyle)
		setCones3DPlot();
	else if (action == crossHairStyle)
		setCrosses3DPlot();
	else if (action == barstyle)
		setBars3DPlot();

	emit modified();
}


void ApplicationWindow::pickCoordSystem( QAction* action)
{
	if (!action)
		return;

	if (action == Box || action == Frame)
	{
		if (action == Box)
			setBoxed3DPlot();
		if (action == Frame)
			setFramed3DPlot();
		grids->setEnabled(true);
	}
	else if (action == None)
	{
		removeAxes3DPlot();
		grids->setEnabled(false);
	}

	emit modified();
}

void ApplicationWindow::pickFloorStyle( QAction* action )
{
	if (!action)
		return;

	if (action == floordata)
		setFloorData3DPlot();
	else if (action == flooriso)
		setFloorIso3DPlot();
	else
		setEmptyFloor3DPlot();

	emit modified();
}

void ApplicationWindow::custom3DActions(QWidget *w)
{
	if (w && w->isA("Graph3D"))
	{
		Graph3D* plot = (Graph3D*)w;
		actionAnimate->setOn(plot->isAnimated());
		actionPerspective->setOn(!plot->isOrthogonal());
		switch(plot->plotStyle())
		{
			case FILLEDMESH:
				wireframe->setChecked( false );
				hiddenline->setChecked( false );
				polygon->setChecked( false );
				filledmesh->setChecked( true );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			case FILLED:
				wireframe->setChecked( false );
				hiddenline->setChecked( false );
				polygon->setChecked( true );
				filledmesh->setChecked( false );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			case Qwt3D::USER:
				wireframe->setChecked( false );
				hiddenline->setChecked( false );
				polygon->setChecked( false );
				filledmesh->setChecked( false );

				if (plot->pointType() == Graph3D::VerticalBars)
				{
					pointstyle->setChecked( false );
					conestyle->setChecked( false );
					crossHairStyle->setChecked( false );
					barstyle->setChecked( true );
				}
				else if (plot->pointType() == Graph3D::Dots)
				{
					pointstyle->setChecked( true );
					barstyle->setChecked( false );
					conestyle->setChecked( false );
					crossHairStyle->setChecked( false );
				}
				else if (plot->pointType() == Graph3D::HairCross)
				{
					pointstyle->setChecked( false );
					barstyle->setChecked( false );
					conestyle->setChecked( false );
					crossHairStyle->setChecked( true );
				}
				else if (plot->pointType() == Graph3D::Cones)
				{
					pointstyle->setChecked( false );
					barstyle->setChecked( false );
					conestyle->setChecked( true );
					crossHairStyle->setChecked( false );
				}
				break;

			case WIREFRAME:
				wireframe->setChecked( true );
				hiddenline->setChecked( false );
				polygon->setChecked( false );
				filledmesh->setChecked( false );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			case HIDDENLINE:
				wireframe->setChecked( false );
				hiddenline->setChecked( true );
				polygon->setChecked( false );
				filledmesh->setChecked( false );
				pointstyle->setChecked( false );
				barstyle->setChecked( false );
				conestyle->setChecked( false );
				crossHairStyle->setChecked( false );
				break;

			default:
				break;
		}

		switch(plot->coordStyle())
		{
			case Qwt3D::NOCOORD:
				None->setChecked( true );
				Box->setChecked( false );
				Frame->setChecked( false );
				break;

			case Qwt3D::BOX:
				None->setChecked( false );
				Box->setChecked( true );
				Frame->setChecked( false );
				break;

			case Qwt3D::FRAME:
				None->setChecked(false );
				Box->setChecked( false );
				Frame->setChecked(true );
				break;
		}

		switch(plot->floorStyle())
		{
			case NOFLOOR:
				floornone->setChecked( true );
				flooriso->setChecked( false );
				floordata->setChecked( false );
				break;

			case FLOORISO:
				floornone->setChecked( false );
				flooriso->setChecked( true );
				floordata->setChecked( false );
				break;

			case FLOORDATA:
				floornone->setChecked(false );
				flooriso->setChecked( false );
				floordata->setChecked(true );
				break;
		}
		custom3DGrids(plot->grids());
	}
}

void ApplicationWindow::custom3DGrids(int grids)
{
	if (Qwt3D::BACK & grids)
		back->setChecked(true);
	else
		back->setChecked(false);

	if (Qwt3D::FRONT & grids)
		front->setChecked(true);
	else
		front->setChecked(false);

	if (Qwt3D::CEIL & grids)
		ceil->setChecked(true);
	else
		ceil->setChecked(false);

	if (Qwt3D::FLOOR & grids)
		floor->setChecked(true);
	else
		floor->setChecked(false);

	if (Qwt3D::RIGHT & grids)
		right->setChecked(true);
	else
		right->setChecked(false);

	if (Qwt3D::LEFT & grids)
		left->setChecked(true);
	else
		left->setChecked(false);
}

void ApplicationWindow::initPlot3DToolBar()
{
	plot3DTools = new QToolBar( tr( "3D Surface" ), this );
	plot3DTools->setObjectName("plot3DTools"); // this is needed for QMainWindow::restoreState()
	plot3DTools->setIconSize( QSize(20,20) );
	addToolBarBreak( Qt::TopToolBarArea );
	addToolBar( Qt::TopToolBarArea, plot3DTools );

	coord = new QActionGroup( this );
	Box = new QAction( coord );
	Box->setIcon(QIcon(QPixmap(box_xpm)));
	Box->setCheckable(true);

	Frame = new QAction( coord );
	Frame->setIcon(QIcon(QPixmap(free_axes_xpm)) );
	Frame->setCheckable(true);

	None = new QAction( coord );
	None->setIcon(QIcon(QPixmap(no_axes_xpm)) );
	None->setCheckable(true);

	plot3DTools->addAction(Frame);
	plot3DTools->addAction(Box);
	plot3DTools->addAction(None);
	Box->setChecked( true );

	plot3DTools->addSeparator();

	// grid actions
	grids = new QActionGroup( this );
	grids->setEnabled( true );
	grids->setExclusive( false );
	front = new QAction( grids );
	front->setCheckable( true );
	front->setIcon(QIcon(QPixmap(frontGrid_xpm)) );
	back = new QAction( grids );
	back->setCheckable( true );
	back->setIcon(QIcon(QPixmap(backGrid_xpm)));
	right = new QAction( grids );
	right->setCheckable( true );
	right->setIcon(QIcon(QPixmap(leftGrid_xpm)) );
	left = new QAction( grids );
	left->setCheckable( true );
	left->setIcon(QIcon(QPixmap(rightGrid_xpm)));
	ceil = new QAction( grids );
	ceil->setCheckable( true );
	ceil->setIcon(QIcon(QPixmap(ceilGrid_xpm)) );
	floor = new QAction( grids );
	floor->setCheckable( true );
	floor->setIcon(QIcon(QPixmap(floorGrid_xpm)) );

	plot3DTools->addAction(front);
	plot3DTools->addAction(back);
	plot3DTools->addAction(right);
	plot3DTools->addAction(left);
	plot3DTools->addAction(ceil);
	plot3DTools->addAction(floor);

	plot3DTools->addSeparator();

	actionPerspective = new QAction( this );
	actionPerspective->setToggleAction( TRUE );
	actionPerspective->setIconSet(QPixmap(perspective_xpm) );
	actionPerspective->addTo( plot3DTools );
	actionPerspective->setOn(!orthogonal3DPlots);
	connect(actionPerspective, SIGNAL(toggled(bool)), this, SLOT(togglePerspective(bool)));

	actionResetRotation = new QAction( this );
	actionResetRotation->setToggleAction( false );
	actionResetRotation->setIconSet(QPixmap(reset_rotation_xpm));
	actionResetRotation->addTo( plot3DTools );
	connect(actionResetRotation, SIGNAL(activated()), this, SLOT(resetRotation()));

	actionFitFrame = new QAction( this );
	actionFitFrame->setToggleAction( false );
	actionFitFrame->setIconSet(QPixmap(fit_frame_xpm));
	actionFitFrame->addTo( plot3DTools );
	connect(actionFitFrame, SIGNAL(activated()), this, SLOT(fitFrameToLayer()));

	plot3DTools->addSeparator();

	//plot style actions
	plotstyle = new QActionGroup( this );
	wireframe = new QAction( plotstyle );
	wireframe->setCheckable( true );
	wireframe->setEnabled( true );
	wireframe->setIcon(QIcon(QPixmap(lineMesh_xpm)) );
	hiddenline = new QAction( plotstyle );
	hiddenline->setCheckable( true );
	hiddenline->setEnabled( true );
	hiddenline->setIcon(QIcon(QPixmap(grid_only_xpm)) );
	polygon = new QAction( plotstyle );
	polygon->setCheckable( true );
	polygon->setEnabled( true );
	polygon->setIcon(QIcon(QPixmap(no_grid_xpm)));
	filledmesh = new QAction( plotstyle );
	filledmesh->setCheckable( true );
	filledmesh->setIcon(QIcon(QPixmap(grid_poly_xpm)) );
	pointstyle = new QAction( plotstyle );
	pointstyle->setCheckable( true );
	pointstyle->setIcon(QIcon(QPixmap(pointsMesh_xpm)) );

	conestyle = new QAction( plotstyle );
	conestyle->setCheckable( true );
	conestyle->setIcon(QIcon(QPixmap(cones_xpm)) );

	crossHairStyle = new QAction( plotstyle );
	crossHairStyle->setCheckable( true );
	crossHairStyle->setIcon(QIcon(QPixmap(crosses_xpm)) );

	barstyle = new QAction( plotstyle );
	barstyle->setCheckable( true );
	barstyle->setIcon(QIcon(QPixmap(plot_bars_xpm)) );

	plot3DTools->addAction(barstyle);
	plot3DTools->addAction(pointstyle);

	plot3DTools->addAction(conestyle);
	plot3DTools->addAction(crossHairStyle);
	plot3DTools->addSeparator();

	plot3DTools->addAction(wireframe);
	plot3DTools->addAction(hiddenline);
	plot3DTools->addAction(polygon);
	plot3DTools->addAction(filledmesh);
	filledmesh->setChecked( true );

	plot3DTools->addSeparator();

	//floor actions
	floorstyle = new QActionGroup( this );
	floordata = new QAction( floorstyle );
	floordata->setCheckable( true );
	floordata->setIcon(QIcon(QPixmap(floor_xpm)) );
	flooriso = new QAction( floorstyle );
	flooriso->setCheckable( true );
	flooriso->setIcon(QIcon(QPixmap(isolines_xpm)) );
	floornone = new QAction( floorstyle );
	floornone->setCheckable( true );
	floornone->setIcon(QIcon(QPixmap(no_floor_xpm)));

	plot3DTools->addAction(floordata);
	plot3DTools->addAction(flooriso);
	plot3DTools->addAction(floornone);
	floornone->setChecked( true );

	plot3DTools->addSeparator();

	actionAnimate = new QAction( this );
	actionAnimate->setToggleAction( true );
	actionAnimate->setIconSet(QPixmap(movie_xpm));
	plot3DTools->addAction(actionAnimate);

	plot3DTools->hide();

	connect(actionAnimate, SIGNAL(toggled(bool)), this, SLOT(toggle3DAnimation(bool)));
	connect( coord, SIGNAL( triggered( QAction* ) ), this, SLOT( pickCoordSystem( QAction* ) ) );
	connect( floorstyle, SIGNAL( triggered( QAction* ) ), this, SLOT( pickFloorStyle( QAction* ) ) );
	connect( plotstyle, SIGNAL( triggered( QAction* ) ), this, SLOT( pickPlotStyle( QAction* ) ) );

	connect( left, SIGNAL( triggered( bool ) ), this, SLOT( setLeftGrid3DPlot(bool) ));
	connect( right, SIGNAL( triggered( bool ) ), this, SLOT( setRightGrid3DPlot( bool ) ) );
	connect( ceil, SIGNAL( triggered( bool ) ), this, SLOT( setCeilGrid3DPlot( bool ) ) );
	connect( floor, SIGNAL( triggered( bool ) ), this, SLOT(setFloorGrid3DPlot( bool ) ) );
	connect( back, SIGNAL( triggered( bool ) ), this, SLOT(setBackGrid3DPlot( bool ) ) );
	connect( front, SIGNAL( triggered( bool ) ), this, SLOT( setFrontGrid3DPlot( bool ) ) );
}

void ApplicationWindow::pixelLineProfile()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer *)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	bool ok;
	int res = QInputDialog::getInteger(
			tr("QtiPlot - Set the number of pixels to average"), tr("Number of averaged pixels"),1, 1, 2000, 2,
			&ok, this );
	if ( !ok )
		return;

	LineProfileTool *lpt = new LineProfileTool(g, this, res);
	g->setActiveTool(lpt);
}

void ApplicationWindow::intensityTable()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer *)ws->activeWindow())->activeGraph();
	if (g){
		ImageMarker *im = (ImageMarker *) g->selectedMarkerPtr();
        if (im){
            QString fn = im->fileName();
            if (!fn.isEmpty())
                importImage(fn);
        }
	}
}

void ApplicationWindow::autoArrangeLayers()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer *)ws->activeWindow();
	plot->setMargins(5, 5, 5, 5);
	plot->setSpacing(5, 5);
	plot->arrangeLayers(true, false);
}

void ApplicationWindow::addLayer()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	MultiLayer* plot = (MultiLayer *)ws->activeWindow();
	switch(QMessageBox::information(this,
				tr("QtiPlot - Guess best origin for the new layer?"),
				tr("Do you want QtiPlot to guess the best position for the new layer?\n Warning: this will rearrange existing layers!"),
				tr("&Guess"), tr("&Top-left corner"), tr("&Cancel"), 0, 2 ) )
	{
		case 0:
			{
				setPreferences(plot->addLayer());
				plot->arrangeLayers(true, false);
			}
			break;

		case 1:
			setPreferences(plot->addLayer(0, 0, plot->size().width(), plot->size().height()));
			break;

		case 2:
			return;
			break;
	}
}

void ApplicationWindow::deleteLayer()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	((MultiLayer *)ws->activeWindow())->confirmRemoveLayer();
}

Note* ApplicationWindow::openNote(ApplicationWindow* app, const QStringList &flist)
{
	QStringList lst=flist[0].split("\t", QString::SkipEmptyParts);
	QString caption = lst[0];
	Note* w = app->newNote(caption);
	if (lst.count() == 2)
	{
		app->setListViewDate(caption, lst[1]);
		w->setBirthDate(lst[1]);
	}
	restoreWindowGeometry(app, w, flist[1]);

	lst=flist[2].split("\t");
	w->setWindowLabel(lst[1]);
	w->setCaptionPolicy((MyWidget::CaptionPolicy)lst[2].toInt());
	app->setListViewLabel(w->objectName(), lst[1]);
	return w;
}

Matrix* ApplicationWindow::openMatrix(ApplicationWindow* app, const QStringList &flist)
{
	QStringList::const_iterator line = flist.begin();

	QStringList list=(*line).split("\t");
	QString caption=list[0];
	int rows = list[1].toInt();
	int cols = list[2].toInt();

	Matrix* w = app->newMatrix(caption, rows, cols);
	app->setListViewDate(caption,list[3]);
	w->setBirthDate(list[3]);

	for (line++; line!=flist.end(); line++)
	{
		QStringList fields = (*line).split("\t");
		if (fields[0] == "geometry") {
			restoreWindowGeometry(app, w, *line);
		} else if (fields[0] == "ColWidth") {
			w->setColumnsWidth(fields[1].toInt());
		} else if (fields[0] == "Formula") {
			w->setFormula(fields[1]);
		} else if (fields[0] == "<formula>") {
			QString formula;
			for (line++; line!=flist.end() && *line != "</formula>"; line++)
				formula += *line + "\n";
			formula.truncate(formula.length()-1);
			w->setFormula(formula);
		} else if (fields[0] == "TextFormat") {
			if (fields[1] == "f")
				w->setTextFormat('f', fields[2].toInt());
			else
				w->setTextFormat('e', fields[2].toInt());
		} else if (fields[0] == "WindowLabel") { // d_file_version > 71
			w->setWindowLabel(fields[1]);
			w->setCaptionPolicy((MyWidget::CaptionPolicy)fields[2].toInt());
			app->setListViewLabel(w->objectName(), fields[1]);
		} else if (fields[0] == "Coordinates") { // d_file_version > 81
			w->setCoordinates(fields[1].toDouble(), fields[2].toDouble(), fields[3].toDouble(), fields[4].toDouble());
		} else if (fields[0] == "ViewType") { // d_file_version > 90
			w->setViewType((Matrix::ViewType)fields[1].toInt());
		} else if (fields[0] == "HeaderViewType") { // d_file_version > 90
			w->setHeaderViewType((Matrix::HeaderViewType)fields[1].toInt());
		} else if (fields[0] == "ColorPolicy"){// d_file_version > 90
			w->setColorMapType((Matrix::ColorMapType)fields[1].toInt());
		} else if (fields[0] == "<ColorMap>"){// d_file_version > 90
			QStringList lst;
			while ( *line != "</ColorMap>" ){
				line++;
				lst << *line;
			}
			lst.pop_back();
			w->setColorMap(lst);
		} else // <data> or values
			break;
	}
	if (*line == "<data>") line++;

	//read and set table values
	for (; line!=flist.end() && *line != "</data>"; line++){
		QStringList fields = (*line).split("\t");
		int row = fields[0].toInt();
		for (int col=0; col<cols; col++){
		    QString cell = fields[col+1];
		    if (cell.isEmpty())
                continue;

		    if (d_file_version < 90)
                w->setCell(row, col, QLocale::c().toDouble(cell));
            else if (d_file_version == 90)
                w->setText(row, col, cell);
			else
				w->setCell(row, col, cell.toDouble());
		}
		qApp->processEvents(QEventLoop::ExcludeUserInput);
	}
	w->resetView();
	return w;
}

Table* ApplicationWindow::openTable(ApplicationWindow* app, const QStringList &flist)
{
	QStringList::const_iterator line = flist.begin();

	QStringList list=(*line).split("\t");
	QString caption=list[0];
	int rows = list[1].toInt();
	int cols = list[2].toInt();

	Table* w = app->newTable(caption, rows, cols);
	app->setListViewDate(caption, list[3]);
	w->setBirthDate(list[3]);

	for (line++; line!=flist.end(); line++)
	{
		QStringList fields = (*line).split("\t");
		if (fields[0] == "geometry" || fields[0] == "tgeometry") {
			restoreWindowGeometry(app, w, *line);
		} else if (fields[0] == "header") {
			fields.pop_front();
			if (d_file_version >= 78)
				w->loadHeader(fields);
			else
			{
				w->setColPlotDesignation(list[4].toInt(), Table::X);
				w->setColPlotDesignation(list[6].toInt(), Table::Y);
				w->setHeader(fields);
			}
		} else if (fields[0] == "ColWidth") {
			fields.pop_front();
			w->setColWidths(fields);
		} else if (fields[0] == "com") { // legacy code
			w->setCommands(*line);
		} else if (fields[0] == "<com>") {
			for (line++; line!=flist.end() && *line != "</com>"; line++)
			{
				int col = (*line).mid(9,(*line).length()-11).toInt();
				QString formula;
				for (line++; line!=flist.end() && *line != "</col>"; line++)
					formula += *line + "\n";
				formula.truncate(formula.length()-1);
				w->setCommand(col,formula);
			}
		} else if (fields[0] == "ColType") { // d_file_version > 65
			fields.pop_front();
			w->setColumnTypes(fields);
		} else if (fields[0] == "Comments") { // d_file_version > 71
			fields.pop_front();
			w->setColComments(fields);
			w->setHeaderColType();
		} else if (fields[0] == "WindowLabel") { // d_file_version > 71
			w->setWindowLabel(fields[1]);
			w->setCaptionPolicy((MyWidget::CaptionPolicy)fields[2].toInt());
			app->setListViewLabel(w->objectName(), fields[1]);
		} else if (fields[0] == "ReadOnlyColumn") { // d_file_version > 91
			fields.pop_front();
			for (int i=0; i < w->numCols(); i++)
				w->setReadOnlyColumn(i, fields[i] == "1");
		} else // <data> or values
			break;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	w->table()->blockSignals(true);
	for (line++; line!=flist.end() && *line != "</data>"; line++)
	{//read and set table values
		QStringList fields = (*line).split("\t");
		int row = fields[0].toInt();
		for (int col=0; col<cols; col++){
		    if (fields.count() >= col+2){
		        QString cell = fields[col+1];
		        if (cell.isEmpty())
                    continue;

				if (w->columnType(col) == Table::Numeric){
		        	if (d_file_version < 90)
                    	w->setCell(row, col, QLocale::c().toDouble(cell.replace(",", ".")));
					else if (d_file_version == 90)
						w->setText(row, col, cell);
					else if (d_file_version >= 91)
						w->setCell(row, col, cell.toDouble());
		        } else
                    w->setText(row, col, cell);
		    }
		}
		QApplication::processEvents(QEventLoop::ExcludeUserInput);
	}
    QApplication::restoreOverrideCursor();

	w->setSpecifications(w->saveToString("geometry\n"));
	w->table()->blockSignals(false);
	return w;
}

TableStatistics* ApplicationWindow::openTableStatistics(const QStringList &flist)
{
	QStringList::const_iterator line = flist.begin();

	QStringList list=(*line++).split("\t");
	QString caption=list[0];

	QList<int> targets;
	for (int i=1; i <= (*line).count('\t'); i++)
		targets << (*line).section('\t',i,i).toInt();

	TableStatistics* w = newTableStatistics(table(list[1]),
			list[2]=="row" ? TableStatistics::row : TableStatistics::column, targets, caption);

	setListViewDate(caption,list[3]);
	w->setBirthDate(list[3]);

	for (line++; line!=flist.end(); line++)
	{
		QStringList fields = (*line).split("\t");
		if (fields[0] == "geometry"){
			restoreWindowGeometry(this, w, *line);}
		else if (fields[0] == "header") {
			fields.pop_front();
			if (d_file_version >= 78)
				w->loadHeader(fields);
			else
			{
				w->setColPlotDesignation(list[4].toInt(), Table::X);
				w->setColPlotDesignation(list[6].toInt(), Table::Y);
				w->setHeader(fields);
			}
		} else if (fields[0] == "ColWidth") {
			fields.pop_front();
			w->setColWidths(fields);
		} else if (fields[0] == "com") { // legacy code
			w->setCommands(*line);
		} else if (fields[0] == "<com>") {
			for (line++; line!=flist.end() && *line != "</com>"; line++)
			{
				int col = (*line).mid(9,(*line).length()-11).toInt();
				QString formula;
				for (line++; line!=flist.end() && *line != "</col>"; line++)
					formula += *line + "\n";
				formula.truncate(formula.length()-1);
				w->setCommand(col,formula);
			}
		} else if (fields[0] == "ColType") { // d_file_version > 65
			fields.pop_front();
			w->setColumnTypes(fields);
		} else if (fields[0] == "Comments") { // d_file_version > 71
			fields.pop_front();
			w->setColComments(fields);
		} else if (fields[0] == "WindowLabel") { // d_file_version > 71
			w->setWindowLabel(fields[1]);
			w->setCaptionPolicy((MyWidget::CaptionPolicy)fields[2].toInt());
			setListViewLabel(w->objectName(), fields[1]);
		}
	}
	return w;
}

Graph* ApplicationWindow::openGraph(ApplicationWindow* app, MultiLayer *plot,
		const QStringList &list)
{
	Graph* ag = 0;
	int curveID = 0;
	for (int j=0;j<(int)list.count()-1;j++){
		QString s=list[j];
		if (s.contains ("ggeometry")){
			QStringList fList=s.split("\t");
			ag =(Graph*)plot->addLayer(fList[1].toInt(), fList[2].toInt(),
					fList[3].toInt(), fList[4].toInt());
            ag->blockSignals(true);
			ag->enableAutoscaling(autoscale2DPlots);
		}
		else if (s.left(10) == "Background"){
			QStringList fList = s.split("\t");
			QColor c = QColor(fList[1]);
			if (fList.count() == 3)
				c.setAlpha(fList[2].toInt());
			ag->setBackgroundColor(c);
		}
		else if (s.contains ("Margin")){
			QStringList fList=s.split("\t");
			ag->plotWidget()->setMargin(fList[1].toInt());
		}
		else if (s.contains ("Border")){
			QStringList fList=s.split("\t");
			ag->setFrame(fList[1].toInt(), QColor(fList[2]));
		}
		else if (s.contains ("EnabledAxes")){
			QStringList fList=s.split("\t");
			fList.pop_front();
			for (int i=0; i<(int)fList.count(); i++)
				ag->enableAxis(i, fList[i].toInt());
		}
		else if (s.contains ("AxesBaseline")){
			QStringList fList = s.split("\t", QString::SkipEmptyParts);
			fList.pop_front();
			for (int i=0; i<(int)fList.count(); i++)
				ag->setAxisMargin(i, fList[i].toInt());
		}
		else if (s.contains ("EnabledTicks"))
		{//version < 0.8.6
			QStringList fList=s.split("\t");
			fList.pop_front();
			fList.replaceInStrings("-1", "3");
			ag->setMajorTicksType(fList);
			ag->setMinorTicksType(fList);
		}
		else if (s.contains ("MajorTicks"))
		{//version >= 0.8.6
			QStringList fList=s.split("\t");
			fList.pop_front();
			ag->setMajorTicksType(fList);
		}
		else if (s.contains ("MinorTicks"))
		{//version >= 0.8.6
			QStringList fList=s.split("\t");
			fList.pop_front();
			ag->setMinorTicksType(fList);
		}
		else if (s.contains ("TicksLength")){
			QStringList fList=s.split("\t");
			ag->setTicksLength(fList[1].toInt(), fList[2].toInt());
		}
		else if (s.contains ("EnabledTickLabels")){
			QStringList fList=s.split("\t");
			fList.pop_front();
			for (int i=0; i<int(fList.count()); i++)
				ag->enableAxisLabels(i, fList[i].toInt());
		}
		else if (s.contains ("AxesColors")){
			QStringList fList = s.split("\t");
			fList.pop_front();
			for (int i=0; i<int(fList.count()); i++)
				ag->setAxisColor(i, QColor(fList[i]));
		}
		else if (s.contains ("AxesNumberColors")){
			QStringList fList=QStringList::split ("\t",s,TRUE);
			fList.pop_front();
			for (int i=0; i<int(fList.count()); i++)
				ag->setAxisLabelsColor(i, QColor(fList[i]));
		}
		else if (s.left(5)=="grid\t"){
			ag->plotWidget()->grid()->load(s.split("\t"));
		}
		else if (s.startsWith ("<Antialiasing>") && s.endsWith ("</Antialiasing>")){
			bool antialiasing = s.remove("<Antialiasing>").remove("</Antialiasing>").toInt();
			ag->setAntialiasing(antialiasing, false);
		}
		else if (s.contains ("PieCurve")){
			QStringList curve=s.split("\t");
			if (!app->renamedTables.isEmpty()){
				QString caption = (curve[1]).left((curve[1]).find("_",0));
				if (app->renamedTables.contains(caption))
				{//modify the name of the curve according to the new table name
					int index = app->renamedTables.findIndex(caption);
					QString newCaption = app->renamedTables[++index];
					curve.replaceInStrings(caption+"_", newCaption+"_");
				}
			}
			QPen pen = QPen(QColor(curve[3]),curve[2].toInt(),Graph::getPenStyle(curve[4]));

			Table *table = app->table(curve[1]);
			if (table){
				int startRow = 0;
				int endRow = table->numRows() - 1;
				int first_color = curve[7].toInt();
				bool visible = true;
				if (d_file_version >= 90)
				{
					startRow = curve[8].toInt();
					endRow = curve[9].toInt();
					visible = ((curve.last() == "1") ? true : false);
				}

				if (d_file_version <= 89)
					first_color = convertOldToNewColorIndex(first_color);

				ag->plotPie(table, curve[1], pen, curve[5].toInt(),
					curve[6].toInt(), first_color, startRow, endRow, visible);
			}
		}else if (s.left(6)=="curve\t"){
			QStringList curve = s.split("\t", QString::SkipEmptyParts);
			if (!app->renamedTables.isEmpty()){
				QString caption = (curve[2]).left((curve[2]).find("_",0));
				if (app->renamedTables.contains(caption))
				{//modify the name of the curve according to the new table name
					int index = app->renamedTables.findIndex (caption);
					QString newCaption = app->renamedTables[++index];
					curve.replaceInStrings(caption+"_", newCaption+"_");
				}
			}

			CurveLayout cl;
			cl.connectType=curve[4].toInt();
			cl.lCol=curve[5].toInt();
			if (d_file_version <= 89)
				cl.lCol = convertOldToNewColorIndex(cl.lCol);
			cl.lStyle=curve[6].toInt();
			cl.lWidth=curve[7].toInt();
			cl.sSize=curve[8].toInt();
			if (d_file_version <= 78)
				cl.sType=Graph::obsoleteSymbolStyle(curve[9].toInt());
			else
				cl.sType=curve[9].toInt();

			cl.symCol=curve[10].toInt();
			if (d_file_version <= 89)
				cl.symCol = convertOldToNewColorIndex(cl.symCol);
			cl.fillCol=curve[11].toInt();
			if (d_file_version <= 89)
				cl.fillCol = convertOldToNewColorIndex(cl.fillCol);
			cl.filledArea=curve[12].toInt();
			cl.aCol=curve[13].toInt();
			if (d_file_version <= 89)
				cl.aCol = convertOldToNewColorIndex(cl.aCol);
			cl.aStyle=curve[14].toInt();
			if(curve.count() < 16)
				cl.penWidth = cl.lWidth;
			else if ((d_file_version >= 79) && (curve[3].toInt() == Graph::Box))
				cl.penWidth = curve[15].toInt();
			else if ((d_file_version >= 78) && (curve[3].toInt() <= Graph::LineSymbols))
				cl.penWidth = curve[15].toInt();
			else
				cl.penWidth = cl.lWidth;

            int plotType = curve[3].toInt();
			Table *w = app->table(curve[2]);
			if (w){
				if(plotType == Graph::VectXYXY || plotType == Graph::VectXYAM){
					QStringList colsList;
					colsList<<curve[2]; colsList<<curve[20]; colsList<<curve[21];
					if (d_file_version < 72)
						colsList.prepend(w->colName(curve[1].toInt()));
					else
                        colsList.prepend(curve[1]);

					int startRow = 0;
					int endRow = -1;
					if (d_file_version >= 90){
						startRow = curve[curve.count()-3].toInt();
						endRow = curve[curve.count()-2].toInt();
					}

					ag->plotVectorCurve(w, colsList, plotType, startRow, endRow);

					if (d_file_version <= 77){
						int temp_index = convertOldToNewColorIndex(curve[15].toInt());
						ag->updateVectorsLayout(curveID, ColorBox::color(temp_index), curve[16].toInt(), curve[17].toInt(),
								curve[18].toInt(), curve[19].toInt(), 0, curve[20], curve[21]);
					} else {
						if(plotType == Graph::VectXYXY)
							ag->updateVectorsLayout(curveID, curve[15], curve[16].toInt(),
								curve[17].toInt(), curve[18].toInt(), curve[19].toInt(), 0);
						else
							ag->updateVectorsLayout(curveID, curve[15], curve[16].toInt(), curve[17].toInt(),
									curve[18].toInt(), curve[19].toInt(), curve[22].toInt());
					}
				} else if(plotType == Graph::Box)
					ag->openBoxDiagram(w, curve, d_file_version);
				else {
					if (d_file_version < 72)
						ag->insertCurve(w, curve[1].toInt(), curve[2], plotType);
					else if (d_file_version < 90)
						ag->insertCurve(w, curve[1], curve[2], plotType);
					else{
						int startRow = curve[curve.count()-3].toInt();
						int endRow = curve[curve.count()-2].toInt();
						ag->insertCurve(w, curve[1], curve[2], plotType, startRow, endRow);
					}
				}

				if(plotType == Graph::Histogram){
				    QwtHistogram *h = (QwtHistogram *)ag->curve(curveID);
					if (d_file_version <= 76)
                        h->setBinning(curve[16].toInt(),curve[17].toDouble(),curve[18].toDouble(),curve[19].toDouble());
					else
						h->setBinning(curve[17].toInt(),curve[18].toDouble(),curve[19].toDouble(),curve[20].toDouble());
                    h->loadData();
				}

				if(plotType == Graph::VerticalBars || plotType == Graph::HorizontalBars ||
						plotType == Graph::Histogram){
					if (d_file_version <= 76)
						ag->setBarsGap(curveID, curve[15].toInt(), 0);
					else
						ag->setBarsGap(curveID, curve[15].toInt(), curve[16].toInt());
				}
				ag->updateCurveLayout(curveID, &cl);
				if (d_file_version >= 88){
					QwtPlotCurve *c = ag->curve(curveID);
					if (c && c->rtti() == QwtPlotItem::Rtti_PlotCurve){
						if (d_file_version < 90)
							c->setAxis(curve[curve.count()-2].toInt(), curve[curve.count()-1].toInt());
						else {
							c->setAxis(curve[curve.count()-5].toInt(), curve[curve.count()-4].toInt());
							c->setVisible(curve.last().toInt());
						}
					}
				}
			} else if(plotType == Graph::Histogram){//histograms from matrices
                Matrix *m = app->matrix(curve[2]);
                ag->restoreHistogram(m, curve);
                ag->updateCurveLayout(curveID, &cl);
			}
			curveID++;
		}
		else if (s.contains ("FunctionCurve")){
			QStringList curve = s.split("\t");
			CurveLayout cl;
			cl.connectType=curve[6].toInt();
			cl.lCol=curve[7].toInt();
			cl.lStyle=curve[8].toInt();
			cl.lWidth=curve[9].toInt();
			cl.sSize=curve[10].toInt();
			cl.sType=curve[11].toInt();
			cl.symCol=curve[12].toInt();
			cl.fillCol=curve[13].toInt();
			cl.filledArea=curve[14].toInt();
			cl.aCol=curve[15].toInt();
			cl.aStyle=curve[16].toInt();
			int current_index = 17;
			if(curve.count() < 16)
				cl.penWidth = cl.lWidth;
			else if ((d_file_version >= 79) && (curve[5].toInt() == Graph::Box))
				{
					cl.penWidth = curve[17].toInt();
					current_index++;
				}
			else if ((d_file_version >= 78) && (curve[5].toInt() <= Graph::LineSymbols))
				{
					cl.penWidth = curve[17].toInt();
					current_index++;
				}
			else
				cl.penWidth = cl.lWidth;

			ag->insertFunctionCurve(curve[1], curve[2].toInt(), d_file_version);
			ag->setCurveType(curveID, curve[5].toInt());
			ag->updateCurveLayout(curveID, &cl);
			if (d_file_version >= 88){
				QwtPlotCurve *c = ag->curve(curveID);
				if (c){
                    if(current_index + 1 < curve.size())
                        c->setAxis(curve[current_index].toInt(), curve[current_index+1].toInt());
					if (d_file_version >= 90 && current_index+2 < curve.size())
						c->setVisible(curve.last().toInt());
                    else
                        c->setVisible(true);
				}

			}
			curveID++;
		}
		else if (s.contains ("ErrorBars")){
			QStringList curve = s.split("\t", QString::SkipEmptyParts);
			if (!app->renamedTables.isEmpty()){
				QString caption = (curve[4]).left((curve[4]).find("_",0));
				if (app->renamedTables.contains(caption))
				{//modify the name of the curve according to the new table name
					int index = app->renamedTables.findIndex (caption);
					QString newCaption = app->renamedTables[++index];
					curve.replaceInStrings(caption+"_", newCaption+"_");
				}
			}
			Table *w = app->table(curve[3]);
			Table *errTable = app->table(curve[4]);
			if (w && errTable){
				ag->addErrorBars(curve[2], curve[3], errTable, curve[4], curve[1].toInt(),
						curve[5].toInt(), curve[6].toInt(), QColor(curve[7]),
						curve[8].toInt(), curve[10].toInt(), curve[9].toInt());
			}
			curveID++;
		}
		else if (s == "<spectrogram>"){
			curveID++;
			QStringList lst;
			while ( s!="</spectrogram>" ){
				s = list[++j];
				lst << s;
			}
			lst.pop_back();
			ag->restoreSpectrogram(app, lst);
		}
		else if (s.left(6)=="scale\t"){
			QStringList scl = s.split("\t");
			scl.pop_front();
			if (d_file_version < 88){
				double step = scl[2].toDouble();
				if (scl[5] == "0")
					step = 0.0;
				ag->setScale(QwtPlot::xBottom, scl[0].toDouble(), scl[1].toDouble(), step,
						scl[3].toInt(), scl[4].toInt(), scl[6].toInt(), bool(scl[7].toInt()));
				ag->setScale(QwtPlot::xTop, scl[0].toDouble(), scl[1].toDouble(), step,
						scl[3].toInt(), scl[4].toInt(), scl[6].toInt(), bool(scl[7].toInt()));

				step = scl[10].toDouble();
				if (scl[13] == "0")
					step = 0.0;
				ag->setScale(QwtPlot::yLeft, scl[8].toDouble(), scl[9].toDouble(), step, scl[11].toInt(),
						scl[12].toInt(), scl[14].toInt(), bool(scl[15].toInt()));
				ag->setScale(QwtPlot::yRight, scl[8].toDouble(), scl[9].toDouble(), step, scl[11].toInt(),
						scl[12].toInt(), scl[14].toInt(), bool(scl[15].toInt()));
			}else
				ag->setScale(scl[0].toInt(), scl[1].toDouble(), scl[2].toDouble(), scl[3].toDouble(),
						scl[4].toInt(), scl[5].toInt(),  scl[6].toInt(), bool(scl[7].toInt()));
		}
		else if (s.contains ("PlotTitle")){
			QStringList fList=s.split("\t");
			ag->setTitle(fList[1]);
			ag->setTitleColor(QColor(fList[2]));
			ag->setTitleAlignment(fList[3].toInt());
		}
		else if (s.contains ("TitleFont")){
			QStringList fList=s.split("\t");
			QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
			fnt.setUnderline(fList[5].toInt());
			fnt.setStrikeOut(fList[6].toInt());
			ag->setTitleFont(fnt);
		}
		else if (s.contains ("AxesTitles")){
			QStringList lst=s.split("\t");
			lst.pop_front();
			for (int i=0; i<4; i++){
			    if (lst.count() > i)
                    ag->setScaleTitle(i, lst[i]);
			}
		}
		else if (s.contains ("AxesTitleColors")){
			QStringList colors = s.split("\t", QString::SkipEmptyParts);
			colors.pop_front();
			for (int i=0; i<int(colors.count()); i++)
				ag->setAxisTitleColor(i, colors[i]);
		}else if (s.contains ("AxesTitleAlignment")){
			QStringList align=s.split("\t", QString::SkipEmptyParts);
			align.pop_front();
			for (int i=0; i<(int)align.count(); i++)
				ag->setAxisTitleAlignment(i, align[i].toInt());
		}else if (s.contains ("ScaleFont")){
			QStringList fList=s.split("\t");
			QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
			fnt.setUnderline(fList[5].toInt());
			fnt.setStrikeOut(fList[6].toInt());

			int axis=(fList[0].right(1)).toInt();
			ag->setAxisTitleFont(axis,fnt);
		}else if (s.contains ("AxisFont")){
			QStringList fList=s.split("\t");
			QFont fnt=QFont (fList[1],fList[2].toInt(),fList[3].toInt(),fList[4].toInt());
			fnt.setUnderline(fList[5].toInt());
			fnt.setStrikeOut(fList[6].toInt());

			int axis=(fList[0].right(1)).toInt();
			ag->setAxisFont(axis,fnt);
		}
		else if (s.contains ("AxesFormulas"))
		{
			QStringList fList=s.split("\t");
			fList.remove(fList.first());
			ag->setAxesFormulas(fList);
		}
		else if (s.startsWith("<AxisFormula "))
		{
			int pos = s.mid(18,s.length()-20).toInt();
			QString formula;
			for (j++; j<(int)list.count() && list[j] != "</AxisFormula>"; j++)
				formula += list[j] + "\n";
			formula.truncate(formula.length()-1);
			ag->setAxisFormula(pos,formula);
		}
		else if (s.contains ("LabelsFormat"))
		{
			QStringList fList=s.split("\t");
			fList.pop_front();
			ag->setLabelsNumericFormat(fList);
		}
		else if (s.contains ("LabelsRotation"))
		{
			QStringList fList=s.split("\t");
			ag->setAxisLabelRotation(QwtPlot::xBottom, fList[1].toInt());
			ag->setAxisLabelRotation(QwtPlot::xTop, fList[2].toInt());
		}
		else if (s.contains ("DrawAxesBackbone"))
		{
			QStringList fList=s.split("\t");
			ag->loadAxesOptions(fList[1]);
		}
		else if (s.contains ("AxesLineWidth"))
		{
			QStringList fList=s.split("\t");
			ag->loadAxesLinewidth(fList[1].toInt());
		}
		else if (s.contains ("CanvasFrame")){
			QStringList lst = s.split("\t");
			ag->setCanvasFrame(lst[1].toInt(), QColor(lst[2]));
		}
		else if (s.contains ("CanvasBackground"))
		{
			QStringList list = s.split("\t");
			QColor c = QColor(list[1]);
			if (list.count() == 3)
				c.setAlpha(list[2].toInt());
			ag->setCanvasBackground(c);
		}
		else if (s.contains ("Legend"))
		{// version <= 0.8.9
			QStringList fList=QStringList::split ("\t",s, true);
			ag->insertLegend(fList, d_file_version);
		}
		else if (s.startsWith ("<legend>") && s.endsWith ("</legend>"))
		{
			QStringList fList=QStringList::split ("\t", s.remove("</legend>"), true);
			ag->insertLegend(fList, d_file_version);
		}
		else if (s.contains ("textMarker"))
		{// version <= 0.8.9
			QStringList fList = QStringList::split ("\t",s, true);
			ag->insertText(fList, d_file_version);
		}
		else if (s.startsWith ("<text>") && s.endsWith ("</text>"))
		{
			QStringList fList = QStringList::split ("\t", s.remove("</text>"), true);
			ag->insertText(fList, d_file_version);
		}
		else if (s.contains ("lineMarker"))
		{// version <= 0.8.9
			QStringList fList=s.split("\t");
			ag->addArrow(fList, d_file_version);
		}
		else if (s.startsWith ("<line>") && s.endsWith ("</line>"))
		{
			QStringList fList=s.remove("</line>").split("\t");
			ag->addArrow(fList, d_file_version);
		}
		else if (s.contains ("ImageMarker") || (s.startsWith ("<image>") && s.endsWith ("</image>")))
		{
			QStringList fList=s.remove("</image>").split("\t");
			ag->insertImageMarker(fList, d_file_version);
		}
		else if (s.contains("AxisType"))
		{
			QStringList fList=s.split("\t");
			for (int i=0; i<4; i++)
			{
				QStringList lst = fList[i+1].split(";", QString::SkipEmptyParts);
				int format = lst[0].toInt();
				if (format == Graph::Day)
					ag->setLabelsDayFormat(i, lst[1].toInt());
				else if (format == Graph::Month)
					ag->setLabelsMonthFormat(i, lst[1].toInt());
				else if (format == Graph::Time || format == Graph::Date)
					ag->setLabelsDateTimeFormat(i, format, lst[1]+";"+lst[2]);
				else if (lst.size() > 1)
				{
					Table *nw = app->table(lst[1]);
					ag->setLabelsTextFormat(i, format, lst[1], nw);
				}
			}
		}
		else if (d_file_version < 69 && s.contains ("AxesTickLabelsCol"))
		{
			QStringList fList = s.split("\t");
			QList<int> axesTypes = ag->axesType();
			for (int i=0; i<4; i++){
				QString colName = fList[i+1];
				Table *nw = app->table(colName);
				ag->setLabelsTextFormat(i, axesTypes[i], colName, nw);
			}
		}
	}
	ag->replot();
	if (ag->isPiePlot()){
        QwtPieCurve *c = (QwtPieCurve *)ag->curve(0);
        if (c) c->updateBoundingRect();
    }

    ag->blockSignals(false);
    ag->setIgnoreResizeEvents(!app->autoResizeLayers);
    ag->setAutoscaleFonts(app->autoScaleFonts);
	ag->setTextMarkerDefaults(app->legendFrameStyle, app->plotLegendFont, app->legendTextColor, app->legendBackground);
	ag->setArrowDefaults(app->defaultArrowLineWidth, app->defaultArrowColor, app->defaultArrowLineStyle,
			app->defaultArrowHeadLength, app->defaultArrowHeadAngle, app->defaultArrowHeadFill);
    return ag;
}

Graph3D* ApplicationWindow::openSurfacePlot(ApplicationWindow* app, const QStringList &lst)
{
	QStringList fList=lst[0].split("\t");
	QString caption=fList[0];
	QString date=fList[1];
	if (date.isEmpty())
		date = QDateTime::currentDateTime().toString(Qt::LocalDate);

	fList=lst[2].split("\t", QString::SkipEmptyParts);
	Graph3D *plot=0;

	if (fList[1].endsWith("(Y)",true))//Ribbon plot
		plot=app->dataPlot3D(caption, fList[1],fList[2].toDouble(),fList[3].toDouble(),
				fList[4].toDouble(),fList[5].toDouble(),fList[6].toDouble(),fList[7].toDouble());
	else if (fList[1].contains("(Z)",true) > 0)
		plot=app->openPlotXYZ(caption, fList[1], fList[2].toDouble(),fList[3].toDouble(),
				fList[4].toDouble(),fList[5].toDouble(),fList[6].toDouble(),fList[7].toDouble());
	else if (fList[1].startsWith("matrix<",true) && fList[1].endsWith(">",false))
		plot=app->openMatrixPlot3D(caption, fList[1], fList[2].toDouble(),fList[3].toDouble(),
				fList[4].toDouble(),fList[5].toDouble(),fList[6].toDouble(),fList[7].toDouble());
	else if (fList[1].contains(",")){
		QStringList l = fList[1].split(",", QString::SkipEmptyParts);
		plot = app->plotParametricSurface(l[0], l[1], l[2], l[3].toDouble(), l[4].toDouble(),
				l[5].toDouble(), l[6].toDouble(), l[7].toInt(), l[8].toInt(), l[9].toInt(), l[10].toInt());
		app->setWindowName(plot, caption);
	} else {
		QStringList l = fList[1].split(";", QString::SkipEmptyParts);
		if (l.count() == 1)
			plot = app->plotSurface(fList[1], fList[2].toDouble(), fList[3].toDouble(),
				fList[4].toDouble(), fList[5].toDouble(), fList[6].toDouble(), fList[7].toDouble());
		else if (l.count() == 3)
			plot = app->plotSurface(l[0], fList[2].toDouble(), fList[3].toDouble(), fList[4].toDouble(),
					fList[5].toDouble(), fList[6].toDouble(), fList[7].toDouble(), l[1].toInt(), l[2].toInt());
		app->setWindowName(plot, caption);
	}

	if (!plot)
		return 0;

	app->setListViewDate(caption, date);
	plot->setBirthDate(date);
	plot->setIgnoreFonts(true);
	restoreWindowGeometry(app, plot, lst[1]);

	fList=lst[4].split("\t", QString::SkipEmptyParts);
	plot->setGrid(fList[1].toInt());

	plot->setTitle(lst[5].split("\t"));
	plot->setColors(lst[6].split("\t", QString::SkipEmptyParts));

	fList=lst[7].split("\t", QString::SkipEmptyParts);
	fList.pop_front();
	plot->setAxesLabels(fList);

	plot->setTicks(lst[8].split("\t", QString::SkipEmptyParts));
	plot->setTickLengths(lst[9].split("\t", QString::SkipEmptyParts));
	plot->setOptions(lst[10].split("\t", QString::SkipEmptyParts));
	plot->setNumbersFont(lst[11].split("\t", QString::SkipEmptyParts));
	plot->setXAxisLabelFont(lst[12].split("\t", QString::SkipEmptyParts));
	plot->setYAxisLabelFont(lst[13].split("\t", QString::SkipEmptyParts));
	plot->setZAxisLabelFont(lst[14].split("\t", QString::SkipEmptyParts));

	fList=lst[15].split("\t", QString::SkipEmptyParts);
	plot->setRotation(fList[1].toDouble(),fList[2].toDouble(),fList[3].toDouble());

	fList=lst[16].split("\t", QString::SkipEmptyParts);
	plot->setZoom(fList[1].toDouble());

	fList=lst[17].split("\t", QString::SkipEmptyParts);
	plot->setScale(fList[1].toDouble(),fList[2].toDouble(),fList[3].toDouble());

	fList=lst[18].split("\t", QString::SkipEmptyParts);
	plot->setShift(fList[1].toDouble(),fList[2].toDouble(),fList[3].toDouble());

	fList=lst[19].split("\t", QString::SkipEmptyParts);
	plot->setMeshLineWidth(fList[1].toDouble());

	if (d_file_version > 71){
		fList=lst[20].split("\t"); // using QString::SkipEmptyParts here causes a crash for empty window labels
		plot->setWindowLabel(fList[1]);
		plot->setCaptionPolicy((MyWidget::CaptionPolicy)fList[2].toInt());
		app->setListViewLabel(plot->objectName(),fList[1]);
	}

	if (d_file_version >= 88){
		fList=lst[21].split("\t", QString::SkipEmptyParts);
		plot->setOrthogonal(fList[1].toInt());
	}

	plot->setStyle(lst[3].split("\t", QString::SkipEmptyParts));
	plot->setIgnoreFonts(true);
	plot->update();
	return plot;
}

void ApplicationWindow::copyActiveLayer()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph *g = ((MultiLayer *)ws->activeWindow())->activeGraph();
	delete lastCopiedLayer;
	lastCopiedLayer = new Graph (0, 0, 0);
	lastCopiedLayer->setAttribute(Qt::WA_DeleteOnClose);
	lastCopiedLayer->setGeometry(0, 0, g->width(), g->height());
	lastCopiedLayer->copy(g);
	g->copyImage();
}

void ApplicationWindow::showDataSetDialog(const QString& whichFit)
{
    if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph *g = ((MultiLayer *)ws->activeWindow())->activeGraph();
	if (!g)
        return;

	DataSetDialog *ad = new DataSetDialog(tr("Curve") + ": ", this);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	ad->setGraph(g);
	ad->setOperationType(whichFit);
	ad->exec();
}

void ApplicationWindow::analyzeCurve(Graph *g, const QString& whichFit, const QString& curveTitle)
{
	if (!g)
		return;

	if(whichFit=="fitLinear" || whichFit=="fitSigmoidal" || whichFit=="fitGauss" || whichFit=="fitLorentz")
	{
		Fit *fitter = 0;
		if (whichFit == "fitLinear")
			fitter = new LinearFit (this, g);
		else if (whichFit == "fitSigmoidal"){
			QwtPlotCurve* c = g->curve(curveTitle);
            if (c){
            	const QwtScaleEngine *sc_eng = g->plotWidget()->axisScaleEngine(c->xAxis());
            	QwtScaleTransformation *tr = sc_eng->transformation();
            	if(tr->type() == QwtScaleTransformation::Log10)
					fitter = new LogisticFit (this, g);
				else
					fitter = new SigmoidalFit (this, g);
            }
		} else if(whichFit == "fitGauss")
			fitter = new GaussFit(this, g);
		else if(whichFit == "fitLorentz")
			fitter = new LorentzFit(this, g);

		if (fitter->setDataFromCurve(curveTitle)){
			if (whichFit != "fitLinear")
				fitter->guessInitialValues();

            fitter->scaleErrors(fit_scale_errors);
            fitter->setOutputPrecision(fit_output_precision);

            if (whichFit == "fitLinear" && d_2_linear_fit_points)
                fitter->generateFunction(generateUniformFitPoints, 2);
            else
                fitter->generateFunction(generateUniformFitPoints, fitPoints);
			fitter->fit();
			if (pasteFitResultsToPlot)
                fitter->showLegend();
			delete fitter;
		}
	} else if(whichFit == "differentiate"){
		Differentiation *diff = new Differentiation(this, g, curveTitle);
		diff->enableGraphicsDisplay(true);
		diff->run();
		delete diff;
	}
}

void ApplicationWindow::analysis(const QString& whichFit)
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	QString curve_title = g->selectedCurveTitle();
	if (!curve_title.isNull()) {
		analyzeCurve(g, whichFit, curve_title);
		return;
	}

    QStringList lst = g->analysableCurvesList();
	if (lst.count() == 1){
		const QwtPlotCurve *c = g->curve(lst[0]);
		if (c)
			analyzeCurve(g, whichFit, lst[0]);
	}
	else
		showDataSetDialog(whichFit);
}

void ApplicationWindow::pickPointerCursor()
{
	btnPointer->setChecked(true);
}

void ApplicationWindow::disableTools()
{
	if (displayBar->isVisible())
		displayBar->hide();

	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows){
		if (w->isA("MultiLayer")){
			QWidgetList lst= ((MultiLayer *)w)->graphPtrs();
			foreach(QWidget *widget, lst)
				((Graph *)widget)->disableTools();
		}
	}
	delete windows;
}

void ApplicationWindow::pickDataTool( QAction* action )
{
	if (!action)
		return;

	disableTools();

	if (action == btnCursor)
		showCursor();
	else if (action == btnSelect)
		showRangeSelectors();
	else if (action == btnPicker)
		showScreenReader();
	else if (action == btnMovePoints)
		movePoints();
	else if (action == btnRemovePoints)
		removePoints();
	else if (action == actionDrawPoints)
		drawPoints();
	else if (action == btnZoomIn)
		zoomIn();
	else if (action == btnZoomOut)
		zoomOut();
	else if (action == btnArrow)
		drawArrow();
	else if (action == btnLine)
		drawLine();
}

void ApplicationWindow::connectSurfacePlot(Graph3D *plot)
{
	connect (plot, SIGNAL(showTitleBarMenu()), this,SLOT(showWindowTitleBarMenu()));
	connect (plot, SIGNAL(showContextMenu()), this,SLOT(showWindowContextMenu()));
	connect (plot, SIGNAL(showOptionsDialog()), this,SLOT(showPlot3dDialog()));
	connect (plot, SIGNAL(closedWindow(MyWidget*)), this, SLOT(closeWindow(MyWidget*)));
	connect (plot, SIGNAL(hiddenWindow(MyWidget*)), this, SLOT(hideWindow(MyWidget*)));
	connect (plot, SIGNAL(statusChanged(MyWidget*)), this, SLOT(updateWindowStatus(MyWidget*)));
	connect (plot, SIGNAL(modified()), this, SIGNAL(modified()));
	connect (plot, SIGNAL(moved()), this, SLOT(modifiedProject()));

	plot->askOnCloseEvent(confirmClosePlot3D);
}

void ApplicationWindow::connectMultilayerPlot(MultiLayer *g)
{
	connect (g,SIGNAL(showTitleBarMenu()),this,SLOT(showWindowTitleBarMenu()));
	connect (g,SIGNAL(showTextDialog()),this,SLOT(showTextDialog()));
	connect (g,SIGNAL(showPlotDialog(int)),this,SLOT(showPlotDialog(int)));
	connect (g,SIGNAL(showScaleDialog(int)), this, SLOT(showScalePageFromAxisDialog(int)));
	connect (g,SIGNAL(showAxisDialog(int)), this, SLOT(showAxisPageFromAxisDialog(int)));
	connect (g,SIGNAL(showCurveContextMenu(int)), this, SLOT(showCurveContextMenu(int)));
	connect (g,SIGNAL(showWindowContextMenu()),this,SLOT(showWindowContextMenu()));
	connect (g,SIGNAL(showCurvesDialog()),this,SLOT(showCurvesDialog()));
	connect (g,SIGNAL(drawLineEnded(bool)), btnPointer, SLOT(setOn(bool)));
	connect (g,SIGNAL(drawTextOff()),this, SLOT(disableAddText()));
	connect (g,SIGNAL(showXAxisTitleDialog()),this,SLOT(showXAxisTitleDialog()));
	connect (g,SIGNAL(showYAxisTitleDialog()),this,SLOT(showYAxisTitleDialog()));
	connect (g,SIGNAL(showRightAxisTitleDialog()),this,SLOT(showRightAxisTitleDialog()));
	connect (g,SIGNAL(showTopAxisTitleDialog()),this,SLOT(showTopAxisTitleDialog()));
	connect (g,SIGNAL(showMarkerPopupMenu()),this,SLOT(showMarkerPopupMenu()));
	connect (g,SIGNAL(closedWindow(MyWidget*)),this, SLOT(closeWindow(MyWidget*)));
	connect (g,SIGNAL(hiddenWindow(MyWidget*)),this, SLOT(hideWindow(MyWidget*)));
	connect (g,SIGNAL(statusChanged(MyWidget*)),this, SLOT(updateWindowStatus(MyWidget*)));
	connect (g,SIGNAL(cursorInfo(const QString&)),info,SLOT(setText(const QString&)));
	connect (g,SIGNAL(showImageDialog()),this,SLOT(showImageDialog()));
	connect (g,SIGNAL(createTable(const QString&,int,int,const QString&)),
			this,SLOT(newTable(const QString&,int,int,const QString&)));
	connect (g,SIGNAL(viewTitleDialog()),this,SLOT(showTitleDialog()));
	connect (g,SIGNAL(modifiedWindow(QWidget*)),this,SLOT(modifiedProject(QWidget*)));
	connect (g,SIGNAL(modifiedPlot()),this,SLOT(modifiedProject()));
	connect (g,SIGNAL(showLineDialog()),this,SLOT(showLineDialog()));
	connect (g,SIGNAL(pasteMarker()),this,SLOT(pasteSelection()));
	connect (g,SIGNAL(showGraphContextMenu()),this,SLOT(showGraphContextMenu()));
	connect (g,SIGNAL(setPointerCursor()),this, SLOT(pickPointerCursor()));
	connect (g,SIGNAL(currentFontChanged(const QFont&)), this, SLOT(setFormatBarFont(const QFont&)));
    connect (g,SIGNAL(enableTextEditor(Graph *)), this, SLOT(enableTextEditor(Graph *)));
	connect (g, SIGNAL(moved()), this, SLOT(modifiedProject()));

	g->askOnCloseEvent(confirmClosePlot2D);
}

void ApplicationWindow::connectTable(Table* w)
{
    connect (w->table(), SIGNAL(selectionChanged()), this, SLOT(customColumnActions()));
	connect (w,SIGNAL(showTitleBarMenu()),this,SLOT(showWindowTitleBarMenu()));
	connect (w,SIGNAL(statusChanged(MyWidget*)),this, SLOT(updateWindowStatus(MyWidget*)));
	connect (w,SIGNAL(hiddenWindow(MyWidget*)),this, SLOT(hideWindow(MyWidget*)));
	connect (w,SIGNAL(closedWindow(MyWidget*)),this, SLOT(closeWindow(MyWidget*)));
	connect (w,SIGNAL(removedCol(const QString&)),this,SLOT(removeCurves(const QString&)));
	connect (w,SIGNAL(modifiedData(Table *, const QString&)),
			this, SLOT(updateCurves(Table *, const QString&)));
	connect (w,SIGNAL(resizedWindow(QWidget*)),this,SLOT(modifiedProject(QWidget*)));
	connect (w,SIGNAL(modifiedWindow(QWidget*)),this,SLOT(modifiedProject(QWidget*)));
	connect (w,SIGNAL(optionsDialog()),this,SLOT(showColumnOptionsDialog()));
	connect (w,SIGNAL(colValuesDialog()),this,SLOT(showColumnValuesDialog()));
	connect (w,SIGNAL(showContextMenu(bool)),this,SLOT(showTableContextMenu(bool)));
	connect (w,SIGNAL(changedColHeader(const QString&,const QString&)),this,SLOT(updateColNames(const QString&,const QString&)));
	connect (w,SIGNAL(createTable(const QString&,int,int,const QString&)),this,SLOT(newTable(const QString&,int,int,const QString&)));
	connect (w, SIGNAL(moved()), this, SLOT(modifiedProject()));

	w->askOnCloseEvent(confirmCloseTable);
}

void ApplicationWindow::setAppColors(const QColor& wc,const QColor& pc,const QColor& tpc)
{
	if (workspaceColor != wc)
	{
		workspaceColor = wc;
		ws->setPaletteBackgroundColor (wc);
	}

	if (panelsColor == pc && panelsTextColor == tpc)
		return;

	panelsColor = pc;
	panelsTextColor = tpc;

	QColorGroup cg;
	cg.setColor(QColorGroup::Base, QColor(panelsColor) );
	qApp->setPalette(QPalette(cg, cg, cg));

	cg.setColor(QColorGroup::Text, QColor(panelsTextColor) );
	cg.setColor(QColorGroup::WindowText, QColor(panelsTextColor) );
	cg.setColor(QColorGroup::HighlightedText, QColor(panelsTextColor) );
	lv->setPalette(QPalette(cg, cg, cg));
	results->setPalette(QPalette(cg, cg, cg));
}

void ApplicationWindow::setPlot3DOptions()
{
	QList<QWidget*> *windows = windowsList();
	foreach (QWidget *w, *windows){
		if (w->isA("Graph3D")){
			Graph3D *g = (Graph3D*)w;
			g->setOrthogonal(orthogonal3DPlots);
			g->setAutoscale(autoscale3DPlots);
			g->setAntialiasing(smooth3DMesh);
		}
	}
	delete windows;
}

void ApplicationWindow::createActions()
{
    actionCustomActionDialog = new QAction(tr("Add &Custom Script Action..."), this);
	connect(actionCustomActionDialog, SIGNAL(activated()), this, SLOT(showCustomActionDialog()));

	actionNewProject = new QAction(QIcon(QPixmap(new_xpm)), tr("New &Project"), this);
	actionNewProject->setShortcut( tr("Ctrl+N") );
	connect(actionNewProject, SIGNAL(activated()), this, SLOT(newProject()));

    actionNewFolder = new QAction(QIcon(QPixmap(newFolder_xpm)), tr("New &Project"), this);
	actionNewProject->setShortcut(Qt::Key_F7);
	connect(actionNewFolder, SIGNAL(activated()), this, SLOT(addFolder()));

	actionNewGraph = new QAction(QIcon(QPixmap(new_graph_xpm)), tr("New &Graph"), this);
	actionNewGraph->setShortcut( tr("Ctrl+G") );
	connect(actionNewGraph, SIGNAL(activated()), this, SLOT(newGraph()));

	actionNewNote = new QAction(QIcon(QPixmap(new_note_xpm)), tr("New &Note"), this);
	connect(actionNewNote, SIGNAL(activated()), this, SLOT(newNote()));

	actionNewTable = new QAction(QIcon(QPixmap(table_xpm)), tr("New &Table"), this);
	actionNewTable->setShortcut( tr("Ctrl+T") );
	connect(actionNewTable, SIGNAL(activated()), this, SLOT(newTable()));

	actionNewMatrix = new QAction(QIcon(QPixmap(new_matrix_xpm)), tr("New &Matrix"), this);
	actionNewMatrix->setShortcut( tr("Ctrl+M") );
	connect(actionNewMatrix, SIGNAL(activated()), this, SLOT(newMatrix()));

	actionNewFunctionPlot = new QAction(QIcon(QPixmap(newF_xpm)), tr("New &Function Plot"), this);
	actionNewFunctionPlot->setShortcut( tr("Ctrl+F") );
	connect(actionNewFunctionPlot, SIGNAL(activated()), this, SLOT(functionDialog()));

	actionNewSurfacePlot = new QAction(QIcon(QPixmap(newFxy_xpm)), tr("New 3D &Surface Plot"), this);
	actionNewSurfacePlot->setShortcut( tr("Ctrl+ALT+Z") );
	connect(actionNewSurfacePlot, SIGNAL(activated()), this, SLOT(newSurfacePlot()));

	actionOpen = new QAction(QIcon(QPixmap(fileopen_xpm)), tr("&Open"), this);
	actionOpen->setShortcut( tr("Ctrl+O") );
	connect(actionOpen, SIGNAL(activated()), this, SLOT(open()));

	actionLoadImage = new QAction(tr("Open Image &File"), this);
	actionLoadImage->setShortcut( tr("Ctrl+I") );
	connect(actionLoadImage, SIGNAL(activated()), this, SLOT(loadImage()));

	actionImportImage = new QAction(tr("Import I&mage..."), this);
	connect(actionImportImage, SIGNAL(activated()), this, SLOT(importImage()));

	actionSaveProject = new QAction(QIcon(QPixmap(filesave_xpm)), tr("&Save Project"), this);
	actionSaveProject->setShortcut( tr("Ctrl+S") );
	connect(actionSaveProject, SIGNAL(activated()), this, SLOT(saveProject()));
	savedProject();

	actionSaveProjectAs = new QAction(tr("Save Project &As..."), this);
	connect(actionSaveProjectAs, SIGNAL(activated()), this, SLOT(saveProjectAs()));

	actionOpenTemplate = new QAction(QIcon(QPixmap(open_template_xpm)),tr("Open Temp&late..."), this);
	connect(actionOpenTemplate, SIGNAL(activated()), this, SLOT(openTemplate()));

	actionSaveTemplate = new QAction(QIcon(QPixmap(save_template_xpm)), tr("Save As &Template..."), this);
	connect(actionSaveTemplate, SIGNAL(activated()), this, SLOT(saveAsTemplate()));

	actionSaveNote = new QAction(tr("Save Note As..."), this);
	connect(actionSaveNote, SIGNAL(activated()), this, SLOT(saveNoteAs()));

	actionLoad = new QAction(QIcon(QPixmap(import_xpm)), tr("&Import ASCII..."), this);
	connect(actionLoad, SIGNAL(activated()), this, SLOT(importASCII()));

	actionUndo = new QAction(QIcon(QPixmap(undo_xpm)), tr("&Undo"), this);
	actionUndo->setShortcut( tr("Ctrl+Z") );
	connect(actionUndo, SIGNAL(activated()), this, SLOT(undo()));
	actionUndo->setEnabled(false);

	actionRedo = new QAction(QIcon(QPixmap(redo_xpm)), tr("&Redo"), this);
	actionRedo->setShortcut( tr("Ctrl+R") );
	connect(actionRedo, SIGNAL(activated()), this, SLOT(redo()));
	actionRedo->setEnabled(false);

	actionCopyWindow = new QAction(QIcon(QPixmap(duplicate_xpm)), tr("&Duplicate"), this);
	connect(actionCopyWindow, SIGNAL(activated()), this, SLOT(clone()));

	actionCutSelection = new QAction(QIcon(QPixmap(cut_xpm)), tr("Cu&t Selection"), this);
	actionCutSelection->setShortcut( tr("Ctrl+X") );
	connect(actionCutSelection, SIGNAL(activated()), this, SLOT(cutSelection()));

	actionCopySelection = new QAction(QIcon(QPixmap(copy_xpm)), tr("&Copy Selection"), this);
	actionCopySelection->setShortcut( tr("Ctrl+C") );
	connect(actionCopySelection, SIGNAL(activated()), this, SLOT(copySelection()));

	actionPasteSelection = new QAction(QIcon(QPixmap(paste_xpm)), tr("&Paste Selection"), this);
	actionPasteSelection->setShortcut( tr("Ctrl+V") );
	connect(actionPasteSelection, SIGNAL(activated()), this, SLOT(pasteSelection()));

	actionClearSelection = new QAction(QIcon(QPixmap(erase_xpm)), tr("&Delete Selection"), this);
	actionClearSelection->setShortcut( tr("Del","delete key") );
	connect(actionClearSelection, SIGNAL(activated()), this, SLOT(clearSelection()));

	actionShowExplorer = explorerWindow->toggleViewAction();
	actionShowExplorer->setIcon(QPixmap(folder_xpm));
	actionShowExplorer->setShortcut( tr("Ctrl+E") );

	actionShowLog = logWindow->toggleViewAction();
	actionShowLog->setIcon(QPixmap(log_xpm));

#ifdef SCRIPTING_CONSOLE
	actionShowConsole = consoleWindow->toggleViewAction();
#endif

	actionAddLayer = new QAction(QIcon(QPixmap(newLayer_xpm)), tr("Add La&yer"), this);
	actionAddLayer->setShortcut( tr("ALT+L") );
	connect(actionAddLayer, SIGNAL(activated()), this, SLOT(addLayer()));

	actionShowLayerDialog = new QAction(QIcon(QPixmap(arrangeLayers_xpm)), tr("Arran&ge Layers"), this);
	actionShowLayerDialog->setShortcut( tr("Shift+A") );
	connect(actionShowLayerDialog, SIGNAL(activated()), this, SLOT(showLayerDialog()));

	actionAutomaticLayout = new QAction(QIcon(QPixmap(auto_layout_xpm)), tr("Automatic Layout"), this);
	connect(actionAutomaticLayout, SIGNAL(activated()), this, SLOT(autoArrangeLayers()));

	actionExportGraph = new QAction(tr("&Current"), this);
	actionExportGraph->setShortcut( tr("Alt+G") );
	connect(actionExportGraph, SIGNAL(activated()), this, SLOT(exportGraph()));

	actionExportAllGraphs = new QAction(tr("&All"), this);
	actionExportAllGraphs->setShortcut( tr("Alt+X") );
	connect(actionExportAllGraphs, SIGNAL(activated()), this, SLOT(exportAllGraphs()));

    actionExportPDF = new QAction(QIcon(QPixmap(pdf_xpm)), tr("&Export PDF"), this);
	actionExportPDF->setShortcut( tr("Ctrl+Alt+P") );
	connect(actionExportPDF, SIGNAL(activated()), this, SLOT(exportPDF()));

	actionPrint = new QAction(QIcon(QPixmap(fileprint_xpm)), tr("&Print"), this);
	actionPrint->setShortcut( tr("Ctrl+P") );
	connect(actionPrint, SIGNAL(activated()), this, SLOT(print()));

	actionPrintAllPlots = new QAction(tr("Print All Plo&ts"), this);
	connect(actionPrintAllPlots, SIGNAL(activated()), this, SLOT(printAllPlots()));

	actionShowExportASCIIDialog = new QAction(tr("E&xport ASCII"), this);
	connect(actionShowExportASCIIDialog, SIGNAL(activated()), this, SLOT(showExportASCIIDialog()));

	actionCloseAllWindows = new QAction(QIcon(QPixmap(quit_xpm)), tr("&Quit"), this);
	actionCloseAllWindows->setShortcut( tr("Ctrl+Q") );
	connect(actionCloseAllWindows, SIGNAL(activated()), qApp, SLOT(closeAllWindows()));

	actionClearLogInfo = new QAction(tr("Clear &Log Information"), this);
	connect(actionClearLogInfo, SIGNAL(activated()), this, SLOT(clearLogInfo()));

	actionDeleteFitTables = new QAction(QIcon(QPixmap(close_xpm)), tr("Delete &Fit Tables"), this);
	connect(actionDeleteFitTables, SIGNAL(activated()), this, SLOT(deleteFitTables()));

	actionShowPlotWizard = new QAction(QIcon(QPixmap(wizard_xpm)), tr("Plot &Wizard"), this);
	actionShowPlotWizard->setShortcut( tr("Ctrl+Alt+W") );
	connect(actionShowPlotWizard, SIGNAL(activated()), this, SLOT(showPlotWizard()));

	actionShowConfigureDialog = new QAction(tr("&Preferences..."), this);
	connect(actionShowConfigureDialog, SIGNAL(activated()), this, SLOT(showPreferencesDialog()));

	actionShowCurvesDialog = new QAction(QIcon(QPixmap(curves_xpm)), tr("Add/Remove &Curve..."), this);
	actionShowCurvesDialog->setShortcut( tr("ALT+C") );
	connect(actionShowCurvesDialog, SIGNAL(activated()), this, SLOT(showCurvesDialog()));

	actionAddErrorBars = new QAction(QIcon(QPixmap(errors_xpm)), tr("Add &Error Bars..."), this);
	actionAddErrorBars->setShortcut( tr("Ctrl+B") );
	connect(actionAddErrorBars, SIGNAL(activated()), this, SLOT(addErrorBars()));

	actionAddFunctionCurve = new QAction(QIcon(QPixmap(fx_xpm)), tr("Add &Function..."), this);
	actionAddFunctionCurve->setShortcut( tr("Ctrl+Alt+F") );
	connect(actionAddFunctionCurve, SIGNAL(activated()), this, SLOT(addFunctionCurve()));

	actionUnzoom = new QAction(QIcon(QPixmap(unzoom_xpm)), tr("&Rescale to Show All"), this);
	actionUnzoom->setShortcut( tr("Ctrl+Shift+R") );
	connect(actionUnzoom, SIGNAL(activated()), this, SLOT(setAutoScale()));

	actionNewLegend = new QAction(QIcon(QPixmap(legend_xpm)), tr("New &Legend"), this);
	actionNewLegend->setShortcut( tr("Ctrl+L") );
	connect(actionNewLegend, SIGNAL(activated()), this, SLOT(newLegend()));

	actionTimeStamp = new QAction(QIcon(QPixmap(clock_xpm)), tr("Add Time Stamp"), this);
	actionTimeStamp->setShortcut( tr("Ctrl+ALT+T") );
	connect(actionTimeStamp, SIGNAL(activated()), this, SLOT(addTimeStamp()));

	actionAddImage = new QAction(QIcon(QPixmap(monalisa_xpm)), tr("Add &Image"), this);
	actionAddImage->setShortcut( tr("ALT+I") );
	connect(actionAddImage, SIGNAL(activated()), this, SLOT(addImage()));

	actionPlotL = new QAction(QIcon(QPixmap(lPlot_xpm)), tr("&Line"), this);
	connect(actionPlotL, SIGNAL(activated()), this, SLOT(plotL()));

	actionPlotP = new QAction(QIcon(QPixmap(pPlot_xpm)), tr("&Scatter"), this);
	connect(actionPlotP, SIGNAL(activated()), this, SLOT(plotP()));

	actionPlotLP = new QAction(QIcon(QPixmap(lpPlot_xpm)), tr("Line + S&ymbol"), this);
	connect(actionPlotLP, SIGNAL(activated()), this, SLOT(plotLP()));

	actionPlotVerticalDropLines = new QAction(QIcon(QPixmap(dropLines_xpm)), tr("Vertical &Drop Lines"), this);
	connect(actionPlotVerticalDropLines, SIGNAL(activated()), this, SLOT(plotVerticalDropLines()));

	actionPlotSpline = new QAction(QIcon(QPixmap(spline_xpm)), tr("&Spline"), this);
	connect(actionPlotSpline, SIGNAL(activated()), this, SLOT(plotSpline()));

	actionPlotHorSteps = new QAction(QPixmap(hor_steps_xpm), tr("&Horizontal Steps"), this);
	connect(actionPlotHorSteps, SIGNAL(activated()), this, SLOT(plotHorSteps()));

	actionPlotVertSteps = new QAction(QIcon(QPixmap(vert_steps_xpm)), tr("&Vertical Steps"), this);
	connect(actionPlotVertSteps, SIGNAL(activated()), this, SLOT(plotVertSteps()));

	actionPlotVerticalBars = new QAction(QIcon(QPixmap(vertBars_xpm)), tr("&Columns"), this);
	connect(actionPlotVerticalBars, SIGNAL(activated()), this, SLOT(plotVerticalBars()));

	actionPlotHorizontalBars = new QAction(QIcon(QPixmap(hBars_xpm)), tr("&Rows"), this);
	connect(actionPlotHorizontalBars, SIGNAL(activated()), this, SLOT(plotHorizontalBars()));

	actionPlotArea = new QAction(QIcon(QPixmap(area_xpm)), tr("&Area"), this);
	connect(actionPlotArea, SIGNAL(activated()), this, SLOT(plotArea()));

	actionPlotPie = new QAction(QIcon(QPixmap(pie_xpm)), tr("&Pie"), this);
	connect(actionPlotPie, SIGNAL(activated()), this, SLOT(plotPie()));

	actionPlotVectXYAM = new QAction(QIcon(QPixmap(vectXYAM_xpm)), tr("Vectors XY&AM"), this);
	connect(actionPlotVectXYAM, SIGNAL(activated()), this, SLOT(plotVectXYAM()));

	actionPlotVectXYXY = new QAction(QIcon(QPixmap(vectXYXY_xpm)), tr("&Vectors &XYXY"), this);
	connect(actionPlotVectXYXY, SIGNAL(activated()), this, SLOT(plotVectXYXY()));

	actionPlotHistogram = new QAction(QIcon(QPixmap(histogram_xpm)), tr("&Histogram"), this);
	connect(actionPlotHistogram, SIGNAL(activated()), this, SLOT(plotHistogram()));

	actionPlotStackedHistograms = new QAction(QIcon(QPixmap(stacked_hist_xpm)), tr("&Stacked Histogram"), this);
	connect(actionPlotStackedHistograms, SIGNAL(activated()), this, SLOT(plotStackedHistograms()));

	actionPlot2VerticalLayers = new QAction(QIcon(QPixmap(panel_v2_xpm)), tr("&Vertical 2 Layers"), this);
	connect(actionPlot2VerticalLayers, SIGNAL(activated()), this, SLOT(plot2VerticalLayers()));

	actionPlot2HorizontalLayers = new QAction(QIcon(QPixmap(panel_h2_xpm)), tr("&Horizontal 2 Layers"), this);
	connect(actionPlot2HorizontalLayers, SIGNAL(activated()), this, SLOT(plot2HorizontalLayers()));

	actionPlot4Layers = new QAction(QIcon(QPixmap(panel_4_xpm)), tr("&4 Layers"), this);
	connect(actionPlot4Layers, SIGNAL(activated()), this, SLOT(plot4Layers()));

	actionPlotStackedLayers = new QAction(QIcon(QPixmap(stacked_xpm)), tr("&Stacked Layers"), this);
	connect(actionPlotStackedLayers, SIGNAL(activated()), this, SLOT(plotStackedLayers()));

	actionPlot3DRibbon = new QAction(QIcon(QPixmap(ribbon_xpm)), tr("&Ribbon"), this);
	connect(actionPlot3DRibbon, SIGNAL(activated()), this, SLOT(plot3DRibbon()));

	actionPlot3DBars = new QAction(QIcon(QPixmap(bars_xpm)), tr("&Bars"), this);
	connect(actionPlot3DBars, SIGNAL(activated()), this, SLOT(plot3DBars()));

	actionPlot3DScatter = new QAction(QIcon(QPixmap(scatter_xpm)), tr("&Scatter"), this);
	connect(actionPlot3DScatter, SIGNAL(activated()), this, SLOT(plot3DScatter()));

	actionPlot3DTrajectory = new QAction(QIcon(QPixmap(trajectory_xpm)), tr("&Trajectory"), this);
	connect(actionPlot3DTrajectory, SIGNAL(activated()), this, SLOT(plot3DTrajectory()));

	actionShowColStatistics = new QAction(QIcon(QPixmap(col_stat_xpm)), tr("Statistics on &Columns"), this);
	connect(actionShowColStatistics, SIGNAL(activated()), this, SLOT(showColStatistics()));

	actionShowRowStatistics = new QAction(QIcon(QPixmap(stat_rows_xpm)), tr("Statistics on &Rows"), this);
	connect(actionShowRowStatistics, SIGNAL(activated()), this, SLOT(showRowStatistics()));

	actionShowIntDialog = new QAction(tr("&Integrate ..."), this);
	connect(actionShowIntDialog, SIGNAL(activated()), this, SLOT(showIntegrationDialog()));

	actionInterpolate = new QAction(tr("Inte&rpolate ..."), this);
	connect(actionInterpolate, SIGNAL(activated()), this, SLOT(showInterpolationDialog()));

	actionLowPassFilter = new QAction(tr("&Low Pass..."), this);
	connect(actionLowPassFilter, SIGNAL(activated()), this, SLOT(lowPassFilterDialog()));

	actionHighPassFilter = new QAction(tr("&High Pass..."), this);
	connect(actionHighPassFilter, SIGNAL(activated()), this, SLOT(highPassFilterDialog()));

	actionBandPassFilter = new QAction(tr("&Band Pass..."), this);
	connect(actionBandPassFilter, SIGNAL(activated()), this, SLOT(bandPassFilterDialog()));

	actionBandBlockFilter = new QAction(tr("&Band Block..."), this);
	connect(actionBandBlockFilter, SIGNAL(activated()), this, SLOT(bandBlockFilterDialog()));

	actionFFT = new QAction(tr("&FFT..."), this);
	connect(actionFFT, SIGNAL(activated()), this, SLOT(showFFTDialog()));

	actionSmoothSavGol = new QAction(tr("&Savitzky-Golay..."), this);
	connect(actionSmoothSavGol, SIGNAL(activated()), this, SLOT(showSmoothSavGolDialog()));

	actionSmoothFFT = new QAction(tr("&FFT Filter..."), this);
	connect(actionSmoothFFT, SIGNAL(activated()), this, SLOT(showSmoothFFTDialog()));

	actionSmoothAverage = new QAction(tr("Moving Window &Average..."), this);
	connect(actionSmoothAverage, SIGNAL(activated()), this, SLOT(showSmoothAverageDialog()));

	actionDifferentiate = new QAction(tr("&Differentiate"), this);
	connect(actionDifferentiate, SIGNAL(activated()), this, SLOT(differentiate()));

	actionFitLinear = new QAction(tr("Fit &Linear"), this);
	connect(actionFitLinear, SIGNAL(activated()), this, SLOT(fitLinear()));

	actionShowFitPolynomDialog = new QAction(tr("Fit &Polynomial ..."), this);
	connect(actionShowFitPolynomDialog, SIGNAL(activated()), this, SLOT(showFitPolynomDialog()));

	actionShowExpDecayDialog = new QAction(tr("&First Order ..."), this);
	connect(actionShowExpDecayDialog, SIGNAL(activated()), this, SLOT(showExpDecayDialog()));

	actionShowTwoExpDecayDialog = new QAction(tr("&Second Order ..."), this);
	connect(actionShowTwoExpDecayDialog, SIGNAL(activated()), this, SLOT(showTwoExpDecayDialog()));

	actionShowExpDecay3Dialog = new QAction(tr("&Third Order ..."), this);
	connect(actionShowExpDecay3Dialog, SIGNAL(activated()), this, SLOT(showExpDecay3Dialog()));

	actionFitExpGrowth = new QAction(tr("Fit Exponential Gro&wth ..."), this);
	connect(actionFitExpGrowth, SIGNAL(activated()), this, SLOT(showExpGrowthDialog()));

	actionFitSigmoidal = new QAction(tr("Fit &Boltzmann (Sigmoidal)"), this);
	connect(actionFitSigmoidal, SIGNAL(activated()), this, SLOT(fitSigmoidal()));

	actionFitGauss = new QAction(tr("Fit &Gaussian"), this);
	connect(actionFitGauss, SIGNAL(activated()), this, SLOT(fitGauss()));

	actionFitLorentz = new QAction(tr("Fit Lorent&zian"), this);
	connect(actionFitLorentz, SIGNAL(activated()), this, SLOT(fitLorentz()));

	actionShowFitDialog = new QAction(tr("Fit &Wizard..."), this);
	actionShowFitDialog->setShortcut( tr("Ctrl+Y") );
	connect(actionShowFitDialog, SIGNAL(activated()), this, SLOT(showFitDialog()));

	actionShowPlotDialog = new QAction(tr("&Plot ..."), this);
	connect(actionShowPlotDialog, SIGNAL(activated()), this, SLOT(showGeneralPlotDialog()));

	actionShowScaleDialog = new QAction(tr("&Scales..."), this);
	connect(actionShowScaleDialog, SIGNAL(activated()), this, SLOT(showScaleDialog()));

	actionShowAxisDialog = new QAction(tr("&Axes..."), this);
	connect(actionShowAxisDialog, SIGNAL(activated()), this, SLOT(showAxisDialog()));

	actionShowGridDialog = new QAction(tr("&Grid ..."), this);
	connect(actionShowGridDialog, SIGNAL(activated()), this, SLOT(showGridDialog()));

	actionShowTitleDialog = new QAction(tr("&Title ..."), this);
	connect(actionShowTitleDialog, SIGNAL(activated()), this, SLOT(showTitleDialog()));

	actionShowColumnOptionsDialog = new QAction(tr("Column &Options ..."), this);
	actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
	connect(actionShowColumnOptionsDialog, SIGNAL(activated()), this, SLOT(showColumnOptionsDialog()));

	actionShowColumnValuesDialog = new QAction(tr("Set Column &Values ..."), this);
	connect(actionShowColumnValuesDialog, SIGNAL(activated()), this, SLOT(showColumnValuesDialog()));
	actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));

	actionTableRecalculate = new QAction(tr("Recalculate"), this);
	actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
	connect(actionTableRecalculate, SIGNAL(activated()), this, SLOT(recalculateTable()));

    actionSwapColumns = new QAction(QIcon(QPixmap(swap_columns_xpm)), tr("&Swap columns"), this);
	connect(actionSwapColumns, SIGNAL(activated()), this, SLOT(swapColumns()));

	actionMoveColRight = new QAction(QIcon(QPixmap(move_col_right_xpm)), tr("Move &Right"), this);
	connect(actionMoveColRight, SIGNAL(activated()), this, SLOT(moveColumnRight()));

	actionMoveColLeft = new QAction(QIcon(QPixmap(move_col_left_xpm)), tr("Move &Left"), this);
	connect(actionMoveColLeft, SIGNAL(activated()), this, SLOT(moveColumnLeft()));

	actionMoveColFirst = new QAction(QIcon(QPixmap(move_col_first_xpm)), tr("Move to F&irst"), this);
	connect(actionMoveColFirst, SIGNAL(activated()), this, SLOT(moveColumnFirst()));

	actionMoveColLast = new QAction(QIcon(QPixmap(move_col_last_xpm)), tr("Move to Las&t"), this);
	connect(actionMoveColLast, SIGNAL(activated()), this, SLOT(moveColumnLast()));

	actionShowColsDialog = new QAction(tr("&Columns..."), this);
	connect(actionShowColsDialog, SIGNAL(activated()), this, SLOT(showColsDialog()));

	actionShowRowsDialog = new QAction(tr("&Rows..."), this);
	connect(actionShowRowsDialog, SIGNAL(activated()), this, SLOT(showRowsDialog()));

    actionDeleteRows = new QAction(tr("&Delete Rows Interval..."), this);
	connect(actionDeleteRows, SIGNAL(activated()), this, SLOT(showDeleteRowsDialog()));

	actionAbout = new QAction(tr("&About QtiPlot"), this);
	actionAbout->setShortcut( tr("F1") );
	connect(actionAbout, SIGNAL(activated()), this, SLOT(about()));

	actionShowHelp = new QAction(tr("&Help"), this);
	actionShowHelp->setShortcut( tr("Ctrl+H") );
	connect(actionShowHelp, SIGNAL(activated()), this, SLOT(showHelp()));

	actionChooseHelpFolder = new QAction(tr("&Choose Help Folder..."), this);
	connect(actionChooseHelpFolder, SIGNAL(activated()), this, SLOT(chooseHelpFolder()));

	actionRename = new QAction(tr("&Rename Window"), this);
	connect(actionRename, SIGNAL(activated()), this, SLOT(rename()));

	actionCloseWindow = new QAction(QIcon(QPixmap(close_xpm)), tr("Close &Window"), this);
	actionCloseWindow->setShortcut( tr("Ctrl+W") );
	connect(actionCloseWindow, SIGNAL(activated()), this, SLOT(closeActiveWindow()));

	actionAddColToTable = new QAction(QIcon(QPixmap(addCol_xpm)), tr("Add Column"), this);
	connect(actionAddColToTable, SIGNAL(activated()), this, SLOT(addColToTable()));

	actionGoToRow = new QAction(tr("&Go to Row..."), this);
	actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));
	connect(actionGoToRow, SIGNAL(activated()), this, SLOT(goToRow()));

	actionClearTable = new QAction(QPixmap(erase_xpm), tr("Clear"), this);
	connect(actionClearTable, SIGNAL(activated()), this, SLOT(clearTable()));

	actionDeleteLayer = new QAction(QIcon(QPixmap(erase_xpm)), tr("&Remove Layer"), this);
	actionDeleteLayer->setShortcut( tr("Alt+R") );
	connect(actionDeleteLayer, SIGNAL(activated()), this, SLOT(deleteLayer()));

	actionResizeActiveWindow = new QAction(QIcon(QPixmap(resize_xpm)), tr("Window &Geometry..."), this);
	connect(actionResizeActiveWindow, SIGNAL(activated()), this, SLOT(resizeActiveWindow()));

	actionHideActiveWindow = new QAction(tr("&Hide Window"), this);
	connect(actionHideActiveWindow, SIGNAL(activated()), this, SLOT(hideActiveWindow()));

	actionShowMoreWindows = new QAction(tr("More windows..."), this);
	connect(actionShowMoreWindows, SIGNAL(activated()), this, SLOT(showMoreWindows()));

	actionPixelLineProfile = new QAction(QIcon(QPixmap(pixelProfile_xpm)), tr("&View Pixel Line Profile"), this);
	connect(actionPixelLineProfile, SIGNAL(activated()), this, SLOT(pixelLineProfile()));

	actionIntensityTable = new QAction(tr("&Intensity Table"), this);
	connect(actionIntensityTable, SIGNAL(activated()), this, SLOT(intensityTable()));

	actionShowLineDialog = new QAction(tr("&Properties"), this);
	connect(actionShowLineDialog, SIGNAL(activated()), this, SLOT(showLineDialog()));

	actionShowImageDialog = new QAction(tr("&Properties"), this);
	connect(actionShowImageDialog, SIGNAL(activated()), this, SLOT(showImageDialog()));

	actionShowTextDialog = new QAction(tr("&Properties"), this);
	connect(actionShowTextDialog, SIGNAL(activated()), this, SLOT(showTextDialog()));

	actionActivateWindow = new QAction(tr("&Activate Window"), this);
	connect(actionActivateWindow, SIGNAL(activated()), this, SLOT(activateWindow()));

	actionMinimizeWindow = new QAction(tr("Mi&nimize Window"), this);
	connect(actionMinimizeWindow, SIGNAL(activated()), this, SLOT(minimizeWindow()));

	actionMaximizeWindow = new QAction(tr("Ma&ximize Window"), this);
	connect(actionMaximizeWindow, SIGNAL(activated()), this, SLOT(maximizeWindow()));

	actionHideWindow = new QAction(tr("&Hide Window"), this);
	connect(actionHideWindow, SIGNAL(activated()), this, SLOT(hideWindow()));

	actionResizeWindow = new QAction(QIcon(QPixmap(resize_xpm)), tr("Re&size Window..."), this);
	connect(actionResizeWindow, SIGNAL(activated()), this, SLOT(resizeWindow()));

	actionPrintWindow = new QAction(QIcon(QPixmap(fileprint_xpm)),tr("&Print Window"), this);
	connect(actionPrintWindow, SIGNAL(activated()), this, SLOT(printWindow()));

	actionEditSurfacePlot = new QAction(tr("&Surface..."), this);
	connect(actionEditSurfacePlot, SIGNAL(activated()), this, SLOT(editSurfacePlot()));

	actionAdd3DData = new QAction(tr("&Data Set..."), this);
	connect(actionAdd3DData, SIGNAL(activated()), this, SLOT(add3DData()));

	actionSetMatrixProperties = new QAction(tr("Set &Properties..."), this);
	connect(actionSetMatrixProperties, SIGNAL(activated()), this, SLOT(showMatrixDialog()));

	actionSetMatrixDimensions = new QAction(tr("Set &Dimensions..."), this);
	connect(actionSetMatrixDimensions, SIGNAL(activated()), this, SLOT(showMatrixSizeDialog()));

	actionSetMatrixValues = new QAction(tr("Set &Values..."), this);
	connect(actionSetMatrixValues, SIGNAL(activated()), this, SLOT(showMatrixValuesDialog()));

    actionImagePlot =  new QAction(QIcon(QPixmap(image_plot_xpm)),tr("&Image Plot"), this);
	connect(actionImagePlot, SIGNAL(activated()), this, SLOT(plotImage()));

	actionTransposeMatrix = new QAction(tr("&Transpose"), this);
	connect(actionTransposeMatrix, SIGNAL(activated()), this, SLOT(transposeMatrix()));

	actionFlipMatrixVertically = new QAction(tr("Flip &V"), this);
	actionFlipMatrixVertically->setShortcut(tr("Ctrl+Shift+V"));
	connect(actionFlipMatrixVertically, SIGNAL(activated()), this, SLOT(flipMatrixVertically()));

	actionFlipMatrixHorizontally = new QAction(tr("Flip &H"), this);
	actionFlipMatrixHorizontally->setShortcut(tr("Ctrl+Shift+H"));
	connect(actionFlipMatrixHorizontally, SIGNAL(activated()), this, SLOT(flipMatrixHorizontally()));

	actionRotateMatrix = new QAction(tr("R&otate 90"), this);
	actionRotateMatrix->setShortcut(tr("Ctrl+Shift+R"));
	connect(actionRotateMatrix, SIGNAL(activated()), this, SLOT(rotateMatrix90()));

    actionRotateMatrixMinus = new QAction(tr("Rotate &-90"), this);
	actionRotateMatrixMinus->setShortcut(tr("Ctrl+Alt+R"));
	connect(actionRotateMatrixMinus, SIGNAL(activated()), this, SLOT(rotateMatrixMinus90()));

	actionInvertMatrix = new QAction(tr("&Invert"), this);
	connect(actionInvertMatrix, SIGNAL(activated()), this, SLOT(invertMatrix()));

	actionMatrixDeterminant = new QAction(tr("&Determinant"), this);
	connect(actionMatrixDeterminant, SIGNAL(activated()), this, SLOT(matrixDeterminant()));

	actionViewMatrixImage = new QAction(tr("&Image mode"), this);
	actionViewMatrixImage->setShortcut(tr("Ctrl+Shift+I"));
	connect(actionViewMatrixImage, SIGNAL(activated()), this, SLOT(viewMatrixImage()));
	actionViewMatrixImage->setCheckable(true);

	actionViewMatrix = new QAction(tr("&Data mode"), this);
	actionViewMatrix->setShortcut(tr("Ctrl+Shift+D"));
	connect(actionViewMatrix, SIGNAL(activated()), this, SLOT(viewMatrixTable()));
	actionViewMatrix->setCheckable(true);

    actionMatrixXY = new QAction(tr("Show &X/Y"), this);
	actionMatrixXY->setShortcut(tr("Ctrl+Shift+X"));
	connect(actionMatrixXY, SIGNAL(activated()), this, SLOT(viewMatrixXY()));
	actionMatrixXY->setCheckable(true);

    actionMatrixColumnRow = new QAction(tr("Show &Column/Row"), this);
	actionMatrixColumnRow->setShortcut(tr("Ctrl+Shift+C"));
	connect(actionMatrixColumnRow, SIGNAL(activated()), this, SLOT(viewMatrixColumnRow()));
	actionMatrixColumnRow->setCheckable(true);

    actionMatrixGrayScale = new QAction(tr("&Gray Scale"), this);
	connect(actionMatrixGrayScale, SIGNAL(activated()), this, SLOT(setMatrixGrayScale()));
	actionMatrixGrayScale->setCheckable(true);

	actionMatrixRainbowScale = new QAction(tr("&Rainbow"), this);
	connect(actionMatrixRainbowScale, SIGNAL(activated()), this, SLOT(setMatrixRainbowScale()));
	actionMatrixRainbowScale->setCheckable(true);

	actionMatrixCustomScale = new QAction(tr("&Custom"), this);
	connect(actionMatrixCustomScale, SIGNAL(activated()), this, SLOT(showColorMapDialog()));
	actionMatrixCustomScale->setCheckable(true);

	actionExportMatrix = new QAction(tr("&Export Image ..."), this);
	connect(actionExportMatrix, SIGNAL(activated()), this, SLOT(exportMatrix()));

	actionConvertMatrix = new QAction(tr("&Convert to Spreadsheet"), this);
	connect(actionConvertMatrix, SIGNAL(activated()), this, SLOT(convertMatrixToTable()));

    actionMatrixFFTDirect = new QAction(tr("&Forward FFT"), this);
	connect(actionMatrixFFTDirect, SIGNAL(activated()), this, SLOT(matrixDirectFFT()));

	actionMatrixFFTInverse = new QAction(tr("&Inverse FFT"), this);
	connect(actionMatrixFFTInverse, SIGNAL(activated()), this, SLOT(matrixInverseFFT()));

	actionConvertTable= new QAction(tr("Convert to &Matrix"), this);
	connect(actionConvertTable, SIGNAL(activated()), this, SLOT(convertTableToMatrix()));

	actionPlot3DWireFrame = new QAction(QIcon(QPixmap(lineMesh_xpm)), tr("3D &Wire Frame"), this);
	connect(actionPlot3DWireFrame, SIGNAL(activated()), this, SLOT(plot3DWireframe()));

	actionPlot3DHiddenLine = new QAction(QIcon(QPixmap(grid_only_xpm)), tr("3D &Hidden Line"), this);
	connect(actionPlot3DHiddenLine, SIGNAL(activated()), this, SLOT(plot3DHiddenLine()));

	actionPlot3DPolygons = new QAction(QIcon(QPixmap(no_grid_xpm)), tr("3D &Polygons"), this);
	connect(actionPlot3DPolygons, SIGNAL(activated()), this, SLOT(plot3DPolygons()));

	actionPlot3DWireSurface = new QAction(QIcon(QPixmap(grid_poly_xpm)), tr("3D Wire &Surface"), this);
	connect(actionPlot3DWireSurface, SIGNAL(activated()), this, SLOT(plot3DWireSurface()));

	actionColorMap = new QAction(QIcon(QPixmap(color_map_xpm)), tr("Contour - &Color Fill"), this);
	connect(actionColorMap, SIGNAL(activated()), this, SLOT(plotColorMap()));

	actionContourMap = new QAction(QIcon(QPixmap(contour_map_xpm)), tr("Contour &Lines"), this);
	connect(actionContourMap, SIGNAL(activated()), this, SLOT(plotContour()));

	actionGrayMap = new QAction(QIcon(QPixmap(gray_map_xpm)), tr("&Gray Scale Map"), this);
	connect(actionGrayMap, SIGNAL(activated()), this, SLOT(plotGrayScale()));

	actionSortTable = new QAction(tr("Sort Ta&ble"), this);
	connect(actionSortTable, SIGNAL(activated()), this, SLOT(sortActiveTable()));

	actionSortSelection = new QAction(tr("Sort Columns"), this);
	connect(actionSortSelection, SIGNAL(activated()), this, SLOT(sortSelection()));

	actionNormalizeTable = new QAction(tr("&Table"), this);
	connect(actionNormalizeTable, SIGNAL(activated()), this, SLOT(normalizeActiveTable()));

	actionNormalizeSelection = new QAction(tr("&Columns"), this);
	connect(actionNormalizeSelection, SIGNAL(activated()), this, SLOT(normalizeSelection()));

	actionCorrelate = new QAction(tr("Co&rrelate"), this);
	connect(actionCorrelate, SIGNAL(activated()), this, SLOT(correlate()));

	actionAutoCorrelate = new QAction(tr("&Autocorrelate"), this);
	connect(actionAutoCorrelate, SIGNAL(activated()), this, SLOT(autoCorrelate()));

	actionConvolute = new QAction(tr("&Convolute"), this);
	connect(actionConvolute, SIGNAL(activated()), this, SLOT(convolute()));

	actionDeconvolute = new QAction(tr("&Deconvolute"), this);
	connect(actionDeconvolute, SIGNAL(activated()), this, SLOT(deconvolute()));

	actionTranslateHor = new QAction(tr("&Horizontal"), this);
	connect(actionTranslateHor, SIGNAL(activated()), this, SLOT(translateCurveHor()));

	actionTranslateVert = new QAction(tr("&Vertical"), this);
	connect(actionTranslateVert, SIGNAL(activated()), this, SLOT(translateCurveVert()));

	actionSetAscValues = new QAction(QIcon(QPixmap(rowNumbers_xpm)),tr("Ro&w Numbers"), this);
	connect(actionSetAscValues, SIGNAL(activated()), this, SLOT(setAscValues()));

	actionSetRandomValues = new QAction(QIcon(QPixmap(randomNumbers_xpm)),tr("&Random Values"), this);
	connect(actionSetRandomValues, SIGNAL(activated()), this, SLOT(setRandomValues()));

    actionReadOnlyCol = new QAction(tr("&Read Only"), this);
    connect(actionReadOnlyCol, SIGNAL(activated()), this, SLOT(setReadOnlyCol()));

	actionSetXCol = new QAction(QIcon(QPixmap(x_col_xpm)), tr("&X"), this);
	connect(actionSetXCol, SIGNAL(activated()), this, SLOT(setXCol()));

	actionSetYCol = new QAction(QIcon(QPixmap(y_col_xpm)), tr("&Y"), this);
	connect(actionSetYCol, SIGNAL(activated()), this, SLOT(setYCol()));

	actionSetZCol = new QAction(QIcon(QPixmap(z_col_xpm)), tr("&Z"), this);
	connect(actionSetZCol, SIGNAL(activated()), this, SLOT(setZCol()));

	actionSetXErrCol = new QAction(tr("X E&rror"), this);
	connect(actionSetXErrCol, SIGNAL(activated()), this, SLOT(setXErrCol()));

	actionSetYErrCol = new QAction(QIcon(QPixmap(errors_xpm)), tr("Y &Error"), this);
	connect(actionSetYErrCol, SIGNAL(activated()), this, SLOT(setYErrCol()));

	actionDisregardCol = new QAction(QIcon(QPixmap(disregard_col_xpm)), tr("&Disregard"), this);
	connect(actionDisregardCol, SIGNAL(activated()), this, SLOT(disregardCol()));

	actionBoxPlot = new QAction(QIcon(QPixmap(boxPlot_xpm)),tr("&Box Plot"), this);
	connect(actionBoxPlot, SIGNAL(activated()), this, SLOT(plotBoxDiagram()));

	actionMultiPeakGauss = new QAction(tr("&Gaussian..."), this);
	connect(actionMultiPeakGauss, SIGNAL(activated()), this, SLOT(fitMultiPeakGauss()));

	actionMultiPeakLorentz = new QAction(tr("&Lorentzian..."), this);
	connect(actionMultiPeakLorentz, SIGNAL(activated()), this, SLOT(fitMultiPeakLorentz()));

	actionCheckUpdates = new QAction(tr("Search for &Updates"), this);
	connect(actionCheckUpdates, SIGNAL(activated()), this, SLOT(searchForUpdates()));

	actionHomePage = new QAction(tr("&QtiPlot Homepage"), this);
	connect(actionHomePage, SIGNAL(activated()), this, SLOT(showHomePage()));

	actionHelpForums = new QAction(tr("QtiPlot &Forums"), this);
	connect(actionHelpForums, SIGNAL(triggered()), this, SLOT(showForums()));

	actionHelpBugReports = new QAction(tr("Report a &Bug"), this);
	connect(actionHelpBugReports, SIGNAL(triggered()), this, SLOT(showBugTracker()));

	actionDownloadManual = new QAction(tr("Download &Manual"), this);
	connect(actionDownloadManual, SIGNAL(activated()), this, SLOT(downloadManual()));

	actionTranslations = new QAction(tr("&Translations"), this);
	connect(actionTranslations, SIGNAL(activated()), this, SLOT(downloadTranslation()));

	actionDonate = new QAction(tr("Make a &Donation"), this);
	connect(actionDonate, SIGNAL(activated()), this, SLOT(showDonationsPage()));

	actionTechnicalSupport = new QAction(tr("Technical &Support"), this);
	connect(actionTechnicalSupport, SIGNAL(activated()), this, SLOT(showSupportPage()));

#ifdef SCRIPTING_DIALOG
	actionScriptingLang = new QAction(tr("Scripting &language"), this);
	connect(actionScriptingLang, SIGNAL(activated()), this, SLOT(showScriptingLangDialog()));
#endif

	actionRestartScripting = new QAction(tr("&Restart scripting"), this);
	connect(actionRestartScripting, SIGNAL(activated()), this, SLOT(restartScriptingEnv()));

	actionNoteExecute = new QAction(tr("E&xecute"), this);
	actionNoteExecute->setShortcut(tr("Ctrl+J"));

	actionNoteExecuteAll = new QAction(tr("Execute &All"), this);
	actionNoteExecuteAll->setShortcut(tr("Ctrl+Shift+J"));

	actionNoteEvaluate = new QAction(tr("&Evaluate Expression"), this);
	actionNoteEvaluate->setShortcut(tr("Ctrl+Return"));

#ifdef SCRIPTING_PYTHON
	actionShowScriptWindow = new QAction(QPixmap(python_xpm), tr("&Script Window"), this);
	actionShowScriptWindow->setShortcut(tr("F3"));
	actionShowScriptWindow->setToggleAction( true );
	connect(actionShowScriptWindow, SIGNAL(activated()), this, SLOT(showScriptWindow()));
#endif

	actionShowCurvePlotDialog = new QAction(tr("&Plot details..."), this);
	connect(actionShowCurvePlotDialog, SIGNAL(activated()), this, SLOT(showCurvePlotDialog()));

	actionShowCurveWorksheet = new QAction(tr("&Worksheet"), this);
	connect(actionShowCurveWorksheet, SIGNAL(activated()), this, SLOT(showCurveWorksheet()));

	actionCurveFullRange = new QAction(tr("&Reset to Full Range"), this);
	connect(actionCurveFullRange, SIGNAL(activated()), this, SLOT(setCurveFullRange()));

	actionEditCurveRange = new QAction(tr("Edit &Range..."), this);
	connect(actionEditCurveRange, SIGNAL(activated()), this, SLOT(showCurveRangeDialog()));

	actionRemoveCurve = new QAction(QPixmap(close_xpm), tr("&Delete"), this);
	connect(actionRemoveCurve, SIGNAL(activated()), this, SLOT(removeCurve()));

	actionHideCurve = new QAction(tr("&Hide"), this);
	connect(actionHideCurve, SIGNAL(activated()), this, SLOT(hideCurve()));

	actionHideOtherCurves = new QAction(tr("Hide &Other Curves"), this);
	connect(actionHideOtherCurves, SIGNAL(activated()), this, SLOT(hideOtherCurves()));

	actionShowAllCurves = new QAction(tr("&Show All Curves"), this);
	connect(actionShowAllCurves, SIGNAL(activated()), this, SLOT(showAllCurves()));

	actionEditFunction = new QAction(tr("&Edit Function..."), this);
	connect(actionEditFunction, SIGNAL(activated()), this, SLOT(showFunctionDialog()));

	actionToolBars = new QAction(tr("Toolbars..."), this);
	actionToolBars->setShortcut(tr("Ctrl+Shift+T"));
	connect(actionToolBars, SIGNAL(activated()), this, SLOT(showToolBarsMenu()));

	actionFontBold = new QAction("B", this);
	actionFontBold->setToolTip(tr("Bold"));
	QFont font = appFont;
	font.setBold(true);
	actionFontBold->setFont(font);
	actionFontBold->setCheckable(true);
	connect(actionFontBold, SIGNAL(toggled(bool)), this, SLOT(setBoldFont(bool)));

	actionFontItalic = new QAction("It", this);
	actionFontItalic->setToolTip(tr("Italic"));
	font = appFont;
	font.setItalic(true);
	actionFontItalic->setFont(font);
	actionFontItalic->setCheckable(true);
	connect(actionFontItalic, SIGNAL(toggled(bool)), this, SLOT(setItalicFont(bool)));

	actionSuperscript = new QAction(QPixmap(exp_xpm), tr("Superscript"), this);
	connect(actionSuperscript, SIGNAL(activated()), this, SLOT(insertSuperscript()));
    actionSuperscript->setEnabled(false);

	actionSubscript = new QAction(QPixmap(index_xpm), tr("Subscript"), this);
	connect(actionSubscript, SIGNAL(activated()), this, SLOT(insertSubscript()));
	actionSubscript->setEnabled(false);

	actionUnderline = new QAction("U", this);
	actionUnderline->setToolTip(tr("Underline (Ctrl+U)"));
	actionUnderline->setShortcut(tr("Ctrl+U"));
    font = appFont;
	font.setUnderline(true);
	actionUnderline->setFont(font);
	connect(actionUnderline, SIGNAL(activated()), this, SLOT(underline()));
	actionUnderline->setEnabled(false);

	actionGreekSymbol = new QAction(QString(QChar(0x3B1)) + QString(QChar(0x3B2)), this);
	actionGreekSymbol->setToolTip(tr("Greek"));
	connect(actionGreekSymbol, SIGNAL(activated()), this, SLOT(insertGreekSymbol()));
}

void ApplicationWindow::translateActionsStrings()
{
    actionFontBold->setToolTip(tr("Bold"));
    actionFontItalic->setToolTip(tr("Italic"));
    actionUnderline->setStatusTip(tr("Underline (Ctrl+U)"));
	actionUnderline->setShortcut(tr("Ctrl+U"));
	actionGreekSymbol->setToolTip(tr("Greek"));

	actionShowCurvePlotDialog->setMenuText(tr("&Plot details..."));
	actionShowCurveWorksheet->setMenuText(tr("&Worksheet"));
	actionRemoveCurve->setMenuText(tr("&Delete"));
	actionEditFunction->setMenuText(tr("&Edit Function..."));

	actionCurveFullRange->setMenuText(tr("&Reset to Full Range"));
	actionEditCurveRange->setMenuText(tr("Edit &Range..."));
	actionHideCurve->setMenuText(tr("&Hide"));
	actionHideOtherCurves->setMenuText(tr("Hide &Other Curves"));
	actionShowAllCurves->setMenuText(tr("&Show All Curves"));

	actionNewProject->setMenuText(tr("New &Project"));
	actionNewProject->setToolTip(tr("Open a new project"));
	actionNewProject->setShortcut(tr("Ctrl+N"));

    actionNewFolder->setMenuText(tr("New Fol&der"));
	actionNewFolder->setToolTip(tr("Create a new folder"));
	actionNewFolder->setShortcut(Qt::Key_F7);

	actionNewGraph->setMenuText(tr("New &Graph"));
	actionNewGraph->setToolTip(tr("Create an empty 2D plot"));
	actionNewGraph->setShortcut(tr("Ctrl+G"));

	actionNewNote->setMenuText(tr("New &Note"));
	actionNewNote->setToolTip(tr("Create an empty note window"));

	actionNewTable->setMenuText(tr("New &Table"));
	actionNewTable->setShortcut(tr("Ctrl+T"));
	actionNewTable->setToolTip(tr("New table"));

	actionNewMatrix->setMenuText(tr("New &Matrix"));
	actionNewMatrix->setShortcut(tr("Ctrl+M"));
	actionNewMatrix->setToolTip(tr("New matrix"));

	actionNewFunctionPlot->setMenuText(tr("New &Function Plot"));
	actionNewFunctionPlot->setToolTip(tr("Create a new 2D function plot"));
	actionNewFunctionPlot->setShortcut(tr("Ctrl+F"));

	actionNewSurfacePlot->setMenuText(tr("New 3D &Surface Plot"));
	actionNewSurfacePlot->setToolTip(tr("Create a new 3D surface plot"));
	actionNewSurfacePlot->setShortcut(tr("Ctrl+ALT+Z"));

	actionOpen->setMenuText(tr("&Open"));
	actionOpen->setShortcut(tr("Ctrl+O"));
	actionOpen->setToolTip(tr("Open project"));

	actionLoadImage->setMenuText(tr("Open Image &File"));
	actionLoadImage->setShortcut(tr("Ctrl+I"));

	actionImportImage->setMenuText(tr("Import I&mage..."));

	actionSaveProject->setMenuText(tr("&Save Project"));
	actionSaveProject->setToolTip(tr("Save project"));
	actionSaveProject->setShortcut(tr("Ctrl+S"));

	actionSaveProjectAs->setMenuText(tr("Save Project &As..."));

	actionOpenTemplate->setMenuText(tr("Open Te&mplate..."));
	actionOpenTemplate->setToolTip(tr("Open template"));

	actionSaveTemplate->setMenuText(tr("Save As &Template..."));
	actionSaveTemplate->setToolTip(tr("Save window as template"));

	actionLoad->setMenuText(tr("&Import ASCII..."));
	actionLoad->setToolTip(tr("Import data file(s)"));
	actionLoad->setShortcut(tr("Ctrl+K"));

	actionUndo->setMenuText(tr("&Undo"));
	actionUndo->setToolTip(tr("Undo changes"));
	actionUndo->setShortcut(tr("Ctrl+Z"));

	actionRedo->setMenuText(tr("&Redo"));
	actionRedo->setToolTip(tr("Redo changes"));
	actionRedo->setShortcut(tr("Ctrl+R"));

	actionCopyWindow->setMenuText(tr("&Duplicate"));
	actionCopyWindow->setToolTip(tr("Duplicate window"));

	actionCutSelection->setMenuText(tr("Cu&t Selection"));
	actionCutSelection->setToolTip(tr("Cut selection"));
	actionCutSelection->setShortcut(tr("Ctrl+X"));

	actionCopySelection->setMenuText(tr("&Copy Selection"));
	actionCopySelection->setToolTip(tr("Copy selection"));
	actionCopySelection->setShortcut(tr("Ctrl+C"));

	actionPasteSelection->setMenuText(tr("&Paste Selection"));
	actionPasteSelection->setToolTip(tr("Paste selection"));
	actionPasteSelection->setShortcut(tr("Ctrl+V"));

	actionClearSelection->setMenuText(tr("&Delete Selection"));
	actionClearSelection->setToolTip(tr("Delete selection"));
	actionClearSelection->setShortcut(tr("Del","delete key"));

	actionShowExplorer->setMenuText(tr("Project &Explorer"));
	actionShowExplorer->setShortcut(tr("Ctrl+E"));
	actionShowExplorer->setToolTip(tr("Show project explorer"));

	actionShowLog->setMenuText(tr("Results &Log"));
	actionShowLog->setToolTip(tr("Show analysis results"));

#ifdef SCRIPTING_CONSOLE
	actionShowConsole->setMenuText(tr("&Console"));
	actionShowConsole->setToolTip(tr("Show Scripting console"));
#endif

#ifdef SCRIPTING_PYTHON
	actionShowScriptWindow->setMenuText(tr("&Script Window"));
	actionShowScriptWindow->setToolTip(tr("Script Window"));
	actionShowScriptWindow->setShortcut(tr("F3"));
#endif

	actionCustomActionDialog->setMenuText(tr("Add &Custom Script Action..."));

	actionAddLayer->setMenuText(tr("Add La&yer"));
	actionAddLayer->setToolTip(tr("Add Layer"));
	actionAddLayer->setShortcut(tr("ALT+L"));

	actionShowLayerDialog->setMenuText(tr("Arran&ge Layers"));
	actionShowLayerDialog->setToolTip(tr("Arrange Layers"));
	actionShowLayerDialog->setShortcut(tr("Shift+A"));

	actionAutomaticLayout->setMenuText(tr("Automatic Layout"));
	actionAutomaticLayout->setToolTip(tr("Automatic Layout"));

	actionExportGraph->setMenuText(tr("&Current"));
	actionExportGraph->setShortcut(tr("Alt+G"));
	actionExportGraph->setToolTip(tr("Export current graph"));

	actionExportAllGraphs->setMenuText(tr("&All"));
	actionExportAllGraphs->setShortcut(tr("Alt+X"));
	actionExportAllGraphs->setToolTip(tr("Export all graphs"));

    actionExportPDF->setMenuText(tr("&Export PDF"));
	actionExportPDF->setShortcut(tr("Ctrl+Alt+P"));
	actionExportPDF->setToolTip(tr("Export to PDF"));

	actionPrint->setMenuText(tr("&Print"));
	actionPrint->setShortcut(tr("Ctrl+P"));
	actionPrint->setToolTip(tr("Print window"));

	actionPrintAllPlots->setMenuText(tr("Print All Plo&ts"));
	actionShowExportASCIIDialog->setMenuText(tr("E&xport ASCII"));

	actionCloseAllWindows->setMenuText(tr("&Quit"));
	actionCloseAllWindows->setShortcut(tr("Ctrl+Q"));

	actionClearLogInfo->setMenuText(tr("Clear &Log Information"));
	actionDeleteFitTables->setMenuText(tr("Delete &Fit Tables"));

    actionToolBars->setMenuText(tr("Toolbars..."));
	actionToolBars->setShortcut(tr("Ctrl+Shift+T"));

	actionShowPlotWizard->setMenuText(tr("Plot &Wizard"));
	actionShowPlotWizard->setShortcut(tr("Ctrl+Alt+W"));

	actionShowConfigureDialog->setMenuText(tr("&Preferences..."));

	actionShowCurvesDialog->setMenuText(tr("Add/Remove &Curve..."));
	actionShowCurvesDialog->setShortcut(tr("ALT+C"));
	actionShowCurvesDialog->setToolTip(tr("Add curve to graph"));

	actionAddErrorBars->setMenuText(tr("Add &Error Bars..."));
	actionAddErrorBars->setToolTip(tr("Add Error Bars..."));
	actionAddErrorBars->setShortcut(tr("Ctrl+B"));

	actionAddFunctionCurve->setMenuText(tr("Add &Function..."));
	actionAddFunctionCurve->setToolTip(tr("Add Function..."));
	actionAddFunctionCurve->setShortcut(tr("Ctrl+Alt+F"));

	actionUnzoom->setMenuText(tr("&Rescale to Show All"));
	actionUnzoom->setShortcut(tr("Ctrl+Shift+R"));
	actionUnzoom->setToolTip(tr("Best fit"));

	actionNewLegend->setMenuText( tr("New &Legend"));
	actionNewLegend->setShortcut(tr("Ctrl+L"));
	actionNewLegend->setToolTip(tr("Add new legend"));

	actionTimeStamp->setMenuText(tr("Add Time Stamp"));
	actionTimeStamp->setShortcut(tr("Ctrl+ALT+T"));
	actionTimeStamp->setToolTip(tr("Date & time "));

	actionAddImage->setMenuText(tr("Add &Image"));
	actionAddImage->setToolTip(tr("Add Image"));
	actionAddImage->setShortcut(tr("ALT+I"));

	actionPlotL->setMenuText(tr("&Line"));
	actionPlotL->setToolTip(tr("Plot as line"));

	actionPlotP->setMenuText(tr("&Scatter"));
	actionPlotP->setToolTip(tr("Plot as symbols"));

	actionPlotLP->setMenuText(tr("Line + S&ymbol"));
	actionPlotLP->setToolTip(tr("Plot as line + symbols"));

	actionPlotVerticalDropLines->setMenuText(tr("Vertical &Drop Lines"));

	actionPlotSpline->setMenuText(tr("&Spline"));
	actionPlotVertSteps->setMenuText(tr("&Vertical Steps"));
	actionPlotHorSteps->setMenuText(tr("&Horizontal Steps"));

	actionPlotVerticalBars->setMenuText(tr("&Columns"));
	actionPlotVerticalBars->setToolTip(tr("Plot with vertical bars"));

	actionPlotHorizontalBars->setMenuText(tr("&Rows"));
	actionPlotHorizontalBars->setToolTip(tr("Plot with horizontal bars"));

	actionPlotArea->setMenuText(tr("&Area"));
	actionPlotArea->setToolTip(tr("Plot area"));

	actionPlotPie->setMenuText(tr("&Pie"));
	actionPlotPie->setToolTip(tr("Plot pie"));

	actionPlotVectXYXY->setMenuText(tr("&Vectors XYXY"));
	actionPlotVectXYXY->setToolTip(tr("Vectors XYXY"));

	actionPlotVectXYAM->setMenuText(tr("Vectors XY&AM"));
	actionPlotVectXYAM->setToolTip(tr("Vectors XYAM"));

	actionPlotHistogram->setMenuText( tr("&Histogram"));
	actionPlotStackedHistograms->setMenuText(tr("&Stacked Histogram"));
	actionPlot2VerticalLayers->setMenuText(tr("&Vertical 2 Layers"));
	actionPlot2HorizontalLayers->setMenuText(tr("&Horizontal 2 Layers"));
	actionPlot4Layers->setMenuText(tr("&4 Layers"));
	actionPlotStackedLayers->setMenuText(tr("&Stacked Layers"));

	actionPlot3DRibbon->setMenuText(tr("&Ribbon"));
	actionPlot3DRibbon->setToolTip(tr("Plot 3D ribbon"));

	actionPlot3DBars->setMenuText(tr("&Bars"));
	actionPlot3DBars->setToolTip(tr("Plot 3D bars"));

	actionPlot3DScatter->setMenuText(tr("&Scatter"));
	actionPlot3DScatter->setToolTip(tr("Plot 3D scatter"));

	actionPlot3DTrajectory->setMenuText(tr("&Trajectory"));
	actionPlot3DTrajectory->setToolTip(tr("Plot 3D trajectory"));

	actionColorMap->setMenuText(tr("Contour + &Color Fill"));
	actionColorMap->setToolTip(tr("Contour Lines + Color Fill"));

	actionContourMap->setMenuText(tr("Contour &Lines"));
	actionContourMap->setToolTip(tr("Contour Lines"));

	actionGrayMap->setMenuText(tr("&Gray Scale Map"));
	actionGrayMap->setToolTip(tr("Gray Scale Map"));

	actionShowColStatistics->setMenuText(tr("Statistics on &Columns"));
	actionShowColStatistics->setToolTip(tr("Selected columns statistics"));

	actionShowRowStatistics->setMenuText(tr("Statistics on &Rows"));
	actionShowRowStatistics->setToolTip(tr("Selected rows statistics"));
	actionShowIntDialog->setMenuText(tr("&Integrate ..."));
	actionInterpolate->setMenuText(tr("Inte&rpolate ..."));
	actionLowPassFilter->setMenuText(tr("&Low Pass..."));
	actionHighPassFilter->setMenuText(tr("&High Pass..."));
	actionBandPassFilter->setMenuText(tr("&Band Pass..."));
	actionBandBlockFilter->setMenuText(tr("&Band Block..."));
	actionFFT->setMenuText(tr("&FFT..."));
	actionSmoothSavGol->setMenuText(tr("&Savitzky-Golay..."));
	actionSmoothFFT->setMenuText(tr("&FFT Filter..."));
	actionSmoothAverage->setMenuText(tr("Moving Window &Average..."));
	actionDifferentiate->setMenuText(tr("&Differentiate"));
	actionFitLinear->setMenuText(tr("Fit &Linear"));
	actionShowFitPolynomDialog->setMenuText(tr("Fit &Polynomial ..."));
	actionShowExpDecayDialog->setMenuText(tr("&First Order ..."));
	actionShowTwoExpDecayDialog->setMenuText(tr("&Second Order ..."));
	actionShowExpDecay3Dialog->setMenuText(tr("&Third Order ..."));
	actionFitExpGrowth->setMenuText(tr("Fit Exponential Gro&wth ..."));
	actionFitSigmoidal->setMenuText(tr("Fit &Boltzmann (Sigmoidal)"));
	actionFitGauss->setMenuText(tr("Fit &Gaussian"));
	actionFitLorentz->setMenuText(tr("Fit Lorent&zian"));

	actionShowFitDialog->setMenuText(tr("Fit &Wizard..."));
	actionShowFitDialog->setShortcut(tr("Ctrl+Y"));

	actionShowPlotDialog->setMenuText(tr("&Plot ..."));
	actionShowScaleDialog->setMenuText(tr("&Scales..."));
	actionShowAxisDialog->setMenuText(tr("&Axes..."));
	actionShowGridDialog->setMenuText(tr("&Grid ..."));
	actionShowTitleDialog->setMenuText(tr("&Title ..."));
	actionShowColumnOptionsDialog->setMenuText(tr("Column &Options ..."));
	actionShowColumnOptionsDialog->setShortcut(tr("Ctrl+Alt+O"));
	actionShowColumnValuesDialog->setMenuText(tr("Set Column &Values ..."));
	actionShowColumnValuesDialog->setShortcut(tr("Alt+Q"));
	actionTableRecalculate->setMenuText(tr("Recalculate"));
	actionTableRecalculate->setShortcut(tr("Ctrl+Return"));
	actionSwapColumns->setMenuText(tr("&Swap columns"));
	actionSwapColumns->setToolTip(tr("Swap selected columns"));
	actionMoveColRight->setMenuText(tr("Move &Right"));
    actionMoveColRight->setToolTip(tr("Move Right"));
	actionMoveColLeft->setMenuText(tr("Move &Left"));
    actionMoveColLeft->setToolTip(tr("Move Left"));
	actionMoveColFirst->setMenuText(tr("Move to F&irst"));
	actionMoveColFirst->setToolTip(tr("Move to First"));
	actionMoveColLast->setMenuText(tr("Move to Las&t"));
    actionMoveColLast->setToolTip(tr("Move to Last"));
	actionShowColsDialog->setMenuText(tr("&Columns..."));
	actionShowRowsDialog->setMenuText(tr("&Rows..."));

	actionAbout->setMenuText(tr("&About QtiPlot"));
	actionAbout->setShortcut(tr("F1"));

	actionShowHelp->setMenuText(tr("&Help"));
	actionShowHelp->setShortcut(tr("Ctrl+H"));

	actionChooseHelpFolder->setMenuText(tr("&Choose Help Folder..."));
	actionRename->setMenuText(tr("&Rename Window"));

	actionCloseWindow->setMenuText(tr("Close &Window"));
	actionCloseWindow->setShortcut(tr("Ctrl+W"));

	actionAddColToTable->setMenuText(tr("Add Column"));
	actionAddColToTable->setToolTip(tr("Add Column"));

	actionClearTable->setMenuText(tr("Clear"));
	actionGoToRow->setMenuText(tr("&Go to Row..."));
	actionGoToRow->setShortcut(tr("Ctrl+Alt+G"));

	actionDeleteLayer->setMenuText(tr("&Remove Layer"));
	actionDeleteLayer->setShortcut(tr("Alt+R"));

	actionResizeActiveWindow->setMenuText(tr("Window &Geometry..."));
	actionHideActiveWindow->setMenuText(tr("&Hide Window"));
	actionShowMoreWindows->setMenuText(tr("More Windows..."));
	actionPixelLineProfile->setMenuText(tr("&View Pixel Line Profile"));
	actionIntensityTable->setMenuText(tr("&Intensity Table"));
	actionShowLineDialog->setMenuText(tr("&Properties"));
	actionShowImageDialog->setMenuText(tr("&Properties"));
	actionShowTextDialog->setMenuText(tr("&Properties"));
	actionActivateWindow->setMenuText(tr("&Activate Window"));
	actionMinimizeWindow->setMenuText(tr("Mi&nimize Window"));
	actionMaximizeWindow->setMenuText(tr("Ma&ximize Window"));
	actionHideWindow->setMenuText(tr("&Hide Window"));
	actionResizeWindow->setMenuText(tr("Re&size Window..."));
	actionPrintWindow->setMenuText(tr("&Print Window"));
	actionEditSurfacePlot->setMenuText(tr("&Surface..."));
	actionAdd3DData->setMenuText(tr("&Data Set..."));
	actionSetMatrixProperties->setMenuText(tr("Set &Properties..."));
	actionSetMatrixDimensions->setMenuText(tr("Set &Dimensions..."));
	actionSetMatrixValues->setMenuText(tr("Set &Values..."));
    actionImagePlot->setMenuText(tr("&Image Plot"));
	actionTransposeMatrix->setMenuText(tr("&Transpose"));
	actionRotateMatrix->setMenuText(tr("R&otate 90"));
    actionRotateMatrixMinus->setMenuText(tr("Rotate &-90"));
	actionFlipMatrixVertically->setMenuText(tr("Flip &V"));
	actionFlipMatrixHorizontally->setMenuText(tr("Flip &H"));
    actionMatrixXY->setMenuText(tr("Show &X/Y"));
    actionMatrixColumnRow->setMenuText(tr("Show &Column/Row"));
	actionViewMatrix->setMenuText(tr("&Data mode"));
	actionViewMatrixImage->setMenuText(tr("&Image mode"));
    actionMatrixGrayScale->setMenuText(tr("&Gray Scale"));
	actionMatrixRainbowScale->setMenuText(tr("&Rainbow"));
	actionMatrixCustomScale->setMenuText(tr("&Custom"));
	actionInvertMatrix->setMenuText(tr("&Invert"));
	actionMatrixDeterminant->setMenuText(tr("&Determinant"));
	actionConvertMatrix->setMenuText(tr("&Convert to Spreadsheet"));
	actionExportMatrix->setMenuText(tr("&Export Image ..."));

	actionConvertTable->setMenuText(tr("Convert to &Matrix"));
	actionPlot3DWireFrame->setMenuText(tr("3D &Wire Frame"));
	actionPlot3DHiddenLine->setMenuText(tr("3D &Hidden Line"));
	actionPlot3DPolygons->setMenuText(tr("3D &Polygons"));
	actionPlot3DWireSurface->setMenuText(tr("3D Wire &Surface"));
	actionSortTable->setMenuText(tr("Sort Ta&ble"));
	actionSortSelection->setMenuText(tr("Sort Columns"));
	actionNormalizeTable->setMenuText(tr("&Table"));
	actionNormalizeSelection->setMenuText(tr("&Columns"));
	actionCorrelate->setMenuText(tr("Co&rrelate"));
	actionAutoCorrelate->setMenuText(tr("&Autocorrelate"));
	actionConvolute->setMenuText(tr("&Convolute"));
	actionDeconvolute->setMenuText(tr("&Deconvolute"));
	actionTranslateHor->setMenuText(tr("&Horizontal"));
	actionTranslateVert->setMenuText(tr("&Vertical"));
	actionSetAscValues->setMenuText(tr("Ro&w Numbers"));
	actionSetRandomValues->setMenuText(tr("&Random Values"));
	actionSetXCol->setMenuText(tr("&X"));
	actionSetYCol->setMenuText(tr("&Y"));
	actionSetZCol->setMenuText(tr("&Z"));
	actionSetXErrCol->setMenuText(tr("X E&rror"));
	actionSetYErrCol->setMenuText(tr("Y &Error"));
	actionDisregardCol->setMenuText(tr("&Disregard"));
	actionReadOnlyCol->setMenuText(tr("&Read Only"));

	actionBoxPlot->setMenuText(tr("&Box Plot"));
	actionBoxPlot->setToolTip(tr("Box and whiskers plot"));

	actionMultiPeakGauss->setMenuText(tr("&Gaussian..."));
	actionMultiPeakLorentz->setMenuText(tr("&Lorentzian..."));
	actionHomePage->setMenuText(tr("&QtiPlot Homepage"));
	actionCheckUpdates->setMenuText(tr("Search for &Updates"));
	actionHelpForums->setText(tr("Visit QtiPlot &Forums"));
	actionHelpBugReports->setText(tr("Report a &Bug"));
	actionDownloadManual->setMenuText(tr("Download &Manual"));
	actionTranslations->setMenuText(tr("&Translations"));
	actionDonate->setMenuText(tr("Make a &Donation"));
	actionTechnicalSupport->setMenuText(tr("Technical &Support"));

#ifdef SCRIPTING_DIALOG
	actionScriptingLang->setMenuText(tr("Scripting &language"));
#endif
	actionRestartScripting->setMenuText(tr("&Restart scripting"));

	actionNoteExecute->setMenuText(tr("E&xecute"));
	actionNoteExecute->setShortcut(tr("Ctrl+J"));

	actionNoteExecuteAll->setMenuText(tr("Execute &All"));
	actionNoteExecuteAll->setShortcut(tr("Ctrl+Shift+J"));

	actionNoteEvaluate->setMenuText(tr("&Evaluate Expression"));
	actionNoteEvaluate->setShortcut(tr("Ctrl+Return"));

	btnPointer->setMenuText(tr("Disable &tools"));
	btnPointer->setToolTip( tr( "Pointer" ) );

	btnZoomIn->setMenuText(tr("&Zoom In"));
	btnZoomIn->setShortcut(tr("Ctrl++"));
	btnZoomIn->setToolTip(tr("Zoom In"));

	btnZoomOut->setMenuText(tr("Zoom &Out"));
	btnZoomOut->setShortcut(tr("Ctrl+-"));
	btnZoomOut->setToolTip(tr("Zoom Out"));

	btnCursor->setMenuText(tr("&Data Reader"));
	btnCursor->setShortcut(tr("CTRL+D"));
	btnCursor->setToolTip(tr("Data reader"));

	btnSelect->setMenuText(tr("&Select Data Range"));
	btnSelect->setShortcut(tr("ALT+S"));
	btnSelect->setToolTip(tr("Select data range"));

	btnPicker->setMenuText(tr("S&creen Reader"));
	btnPicker->setToolTip(tr("Screen reader"));

    actionDrawPoints->setMenuText(tr("&Draw Data Points"));
    actionDrawPoints->setToolTip(tr("Draw Data Points"));

	btnMovePoints->setMenuText(tr("&Move Data Points..."));
	btnMovePoints->setShortcut(tr("Ctrl+ALT+M"));
	btnMovePoints->setToolTip(tr("Move data points"));

	btnRemovePoints->setMenuText(tr("Remove &Bad Data Points..."));
	btnRemovePoints->setShortcut(tr("Alt+B"));
	btnRemovePoints->setToolTip(tr("Remove data points"));

	actionAddText->setMenuText(tr("Add &Text"));
	actionAddText->setToolTip(tr("Add Text"));
	actionAddText->setShortcut(tr("ALT+T"));

	btnArrow->setMenuText(tr("Draw &Arrow"));
	btnArrow->setShortcut(tr("CTRL+ALT+A"));
	btnArrow->setToolTip(tr("Draw arrow"));

	btnLine->setMenuText(tr("Draw &Line"));
	btnLine->setShortcut(tr("CTRL+ALT+L"));
	btnLine->setToolTip(tr("Draw line"));

	// FIXME: is setText necessary for action groups?
	//	coord->setText( tr( "Coordinates" ) );
	//	coord->setMenuText( tr( "&Coord" ) );
	//  coord->setStatusTip( tr( "Coordinates" ) );
	Box->setText( tr( "Box" ) );
	Box->setMenuText( tr( "Box" ) );
	Box->setToolTip( tr( "Box" ) );
	Box->setStatusTip( tr( "Box" ) );
	Frame->setText( tr( "Frame" ) );
	Frame->setMenuText( tr( "&Frame" ) );
	Frame->setToolTip( tr( "Frame" ) );
	Frame->setStatusTip( tr( "Frame" ) );
	None->setText( tr( "No Axes" ) );
	None->setMenuText( tr( "No Axes" ) );
	None->setToolTip( tr( "No axes" ) );
	None->setStatusTip( tr( "No axes" ) );

	front->setToolTip( tr( "Front grid" ) );
	back->setToolTip( tr( "Back grid" ) );
	right->setToolTip( tr( "Right grid" ) );
	left->setToolTip( tr( "Left grid" ) );
	ceil->setToolTip( tr( "Ceiling grid" ) );
	floor->setToolTip( tr( "Floor grid" ) );

	wireframe->setText( tr( "Wireframe" ) );
	wireframe->setMenuText( tr( "Wireframe" ) );
	wireframe->setToolTip( tr( "Wireframe" ) );
	wireframe->setStatusTip( tr( "Wireframe" ) );
	hiddenline->setText( tr( "Hidden Line" ) );
	hiddenline->setMenuText( tr( "Hidden Line" ) );
	hiddenline->setToolTip( tr( "Hidden line" ) );
	hiddenline->setStatusTip( tr( "Hidden line" ) );
	polygon->setText( tr( "Polygon Only" ) );
	polygon->setMenuText( tr( "Polygon Only" ) );
	polygon->setToolTip( tr( "Polygon only" ) );
	polygon->setStatusTip( tr( "Polygon only" ) );
	filledmesh->setText( tr( "Mesh & Filled Polygons" ) );
	filledmesh->setMenuText( tr( "Mesh & Filled Polygons" ) );
	filledmesh->setToolTip( tr( "Mesh & filled Polygons" ) );
	filledmesh->setStatusTip( tr( "Mesh & filled Polygons" ) );
	pointstyle->setText( tr( "Dots" ) );
	pointstyle->setMenuText( tr( "Dots" ) );
	pointstyle->setToolTip( tr( "Dots" ) );
	pointstyle->setStatusTip( tr( "Dots" ) );
	barstyle->setText( tr( "Bars" ) );
	barstyle->setMenuText( tr( "Bars" ) );
	barstyle->setToolTip( tr( "Bars" ) );
	barstyle->setStatusTip( tr( "Bars" ) );
	conestyle->setText( tr( "Cones" ) );
	conestyle->setMenuText( tr( "Cones" ) );
	conestyle->setToolTip( tr( "Cones" ) );
	conestyle->setStatusTip( tr( "Cones" ) );
	crossHairStyle->setText( tr( "Crosshairs" ) );
	crossHairStyle->setMenuText( tr( "Crosshairs" ) );
	crossHairStyle->setToolTip( tr( "Crosshairs" ) );
	crossHairStyle->setStatusTip( tr( "Crosshairs" ) );

	//floorstyle->setText( tr( "Floor Style" ) );
	//floorstyle->setMenuText( tr( "Floor Style" ) );
	//floorstyle->setStatusTip( tr( "Floor Style" ) );
	floordata->setText( tr( "Floor Data Projection" ) );
	floordata->setMenuText( tr( "Floor Data Projection" ) );
	floordata->setToolTip( tr( "Floor data projection" ) );
	floordata->setStatusTip( tr( "Floor data projection" ) );
	flooriso->setText( tr( "Floor Isolines" ) );
	flooriso->setMenuText( tr( "Floor Isolines" ) );
	flooriso->setToolTip( tr( "Floor isolines" ) );
	flooriso->setStatusTip( tr( "Floor isolines" ) );
	floornone->setText( tr( "Empty Floor" ) );
	floornone->setMenuText( tr( "Empty Floor" ) );
	floornone->setToolTip( tr( "Empty floor" ) );
	floornone->setStatusTip( tr( "Empty floor" ) );

	actionAnimate->setText( tr( "Animation" ) );
	actionAnimate->setMenuText( tr( "Animation" ) );
	actionAnimate->setToolTip( tr( "Animation" ) );
	actionAnimate->setStatusTip( tr( "Animation" ) );

	actionPerspective->setText( tr( "Enable perspective" ) );
	actionPerspective->setMenuText( tr( "Enable perspective" ) );
	actionPerspective->setToolTip( tr( "Enable perspective" ) );
	actionPerspective->setStatusTip( tr( "Enable perspective" ) );

	actionResetRotation->setText( tr( "Reset rotation" ) );
	actionResetRotation->setMenuText( tr( "Reset rotation" ) );
	actionResetRotation->setToolTip( tr( "Reset rotation" ) );
	actionResetRotation->setStatusTip( tr( "Reset rotation" ) );

	actionFitFrame->setText( tr( "Fit frame to window" ) );
	actionFitFrame->setMenuText( tr( "Fit frame to window" ) );
	actionFitFrame->setToolTip( tr( "Fit frame to window" ) );
	actionFitFrame->setStatusTip( tr( "Fit frame to window" ) );
}

Graph3D * ApplicationWindow::openMatrixPlot3D(const QString& caption, const QString& matrix_name,
		double xl,double xr,double yl,double yr,double zl,double zr)
{
	QString name = matrix_name;
	name.remove("matrix<", true);
	name.remove(">");
	Matrix* m = matrix(name);
	if (!m)
		return 0;

	Graph3D *plot = new Graph3D("", ws, 0, 0);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	plot->setWindowTitle(caption);
	plot->setName(caption);
	plot->addMatrixData(m, xl, xr, yl, yr, zl, zr);
	plot->update();

	initPlot3D(plot);
	return plot;
}

Graph3D * ApplicationWindow::plot3DMatrix(Matrix *m, int style)
{
	if (!m) {
		m = (Matrix*)ws->activeWindow();
		if (!m || !m->isA("Matrix"))
			return 0;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QString label = generateUniqueName(tr("Graph"));

	Graph3D *plot = new Graph3D("", ws, 0);
	plot->setAttribute(Qt::WA_DeleteOnClose);
	plot->addMatrixData(m);
	plot->customPlotStyle(style);
	customPlot3D(plot);
	plot->update();

	plot->resize(500, 400);
	plot->setWindowTitle(label);
	plot->setName(label);
	initPlot3D(plot);

	emit modified();
	QApplication::restoreOverrideCursor();
	return plot;
}

MultiLayer* ApplicationWindow::plotGrayScale(Matrix *m)
{
	if (!m) {
		m = (Matrix*)ws->activeWindow();
		if (!m || !m->isA("Matrix"))
			return 0;
	}

	return plotSpectrogram(m, Graph::GrayScale);
}

MultiLayer* ApplicationWindow::plotContour(Matrix *m)
{
	if (!m) {
		m = (Matrix*)ws->activeWindow();
		if (!m || !m->isA("Matrix"))
			return 0;
	}

	return plotSpectrogram(m, Graph::Contour);
}

MultiLayer* ApplicationWindow::plotColorMap(Matrix *m)
{
	if (!m) {
		m = (Matrix*)ws->activeWindow();
		if (!m || !m->isA("Matrix"))
			return 0;
	}

	return plotSpectrogram(m, Graph::ColorMap);
}

MultiLayer* ApplicationWindow::plotImage(Matrix *m)
{
    if (!m) {
		m = (Matrix*)ws->activeWindow();
		if (!m || !m->isA("Matrix"))
			return 0;
	}

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    MultiLayer* g = new MultiLayer("", ws, 0);
	g->setAttribute(Qt::WA_DeleteOnClose);

	Graph* plot = g->addLayer();
	setPreferences(plot);

	Spectrogram *s = plot->plotSpectrogram(m, Graph::GrayScale);
	if (!s)
		return 0;

	s->setAxis(QwtPlot::xTop, QwtPlot::yLeft);
	plot->enableAxis(QwtPlot::xTop, true);
	plot->setScale(QwtPlot::xTop, QMIN(m->xStart(), m->xEnd()), QMAX(m->xStart(), m->xEnd()));
	plot->enableAxis(QwtPlot::xBottom, false);
	plot->enableAxis(QwtPlot::yRight, false);
	plot->setScale(QwtPlot::yLeft, QMIN(m->yStart(), m->yEnd()), QMAX(m->yStart(), m->yEnd()),
					0.0, 5, 5, Graph::Linear, true);
	plot->setAxisTitle(QwtPlot::yLeft, QString::null);
	plot->setAxisTitle(QwtPlot::xTop, QString::null);
	plot->setTitle(QString::null);

    initMultilayerPlot(g, generateUniqueName(tr("Graph")));
    g->setMargins(5, 5, 5, 5);
	g->arrangeLayers(true, false);

	emit modified();
	QApplication::restoreOverrideCursor();
	return g;
}

MultiLayer* ApplicationWindow::plotSpectrogram(Matrix *m, Graph::CurveType type)
{
	if (type == Graph::ImagePlot)
		return plotImage(m);
	else if (type == Graph::Histogram)
		return plotHistogram(m);

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	MultiLayer* g = multilayerPlot(generateUniqueName(tr("Graph")));
	Graph* plot = g->addLayer();
	setPreferences(plot);

	plot->plotSpectrogram(m, type);
	g->showNormal();

	emit modified();
	QApplication::restoreOverrideCursor();
	return g;
}

ApplicationWindow* ApplicationWindow::importOPJ(const QString& filename)
{
    if (filename.endsWith(".opj", Qt::CaseInsensitive) || filename.endsWith(".ogg", Qt::CaseInsensitive))
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        ApplicationWindow *app = new ApplicationWindow();
        app->applyUserSettings();
        app->setWindowTitle("QtiPlot - " + filename);
		app->restoreApplicationGeometry();
        app->projectname = filename;
        app->recentProjects.remove(filename);
        app->recentProjects.push_front(filename);
        app->updateRecentProjectsList();

        ImportOPJ(app, filename);

        QApplication::restoreOverrideCursor();
        return app;
    }
    else if (filename.endsWith(".ogm", Qt::CaseInsensitive) || filename.endsWith(".ogw", Qt::CaseInsensitive))
    {
		ImportOPJ(this, filename);
        recentProjects.remove(filename);
        recentProjects.push_front(filename);
        updateRecentProjectsList();
        return this;
    }
	return 0;
}

void ApplicationWindow::deleteFitTables()
{
	QList<QWidget*>* mLst = new QList<QWidget*>();
	QList<QWidget*> *windows = windowsList();
	for (int i = 0; i < int(windows->count());i++ )
	{
		if (windows->at(i)->isA("MultiLayer"))
			mLst->append(windows->at(i));
	}
	delete windows;

	foreach(QWidget *ml, *mLst){
		if (ml->isA("MultiLayer")){
			QWidgetList lst = ((MultiLayer*)ml)->graphPtrs();
			foreach(QWidget *widget, lst){
				QList<QwtPlotCurve *> curves = ((Graph *)widget)->fitCurvesList();
				foreach(QwtPlotCurve *c, curves){
					if (((PlotCurve *)c)->type() != Graph::Function){
						Table *t = ((DataCurve *)c)->table();
						if (!t)
							continue;

						t->askOnCloseEvent(false);
						t->close();
					}
				}
			}
		}
	}
	delete mLst;
}

QWidgetList* ApplicationWindow::windowsList()
{
	QWidgetList *lst = new QWidgetList;

    Folder *project_folder = projectFolder();
	FolderListItem *item = project_folder->folderListItem();
	int initial_depth = item->depth();
	while (item && item->depth() >= initial_depth){
		QList<MyWidget *> folderWindows = item->folder()->windowsList();
		foreach(MyWidget *w, folderWindows)
			lst->append(w);
		item = (FolderListItem *)item->itemBelow();
	}

	foreach(QWidget *w, *outWindows)
		lst->append(w);

	return lst;
}

void ApplicationWindow::updateRecentProjectsList()
{
    if (recentProjects.isEmpty())
        return;

	while ((int)recentProjects.size() > MaxRecentProjects)
		recentProjects.pop_back();

	recent->clear();

	for (int i = 0; i<(int)recentProjects.size(); i++ )
		recent->insertItem("&" + QString::number(i+1) + " " + recentProjects[i]);
}

void ApplicationWindow::translateCurveHor()
{
	QWidget *w=ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	MultiLayer *plot = (MultiLayer*)w;
	if (plot->isEmpty())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g)
		return;

	if (g->isPiePlot())
	{
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("This functionality is not available for pie plots!"));

		btnPointer->setChecked(true);
		return;
	}
	else if (g->validCurvesDataSize())
	{
		btnPointer->setChecked(true);
		g->setActiveTool(new TranslateCurveTool(g, this, TranslateCurveTool::Horizontal, info, SLOT(setText(const QString&))));
		displayBar->show();
	}
}

void ApplicationWindow::translateCurveVert()
{
	QWidget *w=ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	MultiLayer *plot = (MultiLayer*)w;
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g)
		return;

	if (g->isPiePlot()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("This functionality is not available for pie plots!"));

		btnPointer->setChecked(true);
		return;
	} else if (g->validCurvesDataSize()) {
		btnPointer->setChecked(true);
		g->setActiveTool(new TranslateCurveTool(g, this, TranslateCurveTool::Vertical, info, SLOT(setText(const QString&))));
		displayBar->show();
	}
}

void ApplicationWindow::setReadOnlyCol()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table *)ws->activeWindow();
    QStringList list = t->selectedColumns();
	for (int i=0; i<(int) list.count(); i++)
		t->setReadOnlyColumn(t->colIndex(list[i]), actionReadOnlyCol->isChecked());
}

void ApplicationWindow::setReadOnlyColumns()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table *)ws->activeWindow();
    QStringList list = t->selectedColumns();
	for (int i=0; i<(int) list.count(); i++)
		t->setReadOnlyColumn(t->colIndex(list[i]));
}

void ApplicationWindow::setReadWriteColumns()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = (Table *)ws->activeWindow();
    QStringList list = t->selectedColumns();
	for (int i=0; i<(int) list.count(); i++)
		t->setReadOnlyColumn(t->colIndex(list[i]), false);
}

void ApplicationWindow::setAscValues()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setAscValues();
}

void ApplicationWindow::setRandomValues()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setRandomValues();
}

void ApplicationWindow::setXErrCol()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setPlotDesignation(Table::xErr);
}

void ApplicationWindow::setYErrCol()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setPlotDesignation(Table::yErr);
}

void ApplicationWindow::setXCol()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setPlotDesignation(Table::X);
}

void ApplicationWindow::setYCol()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setPlotDesignation(Table::Y);
}

void ApplicationWindow::setZCol()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setPlotDesignation(Table::Z);
}

void ApplicationWindow::disregardCol()
{
	if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	((Table *)ws->activeWindow())->setPlotDesignation(Table::None);
}

void ApplicationWindow::fitMultiPeakGauss()
{
	fitMultiPeak((int)MultiPeakFit::Gauss);
}

void ApplicationWindow::fitMultiPeakLorentz()
{
	fitMultiPeak((int)MultiPeakFit::Lorentz);
}

void ApplicationWindow::fitMultiPeak(int profile)
{
	QWidget *w=ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	MultiLayer *plot = (MultiLayer*)w;
	if (plot->isEmpty()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("<h4>There are no plot layers available in this window.</h4>"
					"<p><h4>Please add a layer and try again!</h4>"));
		btnPointer->setChecked(true);
		return;
	}

	Graph* g = (Graph*)plot->activeGraph();
	if (!g || !g->validCurvesDataSize())
		return;

	if (g->isPiePlot()){
		QMessageBox::warning(this,tr("QtiPlot - Warning"),
				tr("This functionality is not available for pie plots!"));
		return;
	} else {
		bool ok;
		int peaks = QInputDialog::getInteger(tr("QtiPlot - Enter the number of peaks"),
				tr("Peaks"), 2, 2, 1000000, 1, &ok, this);
		if (ok && peaks){
			g->setActiveTool(new MultiPeakFitTool(g, this, (MultiPeakFit::PeakProfile)profile, peaks, info, SLOT(setText(const QString&))));
			displayBar->show();
		}
	}
}

void ApplicationWindow::showSupportPage()
{
	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/contracts.html"));
}


void ApplicationWindow::showDonationsPage()
{
	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/why_donate.html"));
}

void ApplicationWindow::downloadManual()
{
	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/manuals.html"));
}

void ApplicationWindow::downloadTranslation()
{
	QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/translations.html"));
}

void ApplicationWindow::showHomePage()
{
	QDesktopServices::openUrl(QUrl("http://www.qtiplot.ro"));
}

void ApplicationWindow::showForums()
{
	QDesktopServices::openUrl(QUrl("https://developer.berlios.de/forum/?group_id=6626"));
}

void ApplicationWindow::showBugTracker()
{
	QDesktopServices::openUrl(QUrl("https://developer.berlios.de/bugs/?group_id=6626"));
}

void ApplicationWindow::showDonationDialog()
{
	if (askForSupport)
	{
		QString s= tr("<font size=+2, color = darkBlue><b>QtiPlot is open-source software and its development required hundreds of hours of work.<br><br>\
				If you like it, you're using it in your work and you would like to see it \
				constantly improved,<br> please support its authors by making a donation.<br><br> \
				Would you like to make a donation for QtiPlot now?</b></font>");
		switch( QMessageBox::information(this, tr("Please support QtiPlot!"), s,
					tr("Yes, I'd love to!"), tr("Ask me again later!"), tr("No, stop bothering me!"), 0, 1 ) )
		{
			case 0:
				showDonationsPage();
				break;

			case 1:
				break;

			case 2:
				askForSupport = false;
				break;
		}
	}
}

void ApplicationWindow::parseCommandLineArguments(const QStringList& args)
{
	int num_args = args.count();
	if(num_args == 0) return;

	QString str;
	bool exec = false;
	bool default_settings = false;
	foreach(str, args){
		if( (str == "-a" || str == "--about") ||
				(str == "-m" || str == "--manual") )
		{
			QMessageBox::critical(this, tr("QtiPlot - Error"),
			tr("<b> %1 </b>: This command line option must be used without other arguments!").arg(str));
		}
		else if( (str == "-d" || str == "--default-settings"))
		{
			default_settings = true;
		}
		else if (str == "-v" || str == "--version")
		{
			QString s = versionString() + "\n";
			s += QString(copyright_string) + "\n";
			s += tr("Released") + ": " + release_date + "\n";
			#ifdef Q_OS_WIN
                hide();
				QMessageBox::information(this, tr("QtiPlot") + " - " + tr("Version"), s);
			#else
				std::wcout << s.toStdWString();
			#endif
			exit(0);
		}
		else if (str == "-h" || str == "--help")
		{
			QString s = "\n" + tr("Usage") + ": ";
			s += "qtiplot [" + tr("options") + "] [" + tr("file") + "_" + tr("name") + "]\n\n";
			s += tr("Valid options are") + ":\n";
			s += "-a " + tr("or") + " --about: " + tr("show about dialog and exit") + "\n";
			s += "-d " + tr("or") + " --default-settings: " + tr("start QtiPlot with the default settings") + "\n";
			s += "-h " + tr("or") + " --help: " + tr("show command line options") + "\n";
			s += "-l=XX " + tr("or") + " --lang=XX: " + tr("start QtiPlot in language") + " XX ('en', 'fr', 'de', ...)\n";
			s += "-m " + tr("or") + " --manual: " + tr("show QtiPlot manual in a standalone window") + "\n";
			s += "-v " + tr("or") + " --version: " + tr("print QtiPlot version and release date") + "\n";
			s += "-x " + tr("or") + " --execute: " + tr("execute the script file given as argument") + "\n\n";
			s += "'" + tr("file") + "_" + tr("name") + "' " + tr("can be any .qti, qti.gz, .opj, .ogm, .ogw, .ogg, .py or ASCII file") + "\n";
			#ifdef Q_OS_WIN
                hide();
				QMessageBox::information(this, tr("QtiPlot") + " - " + tr("Help"), s);
			#else
				std::wcout << s.toStdWString();
			#endif
			exit(0);
		}
		else if (str.startsWith("--lang=") || str.startsWith("-l="))
		{
			QString locale = str.mid(str.find('=')+1);
			if (locales.contains(locale))
				switchToLanguage(locale);

			if (!locales.contains(locale))
				QMessageBox::critical(this, tr("QtiPlot - Error"),
						tr("<b> %1 </b>: Wrong locale option or no translation available!").arg(locale));
		}
		else if (str.startsWith("--execute") || str.startsWith("-x"))
			exec = true;
		else if (str.startsWith("-") || str.startsWith("--"))
		{
			QMessageBox::critical(this, tr("QtiPlot - Error"),
			tr("<b> %1 </b> unknown command line option!").arg(str) + "\n" + tr("Type %1 to see the list of the valid options.").arg("'qtiplot -h'"));
		}
	}

	QString file_name = args[num_args-1]; // last argument
	if(file_name.startsWith("-")) return; // no file name given

	if (!file_name.isEmpty()){
		QFileInfo fi(file_name);
		if (fi.isDir()){
			QMessageBox::critical(this, tr("QtiPlot - File openning error"),
					tr("<b>%1</b> is a directory, please specify a file name!").arg(file_name));
			return;
		} else if (!fi.isReadable()) {
			QMessageBox::critical(this, tr("QtiPlot - File openning error"),
					tr("You don't have the permission to open this file: <b>%1</b>").arg(file_name));
			return;
		} else if (!fi.exists()) {
			QMessageBox::critical(this, tr("QtiPlot - File openning error"),
					tr("The file: <b>%1</b> doesn't exist!").arg(file_name));
			return;
		}

		workingDir = fi.dirPath(true);
		saveSettings();//the recent projects must be saved

		ApplicationWindow *a;
		if (exec)
			a = loadScript(file_name, exec, default_settings);
		else
			a = open(file_name, default_settings);

		if (a){
			a->workingDir = workingDir;
			close();
		}
	}

}

void ApplicationWindow::createLanguagesList()
{
	appTranslator = new QTranslator(this);
	qtTranslator = new QTranslator(this);
	qApp->installTranslator(appTranslator);
	qApp->installTranslator(qtTranslator);

	QString qmPath = qApp->applicationDirPath() + "/translations";
	QDir dir(qmPath);
	QStringList fileNames = dir.entryList("qtiplot_*.qm");
	for (int i=0; i < (int)fileNames.size(); i++)
	{
		QString locale = fileNames[i];
		locale = locale.mid(locale.find('_')+1);
		locale.truncate(locale.find('.'));
		locales.push_back(locale);
	}
	locales.push_back("en");
	locales.sort();

	if (appLanguage != "en")
	{
		appTranslator->load("qtiplot_" + appLanguage, qmPath);
		qtTranslator->load("qt_" + appLanguage, qmPath+"/qt");
	}
}

void ApplicationWindow::switchToLanguage(int param)
{
	if (param < (int)locales.size())
		switchToLanguage(locales[param]);
}

void ApplicationWindow::switchToLanguage(const QString& locale)
{
	if (!locales.contains(locale) || appLanguage == locale)
		return;

	appLanguage = locale;
	if (locale == "en")
	{
		qApp->removeTranslator(appTranslator);
		qApp->removeTranslator(qtTranslator);
		delete appTranslator;
		delete qtTranslator;
		appTranslator = new QTranslator(this);
		qtTranslator = new QTranslator(this);
		qApp->installTranslator(appTranslator);
		qApp->installTranslator(qtTranslator);
	}
	else
	{
		QString qmPath = qApp->applicationDirPath() + "/translations";
		appTranslator->load("qtiplot_" + locale, qmPath);
		qtTranslator->load("qt_" + locale, qmPath+"/qt");
	}
	insertTranslatedStrings();
}

QStringList ApplicationWindow::matrixNames()
{
	QStringList names;
	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows){
		if (w->isA("Matrix"))
			names << w->objectName();
	}
	delete windows;
	return names;
}

bool ApplicationWindow::alreadyUsedName(const QString& label)
{
	QWidgetList *windows = windowsList();
	foreach(QWidget *w, *windows){
		if (w->objectName() == label){
			delete windows;
			return true;
		}
	}
	delete windows;
	return false;
}

bool ApplicationWindow::projectHas2DPlots()
{
	QWidgetList *windows = windowsList();
	bool hasPlots = false;
	foreach(QWidget *w, *windows){
		if (w->isA("MultiLayer")){
			hasPlots = true;
			break;
		}
	}
	delete windows;
	return hasPlots;
}

void ApplicationWindow::appendProject()
{
	OpenProjectDialog *open_dialog = new OpenProjectDialog(this, false);
	open_dialog->setDirectory(workingDir);
	open_dialog->setExtensionWidget(0);
	if (open_dialog->exec() != QDialog::Accepted || open_dialog->selectedFiles().isEmpty())
		return;
	workingDir = open_dialog->directory().path();
	appendProject(open_dialog->selectedFiles()[0]);
}

Folder* ApplicationWindow::appendProject(const QString& fn, Folder* parentFolder)
{
	if (fn.isEmpty())
		return 0;

	QFileInfo fi(fn);
	workingDir = fi.dirPath(true);

	if (fn.contains(".qti") || fn.contains(".opj", Qt::CaseInsensitive) ||
		fn.contains(".ogm", Qt::CaseInsensitive) || fn.contains(".ogw", Qt::CaseInsensitive) ||
        fn.contains(".ogg", Qt::CaseInsensitive)){
		QFileInfo f(fn);
		if (!f.exists ()){
			QMessageBox::critical(this, tr("QtiPlot - File opening error"),
					tr("The file: <b>%1</b> doesn't exist!").arg(fn));
			return 0;
		}
	}else{
		QMessageBox::critical(this,tr("QtiPlot - File opening error"),
				tr("The file: <b>%1</b> is not a QtiPlot or Origin project file!").arg(fn));
		return 0;
	}

    recentProjects.remove(fn);
    recentProjects.push_front(fn);
    updateRecentProjectsList();

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QString fname = fn;
	if (fn.contains(".qti.gz")){//decompress using zlib
		file_uncompress((char *)fname.ascii());
		fname.remove(".gz");
	}

	Folder *cf = current_folder;
	if (parentFolder)
		changeFolder(parentFolder, true);

	FolderListItem *item = (FolderListItem *)current_folder->folderListItem();
	folders->blockSignals (true);
	blockSignals (true);

	QString baseName = fi.baseName();
	QStringList lst = current_folder->subfolders();
	int n = lst.count(baseName);
	if (n){//avoid identical subfolder names
		while (lst.count(baseName + QString::number(n)))
			n++;
		baseName += QString::number(n);
	}

	Folder *new_folder;
	if (parentFolder)
		new_folder = new Folder(parentFolder, baseName);
	else
		new_folder = new Folder(current_folder, baseName);

	current_folder = new_folder;
	FolderListItem *fli = new FolderListItem(item, current_folder);
	current_folder->setFolderListItem(fli);

	if (fn.contains(".opj", Qt::CaseInsensitive) || fn.contains(".ogm", Qt::CaseInsensitive) ||
        fn.contains(".ogw", Qt::CaseInsensitive) || fn.contains(".ogg", Qt::CaseInsensitive))
		ImportOPJ(this, fn);
	else{
		QFile f(fname);
		QTextStream t( &f );
		t.setEncoding(QTextStream::UnicodeUTF8);
		f.open(QIODevice::ReadOnly);

		QString s = t.readLine();
		lst = s.split(QRegExp("\\s"), QString::SkipEmptyParts);
		QString version = lst[1];
		lst = version.split(".", QString::SkipEmptyParts);
		d_file_version =100*(lst[0]).toInt()+10*(lst[1]).toInt()+(lst[2]).toInt();

		t.readLine();
		if (d_file_version < 73)
			t.readLine();

		//process tables and matrix information
		while ( !t.atEnd()){
			s = t.readLine();
			lst.clear();
			if  (s.left(8) == "<folder>"){
				lst = s.split("\t");
				Folder *f = new Folder(current_folder, lst[1]);
				f->setBirthDate(lst[2]);
				f->setModificationDate(lst[3]);
				if(lst.count() > 4)
					if (lst[4] == "current")
						cf = f;

				FolderListItem *fli = new FolderListItem(current_folder->folderListItem(), f);
				fli->setText(0, lst[1]);
				f->setFolderListItem(fli);

				current_folder = f;
			}else if  (s == "<table>"){
				while ( s!="</table>" ){
					s=t.readLine();
					lst<<s;
				}
				lst.pop_back();
				openTable(this,lst);
			}else if  (s == "<matrix>"){
				while ( s != "</matrix>" ){
					s=t.readLine();
					lst<<s;
				}
				lst.pop_back();
				openMatrix(this, lst);
			}else if  (s == "<note>"){
				for (int i=0; i<3; i++){
					s = t.readLine();
					lst << s;
				}
				Note* m = openNote(this, lst);
				QStringList cont;
				while ( s != "</note>" ){
					s=t.readLine();
					cont << s;
				}
				cont.pop_back();
				m->restore(cont);
			}else if  (s == "</folder>"){
				Folder *parent = (Folder *)current_folder->parent();
				if (!parent)
					current_folder = projectFolder();
				else
					current_folder = parent;
			}
		}
		f.close();

		//process the rest
		f.open(QIODevice::ReadOnly);

		MultiLayer *plot=0;
		while ( !t.atEnd()){
			s=t.readLine();
			if  (s.left(8) == "<folder>"){
				lst = s.split("\t");
				current_folder = current_folder->findSubfolder(lst[1]);
			}else if  (s == "<multiLayer>"){//process multilayers information
				s=t.readLine();
				QStringList graph=s.split("\t");
				QString caption=graph[0];
				plot=multilayerPlot(caption);
				plot->setCols(graph[1].toInt());
				plot->setRows(graph[2].toInt());
				setListViewDate(caption, graph[3]);
				plot->setBirthDate(graph[3]);
				plot->blockSignals(true);

				restoreWindowGeometry(this, plot, t.readLine());

				if (d_file_version > 71){
					QStringList lst=t.readLine().split("\t");
					plot->setWindowLabel(lst[1]);
					setListViewLabel(plot->objectName(),lst[1]);
					plot->setCaptionPolicy((MyWidget::CaptionPolicy)lst[2].toInt());
				}

				if (d_file_version > 83){
					QStringList lst=t.readLine().split("\t", QString::SkipEmptyParts);
					plot->setMargins(lst[1].toInt(),lst[2].toInt(),lst[3].toInt(),lst[4].toInt());
					lst=t.readLine().split("\t", QString::SkipEmptyParts);
					plot->setSpacing(lst[1].toInt(),lst[2].toInt());
					lst=t.readLine().split("\t", QString::SkipEmptyParts);
					plot->setLayerCanvasSize(lst[1].toInt(),lst[2].toInt());
					lst=t.readLine().split("\t", QString::SkipEmptyParts);
					plot->setAlignement(lst[1].toInt(),lst[2].toInt());
				}

				while ( s!="</multiLayer>" ){//open layers
					s=t.readLine();
					if (s.left(7)=="<graph>"){
						lst.clear();
						while ( s!="</graph>" ){
							s=t.readLine();
							lst<<s;
						}
						openGraph(this, plot, lst);
					}
				}
				plot->blockSignals(false);
			}else if  (s == "<SurfacePlot>"){//process 3D plots information
				lst.clear();
				while ( s!="</SurfacePlot>" ){
					s=t.readLine();
					lst<<s;
				}
				openSurfacePlot(this,lst);
			}else if  (s == "</folder>"){
				Folder *parent = (Folder *)current_folder->parent();
				if (!parent)
					current_folder = projectFolder();
				else
					current_folder = parent;
			}
		}
		f.close();
	}

	folders->blockSignals (false);
	//change folder to user defined current folder
	changeFolder(cf);
	blockSignals (false);
	renamedTables = QStringList();
	QApplication::restoreOverrideCursor();
	return new_folder;
}

#ifdef QTIPLOT_DEMO
void ApplicationWindow::showDemoVersionMessage()
{
	QMessageBox::critical(this, tr("QtiPlot - Demo Version"),
			tr("You are using the demonstration version of Qtiplot.\
				It is identical with the full version, except that you can't save your work to project files and you can't use it for more than 10 minutes per session.\
				<br><br>\
				If you want to have ready-to-use, fully functional binaries, please subscribe for a\
				<a href=\"http://soft.proindependent.com/individual_contract.html\">single-user binaries maintenance contract</a>.\
				<br><br>\
				QtiPlot is free software in the sense of free speech.\
				If you know how to use it, you can get\
				<a href=\"http://developer.berlios.de/project/showfiles.php?group_id=6626\">the source code</a>\
				free of charge.\
				Nevertheless, you are welcome to\
				<a href=\"http://soft.proindependent.com/why_donate.html\">make a donation</a>\
				in order to support the further development of QtiPlot."));
}
#endif

void ApplicationWindow::saveFolder(Folder *folder, const QString& fn, bool compress)
{
	QFile f( fn );
	if (d_backup_files && f.exists())
	{// make byte-copy of current file so that there's always a copy of the data on disk
		while (!f.open(QIODevice::ReadOnly)){
			if (f.isOpen())
				f.close();
			int choice = QMessageBox::warning(this, tr("QtiPlot - File backup error"),
					tr("Cannot make a backup copy of <b>%1</b> (to %2).<br>If you ignore this, you run the risk of <b>data loss</b>.").arg(projectname).arg(projectname+"~"),
					QMessageBox::Retry|QMessageBox::Default, QMessageBox::Abort|QMessageBox::Escape, QMessageBox::Ignore);
			if (choice == QMessageBox::Abort)
				return;
			if (choice == QMessageBox::Ignore)
				break;
		}

		if (f.isOpen()){
            QFile::copy (fn, fn + "~");
			f.close();
		}
	}

	if ( !f.open( QIODevice::WriteOnly ) ){
		QMessageBox::about(this, tr("QtiPlot - File save error"), tr("The file: <br><b>%1</b> is opened in read-only mode").arg(fn));
		return;
	}
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	QList<MyWidget *> lst = folder->windowsList();
	int windows = 0;
	QString text;
	foreach(MyWidget *w, lst){
		text += w->saveToString(windowGeometryInfo(w));
		windows++;
	}

	FolderListItem *fi = folder->folderListItem();
	FolderListItem *item = (FolderListItem *)fi->firstChild();
	int opened_folders = 0;
	int initial_depth = fi->depth();
	while (item && item->depth() > initial_depth){
		Folder *dir = (Folder *)item->folder();
		text += "<folder>\t"+QString(dir->objectName())+"\t"+dir->birthDate()+"\t"+dir->modificationDate();
		if (dir == current_folder)
			text += "\tcurrent\n";
		else
			text += "\n";  // FIXME: Having no 5th string here is not a good idea

		lst = dir->windowsList();
		foreach(MyWidget *w, lst){
            text += w->saveToString(windowGeometryInfo(w));
			windows++;
		}

		if ( (dir->children()).isEmpty() )
			text += "</folder>\n";
		else
			opened_folders++;

		int depth = item->depth();
		item = (FolderListItem *)item->itemBelow();
		if (item && item->depth() < depth && item->depth() > initial_depth){
			text += "</folder>\n";
			opened_folders--;
		} else if (!item) {
			for (int i = 0; i<opened_folders; i++)
				text += "</folder>\n";
			opened_folders = 0;
		}
	}
	text += "<log>\n"+logInfo+"</log>";
	text.prepend("<windows>\t"+QString::number(windows)+"\n");
	text.prepend("<scripting-lang>\t"+QString(scriptEnv->name())+"\n");
	text.prepend("QtiPlot " + QString::number(maj_version)+"."+ QString::number(min_version)+"."+
			QString::number(patch_version)+" project file\n");

	QTextStream t( &f );
	t.setEncoding(QTextStream::UnicodeUTF8);
	t << text;
	f.close();

	if (compress)
		file_compress((char *)fn.ascii(), "wb9");

	QApplication::restoreOverrideCursor();
}

void ApplicationWindow::saveAsProject()
{
	saveFolderAsProject(current_folder);
}

void ApplicationWindow::saveFolderAsProject(Folder *f)
{
#ifdef QTIPLOT_DEMO
	showDemoVersionMessage();
	return;
#endif
	QString filter = tr("QtiPlot project")+" (*.qti);;";
	filter += tr("Compressed QtiPlot project")+" (*.qti.gz)";

	QString selectedFilter;
	QString fn = QFileDialog::getSaveFileName(this, tr("Save project as"), workingDir, filter, &selectedFilter);
	if ( !fn.isEmpty() ){
		QFileInfo fi(fn);
		workingDir = fi.dirPath(true);
		QString baseName = fi.fileName();
		if (!baseName.contains("."))
			fn.append(".qti");

		saveFolder(f, fn, selectedFilter.contains(".gz"));
	}
}

void ApplicationWindow::showFolderPopupMenu(Q3ListViewItem *it, const QPoint &p, int)
{
	showFolderPopupMenu(it, p, true);
}

void ApplicationWindow::showFolderPopupMenu(Q3ListViewItem *it, const QPoint &p, bool fromFolders)
{
	if (!it || folders->isRenaming())
		return;

	QMenu cm(this);
	QMenu window(this);
	QMenu viewWindowsMenu(this);
	viewWindowsMenu.setCheckable ( true );

	cm.insertItem(tr("&Find..."), this, SLOT(showFindDialogue()));
	cm.insertSeparator();
	cm.insertItem(tr("App&end Project..."), this, SLOT(appendProject()));
	if (((FolderListItem *)it)->folder()->parent())
		cm.insertItem(tr("Save &As Project..."), this, SLOT(saveAsProject()));
	else
		cm.insertItem(tr("Save Project &As..."), this, SLOT(saveProjectAs()));
	cm.insertSeparator();

	if (fromFolders && show_windows_policy != HideAll)
	{
		cm.insertItem(tr("&Show All Windows"), this, SLOT(showAllFolderWindows()));
		cm.insertItem(tr("&Hide All Windows"), this, SLOT(hideAllFolderWindows()));
		cm.insertSeparator();
	}

	if (((FolderListItem *)it)->folder()->parent())
	{
		cm.insertItem(QPixmap(close_xpm), tr("&Delete Folder"), this, SLOT(deleteFolder()), Qt::Key_F8);
		cm.insertItem(tr("&Rename"), this, SLOT(startRenameFolder()), Qt::Key_F2);
		cm.insertSeparator();
	}

	if (fromFolders)
	{
		window.addAction(actionNewTable);
		window.addAction(actionNewMatrix);
		window.addAction(actionNewNote);
		window.addAction(actionNewGraph);
		window.addAction(actionNewFunctionPlot);
		window.addAction(actionNewSurfacePlot);
		cm.insertItem(tr("New &Window"), &window);
	}

	cm.insertItem(QPixmap(newfolder_xpm), tr("New F&older"), this, SLOT(addFolder()), Qt::Key_F7);
	cm.insertSeparator();

	QStringList lst;
	lst << tr("&None") << tr("&Windows in Active Folder") << tr("Windows in &Active Folder && Subfolders");
	for (int i = 0; i < 3; ++i)
	{
		int id = viewWindowsMenu.insertItem(lst[i],this, SLOT( setShowWindowsPolicy( int ) ) );
		viewWindowsMenu.setItemParameter( id, i );
		viewWindowsMenu.setItemChecked( id, show_windows_policy == i );
	}
	cm.insertItem(tr("&View Windows"), &viewWindowsMenu);
	cm.insertSeparator();
	cm.insertItem(tr("&Properties..."), this, SLOT(folderProperties()));
	cm.exec(p);
}

void ApplicationWindow::setShowWindowsPolicy(int p)
{
	if (show_windows_policy == (ShowWindowsPolicy)p)
		return;

	show_windows_policy = (ShowWindowsPolicy)p;
	if (show_windows_policy == HideAll){
		QList<QWidget*> *lst = windowsList();
		foreach(QWidget *w, *lst){
			hiddenWindows->append(w);
			w->hide();
			setListView(w->objectName(), tr("Hidden"));
		}
		delete lst;
	} else
		showAllFolderWindows();
}

void ApplicationWindow::showFindDialogue()
{
	FindDialog *fd = new FindDialog(this);
	fd->setAttribute(Qt::WA_DeleteOnClose);
	fd->exec();
}

void ApplicationWindow::startRenameFolder()
{
	FolderListItem *fi = current_folder->folderListItem();
	if (!fi)
		return;

	disconnect(folders, SIGNAL(currentChanged(Q3ListViewItem *)), this, SLOT(folderItemChanged(Q3ListViewItem *)));
	fi->setRenameEnabled (0, true);
	fi->startRename (0);
}

void ApplicationWindow::startRenameFolder(Q3ListViewItem *item)
{
	if (!item || item == folders->firstChild())
		return;

	if (item->listView() == lv && item->rtti() == FolderListItem::RTTI) {
        disconnect(folders, SIGNAL(currentChanged(Q3ListViewItem *)), this, SLOT(folderItemChanged(Q3ListViewItem *)));
		current_folder = ((FolderListItem *)item)->folder();
		FolderListItem *it = current_folder->folderListItem();
		it->setRenameEnabled (0, true);
		it->startRename (0);
	} else {
		item->setRenameEnabled (0, true);
		item->startRename (0);
	}
}

void ApplicationWindow::renameFolder(Q3ListViewItem *it, int col, const QString &text)
{
	Q_UNUSED(col)

		if (!it)
			return;

	Folder *parent = (Folder *)current_folder->parent();
	if (!parent)//the parent folder is the project folder (it always exists)
		parent = projectFolder();

	while(text.isEmpty())
	{
		QMessageBox::critical(this,tr("QtiPlot - Error"), tr("Please enter a valid name!"));
		it->setRenameEnabled (0, true);
		it->startRename (0);
		return;
	}

	QStringList lst = parent->subfolders();
	lst.remove(current_folder->objectName());
	while(lst.contains(text)){
		QMessageBox::critical(this,tr("QtiPlot - Error"),
				tr("Name already exists!")+"\n"+tr("Please choose another name!"));

		it->setRenameEnabled (0, true);
		it->startRename (0);
		return;
	}

	current_folder->setObjectName(text);
	it->setRenameEnabled (0, false);
	connect(folders, SIGNAL(currentChanged(Q3ListViewItem *)),
			this, SLOT(folderItemChanged(Q3ListViewItem *)));
	folders->setCurrentItem(parent->folderListItem());//update the list views
}

void ApplicationWindow::showAllFolderWindows()
{
	QList<MyWidget *> lst = current_folder->windowsList();
	foreach(MyWidget *w, lst)
	{//force show all windows in current folder
		if (w)
		{
			updateWindowLists(w);
			switch (w->status())
			{
				case MyWidget::Hidden:
					w->showNormal();
					break;

				case MyWidget::Normal:
					w->showNormal();
					break;

				case MyWidget::Minimized:
					w->showMinimized();
					break;

				case MyWidget::Maximized:
					w->showMaximized();
					break;
			}
		}
	}

	if ( (current_folder->children()).isEmpty() )
		return;

	FolderListItem *fi = current_folder->folderListItem();
	FolderListItem *item = (FolderListItem *)fi->firstChild();
	int initial_depth = item->depth();
	while (item && item->depth() >= initial_depth)
	{// show/hide windows in all subfolders
		lst = ((Folder *)item->folder())->windowsList();
		foreach(MyWidget *w, lst){
			if (w && show_windows_policy == SubFolders){
				updateWindowLists(w);
				switch (w->status())
				{
					case MyWidget::Hidden:
						w->showNormal();
						break;

					case MyWidget::Normal:
						w->showNormal();
						break;

					case MyWidget::Minimized:
						w->showMinimized();
						break;

					case MyWidget::Maximized:
						w->showMaximized();
						break;
				}
			}
			else
				w->hide();
		}

		item = (FolderListItem *)item->itemBelow();
	}
}

void ApplicationWindow::hideAllFolderWindows()
{
	QList<MyWidget *> lst = current_folder->windowsList();
	foreach(MyWidget *w, lst)
		hideWindow(w);

	if ( (current_folder->children()).isEmpty() )
		return;

	if (show_windows_policy == SubFolders)
	{
		FolderListItem *fi = current_folder->folderListItem();
		FolderListItem *item = (FolderListItem *)fi->firstChild();
		int initial_depth = item->depth();
		while (item && item->depth() >= initial_depth)
		{
			lst = item->folder()->windowsList();
			foreach(MyWidget *w, lst)
				hideWindow(w);

			item = (FolderListItem *)item->itemBelow();
		}
	}
}

void ApplicationWindow::projectProperties()
{
	QString s = QString(current_folder->objectName()) + "\n\n";
	s += "\n\n\n";
	s += tr("Type") + ": " + tr("Project")+"\n\n";
	if (projectname != "untitled")
	{
		s += tr("Path") + ": " + projectname + "\n\n";

		QFileInfo fi(projectname);
		s += tr("Size") + ": " + QString::number(fi.size()) + " " + tr("bytes")+ "\n\n";
	}

	QList<QWidget*> *lst = windowsList();
	s += tr("Contents") + ": " + QString::number(lst->count()) + " " + tr("windows");
	delete lst;

	s += ", " + QString::number(current_folder->subfolders().count()) + " " + tr("folders") + "\n\n";
	s += "\n\n\n";

	if (projectname != "untitled")
	{
		QFileInfo fi(projectname);
		s += tr("Created") + ": " + fi.created().toString(Qt::LocalDate) + "\n\n";
		s += tr("Modified") + ": " + fi.lastModified().toString(Qt::LocalDate) + "\n\n";
	}
	else
		s += tr("Created") + ": " + current_folder->birthDate() + "\n\n";

	QMessageBox *mbox = new QMessageBox ( tr("Properties"), s, QMessageBox::NoIcon,
			QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

	mbox->setIconPixmap(QPixmap( qtiplot_logo_xpm ));
	mbox->show();
}

void ApplicationWindow::folderProperties()
{
	if (!current_folder->parent())
	{
		projectProperties();
		return;
	}

	QString s = QString(current_folder->objectName()) + "\n\n";
	s += "\n\n\n";
	s += tr("Type") + ": " + tr("Folder")+"\n\n";
	s += tr("Path") + ": " + current_folder->path() + "\n\n";
	s += tr("Size") + ": " + current_folder->sizeToString() + "\n\n";
	s += tr("Contents") + ": " + QString::number(current_folder->windowsList().count()) + " " + tr("windows");
	s += ", " + QString::number(current_folder->subfolders().count()) + " " + tr("folders") + "\n\n";
	//s += "\n\n\n";
	s += tr("Created") + ": " + current_folder->birthDate() + "\n\n";
	//s += tr("Modified") + ": " + current_folder->modificationDate() + "\n\n";

	QMessageBox *mbox = new QMessageBox ( tr("Properties"), s, QMessageBox::NoIcon,
			QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

	mbox->setIconPixmap(QPixmap( folder_open_xpm ));
	mbox->show();
}

void ApplicationWindow::addFolder()
{
    if (!explorerWindow->isVisible())
		explorerWindow->show();

	QStringList lst = current_folder->subfolders();
	QString name =  tr("New Folder");
	lst = lst.grep( name );
	if (!lst.isEmpty())
		name += " ("+ QString::number(lst.size()+1)+")";

	Folder *f = new Folder(current_folder, name);
	addFolderListViewItem(f);

	FolderListItem *fi = new FolderListItem(current_folder->folderListItem(), f);
	if (fi){
		f->setFolderListItem(fi);
		fi->setRenameEnabled (0, true);
		fi->startRename(0);
	}
}

Folder* ApplicationWindow::addFolder(QString name, Folder* parent)
{
    if(!parent){
		if (current_folder)
			parent = current_folder;
		else
        	parent = projectFolder();
	}

    QStringList lst = parent->subfolders();
    lst = lst.grep( name );
    if (!lst.isEmpty())
        name += " ("+ QString::number(lst.size()+1)+")";

    Folder *f = new Folder(parent, name);
    addFolderListViewItem(f);

    FolderListItem *fi = new FolderListItem(parent->folderListItem(), f);
    if (fi)
        f->setFolderListItem(fi);

    return f;
}

bool ApplicationWindow::deleteFolder(Folder *f)
{
    if (!f)
        return false;

	if (confirmCloseFolder && QMessageBox::information(this, tr("QtiPlot - Delete folder?"),
				tr("Delete folder '%1' and all the windows it contains?").arg(f->objectName()),
				tr("Yes"), tr("No"), 0, 0))
		return false;
	else {
		Folder *parent = projectFolder();
		if (current_folder){
			if (current_folder->parent())
				parent = (Folder *)current_folder->parent();
		}

		folders->blockSignals(true);

		FolderListItem *fi = f->folderListItem();
		foreach(MyWidget *w, f->windowsList())
            closeWindow(w);

		if ( !(f->children()).isEmpty() ){
			FolderListItem *item = (FolderListItem *)fi->firstChild();
			int initial_depth = item->depth();
			while (item && item->depth() >= initial_depth){
			    Folder *subFolder = (Folder *)item->folder();
			    if (subFolder){
                    foreach(MyWidget *w, subFolder->windowsList()){
                        removeWindowFromLists(w);
                        subFolder->removeWindow(w);
                        delete w;
                    }

                    FolderListItem *old_item = item;
                    item = (FolderListItem *)item->itemBelow();
                    delete subFolder;
                    delete old_item;
			    }
			}
		}

		delete f;
		delete fi;

		current_folder = parent;
		folders->setCurrentItem(parent->folderListItem());
		changeFolder(parent, true);
		folders->blockSignals(false);
		folders->setFocus();
		return true;
	}
}

void ApplicationWindow::deleteFolder()
{
	Folder *parent = (Folder *)current_folder->parent();
	if (!parent)
		parent = projectFolder();

	folders->blockSignals(true);

	if (deleteFolder(current_folder)){
		current_folder = parent;
		folders->setCurrentItem(parent->folderListItem());
		changeFolder(parent, true);
	}

	folders->blockSignals(false);
	folders->setFocus();
}

void ApplicationWindow::folderItemDoubleClicked(Q3ListViewItem *it)
{
	if (!it || it->rtti() != FolderListItem::RTTI)
		return;

	FolderListItem *item = ((FolderListItem *)it)->folder()->folderListItem();
	folders->setCurrentItem(item);
}

void ApplicationWindow::folderItemChanged(Q3ListViewItem *it)
{
	if (!it)
		return;

	it->setOpen(true);
	changeFolder (((FolderListItem *)it)->folder());
	folders->setFocus();
}

void ApplicationWindow::hideFolderWindows(Folder *f)
{
	QList<MyWidget *> lst = f->windowsList();
	foreach(MyWidget *w, lst)
		w->hide();

	if ( (f->children()).isEmpty() )
		return;

	FolderListItem *fi = f->folderListItem();
	FolderListItem *item = (FolderListItem *)fi->firstChild();
	int initial_depth = item->depth();
	while (item && item->depth() >= initial_depth){
		lst = item->folder()->windowsList();
		foreach(MyWidget *w, lst)
			w->hide();
		item = (FolderListItem *)item->itemBelow();
	}
}

bool ApplicationWindow::changeFolder(Folder *newFolder, bool force)
{
    if (!newFolder)
        return false;

	if (current_folder == newFolder && !force)
		return false;

    desactivateFolders();
	newFolder->folderListItem()->setActive(true);

    Folder *oldFolder = current_folder;
    MyWidget::Status old_active_window_state = MyWidget::Normal;
    MyWidget *old_active_window = oldFolder->activeWindow();
    if (old_active_window)
        old_active_window_state = old_active_window->status();

    MyWidget::Status active_window_state = MyWidget::Normal;
    MyWidget *active_window = newFolder->activeWindow();

    if (active_window)
        active_window_state = active_window->status();

    ws->blockSignals(true);
	hideFolderWindows(oldFolder);
	current_folder = newFolder;

	lv->clear();

	QObjectList folderLst = newFolder->children();
	if(!folderLst.isEmpty()){
		foreach(QObject *f, folderLst)
			addFolderListViewItem(static_cast<Folder *>(f));
	}

	QList<MyWidget *> lst = newFolder->windowsList();
	foreach(MyWidget *w, lst){
        w->blockSignals(true);
        if (!hiddenWindows->contains(w) && !outWindows->contains(w) && show_windows_policy != HideAll){
            //show only windows in the current folder which are not hidden by the user
            if(w->status() == MyWidget::Normal)
                w->showNormal();
            else if(w->status() == MyWidget::Minimized)
                w->showMinimized();
        } else
            w->setStatus(MyWidget::Hidden);

        addListViewItem(w);
	}

	if (!(newFolder->children()).isEmpty()){
        FolderListItem *fi = newFolder->folderListItem();
        FolderListItem *item = (FolderListItem *)fi->firstChild();
        int initial_depth = item->depth();
        while (item && item->depth() >= initial_depth)
        {//show/hide windows in subfolders
            lst = ((Folder *)item->folder())->windowsList();
            foreach(MyWidget *w, lst){
                if (!hiddenWindows->contains(w) && !outWindows->contains(w)){
                    if (show_windows_policy == SubFolders){
                        if (w->status() == MyWidget::Normal || w->status() == MyWidget::Maximized)
                            w->showNormal();
                        else if (w->status() == MyWidget::Minimized)
                            w->showMinimized();
                    }
				else if (w->isVisible())
					w->hide();
                }
            }
		item = (FolderListItem *)item->itemBelow();
        }
	}

    ws->blockSignals(false);

    if (active_window){
        ws->setActiveWindow(active_window);
        if (active_window_state == MyWidget::Minimized)
            active_window->showMinimized();//ws->setActiveWindow() makes minimized windows to be shown normally
        else if (active_window_state == MyWidget::Maximized){
            if (active_window->isA("Graph3D"))
                ((Graph3D *)active_window)->setIgnoreFonts(true);
            active_window->showMaximized();
            if (active_window->isA("Graph3D"))
                ((Graph3D *)active_window)->setIgnoreFonts(false);
            }
        current_folder->setActiveWindow(active_window);
        customMenu(active_window);
        customToolBars(active_window);
        }

     if (old_active_window){
        old_active_window->setStatus(old_active_window_state);
        oldFolder->setActiveWindow(old_active_window);
     }

    foreach(MyWidget *w, newFolder->windowsList())
        w->blockSignals(false);

	return true;
}

void ApplicationWindow::desactivateFolders()
{
	FolderListItem *item = (FolderListItem *)folders->firstChild();
	while (item)
	{
		item->setActive(false);
		item = (FolderListItem *)item->itemBelow();
	}
}

void ApplicationWindow::addListViewItem(MyWidget *w)
{
	if (!w)
		return;

	WindowListItem* it = new WindowListItem(lv, w);
	if (w->isA("Matrix")){
		it->setPixmap(0, QPixmap(matrix_xpm));
		it->setText(1, tr("Matrix"));
	}
	else if (w->inherits("Table")){
		it->setPixmap(0, QPixmap(worksheet_xpm));
		it->setText(1, tr("Table"));
	}
	else if (w->isA("Note")){
		it->setPixmap(0, QPixmap(note_xpm));
		it->setText(1, tr("Note"));
	}
	else if (w->isA("MultiLayer")){
		it->setPixmap(0, QPixmap(graph_xpm));
		it->setText(1, tr("Graph"));
	}
	else if (w->isA("Graph3D")){
		it->setPixmap(0, QPixmap(trajectory_xpm));
		it->setText(1, tr("3D Graph"));
	}

	it->setText(0, w->objectName());
    it->setText(2, w->aspect());
	it->setText(3, w->sizeToString());
	it->setText(4, w->birthDate());
	it->setText(5, w->windowLabel());
}

void ApplicationWindow::windowProperties()
{
	WindowListItem *it = (WindowListItem *)lv->currentItem();
	MyWidget *w = it->window();
	if (!w)
		return;

	QMessageBox *mbox = new QMessageBox ( tr("Properties"), QString(), QMessageBox::NoIcon,
			QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton, this);

	QString s = QString(w->objectName()) + "\n\n";
	s += "\n\n\n";

	s += tr("Label") + ": " + ((MyWidget *)w)->windowLabel() + "\n\n";

	if (w->isA("Matrix")){
		mbox->setIconPixmap(QPixmap(matrix_xpm));
		s +=  tr("Type") + ": " + tr("Matrix") + "\n\n";
	}else if (w->inherits("Table")){
		mbox->setIconPixmap(QPixmap(worksheet_xpm));
		s +=  tr("Type") + ": " + tr("Table") + "\n\n";
	}else if (w->isA("Note")){
		mbox->setIconPixmap(QPixmap(note_xpm));
		s +=  tr("Type") + ": " + tr("Note") + "\n\n";
	}else if (w->isA("MultiLayer")){
		mbox->setIconPixmap(QPixmap(graph_xpm));
		s +=  tr("Type") + ": " + tr("Graph") + "\n\n";
	}else if (w->isA("Graph3D")){
		mbox->setIconPixmap(QPixmap(trajectory_xpm));
		s +=  tr("Type") + ": " + tr("3D Graph") + "\n\n";
	}
	s += tr("Path") + ": " + current_folder->path() + "\n\n";
	s += tr("Size") + ": " + w->sizeToString() + "\n\n";
	s += tr("Created") + ": " + w->birthDate() + "\n\n";
	s += tr("Status") + ": " + it->text(2) + "\n\n";
	mbox->setText(s);
	mbox->show();
}

void ApplicationWindow::addFolderListViewItem(Folder *f)
{
	if (!f)
		return;

	FolderListItem* it = new FolderListItem(lv, f);
	it->setActive(false);
	it->setText(0, f->objectName());
	it->setText(1, tr("Folder"));
	it->setText(3, f->sizeToString());
	it->setText(4, f->birthDate());
}

void ApplicationWindow::find(const QString& s, bool windowNames, bool labels,
		bool folderNames, bool caseSensitive, bool partialMatch, bool subfolders)
{
	if (windowNames || labels){
		MyWidget *w = current_folder->findWindow(s, windowNames, labels, caseSensitive, partialMatch);
		if (w){
			activateWindow(w);
			return;
		}

		if (subfolders){
			FolderListItem *item = (FolderListItem *)folders->currentItem()->firstChild();
			while (item){
				Folder *f = item->folder();
				MyWidget *w = f->findWindow(s,windowNames,labels,caseSensitive,partialMatch);
				if (w){
					folders->setCurrentItem(f->folderListItem());
					activateWindow(w);
					return;
				}
				item = (FolderListItem *)item->itemBelow();
			}
		}
	}

	if (folderNames){
		Folder *f = current_folder->findSubfolder(s, caseSensitive, partialMatch);
		if (f){
			folders->setCurrentItem(f->folderListItem());
			return;
		}

		if (subfolders){
			FolderListItem *item = (FolderListItem *)folders->currentItem()->firstChild();
			while (item){
				Folder *f = item->folder()->findSubfolder(s, caseSensitive, partialMatch);
				if (f){
					folders->setCurrentItem(f->folderListItem());
					return;
				}

				item = (FolderListItem *)item->itemBelow();
			}
		}
	}

	QMessageBox::warning(this, tr("QtiPlot - No match found"),
			tr("Sorry, no match found for string: '%1'").arg(s));
}

void ApplicationWindow::dropFolderItems(Q3ListViewItem *dest)
{
	if (!dest || draggedItems.isEmpty ())
		return;

	Folder *dest_f = ((FolderListItem *)dest)->folder();

	Q3ListViewItem *it;
	QStringList subfolders = dest_f->subfolders();

	foreach(it, draggedItems){
		if (it->rtti() == FolderListItem::RTTI){
			Folder *f = ((FolderListItem *)it)->folder();
			FolderListItem *src = f->folderListItem();
			if (dest_f == f){
				QMessageBox::critical(this, "QtiPlot - Error", tr("Cannot move an object to itself!"));
				return;
			}

			if (((FolderListItem *)dest)->isChildOf(src)){
				QMessageBox::critical(this,"QtiPlot - Error",tr("Cannot move a parent folder into a child folder!"));
				draggedItems.clear();
				folders->setCurrentItem(current_folder->folderListItem());
				return;
			}

			Folder *parent = (Folder *)f->parent();
			if (!parent)
				parent = projectFolder();
			if (dest_f == parent)
				return;

			if (subfolders.contains(f->objectName())){
				QMessageBox::critical(this, tr("QtiPlot") +" - " + tr("Skipped moving folder"),
						tr("The destination folder already contains a folder called '%1'! Folder skipped!").arg(f->objectName()));
			} else
				moveFolder(src, (FolderListItem *)dest);
		} else {
			if (dest_f == current_folder)
				return;

			MyWidget *w = ((WindowListItem *)it)->window();
			if (w){
				current_folder->removeWindow(w);
				w->hide();
				dest_f->addWindow(w);
				delete it;
			}
		}
	}

	draggedItems.clear();
	current_folder = dest_f;
	folders->setCurrentItem(dest_f->folderListItem());
	changeFolder(dest_f, true);
	folders->setFocus();
}

void ApplicationWindow::moveFolder(FolderListItem *src, FolderListItem *dest)
{
	folders->blockSignals(true);

	Folder *dest_f = dest->folder();
	Folder *src_f = src->folder();

	dest_f = new Folder(dest_f, src_f->objectName());
	dest_f->setBirthDate(src_f->birthDate());
	dest_f->setModificationDate(src_f->modificationDate());

	FolderListItem *copy_item = new FolderListItem(dest, dest_f);
	copy_item->setText(0, src_f->objectName());
	dest_f->setFolderListItem(copy_item);

	QList<MyWidget *> lst = QList<MyWidget *>(src_f->windowsList());
	foreach(MyWidget *w, lst)
	{
		src_f->removeWindow(w);
		w->hide();
		dest_f->addWindow(w);
	}

	if ( !(src_f->children()).isEmpty() )
	{
		FolderListItem *item = (FolderListItem *)src->firstChild();
		int initial_depth = item->depth();
		while (item && item->depth() >= initial_depth)
		{
			src_f = (Folder *)item->folder();

			dest_f = new Folder(dest_f, src_f->objectName());
			dest_f->setBirthDate(src_f->birthDate());
			dest_f->setModificationDate(src_f->modificationDate());

			copy_item = new FolderListItem(copy_item, dest_f);
			copy_item->setText(0, src_f->objectName());
			dest_f->setFolderListItem(copy_item);

			lst = QList<MyWidget *>(src_f->windowsList());
			foreach(MyWidget *w, lst)
			{
				src_f->removeWindow(w);
				w->hide();
				dest_f->addWindow(w);
			}

			item = (FolderListItem *)item->itemBelow();
		}
	}

	src_f = src->folder();
	delete src_f;
	delete src;
	folders->blockSignals(false);
}

void ApplicationWindow::searchForUpdates()
{
    int choice = QMessageBox::question(this, tr("QtiPlot"),
					tr("QtiPlot will try to download necessary information about the last available updates. Please modify your firewall settings in order to allow QtiPlot to connect to the internet!") + "\n" +
					tr("Do you wish to continue?"),
					QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape);

    if (choice == QMessageBox::Yes)
    {
        version_buffer.open(IO_WriteOnly);
        http.setHost("soft.proindependent.com");
        http.get("/version.txt", &version_buffer);
        http.closeConnection();
    }
}

void ApplicationWindow::receivedVersionFile(bool error)
{
	if (error)
	{
		QMessageBox::warning(this, tr("QtiPlot - HTTP get version file"),
				tr("Error while fetching version file with HTTP: %1.").arg(http.errorString()));
		return;
	}

	version_buffer.close();

	if (version_buffer.open(IO_ReadOnly))
	{
		QTextStream t( &version_buffer );
		t.setEncoding(QTextStream::UnicodeUTF8);
		QString version = t.readLine();
		version_buffer.close();

		QString currentVersion = QString::number(maj_version) + "." + QString::number(min_version) +
			"." + QString::number(patch_version) + QString(extra_version);

		if (currentVersion != version)
		{
			if(QMessageBox::question(this, tr("QtiPlot - Updates Available"),
						tr("There is a newer version of QtiPlot (%1) available for download. Would you like to download it?").arg(version),
						QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape) == QMessageBox::Yes)
				QDesktopServices::openUrl(QUrl("http://soft.proindependent.com/download.html"));
		}
		else if (!autoSearchUpdatesRequest)
		{
			QMessageBox::information(this, tr("QtiPlot - No Updates Available"),
					tr("No updates available. Your current version %1 is the last version available!").arg(version));
		}
		autoSearchUpdatesRequest = false;
	}
}

/*!
  Turns 3D animation on or off
  */
void ApplicationWindow::toggle3DAnimation(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->animate(on);
}

QString ApplicationWindow::generateUniqueName(const QString& name, bool increment)
{
	int index = 0;
	QWidgetList *windows = windowsList();
	QStringList lst;
	foreach (QWidget *w, *windows){
		lst << QString(w->objectName());
		if (QString(w->objectName()).startsWith(name))
			index++;
	}
	delete windows;

	QString newName = name;
	if (increment)//force return of a different name
		newName += QString::number(++index);
	else if (index>0)
		newName += QString::number(index);

	while(lst.contains(newName))
		newName = name + QString::number(++index);

	return newName;
}

void ApplicationWindow::clearTable()
{
	Table *t = (Table*)ws->activeWindow();
	if (!t || !t->inherits("Table"))
		return;

	if (QMessageBox::question(this, tr("QtiPlot - Warning"),
				tr("This will clear the contents of all the data associated with the table. Are you sure?"),
				tr("&Yes"), tr("&No"), QString(), 0, 1 ) )
		return;
	else
		t->clear();
}

void ApplicationWindow::goToRow()
{
	if (!ws->activeWindow())
		return;

	if (ws->activeWindow()->inherits("Table") || ws->activeWindow()->isA("Matrix")){
		bool ok;
		int row = QInputDialog::getInteger(this, tr("QtiPlot - Enter row number"), tr("Row"),
				1, 0, 1000000, 1, &ok, windowFlags() & ~Qt::WindowContextHelpButtonHint & ~Qt::WindowMinMaxButtonsHint );
		if ( !ok )
			return;

		if (ws->activeWindow()->inherits("Table"))
			((Table *)ws->activeWindow())->goToRow(row);
		else if (ws->activeWindow()->isA("Matrix"))
			((Matrix *)ws->activeWindow())->goToRow(row);
	}
}

void ApplicationWindow::showScriptWindow()
{
	if (!scriptWindow){
		scriptWindow = new ScriptWindow(scriptEnv, this);
		scriptWindow->resize(d_script_win_rect.size());
		scriptWindow->move(d_script_win_rect.topLeft());
		connect(scriptWindow, SIGNAL(visibilityChanged(bool)), actionShowScriptWindow, SLOT(setOn(bool)));
	}

	if (!scriptWindow->isVisible()){
		Qt::WindowFlags flags = 0;
		if (d_script_win_on_top)
			flags |= Qt::WindowStaysOnTopHint;
		scriptWindow->setWindowFlags(flags);
		scriptWindow->show();
		scriptWindow->setFocus();
	} else
		scriptWindow->hide();
}

/*!
  Turns perspective mode on or off
  */
void ApplicationWindow::togglePerspective(bool on)
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setOrthogonal(!on);
}

/*!
  Resets rotation of 3D plots to default values
  */
void ApplicationWindow::resetRotation()
{
	if (ws->activeWindow() && ws->activeWindow()->isA("Graph3D"))
		((Graph3D*)ws->activeWindow())->setRotation(30,0,15);
}

/*!
  Finds best layout for the 3D plot
  */
void ApplicationWindow::fitFrameToLayer()
{
	if (!ws->activeWindow() || !ws->activeWindow()->isA("Graph3D"))
		return;

	((Graph3D *)ws->activeWindow())->findBestLayout();
}

ApplicationWindow::~ApplicationWindow()
{
	if (lastCopiedLayer)
		delete lastCopiedLayer;

	delete hiddenWindows;
	delete outWindows;

	if (scriptWindow)
		scriptWindow->close();

    if (d_text_editor)
		delete d_text_editor;

	QApplication::clipboard()->clear(QClipboard::Clipboard);
}

QString ApplicationWindow::versionString()
{
	return "QtiPlot " + QString::number(maj_version) + "." +
		QString::number(min_version) + "." + QString::number(patch_version) + extra_version;
}


int ApplicationWindow::convertOldToNewColorIndex(int cindex)
{
	if( (cindex == 13) || (cindex == 14) ) // white and light gray
		return cindex + 4;

	if(cindex == 15) // dark gray
		return cindex + 8;

	return cindex;
}

void ApplicationWindow::cascade()
{
	QList<QWidget*> windows = ws->windowList(QWorkspace::StackingOrder);

    const int xoffset = 13;
    const int yoffset = 20;

    int x = 0;
    int y = 0;

    foreach (QWidget *w, windows){
        w->setActiveWindow();
        w->showNormal();
        ((MyWidget *)w)->setStatus(MyWidget::Normal);
        updateWindowStatus((MyWidget *)w);

        w->parentWidget()->setGeometry(x, y, w->parentWidget()->geometry().width(),
                                            w->parentWidget()->geometry().height());
        w->raise();
        x += xoffset;
        y += yoffset;
    }
    modifiedProject();
}

ApplicationWindow * ApplicationWindow::loadScript(const QString& fn, bool execute, bool factorySettings)
{
#ifdef SCRIPTING_PYTHON
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	ApplicationWindow *app= new ApplicationWindow(factorySettings);
	app->applyUserSettings();
	app->setScriptingLanguage("Python");
	app->restoreApplicationGeometry();
	app->showScriptWindow();
	app->scriptWindow->open(fn);
	QApplication::restoreOverrideCursor();
	if (execute)
		app->scriptWindow->executeAll();
	return app;
#else
    QMessageBox::critical(this, tr("QtiPlot") + " - " + tr("Error"),
    tr("QtiPlot was not built with Python scripting support included!"));
#endif

}

bool ApplicationWindow::validFor2DPlot(Table *table)
{
	if (!table->selectedYColumns().count()){
  		QMessageBox::warning(this, tr("QtiPlot - Error"), tr("Please select a Y column to plot!"));
  	    return false;
  	} else if (table->numCols()<2) {
		QMessageBox::critical(this, tr("QtiPlot - Error"),tr("You need at least two columns for this operation!"));
		return false;
	} else if (table->noXColumn()) {
		QMessageBox::critical(this, tr("QtiPlot - Error"), tr("Please set a default X column for this table, first!"));
		return false;
	}
	return true;
}

MultiLayer* ApplicationWindow::generate2DGraph(Graph::CurveType type)
{
	if (!ws->activeWindow())
		return 0;

    if (ws->activeWindow()->inherits("Table")){
        Table *table = static_cast<Table *>(ws->activeWindow());
        if (!validFor2DPlot(table))
            return 0;

        Q3TableSelection sel = table->getSelection();
        return multilayerPlot(table, table->drawableColumnSelection(), type, sel.topRow(), sel.bottomRow());
    } else if (ws->activeWindow()->isA("Matrix")){
        Matrix *m = static_cast<Matrix *>(ws->activeWindow());
        return plotHistogram(m);
    }
}

bool ApplicationWindow::validFor3DPlot(Table *table)
{
	if (table->numCols()<2){
		QMessageBox::critical(0,tr("QtiPlot - Error"),tr("You need at least two columns for this operation!"));
		return false;
	}
	if (table->selectedColumn() < 0 || table->colPlotDesignation(table->selectedColumn()) != Table::Z){
		QMessageBox::critical(0,tr("QtiPlot - Error"),tr("Please select a Z column for this operation!"));
		return false;
	}
	if (table->noXColumn()){
		QMessageBox::critical(0,tr("QtiPlot - Error"),tr("You need to define a X column first!"));
		return false;
	}
	if (table->noYColumn()){
		QMessageBox::critical(0,tr("QtiPlot - Error"),tr("You need to define a Y column first!"));
		return false;
	}
	return true;
}

void ApplicationWindow::hideSelectedWindows()
{
	Q3ListViewItem *item;
	QList<Q3ListViewItem *> lst;
	for (item = lv->firstChild(); item; item = item->nextSibling()){
		if (item->isSelected())
			lst.append(item);
	}

	folders->blockSignals(true);
	foreach(item, lst){
		if (item->rtti() != FolderListItem::RTTI)
			hideWindow(((WindowListItem *)item)->window());
	}
	folders->blockSignals(false);
}

void ApplicationWindow::showSelectedWindows()
{
	Q3ListViewItem *item;
	QList<Q3ListViewItem *> lst;
	for (item = lv->firstChild(); item; item = item->nextSibling()){
		if (item->isSelected())
			lst.append(item);
	}

	folders->blockSignals(true);
	foreach(item, lst){
		if (item->rtti() != FolderListItem::RTTI)
			activateWindow(((WindowListItem *)item)->window());
	}
	folders->blockSignals(false);
}

void ApplicationWindow::swapColumns()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = static_cast<Table*>(ws->activeWindow());
	QStringList lst = t->selectedColumns();
	if(lst.count() != 2)
        return;

	t->swapColumns(t->colIndex(lst[0]), t->colIndex(lst[1]));
}

void ApplicationWindow::moveColumnRight()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = static_cast<Table*>(ws->activeWindow());
	if (t)
    	t->moveColumnBy(1);
}

void ApplicationWindow::moveColumnLeft()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = static_cast<Table*>(ws->activeWindow());
	if (t)
    	t->moveColumnBy(-1);
}

void ApplicationWindow::moveColumnFirst()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = static_cast<Table*>(ws->activeWindow());
	if (t)
    	t->moveColumnBy(0 - t->selectedColumn());
}

void ApplicationWindow::moveColumnLast()
{
    if (!ws->activeWindow() || !ws->activeWindow()->inherits("Table"))
		return;

	Table *t = static_cast<Table*>(ws->activeWindow());
	if (t)
    	t->moveColumnBy(t->numCols() - t->selectedColumn() - 1);
}

void ApplicationWindow::restoreApplicationGeometry()
{
	if (d_app_rect.isNull())
		showMaximized();
	else {
		resize(d_app_rect.size());
		move(d_app_rect.topLeft());
		show();
	}
}

void ApplicationWindow::scriptsDirPathChanged(const QString& path)
{
	scriptsDirPath = path;

	QList<QWidget*> *lst = windowsList();
	foreach(QWidget *w, *lst){
		if (w->isA("Note"))
			((Note*)w)->setDirPath(path);
	}
	delete lst;
}

void ApplicationWindow::showToolBarsMenu()
{
	QMenu toolBarsMenu;

	QAction *actionFileTools = new QAction(fileTools->windowTitle(), this);
	actionFileTools->setCheckable(true);
	actionFileTools->setChecked(fileTools->isVisible());
	connect(actionFileTools, SIGNAL(toggled(bool)), fileTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionFileTools);

	QAction *actionEditTools = new QAction(editTools->windowTitle(), this);
	actionEditTools->setCheckable(true);
	actionEditTools->setChecked(editTools->isVisible());
	connect(actionEditTools, SIGNAL(toggled(bool)), editTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionEditTools);

	QAction *actionTableTools = new QAction(tableTools->windowTitle(), this);
	actionTableTools->setCheckable(true);
	actionTableTools->setChecked(tableTools->isVisible());
	connect(actionTableTools, SIGNAL(toggled(bool)), tableTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionTableTools);

	QAction *actionColumnTools = new QAction(columnTools->windowTitle(), this);
	actionColumnTools->setCheckable(true);
	actionColumnTools->setChecked(columnTools->isVisible());
	connect(actionColumnTools, SIGNAL(toggled(bool)), columnTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionColumnTools);

	QAction *actionPlotTools = new QAction(plotTools->windowTitle(), this);
	actionPlotTools->setCheckable(true);
	actionPlotTools->setChecked(plotTools->isVisible());
	connect(actionPlotTools, SIGNAL(toggled(bool)), plotTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionPlotTools);

	QAction *actionMatrixTools = new QAction(plotMatrixBar->windowTitle(), this);
	actionMatrixTools->setCheckable(true);
	actionMatrixTools->setChecked(plotMatrixBar->isVisible());
	connect(actionMatrixTools, SIGNAL(toggled(bool)), plotMatrixBar, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionMatrixTools);

	QAction *actionPlot3DTools = new QAction(plot3DTools->windowTitle(), this);
	actionPlot3DTools->setCheckable(true);
	actionPlot3DTools->setChecked(plot3DTools->isVisible());
	connect(actionPlot3DTools, SIGNAL(toggled(bool)), plot3DTools, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionPlot3DTools);

	QAction *actionDisplayBar = new QAction(displayBar->windowTitle(), this);
	actionDisplayBar->setCheckable(true);
	actionDisplayBar->setChecked(displayBar->isVisible());
	connect(actionDisplayBar, SIGNAL(toggled(bool)), displayBar, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionDisplayBar);

	QAction *actionFormatToolBar = new QAction(formatToolBar->windowTitle(), this);
	actionFormatToolBar->setCheckable(true);
	actionFormatToolBar->setChecked(formatToolBar->isVisible());
	connect(actionFormatToolBar, SIGNAL(toggled(bool)), formatToolBar, SLOT(setVisible(bool)));
	toolBarsMenu.addAction(actionFormatToolBar);

	QAction *action = toolBarsMenu.exec(QCursor::pos());
	if (!action)
		return;

	QWidget *w = ws->activeWindow();

	if (action->text() == plotMatrixBar->windowTitle()){
		d_matrix_tool_bar = action->isChecked();
		plotMatrixBar->setEnabled(w && w->isA("Matrix"));
	} else if (action->text() == tableTools->windowTitle()){
		d_table_tool_bar = action->isChecked();
		tableTools->setEnabled(w && w->inherits("Table"));
	} else if (action->text() == columnTools->windowTitle()){
		d_column_tool_bar = action->isChecked();
		columnTools->setEnabled(w && w->inherits("Table"));
	} else if (action->text() == plotTools->windowTitle()){
		d_plot_tool_bar = action->isChecked();
		plotTools->setEnabled(w && w->isA("MultiLayer"));
	} else if (action->text() == plot3DTools->windowTitle()){
		d_plot3D_tool_bar = action->isChecked();
		plot3DTools->setEnabled(w && w->isA("Graph3D"));
	} else if (action->text() == fileTools->windowTitle()){
		d_file_tool_bar = action->isChecked();
	} else if (action->text() == editTools->windowTitle()){
		d_edit_tool_bar = action->isChecked();
	} else if (action->text() == displayBar->windowTitle()){
		d_display_tool_bar = action->isChecked();
	} else if (action->text() == formatToolBar->windowTitle()){
		d_format_tool_bar = action->isChecked();
	}
}

void ApplicationWindow::saveFitFunctions(const QStringList& lst)
{
	if (!lst.count())
		return;

    QString explain = tr("Starting with version 0.9.1 QtiPlot stores the user defined fit models to a different location.");
    explain += " " + tr("If you want to save your already defined models, please choose a destination folder.");
    if (QMessageBox::Ok != QMessageBox::information(this, tr("QtiPlot") + " - " + tr("Import fit models"), explain,
                            QMessageBox::Ok, QMessageBox::Cancel)) return;

	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory to export the fit models to"), fitModelsPath, QFileDialog::ShowDirsOnly);
	if (!dir.isEmpty()){
	    fitModelsPath = dir;

        for (int i = 0; i<lst.count(); i++){
            QString s = lst[i].simplified();
            if (!s.isEmpty()){
                NonLinearFit *fit = new NonLinearFit(this, 0);

                int pos1 = s.find("(", 0);
                fit->setObjectName(s.left(pos1));

                int pos2 = s.find(")", pos1);
                QString par = s.mid(pos1+4, pos2-pos1-4);
                QStringList paramList = par.split(QRegExp("[,;]+[\\s]*"), QString::SkipEmptyParts);
                fit->setParametersList(paramList);

                QStringList l = s.split("=");
                if (l.count() == 2)
                    fit->setFormula(l[1]);

                fit->save(fitModelsPath + "/" + fit->objectName() + ".fit");
            }
        }
	}
}

void ApplicationWindow::matrixDirectFFT()
{
#ifdef QTIPLOT_PRO
    Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->fft();
	QApplication::restoreOverrideCursor();
#else
    QString s = tr("This feature is only available to users having subscribed for a binaries maintenance contract!");
	s += " " + tr("Please visit the following web page for more details:");
	s += "<p><a href = http://soft.proindependent.com/pricing.html>http://soft.proindependent.com/pricing.html</a></p>";
	QMessageBox::critical(this, tr("QtiPlot"), s);
    return;
#endif
}

void ApplicationWindow::matrixInverseFFT()
{
#ifdef QTIPLOT_PRO
    Matrix* m = (Matrix*)ws->activeWindow();
	if (!m)
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	m->fft(true);
	QApplication::restoreOverrideCursor();
#else
    QString s = tr("This feature is only available to users having subscribed for a binaries maintenance contract!");
	s += " " + tr("Please visit the following web page for more details:");
	s += "<p><a href = http://soft.proindependent.com/pricing.html>http://soft.proindependent.com/pricing.html</a></p>";
	QMessageBox::critical(this, tr("QtiPlot"), s);
    return;
#endif
}

void ApplicationWindow::setFormatBarFont(const QFont& font)
{
	formatToolBar->setEnabled(true);

	QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
	fb->blockSignals(true);
	fb->setCurrentFont(font);
	fb->blockSignals(false);

	QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
	sb->blockSignals(true);
	sb->setValue(font.pointSize());
	sb->blockSignals(false);

    actionFontBold->blockSignals(true);
	actionFontBold->setChecked(font.bold());
	actionFontBold->blockSignals(false);

	actionFontItalic->blockSignals(true);
	actionFontItalic->setChecked(font.italic());
    actionFontItalic->blockSignals(false);

    actionSubscript->setEnabled(false);
    actionSuperscript->setEnabled(false);
    actionUnderline->setEnabled(false);
}

void ApplicationWindow::setFontSize(int size)
{
	QWidget *w = ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
	QFont f(fb->currentFont().family(), size);
	f.setBold(actionFontBold->isChecked());
	f.setItalic(actionFontItalic->isChecked());
	g->setCurrentFont(f);
}

void ApplicationWindow::setFontFamily(const QFont& font)
{
	QWidget *w = ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
	QFont f(font.family(), sb->value());
	f.setBold(actionFontBold->isChecked());
	f.setItalic(actionFontItalic->isChecked());
	g->setCurrentFont(f);
}

void ApplicationWindow::setItalicFont(bool italic)
{
	QWidget *w = ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
	QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
	QFont f(fb->currentFont().family(), sb->value());
	f.setBold(actionFontBold->isChecked());
	f.setItalic(italic);
	g->setCurrentFont(f);
}

void ApplicationWindow::setBoldFont(bool bold)
{
	QWidget *w = ws->activeWindow();
	if (!w || !w->isA("MultiLayer"))
		return;

	Graph* g = ((MultiLayer*)ws->activeWindow())->activeGraph();
	if (!g)
		return;

	QFontComboBox *fb = (QFontComboBox *)formatToolBar->widgetForAction(actionFontBox);
	QSpinBox *sb = (QSpinBox *)formatToolBar->widgetForAction(actionFontSize);
	QFont f(fb->currentFont().family(), sb->value());
	f.setBold(bold);
	f.setItalic(actionFontItalic->isChecked());
	g->setCurrentFont(f);
}

void ApplicationWindow::enableTextEditor(Graph *g)
{
	if (!g){
        formatToolBar->setEnabled(false);
	    if (d_text_editor){
            d_text_editor->close();
            d_text_editor = NULL;
	    }
	} else if (g) {
        d_text_editor = new TextEditor(g);
        actionSubscript->setEnabled(true);
        actionSuperscript->setEnabled(true);
        actionUnderline->setEnabled(true);
	}
}

void ApplicationWindow::insertSuperscript()
{
    if (!d_text_editor)
        return;

    d_text_editor->formatText("<sup>","</sup>");
}

void ApplicationWindow::insertSubscript()
{
    if (!d_text_editor)
        return;

    d_text_editor->formatText("<sub>","</sub>");
}

void ApplicationWindow::underline()
{
    if (!d_text_editor)
        return;

    d_text_editor->formatText("<u>","</u>");
}

void ApplicationWindow::insertGreekSymbol()
{
    if (!d_text_editor)
        return;

    SymbolDialog *greekLetters = new SymbolDialog(SymbolDialog::lowerGreek, this, Qt::Tool);
	greekLetters->setAttribute(Qt::WA_DeleteOnClose);
	connect(greekLetters, SIGNAL(addLetter(const QString&)), d_text_editor, SLOT(addSymbol(const QString&)));
	greekLetters->show();
	greekLetters->setFocus();
}

void ApplicationWindow::showCustomActionDialog()
{
    CustomActionDialog *ad = new CustomActionDialog(this);
	ad->setAttribute(Qt::WA_DeleteOnClose);
	ad->show();
	ad->setFocus();
}

void ApplicationWindow::addCustomAction(QAction *action, const QString& parentName)
{
    if (!action)
        return;

	QList<QToolBar *> toolBars = toolBarsList();
    foreach (QToolBar *t, toolBars){
        if (t->objectName() == parentName){
            t->addAction(action);
            d_user_actions << action;
            return;
        }
    }

    QList<QMenu *> menus = customizableMenusList();
    foreach (QMenu *m, menus){
        if (m->objectName() == parentName){
            m->addAction(action);
            d_user_actions << action;
            return;
        }
    }
}

void ApplicationWindow::reloadCustomActions()
{
    QList<QMenu *> menus = customizableMenusList();
	foreach(QAction *a, d_user_actions){
		if (!a->statusTip().isEmpty()){
    		foreach (QMenu *m, menus){
        		if (m->objectName() == a->statusTip()){
        		    QList<QAction *> lst = m->actions();
        		    if (!lst.contains(a))
                        m->addAction(a);
        		}
        	}
		}
	}
}

void ApplicationWindow::removeCustomAction(QAction *action)
{
    int index = d_user_actions.indexOf(action);
    if (index >= 0 && index < d_user_actions.count()){
        d_user_actions.removeAt(index);
        delete action;
    }
}

void ApplicationWindow::performCustomAction(QAction *action)
{
	if (!d_user_actions.contains(action))
		return;

#ifdef SCRIPTING_PYTHON
	setScriptingLanguage("Python");

    QString fileName = action->data().toString();
    Note* n = new Note(scriptEnv, "", 0);
    n->importASCII(fileName);
    n->executeAll();
    delete n;
#else
    QMessageBox::critical(this, tr("QtiPlot") + " - " + tr("Error"),
    tr("QtiPlot was not built with Python scripting support included!"));
#endif
}

void ApplicationWindow::loadCustomActions()
{
    QString path = customActionsDirPath + "/";
	QDir dir(path);
	QStringList lst = dir.entryList(QDir::Files|QDir::NoSymLinks, QDir::Name);
	for (int i=0; i<lst.count(); i++){
	    QString fileName = path + lst[i];
        QFile file(fileName);
        QFileInfo fi(file);
        if (!file.open(QFile::ReadOnly | QFile::Text))
            continue;

        QAction *action = new QAction(this);
        CustomActionHandler handler(action);
        QXmlSimpleReader reader;
        reader.setContentHandler(&handler);
        reader.setErrorHandler(&handler);

        QXmlInputSource xmlInputSource(&file);
		if (reader.parse(xmlInputSource))
			addCustomAction(action, handler.parentName());
	}
}

QList<QMenu *> ApplicationWindow::customizableMenusList()
{
	QList<QMenu *> lst;
	lst << 	windowsMenu << view << graph << fileMenu << format << edit;
	lst << help << plot2DMenu << analysisMenu << multiPeakMenu;
	lst << matrixMenu << plot3DMenu << plotDataMenu << scriptingMenu;
	lst << tableMenu << fillMenu << normMenu << newMenu << exportPlotMenu << smoothMenu;
	lst << filterMenu << decayMenu;
	return lst;
}

QList<QMenu *> ApplicationWindow::menusList()
{
	QList<QMenu *> lst;
	QObjectList children = this->children();
	foreach (QObject *w, children){
        if (w->isA("QMenu"))
            lst << (QMenu *)w;
    }
	return lst;
}

QList<QToolBar *> ApplicationWindow::toolBarsList()
{
	QList<QToolBar *> lst;
	QObjectList children = this->children();
	foreach (QObject *w, children){
        if (w->isA("QToolBar"))
            lst << (QToolBar *)w;
    }
	return lst;
}
