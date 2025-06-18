#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QDockWidget>
#include <QFileDialog>
#include <QGroupBox>
#include "mainwindow.h"
#include "imagewidget.h"
#include "openglwidget.h"
#include "editviewwidget.h"
#include "paletteviewwidget.h"
#include "Qt-Color-Widgets\include\QtColorWidgets\color_dialog.hpp"
#include "data.h"
using namespace color_widgets;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent){
    setWindowTitle(title);
	setGeometry(100, 100, 520, 520);
    QWidget *mainWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout();

	//top main menu==============================================================================================
    QHBoxLayout* firstRowLayout = new QHBoxLayout();
    QPushButton* openImageAndFeatBtn = new QPushButton("Open Image and Features");
    QPushButton* CalcWeights= new QPushButton("Extract palette and weights ");

    firstRowLayout->addWidget(openImageAndFeatBtn);
    firstRowLayout->addWidget(CalcWeights);
    mainLayout->addLayout(firstRowLayout);

    data = new Data();
    //original image and recolored image=========================================================================
    ImageWidget* recolored_image = new ImageWidget(true);
    recolored_image->setData(data);
    ImageWidget* original_image = new ImageWidget(false);
    original_image->setData(data);

    imageBeforeDockWidget = new QDockWidget();
    imageBeforeDockWidget->setWidget(original_image);
    imageBeforeDockWidget->setWindowTitle("Original Image");
    addDockWidget(Qt::TopDockWidgetArea, imageBeforeDockWidget);
    imageBeforeDockWidget->setFloating(true);
    imageBeforeDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    imageBeforeDockWidget->setGeometry(760, 100, 400, 400);
    imageBeforeDockWidget->hide();

    imageAfterDockWidget = new QDockWidget();
    imageAfterDockWidget->setWidget(recolored_image);
    imageAfterDockWidget->setWindowTitle("Recolored Image");
    imageAfterDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::TopDockWidgetArea, imageAfterDockWidget);
    imageAfterDockWidget->setFloating(true);
    imageAfterDockWidget->setGeometry(760, 100, 400, 400);
    imageAfterDockWidget->hide();

    PaletteViewWidget* paletteViewWidget = new PaletteViewWidget();
    paletteViewWidget->setData(data);

    QDockWidget* paletteViewDockWidget = new QDockWidget();
    paletteViewDockWidget->setWidget(paletteViewWidget);
    paletteViewDockWidget->setWindowTitle("Palette View");
    paletteViewDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::TopDockWidgetArea, paletteViewDockWidget);
    paletteViewDockWidget->setFloating(true);
    paletteViewDockWidget->setGeometry(760, 100, 500, 150);
    paletteViewDockWidget->hide();

    //import and export =========================================================================================
    QHBoxLayout* secondRowLayout = new QHBoxLayout();
    QHBoxLayout* thirdRowLayout = new QHBoxLayout();
    QPushButton* exportImageeBtn = new QPushButton("Export Image");
    QPushButton* importEditBtn = new QPushButton("Import Edit");
    QPushButton* exportEditBtn = new QPushButton("Export Edit");
    QPushButton* importPaletteBtn = new QPushButton("Import Palette");
    QPushButton* exportPaletteBtn = new QPushButton("Export Palette");

    secondRowLayout->addWidget(exportImageeBtn);
    secondRowLayout->addWidget(importEditBtn);
    secondRowLayout->addWidget(exportEditBtn);
    thirdRowLayout->addWidget(importPaletteBtn);
    thirdRowLayout->addWidget(exportPaletteBtn);
    

    QGroupBox* groupBox = new QGroupBox(tr("Import and Export"));
    QVBoxLayout* vbox = new QVBoxLayout;
    vbox->addLayout(secondRowLayout);
    vbox->addLayout(thirdRowLayout);
    groupBox->setLayout(vbox);

    QDockWidget* dockEditWidget = new QDockWidget();
    EditViewWidget* editWidget = new EditViewWidget();
    editWidget->setMinimumSize(500, 150);
    editWidget->setData(data);

    dockEditWidget->setWidget(editWidget);
    dockEditWidget->setWindowTitle("Edit");
    addDockWidget(Qt::RightDockWidgetArea, dockEditWidget);
    dockEditWidget->setFloating(true);
    dockEditWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

    color_widgets::ColorDialog* dialog = new color_widgets::ColorDialog();
    dialog->setWindowTitle("Color picker");

    QDockWidget* dockColorWidget = new QDockWidget();
    dockColorWidget->setWidget(dialog);
    dockColorWidget->setWindowTitle("Color picker");
    addDockWidget(Qt::RightDockWidgetArea, dockColorWidget);
    dockColorWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);

    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(dockEditWidget);
    mainLayout->addWidget(dockColorWidget);
    mainWidget->setLayout(mainLayout);
    this->setCentralWidget(mainWidget);

    connect(openImageAndFeatBtn, &QPushButton::clicked, [=]() {this->openFile(true); });
    connect(CalcWeights, &QPushButton::clicked, data, &Data::ComputeWeights);
    connect(dialog, &ColorDialog::colorChanged, editWidget, &EditViewWidget::getColor);
    connect(editWidget, &EditViewWidget::setColor, dialog, &ColorDialog::setColor);
    connect(exportImageeBtn, &QPushButton::clicked, [=]() { this->exportImage(); });
    connect(importEditBtn, &QPushButton::clicked, [=]() { this->importEdit(); });
    connect(exportEditBtn, &QPushButton::clicked, [=]() { this->exportEdit(); });
    connect(importPaletteBtn, &QPushButton::clicked, [=]() { this->importPalette(); });
    connect(exportPaletteBtn, &QPushButton::clicked, [=]() { this->exportPalette(); });

}

MainWindow::~MainWindow(){
}

// open image 
void MainWindow::openFile(bool merge){
    if(data == nullptr) return;

    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("*.jpg *.png"));
    dialog.setViewMode(QFileDialog::Detail);

    QString image_name = "", semantic_name = "";
    if (dialog.exec()){
        QStringList fileName = dialog.selectedFiles();
        if (!fileName.isEmpty())
            image_name = fileName[0];
    }
    else return;
    
    QFileDialog dialog2(this);
    dialog2.setFileMode(QFileDialog::ExistingFile);
    dialog2.setNameFilter(tr("*.data"));
    dialog2.setFileMode(merge ? QFileDialog::ExistingFiles : QFileDialog::ExistingFile);
    if (dialog2.exec()) {
        QStringList fileName = dialog2.selectedFiles();
        if (!fileName.isEmpty())
            semantic_name = fileName[0];
    }
    else return;

    if ((!image_name.isEmpty()) && (!semantic_name.isEmpty())) {
        data->Reset();
        data->OpenImage(image_name, semantic_name);
        imageBeforeDockWidget->show();
        imageAfterDockWidget->show();
    }
}

void MainWindow::exportImage() {
	if (data == nullptr) return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Data"), ".", tr("png File (*.png)"));

    if (!fileName.isEmpty())
        data->ExportRecoloredImage(fileName.toStdString());
}

void MainWindow::importEdit() {
    if (data == nullptr) return;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Data"), ".", tr("txt File (*.txt)"));
    if (!fileName.isEmpty())
        data->ImportEdit(fileName.toStdString());
}

void MainWindow::exportEdit() {
    if (data == nullptr) return;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Data"), ".", tr("txt File (*.txt)"));
    if (!fileName.isEmpty())
        data->ExportEdit(fileName.toStdString());
}

void MainWindow::importPalette() {
    if (data == nullptr) return;
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Data"), ".", tr("txt File (*.txt)"));
    if (!fileName.isEmpty())
        data->ImportPalette(fileName.toStdString());
}

void MainWindow::exportPalette() {
    if (data == nullptr) return;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Data"), ".", tr("txt File (*.txt)"));
    if (!fileName.isEmpty())
        data->ExportPalette(fileName.toStdString());
}