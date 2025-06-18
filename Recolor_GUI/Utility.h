#ifndef UTILITY_H
#define UTILITY_H

#include <QDebug>
#include <vector>
#include <opencv2/opencv.hpp>
#include <fstream>
using namespace std;

#define SDIM 3

struct Feature { //rgbxys
	cv::Vec3d rgb;
	cv::Vec2d xy;
	vector<double> semantic;
	int clusterID = -1;

	Feature() {
		rgb = cv::Vec3d(0, 0, 0);
		xy = cv:: Vec2d(0, 0);
		semantic.resize(SDIM, 0);
	}

	cv::Vec3d GetColor() {
		return rgb;
	}

	cv::Vec2d GetCoord() {
		return xy;
	}

	vector<double> GetSemantic() {
		return semantic;
	}

	Feature operator * (const double& scalar) const {
		Feature f;
		f.rgb = rgb * scalar;
		f.xy = xy * scalar;
		for (int i = 0; i < semantic.size(); i++)
			f.semantic[i] = semantic[i] * scalar;
		return f;
	}

	void operator *= (double n) {
		rgb *= n;
		xy *= n;
		for (int i = 0; i < semantic.size(); i++)
			semantic[i] *= n;
	}

	void operator /= (double n) {
		rgb /= n;
		xy /= n;
		for (int i = 0; i < semantic.size(); i++)
			semantic[i] /= n;
	}

	void operator += (Feature f) {
		rgb += f.rgb;
		xy += f.xy;
		for (int i = 0; i < semantic.size(); i++)
			semantic[i] += f.semantic[i];
	}

	Feature operator - (Feature f) {
		Feature ans;
		ans.rgb = rgb - f.rgb;
		ans.xy = xy - xy;
		for (int i = 0; i < semantic.size(); i++)
			ans.semantic[i] = semantic[i] - f.semantic[i];
		return ans;
	}

	double norm() {
		double rgb_norm2 = cv::norm(rgb, cv::NORM_L2SQR);
		double xy_norm2 = cv::norm(xy, cv::NORM_L2SQR);
		double seman_norm2 = cv::norm(cv::Vec3d(semantic[0], semantic[1], semantic[2]), cv::NORM_L2SQR);
		return sqrt(rgb_norm2 + xy_norm2 + seman_norm2);
	}
};

inline double GetNormOf(vector<double> v1, vector<double> v2) {
	double ans = 0;
	for (int i = 0; i < v1.size(); i++)
		ans += (v1[i] - v2[i]) * (v1[i] - v2[i]);
	return sqrt(ans);
}

inline double GetColorDistanceOf(Feature f1, Feature f2) {
	cv::Vec3d color_diff(f1.rgb - f2.rgb);
	double dis_color = cv::norm(color_diff);
	return dis_color;
}

inline double GetPositionDistanceOf(Feature f1, Feature f2) {
	cv::Vec2d space_diff(f1.xy - f2.xy);
	double dis_space = cv::norm(space_diff);
	return dis_space;
}

inline double GetSemanticDistanceOf(Feature f1, Feature f2) {
	vector<double> semantic_diff(f1.semantic.size());
	double semantic_diff_len = 0;
	for (int i = 0; i < SDIM; i++) {
		semantic_diff[i] = f1.semantic[i] - f2.semantic[i];
		semantic_diff_len += semantic_diff[i] * semantic_diff[i];
	}
	return sqrt(semantic_diff_len);
}

inline double GetWeightedDistanceOf(Feature f1, Feature f2) {
	double w_color = 1;
	double w_semantic = 3;
	double dis_color = GetColorDistanceOf(f1, f2);
	double dis_seman = GetSemanticDistanceOf(f1, f2);
	double dis = w_color * dis_color + w_semantic * dis_seman;
	return dis;
}

struct Image {
	std::vector<Feature> features;
	cv::Mat img;
	int img_h, img_w, img_c;

	Image() {
		img_h = 0;
		img_w = 0;
		img_c = 0;
	}

	Image(std::string img_path, std::string seman_img_path) {
		img = cv::imread(img_path);
		img_h = img.rows;
		img_w = img.cols;
		img_c = img.channels();
		features.resize(img_h * img_w);

		#pragma omp parallel for
		for (int row = 0; row < img_h; row++) {
			uchar* data = img.ptr<uchar>(row);
			for (int col = 0; col < img_w; col++) {
				uchar B = data[img_c * col + 0];
				uchar G = data[img_c * col + 1];
				uchar R = data[img_c * col + 2];

				int k = row * img_w + col;
				features[k].rgb = cv::Vec3d(R / 255.0, G / 255.0, B / 255.0);
				features[k].xy = cv::Vec2d(row / (img_h - 1.0), col / (img_w - 1.0));
			}
		}

		int n = img_h * img_w;
		double* feats = new double[n * SDIM];
		std::ifstream in(seman_img_path, std::ios::in | std::ios::binary);
		in.read((char*)feats, n * SDIM * sizeof(double));

		for (int i = 0; i < n; i++) {
			for (int j = 0; j < SDIM; j++) {
				features[i].semantic[j] = feats[SDIM * i + j];
			}
		}
		in.close();
		free(feats);
	}

	std::vector<double> GetSemanticAt(int pos) {
		vector<double> res(SDIM,0);
		int y = pos / img_w, x = pos % img_w, k = 2, cnt = 0, id;
		for (int r = y - k; r <= y + k; r++) {
			for (int c = x - k; c <= x + k; c++) {
				if (0 <= r && r < img_h && 0 <= c && c < img_w) {
					id = r * img_w + c; cnt++;
					for (int i = 0; i < SDIM; i++)
						res[i] += features[id].semantic[i];
				}
			}
		}
		for (int i = 0; i < SDIM; i++)
			res[i] /= cnt;
		return res;
	}

	std::vector<int> GetNeighborsOf(int pos) {
		int r = pos / img_w, c = pos % img_w;
		int r_[8] = { -1,0,1,0,-1,1,-1,1 };
		int c_[8] = { 0,1,0,-1,-1,1,1,-1 };
		std::vector<int> nb;
		for (int i = 0; i < 8; i++) {
			int new_r = r + r_[i];
			int new_c = c + c_[i];
			if (0 <= new_r && new_r < img_h && 0 <= new_c && new_c < img_w)
				nb.push_back(new_r * img_w + new_c);
		}
		return nb;
	}

	std::vector<Feature> GetFeaturesWith(std::vector<cv::Vec2i>& pos) {
		std::vector<Feature> feats(pos.size());
		for (int i = 0; i < pos.size(); i++) {
			int id = pos[i][0] * img_w + pos[i][1];
			feats[i] = features[id];
		}
		return feats;
	}

	cv::Mat GetAllPixelsRGB() {
		cv::Mat colors(img_h, img_w, CV_8UC3);
		for (int i = 0; i < img_h; i++) {
			for (int j = 0; j < img_w; j++)
				colors.at<cv::Vec3b>(i, j) = features[i * img_w + j].GetColor()*255;
		}
		return colors;
	}
};
#endif // UTILITY_H