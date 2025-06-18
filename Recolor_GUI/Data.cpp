#include "data.h"
#include "utility.h"
#include <algorithm>
#include <cmath>
#include <QMessagebox>
#include <QTime>
#include <omp.h>
#include <map>
#include <fstream>
#include <opencv2/opencv.hpp>
using namespace std;

Data::Data(){

}

void Data::OpenImage(QString fileName1, QString fileName2 ){
	originalImg = Image(fileName1.toStdString(), fileName2.toStdString());
	recoloredImg = originalImg.GetAllPixelsRGB();
	layer_path = fileName1.toStdString();
	layer_path = layer_path.substr(0, layer_path.rfind('/'));
	Spix = SuperpixSeg(fileName1.toStdString(), 20);
	emit updated();
}

void Data::Reset() { 
	originalImg = Image();
	recoloredImg.release();
	Spix = SuperpixSeg();
	Pbr = PaletteBasedRecoloring();
	ori_color.clear();
	edit_color.clear();
	edit_positions.clear();
}

void Data::ComputeWeights() {
	cout << "compute weights" << endl;
	Pbr = PaletteBasedRecoloring(originalImg, Spix);
	cout << "got superpixels" << endl;
	Pbr.ExtractPaletteWithKmenas();
	Pbr.PreComputeWeights();
	cout << "calculated weights" << endl;
	QMessageBox::information(NULL, "INFO", "Computing Weights finished", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
	Pbr.PrintWeightMaps(layer_path);
}

void Data::AddEditColor(QPoint pos) {
	int index = pos.y() * originalImg.img_w + pos.x(); //pos.y()表示竖直方向(高)
	edit_positions.push_back(cv::Vec2i(pos.y(), pos.x()));
	edit_color.push_back(originalImg.features[index].GetColor());
	ori_color.push_back(originalImg.features[index].GetColor());
}

void Data::Recolor() {
	cout << "exe recolor" << endl;
	if (!Pbr.weight_calculated) {
		QMessageBox::information(NULL, "WARNING", "Yet Computing Weights", QMessageBox::Yes, QMessageBox::Yes);
		return;
	}
	if (edit_color.empty())
		return;

	QTime time1;
	time1.start();
	recoloredImg = Pbr.RecolorWithEditedPixels(edit_color, edit_positions);
	qDebug() << "Recolor time:" << time1.elapsed() / 1000.0 << "s";
	emit updated();
}

void Data::SetEditColor(int id, QColor c) {
	double r = qRed(c.rgb())	/ 255.0;
	double g = qGreen(c.rgb())	/ 255.0;
	double b = qBlue(c.rgb())	/ 255.0;
	edit_color.at(id) = cv::Vec3d(r, g, b);
}

void Data::ResetEditColor(int id) {
	edit_color.at(id) = ori_color.at(id);
	Recolor();
	emit updated();
}

void Data::ResetAllEditColors() {
	for (int i = 0; i < ori_color.size(); i++)
		edit_color[i] = ori_color[i];
	recoloredImg = originalImg.GetAllPixelsRGB();
	emit updated();
}

void Data::DeleteEditColor(int id) {
	if (id < 0 || id >= ori_color.size())
		return;
	ori_color.erase(ori_color.begin() + id);
	edit_color.erase(edit_color.begin() + id);
	edit_positions.erase(edit_positions.begin() + id);
	if (edit_color.empty()) {
		recoloredImg = originalImg.GetAllPixelsRGB();
		emit updated();
	}
}

void Data::ExportRecoloredImage(string path) { 
	QImage image(originalImg.img_w, originalImg.img_h, QImage::Format_RGB888); //RGB
	for (int i = 0; i < originalImg.img_h; i++) {
		uchar* const row = image.scanLine(i);
		for (int j = 0; j < originalImg.img_w; j++) {
			cv::Vec3b pixel(recoloredImg.at<cv::Vec3b>(i, j));
			row[j * 3] = pixel[0];
			row[j * 3 + 1] = pixel[1];
			row[j * 3 + 2] = pixel[2];
		}
	}
	if (path.substr(path.size() - 4) != ".png")
		path += ".png";
	image.save(path.c_str(), "PNG", 100);
}

void Data::ImportEdit(string path) {
	std::ifstream ifs(path);
	int n; ifs >> n;
	ori_color.resize(n);
	edit_color.resize(n);
	edit_positions.resize(n);
	for (int i = 0; i < n; i++) {
		ifs >> ori_color[i][0] >> ori_color[i][1] >> ori_color[i][2];
		ifs >> edit_color[i][0] >> edit_color[i][1] >> edit_color[i][2];
		ifs >> edit_positions[i][0] >> edit_positions[i][1];
	}
	Recolor();
}

void Data::ExportEdit(string path) {
	std::ofstream ofs(path);
	ofs << ori_color.size() << std::endl;
	for (int i = 0; i < ori_color.size(); i++) {
		ofs << ori_color[i][0]	<< " "
			<< ori_color[i][1]	<< " "
			<< ori_color[i][2]	<< " "
			<< edit_color[i][0] << " "
			<< edit_color[i][1] << " "
			<< edit_color[i][2] << " "
			<< edit_positions[i][0] << " "
			<< edit_positions[i][1] << std::endl;
	}
}

void Data::ImportPalette(string path) {
	std::ifstream ifs(path);
	int n; ifs >> n;
	std::vector<Feature> palette(n);
	for (int i = 0; i < n; i++) {
		Feature feature = Feature();
		ifs >> feature.rgb[0] >> feature.rgb[1] >> feature.rgb[2];
		ifs >> feature.xy[0] >> feature.xy[1];
		for (int j = 0; j < SDIM; j++)
			ifs >> feature.semantic[j];
		palette[i] = feature;
	}
	recoloredImg = Pbr.TransferColor(palette);
	emit updated();
}

void Data::ExportPalette(string path) {
	std::cout << "path:" << path << std::endl;
	std::vector<Feature> recolor_palette = GetChangedPaletteAllMsg();

	std::ofstream ofs(path);
	ofs << recolor_palette.size() << std::endl;
	for (int i = 0; i < recolor_palette.size(); i++) {
		ofs << recolor_palette[i].rgb[0] << " "
			<< recolor_palette[i].rgb[1] << " "
			<< recolor_palette[i].rgb[2] << " "
			<< recolor_palette[i].xy[0] << " "
			<< recolor_palette[i].xy[1] << " ";
		for (int j = 0; j < SDIM; j++)
			ofs << recolor_palette[i].semantic[j] << " ";
		ofs << std::endl;
	}

	std::vector<cv::Vec3d> originals = GetOriginalPalette();
	std::vector<cv::Vec3d> recolors = GetChangedPalette();
	int margin = 20, shape = 160;

	cv::Mat savedOrPalette(shape + margin * 2, originals.size() * (margin + shape) + margin, CV_8UC3, cv::Vec3b(255, 255, 255));
	cv::Mat savedRePalette(shape + margin * 2, recolors.size() * (margin + shape) + margin, CV_8UC3, cv::Vec3b(255, 255, 255));
	for (int k = 0; k < originals.size(); k++) {
		for (int i = 0; i < shape; i++) {
			for (int j = 0; j < shape; j++) {
				savedOrPalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[0] = min(max(originals[k][2], 0.0), 1.0) * 255;
				savedOrPalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[1] = min(max(originals[k][1], 0.0), 1.0) * 255;
				savedOrPalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[2] = min(max(originals[k][0], 0.0), 1.0) * 255;

				savedRePalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[0] = min(max(recolors[k][2], 0.0), 1.0) * 255;
				savedRePalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[1] = min(max(recolors[k][1], 0.0), 1.0) * 255;
				savedRePalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[2] = min(max(recolors[k][0], 0.0), 1.0) * 255;
			}
		}
	}

	int end = path.find(".");
	string originals_saved_path = path.substr(0, end) + "_palettes.png";
	string recolors_saved_path = path.substr(0, end) + "_repalettes.png";
	cv::imwrite(originals_saved_path, savedOrPalette);
	cv::imwrite(recolors_saved_path, savedRePalette);
}