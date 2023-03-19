#define _USE_MATH_DEFINES

#include "opencv2/opencv.hpp"
#include <iostream>
#include <dirent.h>

typedef std::vector<std::string> stringvec;

void readDirectory(const std::string &name, stringvec &v)
{
	DIR *dirp = opendir(name.c_str());
	if (dirp == NULL)
	{
		printf("%s [IMAGE_PATH_ERROR] %s\n","\033[31m","\033[39m");
	}
	struct dirent *dp;

	while ((dp = readdir(dirp)) != NULL)
	{
		std::string str = std::string(dp->d_name);
		if (str.find(".png") != std::string::npos || str.find(".jpg") != std::string::npos){
			v.push_back(dp->d_name);
		}
	}
	closedir(dirp);
}

void getFilterLines(cv::Mat InputImage, std::vector<cv::Vec4i> Lines, std::vector<std::vector<double>> &FinalLines)
{
	float REJECT_DEGREE_TH = 10;
	
	// 모든 line에 대한 값
	for (int i = 0; i < (int)Lines.size(); i++)
	{
		// line위에있는 두개의 점
		cv::Vec4i Line = Lines[i];
		int x1 = Line[0], y1 = Line[1];
		int x2 = Line[2], y2 = Line[3];
		
		double m = 0, c = 0;

		// Calculating equation of the line : y = mx + c
		if (x1 != x2)
		{
			m = (double)(y2 - y1) / (double)(x2 - x1);
		}
		else
		{
			m = 100000000.0;
		}
			
		c = y2 - m * x2;
		
		// theta will contain values between - 90 -> + 90.
		// 각도에 따른 필터링
		double theta = atan(m) * (180.0 / M_PI);
		
		if (REJECT_DEGREE_TH <= abs(theta) && abs(theta) <= (90.0 - REJECT_DEGREE_TH))
		{
			// 이미지 중앙보다 아래만 ROI 영역 표시
			if(InputImage.rows/2 < y2)
			{
				if (((m > 0) && (InputImage.cols/2 < x1)) || ((m < 0) && (InputImage.cols/2 > x1)))
				{
					double lineLength = pow((pow((y2 - y1), 2) + pow((x2 - x1), 2)), 0.5);	// length of the line
					std::vector<double> FinalLine{ (double)x1, (double)y1, (double)x2, (double)y2, m, c, lineLength};
					FinalLines.push_back(FinalLine);
				}
			}
		}
	}

	// Removing extra lines
	// (we might get many lines, so we are going to take only longest 15 lines 
	// for further computation because more than this number of lines will only
	// contribute towards slowing down of our algo.)
	if (FinalLines.size() > 15)
	{
		std::sort(FinalLines.begin(), FinalLines.end(), 
				  [](const std::vector< double >& a, 
					 const std::vector< double >& b) 
					{ return a[6] > b[6]; });
		
		FinalLines = std::vector<std::vector<double>>(FinalLines.begin(), FinalLines.begin() + 15);
	}
}

void getLines(cv::Mat Image, cv::Mat &edgeImage,std::vector<cv::Vec4i>&allLines)
{
	// Finding Lines in the image
	cv::HoughLinesP(edgeImage, allLines, 1, CV_PI / 180, 50, 100,5);
	// Check if lines found and exit if not.
	std::cout << "Line : " << allLines.size() << std::endl;
	if (allLines.size() == 0)
	{
		std::cout << "Could Not Detection lines." << std::endl;
	}
}

void getVanishingPoint(std::vector<std::vector<double>> Lines,cv::Point2i &VanishingPoint)
{
	// We will apply RANSAC inspired algorithm for this.We will take combination
	// of 2 lines one by one, find their intersection point, and calculate the
	// total error(loss) of that point.Error of the point means root of sum of
	// squares of distance of that point from each line.

	double MinError = 1000000000.0;

	for (int i = 0; i < (int)Lines.size(); i++)
	{
		for (int j = i + 1; j < (int)Lines.size(); j++)
		{
			double m1 = Lines[i][4], c1 = Lines[i][5];
			double m2 = Lines[j][4], c2 = Lines[j][5];
			
			//반대 차선끼리 비교
			if((m1*m2) < 0)
			{
				double x0 = (c1 - c2) / (m2 - m1);
				double y0 = m1 * x0 + c1;

				double err = 0;
				for (int k = 0; k < (int)Lines.size(); k++)
				{
					double m = Lines[k][4], c = Lines[k][5];
					double m_ = (-1 / m);
					double c_ = y0 - m_ * x0;

					double x_ = (c - c_) / (m_ - m);
					double y_ = m_ * x_ + c_;

					double l = pow((pow((y_ - y0), 2) + pow((x_ - x0), 2)), 0.5);

					err += pow(l, 2);
				}

				err = pow(err, 0.5);

				if (MinError > err)
				{
					MinError = err;
					VanishingPoint.x = (int)x0;
					VanishingPoint.y = (int)y0;
				}
			}
		}
	}
}

void extendLine(cv::Mat img, cv::Point pt1, cv::Point pt2, const cv::Scalar& color, int thickness = 1, int lineType = cv::LINE_8, int shift = 0)
{
	double m = (double)(pt2.y - pt1.y) / (double)(pt2.x - pt1.x);
	double c = (double)pt2.y - m * (double)pt2.x;
	cv::line(img, pt1, pt2,color, thickness);
	
	std::cout << "img size :" << img.rows<<std::endl;
	std::cout << "m :" << m<<std::endl;

	if(m<0.0)
	{
		if(c < (double)img.rows)
		{
			// case 1
			if (-c/m < (double)img.cols)
			{
				cv::Point pt3(0,c);
				cv::Point pt4(-c/m,0);	
				std::cout << "pt3 : " << pt3 << std::endl;
				std::cout << "pt4 : " << pt4 << std::endl;
				cv::line(img, pt3, pt4,cv::Scalar(255,0,0), 3);
			}
			// case 2
			else
			{
				cv::Point pt3(0,c);
				cv::Point pt4(img.cols,img.cols*m+c);	
				std::cout << "pt3 : " << pt3 << std::endl;
				std::cout << "pt4 : " << pt4 << std::endl;
				cv::line(img, pt3, pt4,cv::Scalar(255,153,103), 3);
			}
		}
		else
		{
			if()
			cv::Point pt3(((double)img.rows-c)/m,(double)img.rows);
			cv::Point pt4(-c/m,0);
			std::cout << "pt3 : " << pt3 << std::endl;
			std::cout << "pt4 : " << pt4 << std::endl;
			cv::line(img, pt3, pt4,cv::Scalar(0,255,0), 3);
		}
	}


	// if ((m < 0.0) && (c < (double)img.rows))
	// {
	// 	cv::Point pt3(0,c);
	// 	cv::Point pt4(-c/m,0);	
	// 	std::cout << "pt3 : " << pt3 << std::endl;
	// 	std::cout << "pt4 : " << pt4 << std::endl;
	// 	cv::line(img, pt3, pt4,cv::Scalar(255,0,0), 3);
	// }
	// else if((m < 0.0) && (c > (double)img.rows))
	// {
	// 	cv::Point pt3(((double)img.rows-c)/m,(double)img.rows);
	// 	cv::Point pt4(-c/m,0);
	// 	std::cout << "pt3 : " << pt3 << std::endl;
	// 	std::cout << "pt4 : " << pt4 << std::endl;
	// 	cv::line(img, pt3, pt4,cv::Scalar(0,255,0), 3);
	// }
	// else if((m > 0.0) && (c < (double)img.rows))
	// {
	// 	cv::Point pt3((double)img.cols,m*(double)img.cols+c);
	// 	cv::Point pt4(-c/m,0);
	// 	std::cout << "pt3 : " << pt3 << std::endl;
	// 	std::cout << "pt4 : " << pt4 << std::endl;
	// 	cv::line(img, pt3, pt4,cv::Scalar(0,0,255), 3);
	// }

	// else if((m > 0.0) && (c > (double)img.rows))
	// {
	// 	cv::Point pt3(((double)img.rows-c)/m,(double)img.rows);
	// 	cv::Point pt4(-c/m,0);
	// 	std::cout << "pt3 : " << pt3 << std::endl;
	// 	std::cout << "pt4 : " << pt4 << std::endl;
	// 	cv::line(img, pt3, pt4,cv::Scalar(0,255,255), 3);
	// }

	// else
	// {
	// 	std::cout << "여기라고?" << std::endl;
	// }
}

int main(int argc, char** argv)
{
	std::string rawPath;

	unsigned int startFrame = 0;
	unsigned int isStop = 0;
	float imageWidth = 1;
	
	if (argc < 2){
		printf("%sCHECK ReadMe.md or ./bin/ld --help %s\n", "\033[31m", "\033[39m");
		return 0;
	}
	else{
		for (int i = 1; i < argc; i += 2){
			
			std::string str = std::string(argv[i]);
			
			if(str.compare("--raw") == 0)
			{
				rawPath = std::string(argv[i+1]);
			}

			else if(str.compare("--start") == 0)
			{
				startFrame = atoi(argv[i+1]);
			}

			else if(str.compare("--imageWidth") == 0)
			{
				imageWidth = atof(argv[i+1]);
			}

			else
			{
				printf("%sERROR!! ./bin/ld --help%s\n", "\033[31m", "\033[39m");	
				return -1;
			}	
		}
	}

	printf("%sSTART      FRAME  : %u %s\n", "\033[32m", startFrame, "\033[39m");
	printf("%sRAW        PATH   : %s %s\n", "\033[32m", rawPath.c_str(), "\033[39m");
	
	/* Image Load*/
	stringvec imgList;	
	readDirectory(rawPath, imgList);
	sort(imgList.begin(), imgList.end());
	
	unsigned int index = startFrame;

	std::string tempText;
	while(1)
	{
		/* Image Processing */

		cv::Mat rawImage = cv::imread(rawPath + imgList[index],cv::IMREAD_COLOR);
		// cv::imshow("Test",rawImage);
		// cv::waitKey(0);
		cv::Mat grayImage, blurImage, edgeImage;
		
		// Converting Grayscale Image
		cv::cvtColor(rawImage, grayImage, cv::COLOR_BGR2GRAY);

		// Blurring image to reduce noise.
		//cv::GaussianBlur(grayImage, blurImage, cv::Size(5, 5), 1);

		// Generating Edge Image
		cv::Canny(grayImage, edgeImage, 125, 255);

		// Getting Alllines
		std::vector<cv::Vec4i> allLines;
		getLines(rawImage,edgeImage,allLines);
		std::cout << "Allline Size : " << allLines.size() << std::endl;

		// Draw All detected lines
		for (int i = 0; i< (int)allLines.size(); i++)
		{
			cv::line(rawImage, cv::Point(allLines[i][0],allLines[i][1]), cv::Point(allLines[i][2], allLines[i][3]), cv::Scalar(255, 0, 255), 8);
			//tempText = "Theta : "+ std::to_string(atan(double(allLines[i][3]-allLines[i][1])/double(allLines[i][2]-allLines[i][0]))*180/M_PI);
			//cv::putText(rawImage, tempText, cv::Point(allLines[i][2]+10, allLines[i][3]-10), 1, 2, cv::Scalar(255, 0,0), 2);
		}

		// Getting Filtered Lines
		std::vector<std::vector<double>> filterLines;
		getFilterLines(rawImage,allLines,filterLines);
		std::cout << "Filterline Size : " << filterLines.size() << std::endl;

		// Draw Filtered lines
		for (int i = 0; i < (int)filterLines.size(); i++)
		{
			//cv::line(rawImage, cv::Point((int)filterLines[i][0], (int)filterLines[i][1]), cv::Point((int)filterLines[i][2], (int)filterLines[i][3]), cv::Scalar(0, 255, 0), 2);
			extendLine(rawImage, cv::Point((int)filterLines[i][0], (int)filterLines[i][1]), cv::Point((int)filterLines[i][2], (int)filterLines[i][3]), cv::Scalar(0, 255, 0), 2);
			std::cout << "m : " << filterLines[i][4] << std::endl;
			std::cout << "c : " << filterLines[i][5] << std::endl;
			std::cout << "length : " << filterLines[i][6] << std::endl;
			std::cout << "theta : " << atan(filterLines[i][4]) * (180.0 / M_PI) << std::endl;

		}

		// Get vanishing point

		cv::Point2i VanishingPoint = {-1,-1};
		getVanishingPoint(filterLines,VanishingPoint);

		// Checking if vanishing point found
		if (VanishingPoint.x == -1 && VanishingPoint.x == -1)
		{
			std::cout << "Vanishing Point not found. Possible reason is that not enough lines are found in the image for determination of vanishing point." << std::endl;
			//continue;
		}
		else
		{
			// Drawing Vanishing Point
			//cv::circle(rawImage, cv::Point(VanishingPoint[0], VanishingPoint[1]), 10, cv::Scalar(0, 0, 255), -1);

			// Drawing Vanishing line
			//cv::line(rawImage, cv::Point(0,VanishingPoint.y), cv::Point(rawImage.cols, VanishingPoint.y), cv::Scalar(0, 0, 255), 4);

			std::cout << "VanishingPoint[0] : " << VanishingPoint.x << std::endl;
			std::cout << "VanishingPoint[1] : " << VanishingPoint.y << std::endl;
		}
	
		/* Debugging */
		tempText = std::to_string(index) + "/" + std::to_string(imgList.size());
		cv::putText(rawImage, tempText, cv::Point(0, 30), 1, 1.5, cv::Scalar(0, 0, 255), 2);
		
		/* Output */
		
		// cv::Mat resizeImage;
		// cv::resize(rawImage,resizeImage,cv::Size(rawImage.cols/2.5,rawImage.rows/2.5));

		// cv::Mat resizeEdgeImage;
		// cv::resize(edgeImage,resizeEdgeImage,cv::Size(edgeImage.cols/2.5,edgeImage.rows/2.5));
		cv::cvtColor(edgeImage,edgeImage,cv::COLOR_GRAY2RGB);

		cv::Mat combineImage;
		cv::hconcat(rawImage,edgeImage,combineImage);
		
		cv::Mat resizeCombineImage;
		#define FHDratio (1920./1080.)
		cv::resize(combineImage,resizeCombineImage,cv::Size(imageWidth,((imageWidth/2.)/FHDratio)));

		cv::imshow("OutputImage", resizeCombineImage);
		int key = cv::waitKey(isStop);
		
		if (key == 'a')
		{
			index = index - 2;
		}
		else if (key == 'q')
		{
			break;
		}
		else if (key == 'd')
		{
			if (isStop == 1)
			{
				isStop = 0;
			}
			else
			{
				isStop = 1;
			}
		}

		index++;
	}

	return 0;
}