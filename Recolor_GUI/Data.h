#ifndef DATA_H
#define DATA_H

#include <QString>
#include <QObject>
#include <vector>
#include <QThread>
#include <string>
#include "utility.h"
#include "./Algorithm/SuperpixSeg.h"
#include "./Algorithm/PaletteBasedRecoloring.h"
using namespace std;

class Data : public QObject{
	Q_OBJECT

public:
	Data();
	void OpenImage(QString fileName1, QString fileNmae2);
	void Reset();

	cv::Mat GetImage(bool isAfter = true) { return isAfter ? recoloredImg : originalImg.GetAllPixelsRGB(); }
	vector<cv::Vec3d> GetOriginalPalette() { return Pbr.GetPalette(); }
	vector<cv::Vec3d> GetChangedPalette() { return Pbr.GetRePaletteRGB(); }
	vector<cv::Vec3d> GetPalette() { return Pbr.weight_calculated ? Pbr.GetRePaletteRGB() : Pbr.GetPalette(); }
	vector<Feature> GetChangedPaletteAllMsg() { return Pbr.GetRePalette(); }
	vector<cv::Vec3d> GetOriginalEdit()	const { return ori_color; }
	vector<cv::Vec3d> GetChangedEdit()	const { return edit_color; }

	void ComputeWeights();
	void Recolor();
	void AddEditColor(QPoint pos);
	void SetEditColor(int id, QColor c);
	void ResetEditColor(int id);
	void ResetAllEditColors();
	void DeleteEditColor(int id);

	void ExportRecoloredImage(string path);
	void ImportEdit(string path);
	void ExportEdit(string path);
	void ImportPalette(string path);
	void ExportPalette(string path);

	int GetFrameWidth() const { return originalImg.img_w; }
	int GetFrameHeight() const { return originalImg.img_h; }
	int GetFrameDepth() const { return originalImg.img_c; }

public slots:
signals:
	void updated();

private:
	Image originalImg;
	cv::Mat recoloredImg;
	SuperpixSeg Spix;
	PaletteBasedRecoloring Pbr;
	vector<cv::Vec2i> edit_positions;
	vector<cv::Vec3d> ori_color;
	vector<cv::Vec3d> edit_color;

	string layer_path;
};

#endif // DATA_H