#pragma once
#include "PaletteSolver.h" //this head file should at the top
#include "SuperpixSeg.h"
#include "Kmeans.h"
#include <algorithm>
#include <vector>

class PaletteBasedRecoloring {
private:
	Image m_simg;
	SuperpixSeg m_spix;
	std::vector<Feature> m_palette;
	std::vector<Feature> m_repalette;
	std::vector<std::vector<double>> m_weights;
	int m_sample_num;
	double m_sigma_rgb, m_sigma_semantic;

public:
	bool weight_calculated;

	PaletteBasedRecoloring() {
		weight_calculated = false;
	}

	PaletteBasedRecoloring(Image& si, SuperpixSeg& spix) {
		m_simg = si;
		m_spix = spix;
		m_sample_num = 256;
		weight_calculated = false;
	}

	void ExtractPaletteWithKmenas() {
		vector<spixCenter> spixCenters = m_spix.GetSuperpixCenters();
		vector<cv::Vec2i> spixCenterPos(spixCenters.size());
		vector<double> pixCntInSpix(spixCenters.size());
		for (int i = 0; i < spixCenters.size(); i++) {
			spixCenterPos[i] = cv::Vec2i(spixCenters[i].y, spixCenters[i].x); //row, col
			pixCntInSpix[i] = spixCenters[i].pix_cnt;
		}
		vector<Feature> features = m_simg.GetFeaturesWith(spixCenterPos);

		ModifiedKmeans MK(features, pixCntInSpix);
		m_palette = MK.GetClusterCenters();
		MK.ShowMeanShiftResult(m_simg.img);
	}

	void ComputeMeanOfPalette() {
		int tot = 0;
		m_sigma_rgb = 0, m_sigma_semantic = 0;
		for (int i = 0; i < m_palette.size(); i++) {
			for (int j = i + 1; j < m_palette.size(); j++) {
				m_sigma_rgb += GetColorDistanceOf(m_palette[i], m_palette[j]);
				m_sigma_semantic += GetSemanticDistanceOf(m_palette[i], m_palette[j]);
				tot++;
			}
		}
		m_sigma_rgb /= tot;
		m_sigma_semantic /= tot;
	}

	double GetPhiOf(Feature f1, Feature f2) {
		double dis_rgb = GetColorDistanceOf(f1, f2);
		double dis_semantic = GetSemanticDistanceOf(f1, f2);
		double G_rgb = exp(-dis_rgb * dis_rgb / (2 * m_sigma_rgb * m_sigma_rgb));
		double G_semantic = exp(-dis_semantic * dis_semantic / (2 * m_sigma_semantic * m_sigma_semantic));
		return G_rgb * G_semantic;
	}

	double GetPhiOf(double dis_rgb, double dis_semantic) {
		double G_rgb = exp(-dis_rgb * dis_rgb / (2 * m_sigma_rgb * m_sigma_rgb));
		double G_semantic = exp(-dis_semantic * dis_semantic / (2 * m_sigma_semantic * m_sigma_semantic));
		return G_rgb * G_semantic;
	}

	cv::Mat PreComputeLamda() {
		cv::Mat phi_mat = cv::Mat_<double>(m_palette.size(), m_palette.size());

		for (int i = 0; i < m_palette.size(); i++)
			for (int j = 0; j < m_palette.size(); j++) {
				double phi = GetPhiOf(m_palette[i], m_palette[j]);
				phi_mat.at<double>(i, j) = phi;
			}
		
		cv::Mat lambda_mat;
		invert(phi_mat, lambda_mat);

		return lambda_mat;
	}

	bool PreComputeWeights() {
		int pn = m_palette.size();
		if (pn == 0) return false;
		ComputeMeanOfPalette();


		m_weights = vector<vector<double>>(m_simg.img_h * m_simg.img_w);
		cv::Mat lambda_mat = PreComputeLamda();

		#pragma omp parallel for
		for (int k = 0; k < m_simg.features.size(); k++) {
			Feature f = m_simg.features[k];

			vector<double> dis_rgb(pn);
			vector<double> dis_xy(pn);
			vector<double> dis_semantic(pn);

			for (int i = 0; i < pn; i++) {
				dis_rgb[i] = GetColorDistanceOf(f, m_palette[i]);
				dis_xy[i] = GetPositionDistanceOf(f, m_palette[i]);
				dis_semantic[i] = GetSemanticDistanceOf(f, m_palette[i]);
			}

			vector<double> w_f(pn, 0);
			for (int i = 0; i < pn; i++) {
				for (int j = 0; j < pn; j++)
					w_f[i] += lambda_mat.at<double>(i, j) * GetPhiOf(dis_rgb[j],dis_semantic[j]);
			}

			double w_sum = 0;
			int max_w_id = 0;
			for (int i = 0; i < pn; i++) {
				if (w_f[i] < 0)
					w_f[i] = 0;
				if (w_f[max_w_id] < w_f[i])
					max_w_id = i;
				w_sum += w_f[i];
			}

			for (int i = 0; i < pn; i++) {
				w_f[i] /= w_sum;
			}

			for (int i = 0; i < pn; i++) {
				if (w_f[i] < 0.4) {
					w_f[max_w_id] += w_f[i];
					w_f[i] = 0;
				}
			}
			
			m_weights[k] = w_f;
		}
		weight_calculated = true;
		return true;
	}

	cv::Mat TransferColor(std::vector<Feature> modified_palette) {
		std::vector<Feature> delta_palette(m_palette.size());
		for (int i = 0; i < m_palette.size(); i++)
			delta_palette[i] = modified_palette[i] - m_palette[i];
		std::vector<cv::Vec3d> recolored_img(m_simg.img_h * m_simg.img_w);

		
		for (int i = 0; i < recolored_img.size(); i++) {
			recolored_img[i] = m_simg.features[i].GetColor();
			for (int j = 0; j < m_palette.size(); j++)
				recolored_img[i] += m_weights[i][j] * delta_palette[j].GetColor();
		}
		cv::Mat recolored(m_simg.img_h, m_simg.img_w, CV_8UC3, cv::Scalar(255, 255, 255));

		#pragma omp parallel for
		for (int i = 0; i < recolored_img.size(); i++) {
			int r = i / m_simg.img_w;
			int c = i % m_simg.img_w;
			recolored.at<cv::Vec3b>(r, c)[0] = min(max(recolored_img[i][0], 0.0), 1.0) * 255;
			recolored.at<cv::Vec3b>(r, c)[1] = min(max(recolored_img[i][1], 0.0), 1.0) * 255;
			recolored.at<cv::Vec3b>(r, c)[2] = min(max(recolored_img[i][2], 0.0), 1.0) * 255;
		}
		return recolored;
	}

	cv::Mat RecolorWithEditedPixels(std::vector<cv::Vec3d> edited_colors, std::vector<cv::Vec2i> edit_postions) {
		std::vector<cv::Vec3d> palette(m_palette.size());
		for (int i = 0; i < m_palette.size(); i++) {
			palette[i] = m_palette[i].GetColor();
		}

		int k = edited_colors.size();
		std::vector<std::vector<double>> weights(k);
		std::vector<cv::Vec3d> original_colors(k);
		std::vector<vector<double>> original_semantics(k);
		for (int i = 0; i < edit_postions.size(); i++) {
			int site = edit_postions[i][0] * m_simg.img_w + edit_postions[i][1];
			original_colors[i] = m_simg.features[site].GetColor();
			original_semantics[i] = m_simg.GetSemanticAt(site);
			weights[i] = m_weights[site];
		}

		std::vector<cv::Vec3d> sample_colors(m_sample_num);
		std::vector<vector<double>> sample_semantics(m_sample_num);
		std::vector<std::vector<double>> sample_weights(m_sample_num);
		int interval = m_simg.img_h * m_simg.img_w / m_sample_num;
		interval = interval * 1000 / 1000;
		for (int i = 0; i < m_sample_num; i++) {
			sample_colors[i] = m_simg.features[i * interval].GetColor();
			sample_semantics[i] = m_simg.features[i * interval].GetSemantic();
			sample_weights[i] = m_weights[i * interval];
		}

		PaletteSolver Ps(m_simg, palette, edited_colors, original_semantics, original_colors, weights, sample_semantics, sample_colors, sample_weights);
		std::vector<cv::Vec3d> palette_color = Ps.getPalette();
		m_repalette = m_palette;
		for (int i = 0; i < m_palette.size(); i++) {
			m_repalette[i].rgb = palette_color[i];
		}

		return TransferColor(m_repalette);
	}

	cv::Vec3d GetPaletteColor(int i) {
		return m_palette[i].GetColor();
	}

	std::vector<cv::Vec3d> GetPalette() {
		std::vector<cv::Vec3d> palette;
		for (int i = 0; i < m_palette.size(); i++)
			palette.push_back(m_palette[i].GetColor());
		return palette;
	}

	std::vector<Feature> GetRGBXYPalette() {
		return m_palette;
	}
	
	std::vector<cv::Vec3d> GetRePaletteRGB() {
		std::vector<cv::Vec3d> repalette;
		for (int i = 0; i < m_repalette.size(); i++)
			repalette.push_back(m_repalette[i].GetColor());
		return repalette;
	}

	std::vector<Feature> GetRePalette() {
		return m_repalette;
	}

	void PrintWeightMaps(string path) {
		cout << "print palette relative pixels" << endl;
		for (int j = 0; j < m_palette.size(); j++) {
			cv::Mat img(m_simg.img_h, m_simg.img_w, CV_8UC1);
			for (int i = 0; i < m_simg.img_h * m_simg.img_w; i++) {
				int x = static_cast<int>(m_weights[i][j] * 255);
				if (x > 255) x = 255;
				if (x < 0)   x = 0;
				img.at<uchar>(i / m_simg.img_w, i % m_simg.img_w) = static_cast<uchar>(x);
			}
			imwrite((path + "/Layer" + char(j + '0') + ".png").c_str(), img);
		}
	}

	void SavePaletteImg(vector<Feature>& p, int isrepalette) {
		int margin = 20, shape = 160;
		cv::Mat savedPalette(shape + margin * 2, p.size() * (margin + shape) + margin, CV_8UC3, cv::Vec3b(255, 255, 255));
		for (int k = 0; k < p.size(); k++) {
			for (int i = 0; i < shape; i++) {
				for (int j = 0; j < shape; j++) {
					savedPalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[0] = min(max(p[k].rgb[0], 0.0), 1.0) * 255;
					savedPalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[1] = min(max(p[k].rgb[1], 0.0), 1.0) * 255;
					savedPalette.at<cv::Vec3b>(margin + i, (margin + shape) * k + margin + j)[2] = min(max(p[k].rgb[2], 0.0), 1.0) * 255;
				}
			}
		}
		string path;
		if (isrepalette)
			path = "";
		else
			path = "";

		imwrite(path, savedPalette);
	}
};