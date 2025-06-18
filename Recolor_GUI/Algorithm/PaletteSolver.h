#include<iostream>
#include<opencv2/opencv.hpp>
#include<nlopt.hpp>
#include<vector>
#include<cmath>
#include "utility.h"

//using namespace cv;
using namespace std;

class PaletteSolver {
private:
	vector<cv::Vec3d> m_edited_colors;				//改变的目标像素(已知)
	int m_k;										//改变的像素数量(已知)
	vector<vector<double> > m_weights;				//改变区域的权重值(已知)
	vector<vector<double>> m_original_semantics;	//改变区域原本的信息(已知)
	vector<cv::Vec3d> m_original_colors;			//改变区域原本的信息(已知)
	vector<cv::Vec3d> m_original_palette;			//原始调色板(已知)
	vector<cv::Vec3d> m_solved_palette;
	vector<cv::Vec3d> m_sample_colors;
	vector<vector<double>> m_sample_semantics;
	vector<vector<double>> m_sample_recolor_weights;
	vector<double> m_sample_importance_weights;
public:
	PaletteSolver(Image img, vector<cv::Vec3d> palette, vector<cv::Vec3d> edited_colors, vector<vector<double>>& original_semantics, vector<cv::Vec3d> original_colors,
		vector<vector<double>>& weights, vector<vector<double>>& sample_semantics, vector<cv::Vec3d>& sample_colors, vector<vector<double>>& sample_weights) {
		m_k = edited_colors.size();
		m_original_semantics = original_semantics;
		m_edited_colors = edited_colors;
		m_original_colors = original_colors;
		m_original_palette = palette;
		m_weights = weights;
		m_sample_semantics = sample_semantics;
		m_sample_colors = sample_colors;
		m_sample_recolor_weights = sample_weights;
		m_solved_palette.resize(m_original_palette.size());
		cout << "finished init PaletteSolver" << endl;

		GetImportanceWeightsOfSamples(); 
		ExecuteOptimizer();
	}

	double GetReconsLoss(vector<cv::Vec3d> recolor) {
		double loss = 0.0;
		for (int i = 0; i < m_k; i++)
			loss += norm(recolor[i] - m_edited_colors[i], cv::NORM_L2SQR);
		return loss / m_k;
	}

	void GetImportanceWeightsOfSamples() {
		m_sample_importance_weights.resize(m_sample_colors.size());
		for (int i = 0; i < m_sample_colors.size(); i++) {
			double mindist = 0x3f3f;
			for (int j = 0; j < m_original_colors.size(); j++) {
				double dist = GetNormOf(m_sample_semantics[i], m_original_semantics[j]);
				mindist = min(mindist, dist);
			}
			m_sample_importance_weights[i] = exp(10 * mindist);
		}
	}

	double GetCorrLoss(vector<cv::Vec3d> deltaPalette, vector<cv::Vec3d>& recolor) {
		vector<cv::Vec3d> reSample(m_sample_colors.size());
		for (int i = 0; i < m_sample_colors.size(); i++) {
			reSample[i] = m_sample_colors[i];
			for (int j = 0; j < m_original_palette.size(); j++) {
				reSample[i] += m_sample_recolor_weights[i][j] * deltaPalette[j];
			}
		}
	
		double loss = 0.0, sumWeights = 0.0;
		for (int i = 0; i < m_sample_colors.size(); i++) {
			double norm2 = norm(m_sample_colors[i] - reSample[i], cv::NORM_L2SQR);
			double weight = m_sample_importance_weights[i];
			sumWeights += weight;
			loss += weight * norm2;
		}
		return loss / sumWeights;
	}


	double optimizer(const vector<double>& x, vector<double>& grab, void* data) {
		const double lambda_edit = 10;
		const double lambda_corr = 3; 

		vector<cv::Vec3d> solved_palette;
		for (int i = 0; i < m_original_palette.size(); i++) {
			solved_palette.push_back(cv::Vec3d(x[i * 3 + 0], x[i * 3 + 1], x[i * 3 + 2]));
		}

		vector<cv::Vec3d> deltaPalette;
		for (int i = 0; i < m_original_palette.size(); i++) {
			deltaPalette.push_back(solved_palette[i] - m_original_palette[i]);
		}
		vector<cv::Vec3d> recolor(m_k);
		for (int i = 0; i < m_k; i++) {
			recolor[i] = m_original_colors[i];
			for (int j = 0; j < m_original_palette.size(); j++) {
				recolor[i] += m_weights[i][j] * deltaPalette[j];
			}
		}

		double edit = GetReconsLoss(recolor);
		double corr = GetCorrLoss(deltaPalette, recolor);

		return lambda_edit * edit + lambda_corr * corr;
	}

	void ExecuteOptimizer() {
		int xNum = m_original_palette.size() * 3;
		nlopt::opt opt(nlopt::LN_COBYLA, xNum); //LN_COBYLA

		vector<double> lb(xNum, 0), ub(xNum, 1);

		opt.set_lower_bounds(lb);
		opt.set_upper_bounds(ub);

		opt.set_min_objective([](const std::vector<double>& x, std::vector<double>& grad, void* data) {
			return reinterpret_cast<PaletteSolver*>(data)->optimizer(x, grad, nullptr);
			}, this);
		vector<double> x(xNum);
		for (int i = 0; i < m_original_palette.size(); i++) {
			for (int j = 0; j < 3; j++) {
				x[i * 3 + j] = m_original_palette[i][j];
			}
		}
		double minf;
		opt.set_xtol_rel(1e-6);
		nlopt::result res = opt.optimize(x, minf);

		for (int i = 0; i < m_original_palette.size(); i++) {
			for (int j = 0; j < 3; j++) {
				m_solved_palette[i][j] = x[i * 3 + j];
			}
		}
	}

	vector<cv::Vec3d> getPalette() {
		cout << "\n\nEdited Palette: " << endl;
		for (int i = 0; i < m_solved_palette.size(); i++) {
			cout << "color[" << i << "]:" << endl;
			for (int j = 0; j < 3; j++) {
				cout << m_solved_palette[i][j] << " ";
			}
			cout << endl;
			for (int j = 0; j < 3; j++) {
				cout << m_solved_palette[i][j] * 255 << " ";
			}
			cout << endl;
		}
		return m_solved_palette;
	}
};