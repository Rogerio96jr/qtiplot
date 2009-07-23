/***************************************************************************
    File                 : EnrichmentDialog.cpp
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2008 by Ion Vasilief
    Email (use @ for *)  : ion_vasilief*yahoo.fr
    Description          : A general properties dialog for the FrameWidget, using article
						  "Using a Simple Web Service with Qt" in Qt Quaterly, Issue 23, Q3 2007

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

#include <QtGui>
#include <QHttp>
#include <QIODevice>
#include <QNetworkProxy>

#include "EnrichmentDialog.h"
#include <Graph.h>
#include <MultiLayer.h>
#include <TexWidget.h>
#include <EllipseWidget.h>
#include <FrameWidget.h>
#include <ImageWidget.h>
#include <RectangleWidget.h>
#include <LegendWidget.h>
#include <ApplicationWindow.h>
#include <TextFormatButtons.h>
#include <ColorButton.h>
#include <DoubleSpinBox.h>
#include <PatternBox.h>
#include <PenStyleBox.h>
#include <pixmaps.h>

static const char* choose_folder_xpm[]={
    "16 16 11 1",
    "# c #000000",
    "g c #c0c0c0",
    "e c #303030",
    "a c #ffa858",
    "b c #808080",
    "d c #a0a0a4",
    "f c #585858",
    "c c #ffdca8",
    "h c #dcdcdc",
    "i c #ffffff",
    ". c None",
    "....###.........",
    "....#ab##.......",
    "....#acab####...",
    "###.#acccccca#..",
    "#ddefaaaccccca#.",
    "#bdddbaaaacccab#",
    ".eddddbbaaaacab#",
    ".#bddggdbbaaaab#",
    "..edgdggggbbaab#",
    "..#bgggghghdaab#",
    "...ebhggghicfab#",
    "....#edhhiiidab#",
    "......#egiiicfb#",
    "........#egiibb#",
    "..........#egib#",
    "............#ee#"};

EnrichmentDialog::EnrichmentDialog(WidgetType wt, Graph *g, QWidget *parent)
    : QDialog(parent), d_plot(g), d_widget(NULL), d_widget_type(wt)
{
	setSizeGripEnabled( true );
	setAttribute(Qt::WA_DeleteOnClose);

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
	clearButton = NULL;
	editPage = NULL;
	imagePage = NULL;
	patternPage = NULL;
	textPage = NULL;

	if (wt == Tex){
		setWindowTitle(tr("QtiPlot") + " - " + tr("Tex Equation Editor"));

    	clearButton = buttonBox->addButton(tr("Clea&r"), QDialogButtonBox::ResetRole);
		connect(clearButton, SIGNAL(clicked()), this, SLOT(clearForm()));
	} else if (wt == MDIWindow)
        setWindowTitle(tr("QtiPlot") + " - " + tr("Window Geometry"));
	else
		setWindowTitle(tr("QtiPlot") + " - " + tr("Object Properties"));

    updateButton = buttonBox->addButton(tr("&Apply"), QDialogButtonBox::ApplyRole);
	connect(updateButton, SIGNAL(clicked()), this, SLOT(apply()));

	cancelButton = buttonBox->addButton(tr("&Close"), QDialogButtonBox::RejectRole);
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	tabWidget = new QTabWidget();
	if (wt == Text)
		initTextPage();
	else if (wt == Tex){
		initEditorPage();
		if (!(((ApplicationWindow *)parent)->d_latex_compiler_path).isEmpty()){
			texCompilerBox->setCurrentIndex(1);
			updateCompilerInterface(1);
		}
	} else if (wt == Image)
		initImagePage();
	else if (wt == Frame || wt == Ellipse)
		initPatternPage();

    if (wt != MDIWindow)
        initFramePage();
	initGeometryPage();

    QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(tabWidget);
    layout->addWidget(buttonBox);
    setLayout(layout);

	connect(tabWidget, SIGNAL(currentChanged (QWidget *)), this, SLOT(customButtons(QWidget *)));
}

void EnrichmentDialog::initEditorPage()
{
	http = new QHttp(this);
    connect(http, SIGNAL(done(bool)), this, SLOT(updateForm(bool)));
    http->setHost("mathtran.org");
	QNetworkProxy proxy = QNetworkProxy::applicationProxy();
	if (!proxy.hostName().isEmpty())
		http->setProxy(proxy.hostName(), proxy.port(), proxy.user(), proxy.password());

	compileProcess = NULL;
	dvipngProcess = NULL;

	editPage = new QWidget();

    equationEditor = new QTextEdit;

	texFormatButtons = new TextFormatButtons(equationEditor, TextFormatButtons::Equation);

	texCompilerBox = new QComboBox;
	texCompilerBox->addItem(tr("MathTran (http://www.mathtran.org/)"));
	texCompilerBox->addItem(tr("locally installed"));
	connect(texCompilerBox, SIGNAL(activated(int)), this, SLOT(updateCompilerInterface(int)));

	QHBoxLayout *hl = new QHBoxLayout;
	hl->addWidget(new QLabel(tr("LaTeX Compiler")));
	hl->addWidget(texCompilerBox);

	compilerPathGroupBox = new QGroupBox;
	QHBoxLayout *hl1 = new QHBoxLayout(compilerPathGroupBox);
	compilerPathBox = new QLineEdit;

	compilerPathBox->setText(((ApplicationWindow *)parentWidget())->d_latex_compiler_path);
	connect(compilerPathBox, SIGNAL(editingFinished ()), this, SLOT(validateCompiler()));

	hl1->addWidget(compilerPathBox);
    browseCompilerBtn = new QPushButton;
    browseCompilerBtn->setIcon(QIcon(QPixmap(choose_folder_xpm)));
	connect(browseCompilerBtn, SIGNAL(clicked()), this, SLOT(chooseCompiler()));

    hl1->addWidget(browseCompilerBtn);
    compilerPathGroupBox->hide();

	outputLabel = new QLabel;
    outputLabel->setFrameShape(QFrame::StyledPanel);

	QVBoxLayout *layout = new QVBoxLayout(editPage);
    layout->addWidget(equationEditor, 1);
	layout->addWidget(texFormatButtons);
	layout->addLayout(hl);
	layout->addWidget(compilerPathGroupBox);
	layout->addWidget(new QLabel(tr("Preview:")));
	layout->addWidget(outputLabel);

	tabWidget->addTab(editPage, tr("&Text" ));
}

void EnrichmentDialog::initTextPage()
{
	QGroupBox *gb1 = new QGroupBox();
	QGridLayout * gl1 = new QGridLayout(gb1);
	gl1->addWidget(new QLabel(tr("Color")), 0, 0);

	textColorBtn = new ColorButton();
	connect(textColorBtn, SIGNAL(colorChanged()), this, SLOT(textFormatApplyTo()));
	gl1->addWidget(textColorBtn, 0, 1);

	textFontBtn = new QPushButton(tr( "&Font" ));
	connect(textFontBtn, SIGNAL(clicked()), this, SLOT(customFont()));
	gl1->addWidget(textFontBtn, 0, 2);

    gl1->addWidget(new QLabel(tr("Background")), 1, 0);
	textBackgroundBtn = new ColorButton();
	connect(textBackgroundBtn, SIGNAL(colorChanged()), this, SLOT(textFormatApplyTo()));
	gl1->addWidget(textBackgroundBtn, 1, 1);

	gl1->addWidget(new QLabel(tr("Opacity")), 2, 0);

	boxBackgroundTransparency = new QSpinBox();
	boxBackgroundTransparency->setRange(0, 255);
    boxBackgroundTransparency->setSingleStep(5);
	boxBackgroundTransparency->setWrapping(true);
    boxBackgroundTransparency->setSpecialValueText(tr("Transparent"));
	connect(boxBackgroundTransparency, SIGNAL(valueChanged(int)),
			this, SLOT(updateTransparency(int)));
	gl1->addWidget(boxBackgroundTransparency, 2, 1);

    gl1->addWidget(new QLabel(tr("Rotate (deg.)")), 3, 0);

    boxTextAngle = new QSpinBox();
    boxTextAngle->setRange(-360, 360);
    boxTextAngle->setSingleStep(45);
    boxTextAngle->setWrapping(true);
    connect(boxTextAngle, SIGNAL(valueChanged(int)), this, SLOT(textFormatApplyTo()));
    gl1->addWidget(boxTextAngle, 3, 1);

    autoUpdateTextBox = new QCheckBox(tr("Auto-&update"));
	gl1->addWidget(autoUpdateTextBox, 1, 2);

	gl1->setColumnStretch(4, 1);

    QVBoxLayout *vl = new QVBoxLayout();
    textDefaultBtn = new QPushButton( tr( "Set As &Default" ) );
    connect(textDefaultBtn, SIGNAL(clicked()), this, SLOT(setTextDefaultValues()));
	vl->addWidget(textDefaultBtn);

    textApplyToBtn = new QPushButton(tr("Apply format &to..."));
	connect(textApplyToBtn, SIGNAL(clicked()), this, SLOT(textFormatApplyTo()));
	vl->addWidget(textApplyToBtn);

	textApplyToBox = new QComboBox();
	textApplyToBox->insertItem(tr("Object"));
	textApplyToBox->insertItem(tr("Layer"));
    textApplyToBox->insertItem(tr("Window"));
    textApplyToBox->insertItem(tr("All Windows"));
	vl->addWidget(textApplyToBox);
	vl->addStretch();

    QHBoxLayout *hl = new QHBoxLayout();
	hl->addWidget(gb1);
	hl->addLayout(vl);

	textEditBox = new QTextEdit();
	textEditBox->setTextFormat(Qt::PlainText);

	formatButtons =  new TextFormatButtons(textEditBox, TextFormatButtons::Legend);

	setFocusPolicy(Qt::StrongFocus);
	setFocusProxy(textEditBox);

	textPage = new QWidget();

	QVBoxLayout* ml = new QVBoxLayout(textPage);
	ml->addLayout(hl);
	ml->addWidget(formatButtons);
	ml->addWidget(textEditBox, 1);

	tabWidget->addTab(textPage, tr( "&Text" ) );
}

void EnrichmentDialog::initImagePage()
{
	imagePage = new QWidget();

    QGroupBox *gb = new QGroupBox();
	QGridLayout *gl = new QGridLayout(gb);
    gl->addWidget(new QLabel( tr("File")), 0, 0);

	imagePathBox = new QLineEdit();
	gl->addWidget(imagePathBox, 0, 1);

	QPushButton *browseBtn = new QPushButton();
	connect(browseBtn, SIGNAL(clicked()), this, SLOT(chooseImageFile()));
	browseBtn->setIcon(QIcon(QPixmap(choose_folder_xpm)));
	gl->addWidget(browseBtn, 0, 2);

	boxSaveImagesInternally = new QCheckBox(tr("&Save internally"));
	connect(boxSaveImagesInternally, SIGNAL(toggled(bool)), this, SLOT(saveImagesInternally(bool)));

	gl->addWidget(boxSaveImagesInternally, 1, 1);
	gl->setColumnStretch(1, 1);
	gl->setRowStretch(2, 1);

	QVBoxLayout *layout = new QVBoxLayout(imagePage);
    layout->addWidget(gb);
	tabWidget->addTab(imagePage, tr( "&Image" ) );
}

void EnrichmentDialog::initFramePage()
{
    framePage = new QWidget();

	QGroupBox *gb = new QGroupBox();
	QGridLayout *gl = new QGridLayout(gb);
    gl->addWidget(new QLabel( tr("Shape")), 0, 0);

	frameBox = new QComboBox();
	frameBox->addItem(tr("None"));
	if (d_widget_type == Ellipse)
		frameBox->addItem(tr("Line"));
	else {
		frameBox->addItem(tr("Rectangle"));
		frameBox->addItem(tr("Shadow"));
	}
	connect(frameBox, SIGNAL(activated(int)), this, SLOT(frameApplyTo()));
    gl->addWidget(frameBox, 0, 1);

    gl->addWidget(new QLabel(tr("Color")), 1, 0);
	frameColorBtn = new ColorButton();
	connect(frameColorBtn, SIGNAL(colorChanged()), this, SLOT(frameApplyTo()));
    gl->addWidget(frameColorBtn, 1, 1);

	gl->addWidget(new QLabel(tr( "Line Style" )), 2, 0);
	boxFrameLineStyle = new PenStyleBox();
	connect(boxFrameLineStyle, SIGNAL(activated(int)), this, SLOT(frameApplyTo()));
	gl->addWidget(boxFrameLineStyle, 2, 1);

	gl->setColumnStretch(1, 1);
	gl->addWidget(new QLabel(tr("Width")), 3, 0);
	boxFrameWidth = new DoubleSpinBox();
	if(d_widget_type == Ellipse){
		boxFrameWidth->setDecimals(2);
		boxFrameWidth->setLocale(((ApplicationWindow *)parent())->locale());
		boxFrameWidth->setSingleStep(0.1);
		boxFrameWidth->setRange(0.1, 100);
	} else {
		boxFrameWidth->setRange(1, 100);
		boxFrameWidth->setDecimals(0);
		boxFrameWidth->setSingleStep(1.0);
	}

	connect(boxFrameWidth, SIGNAL(valueChanged(double)), this, SLOT(frameApplyTo()));
	gl->addWidget(boxFrameWidth, 3, 1);
	gl->setRowStretch(4, 1);

	QVBoxLayout *vl = new QVBoxLayout();

	frameDefaultBtn = new QPushButton(tr("Set As &Default"));
	connect(frameDefaultBtn, SIGNAL(clicked()), this, SLOT(setFrameDefaultValues()));
	vl->addWidget(frameDefaultBtn);

	QLabel *l = new QLabel(tr("Apply t&o..."));
	vl->addWidget(l);

	frameApplyToBox = new QComboBox();
	frameApplyToBox->insertItem(tr("Object"));
	frameApplyToBox->insertItem(tr("Layer"));
    frameApplyToBox->insertItem(tr("Window"));
    frameApplyToBox->insertItem(tr("All Windows"));
	vl->addWidget(frameApplyToBox);
	vl->addStretch();
	l->setBuddy(frameApplyToBox);

	QHBoxLayout *hl = new QHBoxLayout(framePage);
	hl->addWidget(gb);
	hl->addLayout(vl);

	tabWidget->addTab(framePage, tr( "&Frame" ) );
}

void EnrichmentDialog::initPatternPage()
{
	patternPage = new QWidget();

	QGroupBox *gb = new QGroupBox();
	QGridLayout *gl = new QGridLayout(gb);
    gl->addWidget(new QLabel( tr("Fill Color")), 0, 0);

	backgroundColorBtn = new ColorButton();
	connect(backgroundColorBtn, SIGNAL(colorChanged()), this, SLOT(patternApplyTo()));
    gl->addWidget(backgroundColorBtn, 0, 1);

	gl->addWidget(new QLabel(tr("Opacity")), 1, 0);
	boxTransparency = new QSpinBox();
	boxTransparency->setRange(0, 255);
    boxTransparency->setSingleStep(5);
	boxTransparency->setWrapping(true);
    boxTransparency->setSpecialValueText(tr("Transparent"));
	connect(boxTransparency, SIGNAL(valueChanged(int)), this, SLOT(patternApplyTo()));
	gl->addWidget(boxTransparency, 1, 1);

	gl->addWidget(new QLabel(tr("Pattern")), 2, 0);
	patternBox = new PatternBox();
	connect(patternBox, SIGNAL(activated(int)), this, SLOT(patternApplyTo()));
	gl->addWidget(patternBox, 2, 1);

	gl->addWidget(new QLabel(tr("Pattern Color")), 3, 0);
	patternColorBtn = new ColorButton();
	connect(patternColorBtn, SIGNAL(colorChanged()), this, SLOT(patternApplyTo()));
	gl->addWidget(patternColorBtn, 3, 1);

	useFrameColorBox = new QCheckBox(tr("Use &Frame Color"));
	connect(useFrameColorBox, SIGNAL(toggled(bool)), this, SLOT(patternApplyTo()));
	connect(useFrameColorBox, SIGNAL(toggled(bool)), patternColorBtn, SLOT(setDisabled(bool)));
	gl->addWidget(useFrameColorBox, 3, 2);

	gl->setColumnStretch(1, 1);
	gl->setRowStretch(4, 1);

	QVBoxLayout *vl = new QVBoxLayout();
	rectangleDefaultBtn = new QPushButton(tr("Set As &Default"));
	connect(rectangleDefaultBtn, SIGNAL(clicked()), this, SLOT(setRectangleDefaultValues()));
	vl->addWidget(rectangleDefaultBtn);

	QLabel *l = new QLabel(tr("Apply t&o..."));
	vl->addWidget(l);

	patternApplyToBox = new QComboBox();
	patternApplyToBox->insertItem(tr("Object"));
	patternApplyToBox->insertItem(tr("Layer"));
    patternApplyToBox->insertItem(tr("Window"));
    patternApplyToBox->insertItem(tr("All Windows"));
	vl->addWidget(patternApplyToBox);
	vl->addStretch();
	l->setBuddy(patternApplyToBox);

	QHBoxLayout *hl = new QHBoxLayout(patternPage);
	hl->addWidget(gb);
	hl->addLayout(vl);

	tabWidget->addTab(patternPage, tr("Fill &Pattern"));
}

void EnrichmentDialog::initGeometryPage()
{
    geometryPage = new QWidget();

	unitBox = new QComboBox();
	unitBox->insertItem(tr("inch"));
	unitBox->insertItem(tr("mm"));
	unitBox->insertItem(tr("cm"));
	unitBox->insertItem(tr("point"));
	unitBox->insertItem(tr("pixel"));
	if (d_widget_type != MDIWindow)
        unitBox->insertItem(tr("scale"));

	QBoxLayout *bl1 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl1->addWidget(new QLabel(tr( "Unit" )));
	bl1->addWidget(unitBox);

	ApplicationWindow *app = (ApplicationWindow *)parent();
	QLocale locale = QLocale();
	if (app)
		locale = app->locale();

    QGroupBox *gb1 = new QGroupBox(tr("Position"));
	xBox = new DoubleSpinBox();
	xBox->setLocale(locale);
	xBox->setDecimals(6);
	xBox->setMinimumWidth(80);
	yBox = new DoubleSpinBox();
	yBox->setLocale(locale);
	yBox->setDecimals(6);

    QGridLayout *gl1 = new QGridLayout();
    gl1->addWidget(new QLabel( tr("X")), 0, 0);
    gl1->addWidget(xBox, 0, 1);
    gl1->addWidget(new QLabel(tr("Y")), 1, 0);
    gl1->addWidget(yBox, 1, 1);
	gl1->setColumnStretch(1, 10);
	gl1->setRowStretch(2, 1);
    gb1->setLayout(gl1);

    QGroupBox *gb2 = new QGroupBox(tr("Size"));
    widthBox = new DoubleSpinBox();
	widthBox->setLocale(locale);
	widthBox->setDecimals(6);
	heightBox = new DoubleSpinBox();
	heightBox->setLocale(locale);
	heightBox->setDecimals(6);

    QGridLayout *gl2 = new QGridLayout();
    gl2->addWidget(new QLabel( tr("Width")), 0, 0);
    gl2->addWidget(widthBox, 0, 1);

    gl2->addWidget(new QLabel(tr("Height")), 1, 0);
    gl2->addWidget(heightBox, 1, 1);

	keepAspectBox = new QCheckBox(tr("&Keep aspect ratio"));
	gl2->addWidget(keepAspectBox, 2, 1);

	bestSizeButton = new QPushButton(tr("&Best size"));
	bestSizeButton->hide();
	gl2->addWidget(bestSizeButton, 3, 1);

	gl2->setColumnStretch(1, 10);
	gl2->setRowStretch(4, 1);
    gb2->setLayout(gl2);

    QBoxLayout *bl2 = new QBoxLayout (QBoxLayout::LeftToRight);
	bl2->addWidget(gb1);
	bl2->addWidget(gb2);

	if (d_widget_type == Text)
		gb2->setEnabled(false);

	QVBoxLayout* vl = new QVBoxLayout(geometryPage);
    vl->addLayout(bl1);
    vl->addLayout(bl2);

	connect(unitBox, SIGNAL(activated(int)), this, SLOT(displayCoordinates(int)));
	connect(widthBox, SIGNAL(valueChanged(double)), this, SLOT(adjustHeight(double)));
	connect(heightBox, SIGNAL(valueChanged(double)), this, SLOT(adjustWidth(double)));
	connect(bestSizeButton, SIGNAL(clicked()), this, SLOT(setBestSize()));

	tabWidget->addTab(geometryPage, tr( "&Geometry" ) );
}

void EnrichmentDialog::customButtons(QWidget *w)
{
	if (d_widget_type == Tex && editPage && w == editPage && clearButton){
		clearButton->show();
		return;
	} else if (clearButton)
		clearButton->hide();

	if (w == framePage)
		updateButton->setEnabled(true);
}

void EnrichmentDialog::setWidget(QWidget *w)
{
	if (!w)
		return;

	ApplicationWindow *app = (ApplicationWindow *)parent();
	if (!app)
		return;

	d_widget = w;

    FrameWidget *fw = qobject_cast<FrameWidget *>(d_widget);
    if (fw){
		frameBox->blockSignals(true);
        frameBox->setCurrentIndex(fw->frameStyle());
		frameBox->blockSignals(false);

		frameColorBtn->blockSignals(true);
        frameColorBtn->setColor(fw->frameColor());
		frameColorBtn->blockSignals(false);

		boxFrameLineStyle->blockSignals(true);
		boxFrameLineStyle->setStyle(fw->framePen().style());
		boxFrameLineStyle->blockSignals(false);

		boxFrameWidth->blockSignals(true);
		if (d_widget_type == Ellipse)
			boxFrameWidth->setValue(fw->framePen().widthF());
		else
			boxFrameWidth->setValue(fw->framePen().width());
		boxFrameWidth->blockSignals(false);

		unitBox->setCurrentIndex(app->d_frame_geometry_unit);
		displayCoordinates(app->d_frame_geometry_unit);
    } else {
		unitBox->setCurrentIndex(FrameWidget::Pixel);
		displayCoordinates(FrameWidget::Pixel);
	}

	if (d_widget_type == Text){
		LegendWidget *l = qobject_cast<LegendWidget *>(d_widget);
		if (l){
			setText(textEditBox, l->text());
			textFont = l->font();
			QFont fnt = textFont;
			fnt.setPointSize(QFont().pointSize() + 2);
			textEditBox->setFont(fnt);
			textColorBtn->blockSignals(true);
			textColorBtn->setColor(l->textColor());
			textColorBtn->blockSignals(false);

			QColor bc = l->backgroundColor();
			boxBackgroundTransparency->blockSignals(true);
			boxBackgroundTransparency->setValue(bc.alpha());
			boxBackgroundTransparency->blockSignals(false);

			textBackgroundBtn->blockSignals(true);
			textBackgroundBtn->setEnabled(bc.alpha());
			textBackgroundBtn->setColor(bc);
			textBackgroundBtn->blockSignals(false);

			boxTextAngle->blockSignals(true);
            boxTextAngle->setValue(l->angle());
			boxTextAngle->blockSignals(false);
			autoUpdateTextBox->setChecked(l->isAutoUpdateEnabled());
		}
	} else if (d_widget_type == Tex){
		TexWidget *tw = qobject_cast<TexWidget *>(d_widget);
		if (tw){
			setText(equationEditor, tw->formula());
			outputLabel->setPixmap(tw->pixmap());
			bestSizeButton->show();
		}
		return;
	} else if (d_widget_type == Image){
		ImageWidget *i = qobject_cast<ImageWidget *>(d_widget);
		if (i){
			imagePathBox->setText(i->fileName());
			boxSaveImagesInternally->blockSignals(true);
			boxSaveImagesInternally->setChecked(i->saveInternally());
			boxSaveImagesInternally->blockSignals(false);
		}
	} else if (d_widget_type == Frame){
		RectangleWidget *r = qobject_cast<RectangleWidget *>(d_widget);
		if (r){
			backgroundColorBtn->blockSignals(true);
			backgroundColorBtn->setColor(r->backgroundColor());
			backgroundColorBtn->blockSignals(false);

			boxTransparency->blockSignals(true);
			boxTransparency->setValue(r->backgroundColor().alpha());
			boxTransparency->blockSignals(false);

			patternBox->blockSignals(true);
			patternBox->setPattern(r->brush().style());
			patternBox->blockSignals(false);

			patternColorBtn->blockSignals(true);
			patternColorBtn->setColor(r->brush().color());
			patternColorBtn->blockSignals(false);
		}
	} else if (d_widget_type == Ellipse){
		EllipseWidget *r = qobject_cast<EllipseWidget *>(d_widget);
		if (r){
			backgroundColorBtn->blockSignals(true);
			backgroundColorBtn->setColor(r->backgroundColor());
			backgroundColorBtn->blockSignals(false);

			boxTransparency->blockSignals(true);
			boxTransparency->setValue(r->backgroundColor().alpha());
			boxTransparency->blockSignals(false);

			patternBox->blockSignals(true);
			patternBox->setPattern(r->brush().style());
			patternBox->blockSignals(false);

			patternColorBtn->blockSignals(true);
			patternColorBtn->setColor(r->brush().color());
			patternColorBtn->blockSignals(false);
		}
	}
}

void EnrichmentDialog::clearForm()
{
    outputLabel->setPixmap(QPixmap());
    equationEditor->clear();
}

void EnrichmentDialog::apply()
{
	if (tabWidget->currentPage() == editPage)
		fetchImage();
	else if (tabWidget->currentPage() == framePage)
		frameApplyTo();
	else if (imagePage && tabWidget->currentPage() == imagePage)
		chooseImageFile(imagePathBox->text());
	else if (tabWidget->currentPage() == geometryPage)
		setCoordinates(unitBox->currentIndex());
	else if (patternPage && tabWidget->currentPage() == patternPage)
		patternApplyTo();
	else if (textPage && tabWidget->currentPage() == textPage){
		LegendWidget *l = qobject_cast<LegendWidget *>(d_widget);
		if (l)
			l->setText(textEditBox->text());

		textFormatApplyTo();
		ApplicationWindow *app = (ApplicationWindow *)this->parent();
		if (app)
			app->setFormatBarFont(textFont);
	}
}

QString EnrichmentDialog::createTempTexFile()
{
	QString path = QDir::tempPath();
	QString name = path + "/" + "QtiPlot_temp.tex";

	QFile file(name);

	if (file.open(QIODevice::WriteOnly)){
		QTextStream t( &file );
		t.setEncoding(QTextStream::UnicodeUTF8);
		t << "\\documentclass{article}\n";
		t << "\\pagestyle{empty}\n";
		t << "\\begin{document}\n";
		t << "\\huge{\\mbox{$";
		t << equationEditor->toPlainText();
		t << "$}}\n";
		t << "\\end{document}";
		file.close();
		return QDir::cleanPath(name);
	}

	return QString();
}

void EnrichmentDialog::fetchImage()
{
	TexWidget *tw = qobject_cast<TexWidget *>(d_widget);
	if (tw && tw->formula() == equationEditor->toPlainText() && !tw->pixmap().isNull())
		return;

	clearButton->setEnabled(false);
    updateButton->setEnabled(false);
    equationEditor->setReadOnly(true);

	if (texCompilerBox->currentIndex() == 1){
		if (compileProcess)
			delete compileProcess;

		if (!validateCompiler())
			return;

		compileProcess = new QProcess(this);
		connect(compileProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
			this, SLOT(finishedCompiling(int, QProcess::ExitStatus)));
		connect(compileProcess, SIGNAL(error(QProcess::ProcessError)),
			this, SLOT(displayCompileError(QProcess::ProcessError)));

		compileProcess->setWorkingDirectory(QDir::tempPath());

		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

		QString program = ((ApplicationWindow *)parentWidget())->d_latex_compiler_path;
		QStringList arguments;
		arguments << createTempTexFile();

		compileProcess->start(program, arguments);
		return;
	}

    QUrl url;
    url.setPath("/cgi-bin/mathtran");
    url.setQueryDelimiters('=', ';');
    url.addQueryItem("D", "3");
    url.addQueryItem("tex", QUrl::toPercentEncoding(
                     equationEditor->toPlainText()));

    http->get(url.toString());

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void EnrichmentDialog::updateForm(bool error)
{
	QApplication::restoreOverrideCursor();

    if (!error) {
        QImage image;
        if (image.loadFromData(http->readAll())) {
            QPixmap pixmap = QPixmap::fromImage(image);
            outputLabel->setPixmap(pixmap);
			TexWidget *tw = qobject_cast<TexWidget *>(d_widget);
			if (tw){
				tw->setPixmap(pixmap);
				tw->setFormula(equationEditor->toPlainText());
				d_plot->multiLayer()->notifyChanges();
			}
        }
    } else {
		QMessageBox::critical((QWidget *)parent(), tr("QtiPlot") + " - " + tr("Network connection error"),
		tr("Error while trying to connect to host %1:").arg("mathtran.org") + "\n\n'" +
		http->errorString() + "'\n\n" + tr("Please verify your network connection!"));
	}

    clearButton->setEnabled(true);
    updateButton->setEnabled(true);
    equationEditor->setReadOnly(false);
}

void EnrichmentDialog::chooseImageFile(const QString& fn)
{
	ApplicationWindow *app = (ApplicationWindow *)parentWidget();
	if (!app)
		return;

	QString path = fn;
	if (path.isEmpty())
		path = ApplicationWindow::getFileName(this, tr("QtiPlot - Import image from file"), app->imagesDirPath,
					ApplicationWindow::imageFilter(), 0, false);

	if (!path.isEmpty()){
		ImageWidget *i = qobject_cast<ImageWidget *>(d_widget);
		if (i && i->load(path)){
			imagePathBox->setText(path);
			QFileInfo fi(path);
			app->imagesDirPath = fi.dirPath(true);
			app->modifiedProject();
		}
	}
}

void EnrichmentDialog::saveImagesInternally(bool save)
{
	ImageWidget *i = qobject_cast<ImageWidget *>(d_widget);
	if (i)
		i->setSaveInternally(boxSaveImagesInternally->isChecked());

	d_plot->multiLayer()->notifyChanges();

	if (save)
		return;

	QString fn = imagePathBox->text();
	if (fn.isEmpty() || !QFile::exists(fn)){
		QMessageBox::warning((ApplicationWindow *)parentWidget(), tr("QtiPlot - Warning"),
		tr("The file %1 doesn't exist. The image cannot be restored when reloading the project file!").arg(fn));
		chooseImageFile();
	}
}

void EnrichmentDialog::setCoordinates(int unit)
{
	if (!d_widget)
		return;

	if (unit == FrameWidget::Scale){//ScaleCoordinates
		double left = xBox->value();
		double top = yBox->value();
		FrameWidget *fw = qobject_cast<FrameWidget *>(d_widget);
        if (fw)
            fw->setCoordinates(left, top, left + widthBox->value(), top - heightBox->value());
	} else
		FrameWidget::setRect(d_widget, xBox->value(), yBox->value(),
        widthBox->value(), heightBox->value(), (FrameWidget::Unit)unit);

    if (d_plot)
        d_plot->multiLayer()->notifyChanges();

	ApplicationWindow *app = (ApplicationWindow *)parentWidget();
	if (app)
		app->d_frame_geometry_unit = unit;
}

void EnrichmentDialog::displayCoordinates(int unit)
{
	if (!d_widget)
		return;

	if (unit == FrameWidget::Pixel || unit == FrameWidget::Point){
		xBox->setFormat('f', 0);
		yBox->setFormat('f', 0);
		widthBox->setFormat('f', 0);
		heightBox->setFormat('f', 0);

		xBox->setSingleStep(1.0);
		yBox->setSingleStep(1.0);
		widthBox->setSingleStep(1.0);
		heightBox->setSingleStep(1.0);
	} else {
		xBox->setFormat('g', 6);
		yBox->setFormat('g', 6);
		widthBox->setFormat('g', 6);
		heightBox->setFormat('g', 6);

		xBox->setSingleStep(0.1);
		yBox->setSingleStep(0.1);
		widthBox->setSingleStep(0.1);
		heightBox->setSingleStep(0.1);
	}

	xBox->setValue(FrameWidget::xIn(d_widget, (FrameWidget::Unit)unit));
	yBox->setValue(FrameWidget::yIn(d_widget, (FrameWidget::Unit)unit));
	widthBox->setValue(FrameWidget::widthIn(d_widget, (FrameWidget::Unit)unit));
	heightBox->setValue(FrameWidget::heightIn(d_widget, (FrameWidget::Unit)unit));

	aspect_ratio = widthBox->value()/heightBox->value();
}

void EnrichmentDialog::adjustHeight(double width)
{
	if (keepAspectBox->isChecked()){
		heightBox->blockSignals(true);
		heightBox->setValue(width/aspect_ratio);
		heightBox->blockSignals(false);
	} else
		aspect_ratio = width/heightBox->value();
}

void EnrichmentDialog::adjustWidth(double height)
{
	if (keepAspectBox->isChecked()){
		widthBox->blockSignals(true);
		widthBox->setValue(height*aspect_ratio);
		widthBox->blockSignals(false);
	} else
		aspect_ratio = widthBox->value()/height;
}

void EnrichmentDialog::setBestSize()
{
	TexWidget *tw = qobject_cast<TexWidget *>(d_widget);
	if (tw){
		tw->setBestSize();
		displayCoordinates(unitBox->currentIndex());
		d_plot->multiLayer()->notifyChanges();
	}
}

void EnrichmentDialog::frameApplyTo()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	switch(frameApplyToBox->currentIndex()){
		case 0://this layer
		{
			FrameWidget *fw = qobject_cast<FrameWidget *>(d_widget);
			if (fw)
				setFrameTo(fw);
		}
		break;

		case 1://this layer
		{
			QList <FrameWidget *> lst = d_plot->enrichmentsList();
			foreach(FrameWidget *fw, lst)
				setFrameTo(fw);
		}
		break;

		case 2://this window
		{
			QList<Graph *> layersLst = d_plot->multiLayer()->layersList();
			foreach(Graph *g, layersLst){
				QList <FrameWidget *> lst = g->enrichmentsList();
				foreach(FrameWidget *fw, lst)
					setFrameTo(fw);
			}
		}
		break;

		case 3://all windows
		{
			QList<MdiSubWindow *> windows = app->windowsList();
			foreach(MdiSubWindow *w, windows){
				MultiLayer *ml = qobject_cast<MultiLayer *>(w);
				if (!ml)
					continue;
				QList<Graph *> layersLst = ml->layersList();
				foreach(Graph *g, layersLst){
					QList <FrameWidget *> lst = g->enrichmentsList();
					foreach(FrameWidget *fw, lst)
						setFrameTo(fw);
				}
			}
		}
		break;

		default:
			break;
	}
	app->modifiedProject();
}

void EnrichmentDialog::setFrameTo(FrameWidget *fw)
{
	fw->setFrameStyle(frameBox->currentIndex());
	QPen pen = QPen(frameColorBtn->color(), boxFrameWidth->value(),
				boxFrameLineStyle->style(), Qt::SquareCap, Qt::MiterJoin);
	fw->setFramePen(pen);
	fw->repaint();
}

void EnrichmentDialog::patternApplyTo()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	switch(patternApplyToBox->currentIndex()){
		case 0://this object
		{
			FrameWidget *r = qobject_cast<FrameWidget *>(d_widget);
			if (r)
				setPatternTo(r);
		}
		break;

		case 1://this layer
		{
			QList <FrameWidget *> lst = d_plot->enrichmentsList();
			foreach(FrameWidget *fw, lst)
				setPatternTo(fw);
		}
		break;

		case 2://this window
		{
			QList<Graph *> layersLst = d_plot->multiLayer()->layersList();
			foreach(Graph *g, layersLst){
				QList <FrameWidget *> lst = g->enrichmentsList();
				foreach(FrameWidget *fw, lst)
						setPatternTo(fw);
			}
		}
		break;

		case 3://all windows
		{
			QList<MdiSubWindow *> windows = app->windowsList();
			foreach(MdiSubWindow *w, windows){
				MultiLayer *ml = qobject_cast<MultiLayer *>(w);
				if (!ml)
					continue;
				QList<Graph *> layersLst = ml->layersList();
				foreach(Graph *g, layersLst){
					QList <FrameWidget *> lst = g->enrichmentsList();
					foreach(FrameWidget *fw, lst)
						setPatternTo(fw);
				}
			}
		}
		break;

		default:
			break;
	}
	app->modifiedProject();
}

void EnrichmentDialog::setPatternTo(FrameWidget *r)
{
	QColor c = backgroundColorBtn->color();
	c.setAlpha(boxTransparency->value());
	r->setBackgroundColor(c);

	QColor patternColor = patternColorBtn->color();
	if (useFrameColorBox->isChecked())
		patternColor = frameColorBtn->color();
	r->setBrush(QBrush(patternColor, patternBox->getSelectedPattern()));

	r->repaint();
}

void EnrichmentDialog::setText(QTextEdit *editor, const QString & t)
{
	if (!editor)
		return;

	QTextCursor cursor = editor->textCursor();
	// select the whole (old) text
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
	// replace old text
	cursor.insertText(t);
	// select the whole (new) text
	cursor.movePosition(QTextCursor::Start);
	cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
	// this line makes the selection visible to the user
	// (the 2 lines above only change the selection in the
	// underlying QTextDocument)
	editor->setTextCursor(cursor);
	// give focus back to text edit
	editor->setFocus();
}

void EnrichmentDialog::customFont()
{
	bool okF;
	QFont fnt = QFontDialog::getFont( &okF, textFont, this);
	if (okF && fnt != textFont){
		textFont = fnt;

		fnt.setPointSize(QFont().pointSize() + 2);
		textEditBox->setFont(fnt);
	}

}

void EnrichmentDialog::updateTransparency(int alpha)
{
	textBackgroundBtn->setEnabled(alpha);
	textFormatApplyTo();
}

void EnrichmentDialog::textFormatApplyTo()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	switch(textApplyToBox->currentIndex()){
		case 0://this object
		{
			LegendWidget *l = qobject_cast<LegendWidget *>(d_widget);
			if (l)
				setTextFormatTo(l);
		}
		break;

		case 1://this layer
		{
			QList <FrameWidget *> lst = d_plot->enrichmentsList();
			foreach(FrameWidget *fw, lst){
				LegendWidget *l = qobject_cast<LegendWidget *>(fw);
				if (l)
					setTextFormatTo(l);
			}
		}
		break;

		case 2://this window
		{
			QList<Graph *> layersLst = d_plot->multiLayer()->layersList();
			foreach(Graph *g, layersLst){
				QList <FrameWidget *> lst = g->enrichmentsList();
				foreach(FrameWidget *fw, lst){
					LegendWidget *l = qobject_cast<LegendWidget *>(fw);
                    if (l)
                        setTextFormatTo(l);
				}
			}
		}
		break;

		case 3://all windows
		{
			QList<MdiSubWindow *> windows = app->windowsList();
			foreach(MdiSubWindow *w, windows){
				MultiLayer *ml = qobject_cast<MultiLayer *>(w);
				if (!ml)
					continue;
				QList<Graph *> layersLst = ml->layersList();
				foreach(Graph *g, layersLst){
					QList <FrameWidget *> lst = g->enrichmentsList();
					foreach(FrameWidget *fw, lst){
						LegendWidget *l = qobject_cast<LegendWidget *>(fw);
                        if (l)
                            setTextFormatTo(l);
					}
				}
			}
		}
		break;

		default:
			break;
	}
	app->modifiedProject();
}

void EnrichmentDialog::setTextFormatTo(LegendWidget *l)
{
    QColor c = textBackgroundBtn->color();
    c.setAlpha(boxBackgroundTransparency->value());
    l->setBackgroundColor(c);
    l->setTextColor(textColorBtn->color());
    l->setFont(textFont);
    l->setAngle(boxTextAngle->value());
    l->setAutoUpdate(autoUpdateTextBox->isChecked());
    l->repaint();
}

void EnrichmentDialog::setTextDefaultValues()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
		return;

	app->legendTextColor = textColorBtn->color();
	app->plotLegendFont = textFont;

	QColor c = textBackgroundBtn->color();
	c.setAlpha(boxBackgroundTransparency->value());
	app->legendBackground = c;
	app->d_legend_default_angle = boxTextAngle->value();
	app->saveSettings();
}

void EnrichmentDialog::setFrameDefaultValues()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
		return;

	app->legendFrameStyle = frameBox->currentIndex();
	app->d_frame_widget_pen = QPen(frameColorBtn->color(), boxFrameWidth->value(), boxFrameLineStyle->style());
	app->saveSettings();
}

void EnrichmentDialog::setRectangleDefaultValues()
{
	ApplicationWindow *app = (ApplicationWindow *)this->parent();
	if (!app)
		return;

	QColor c = backgroundColorBtn->color();
	c.setAlpha(boxTransparency->value());
	app->d_rect_default_background = c;

	QColor patternColor = patternColorBtn->color();
	if (useFrameColorBox->isChecked())
		patternColor = frameColorBtn->color();

	app->d_rect_default_brush = QBrush(patternColor, patternBox->getSelectedPattern());
	app->saveSettings();
}

void EnrichmentDialog::createImage()
{
	QApplication::restoreOverrideCursor();

	QString path = QDir::tempPath();
	QString fileName = QDir::cleanPath(path + "/" + "QtiPlot_temp.png");

	QImage image;
	if (image.load(fileName)){
		QPixmap pixmap = QPixmap::fromImage(image);
		outputLabel->setPixmap(pixmap);
		TexWidget *tw = qobject_cast<TexWidget *>(d_widget);
		if (tw){
			tw->setPixmap(pixmap);
			tw->setFormula(equationEditor->toPlainText());
			d_plot->multiLayer()->notifyChanges();
		}
	}

    clearButton->setEnabled(true);
    updateButton->setEnabled(true);
    equationEditor->setReadOnly(false);

	if (dvipngProcess)
		delete dvipngProcess;
    dvipngProcess = NULL;
    QFile::remove(fileName);

    fileName = QDir::cleanPath(path + "/" + "QtiPlot_temp.dvi");
    QFile::remove(fileName);
}

void EnrichmentDialog::finishedCompiling(int exitCode, QProcess::ExitStatus exitStatus)
{
	QApplication::restoreOverrideCursor();
	if (exitStatus != QProcess::NormalExit){
		QMessageBox::critical(this, tr("Compile process ended"),
		tr("Compiling process ended with exit code: %1").arg(exitCode));
		return;
	}

	QString compiler = ((ApplicationWindow *)parentWidget())->d_latex_compiler_path;
	QFileInfo fi(compiler);

	QString dir = fi.dir().absolutePath();
        QString program = dir + "/dvipng";
        #ifdef Q_OS_WIN
            program += ".exe";
        #endif

	QStringList arguments;
	arguments << "-T" << "tight" << "QtiPlot_temp.dvi" << "-o" << "QtiPlot_temp.png";

	if (compileProcess)
		delete compileProcess;
	compileProcess = NULL;

	dvipngProcess = new QProcess(this);
	connect(dvipngProcess, SIGNAL(error(QProcess::ProcessError)),
		this, SLOT(displayCompileError(QProcess::ProcessError)));
	connect(dvipngProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
		this, SLOT(createImage()));

	dvipngProcess->setWorkingDirectory (QDir::tempPath());

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	dvipngProcess->start(program, arguments);
	return;
}

void EnrichmentDialog::displayCompileError(QProcess::ProcessError error)
{
	QString process = QString::null;
	if (compileProcess)
		process = tr("LaTeX compile process");
	else if (dvipngProcess)
		process = tr("dvipng process");

	QString msg;
	switch(error){
		case QProcess::FailedToStart:
			if (compileProcess)
				msg = process + " " + tr("failed to start!");
			else if (dvipngProcess)
				msg = process + " " + tr("failed to start!") + " " +
				tr("Please verify that you have dvipng installed in the same folder as your LaTeX compiler!");
		break;

		case QProcess::Crashed:
			msg = process + " " + tr("crashed");
		break;

		case QProcess::Timedout:
			msg = process + " " + tr("timedout");
		break;

		case QProcess::WriteError:
			msg = process + " " + tr("write error");
		break;

		case QProcess::ReadError:
			msg = process + " " + tr("read error");
		break;

		case QProcess::UnknownError:
			msg =  process + " " + tr("unknown error");
		break;
	}

	QApplication::restoreOverrideCursor();
	if (compileProcess){
		compileProcess->kill();
		compileProcess = NULL;
	} else if (dvipngProcess){
		dvipngProcess->kill();
		dvipngProcess = NULL;
	}

	QMessageBox::critical(this, tr("Compile error"), msg);

	clearButton->setEnabled(true);
    updateButton->setEnabled(true);
    equationEditor->setReadOnly(false);
}

void EnrichmentDialog::updateCompilerInterface(int compiler)
{
	switch(compiler){
		case 0:
			compilerPathGroupBox->hide();
			((ApplicationWindow *)parentWidget())->d_latex_compiler_path = QString::null;
		break;

		case 1:
			compilerPathGroupBox->show();
		break;
	}
}

void EnrichmentDialog::chooseCompiler()
{
	ApplicationWindow *app = (ApplicationWindow *)parentWidget();
	if (!app)
		return;

	QFileInfo tfi(app->d_latex_compiler_path);
	QString compiler = ApplicationWindow::getFileName(this, tr("Choose the location of the LaTeX compiler!"),
	app->d_latex_compiler_path, QString(), 0, false);

	if (!compiler.isEmpty()){
		app->d_latex_compiler_path = QDir::toNativeSeparators(compiler);
		compilerPathBox->setText(app->d_latex_compiler_path);
	}
}

bool EnrichmentDialog::validateCompiler()
{
	QString path = compilerPathBox->text();
	if (path.isEmpty())
		return false;

	ApplicationWindow *app = (ApplicationWindow *)parentWidget();
	QFileInfo fi(path);
	if (!fi.exists()){
		QMessageBox::critical(this, tr("QtiPlot - File Not Found!"),
		tr("The file %1 doesn't exist.<br>Please choose another file!").arg(path));
		compilerPathBox->setText(app->d_latex_compiler_path);
		return false;
	}

	if (fi.isDir()){
		QMessageBox::critical(this, tr("QtiPlot - File Not Found!"),
		tr("%1 is a folder.<br>Please choose a file!").arg(path));
		compilerPathBox->setText(app->d_latex_compiler_path);
		return false;
	}

	if (!fi.isReadable()){
		QMessageBox::critical(this, tr("QtiPlot"),
		tr("You don't have read access rights to file %1.<br>Please choose another file!").arg(path));
		compilerPathBox->setText(app->d_latex_compiler_path);
		return false;
	}

	app->d_latex_compiler_path = QDir::toNativeSeparators(path);
	compilerPathBox->setText(app->d_latex_compiler_path);
	return true;
}

EnrichmentDialog::~EnrichmentDialog()
{
	QApplication::restoreOverrideCursor();

	TexWidget *tw = qobject_cast<TexWidget *>(d_widget);
	if (tw && (tw->formula().isEmpty() || tw->pixmap().isNull())){
		d_plot->remove(tw);
		d_plot->setActiveTool(NULL);
		d_widget->close();
		d_widget = NULL;
	}
}
