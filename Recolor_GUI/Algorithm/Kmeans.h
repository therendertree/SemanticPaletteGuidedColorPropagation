#pragma once
#include "Utility.h"
#include <algorithm>

class ModifiedKmeans {
private:
	vector<Feature> m_data;
	vector<Feature> m_centers;
	vector<Feature> m_image;
	vector<double> m_weights;
	vector<int> m_labels;

public:
	ModifiedKmeans() {
		
	}

	ModifiedKmeans(vector<Feature>& feats, vector<double>& weights) {
		m_data = feats;
		m_weights = weights;
		m_labels.resize(m_data.size());
		Clustering();
	}

	vector<Feature> GetClusterCenters() {
		return m_centers;
	}


	void SelectInitCenters() {
		//normalize the weights
		double sigma2 = 1;
		vector<bool> selected(m_data.size(), false);
		double max_ = *max_element(m_weights.begin(), m_weights.end());
		for (int i = 0; i < m_weights.size(); i++) {
			m_weights[i] /= max_;
		}

		//select initial cluster centers
		for (int i = 0; i < 20; i++) {
			//1. select the data with max weights as the initial center
			int max_id = 0;
			double max_w = -1;
			for (int j = 0; j < m_weights.size(); j++) {
				if (!selected[j] && (max_w < m_weights[j])) {
					max_w = m_weights[j];
					max_id = j;
				}
			}
			if (max_w < 0.80) { 
				break;
			}
			m_centers.push_back(m_data[max_id]);
			selected[max_id] = true;
	
			cout << "weight = " << max_w << endl;

			//2. update other data's weights
			double dis = 0, w = 0;
			for (int j = 0; j < m_data.size(); j++) {
				if (!selected[j]) {
					dis = GetWeightedDistanceOf(m_centers[i], m_data[j]);
					w = 1 - exp(-dis * dis / sigma2);
					m_weights[j] = w * m_weights[j];
				}
			}
		}
	}

	void Clustering() {
		SelectInitCenters();

		int it = 0;
		double diff = 0;
		while (it++ < 100) {
			UpdateDataLabel();
			vector<Feature> old_centers = m_centers;
			UpdateClusterCenter();

			diff = 0;
			for (int i = 0; i < m_centers.size(); i++)
				diff += (m_centers[i] - old_centers[i]).norm();
			if (diff < 0.0001) break;
		}
	}

	void UpdateDataLabel() {
		for (int i = 0; i < m_data.size(); i++) {
			int min_center_id = 0;
			double min_dis = DBL_MAX, dis = 0;
			for (int j = 0; j < m_centers.size(); j++) {
				dis = GetWeightedDistanceOf(m_data[i], m_centers[j]);
				if (min_dis > dis) {
					min_dis = dis;
					min_center_id = j;
				}
			}
			m_labels[i] = min_center_id;
		}
	}

	void UpdateClusterCenter() {
		m_centers = vector<Feature>(m_centers.size());
		vector<int> data_num_in_each_cluster(m_centers.size(), 0);

		for (int i = 0; i < m_data.size(); i++) {
			int label = m_labels[i];
			m_centers[label] += m_data[i];
			data_num_in_each_cluster[label]++;
		}

		for (int i = 0; i < m_centers.size(); i++)
			m_centers[i] /= data_num_in_each_cluster[i];
	}

	void ShowMeanShiftResult(cv::Mat& img) {
		cout << "in drawKmeansResult centers number:" << m_centers.size() << endl;
		for (int i = 0; i < m_centers.size(); i++) {
			int row = m_centers[i].xy[0] * img.rows;
			int col = m_centers[i].xy[1] * img.cols;
			circle(img, cv::Point(col, row), 5, cv::Scalar(255, 0, 0), 2);
			putText(img, to_string(i), cv::Point(col, row), cv::HersheyFonts::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(100, 200, 200), 1);
		}

		imshow("Kmeans res", img);
		cv::waitKey(0);
	}
};