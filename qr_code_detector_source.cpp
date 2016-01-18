#include <opencv2/opencv.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <vector>
#include <iostream>

#define CONT vector<Point>

using namespace cv;
using namespace std;

//Шаблон QR-кода: три квадрата, находящиеся в трёх углах кода
struct FinderPattern
{
    Point topleft; //левый верхний угол
    Point topright; //правый верхний угол
    Point bottomleft; //левый нижний угол
    FinderPattern(Point a, Point b, Point c) : topleft(a), topright(b), bottomleft(c) {} //конструктор класса
};

//Сравнение площадей контуров
bool compareContourAreas (CONT contour1, CONT contour2)
{
    double i = fabs(contourArea(Mat(contour1))); //площадь 1-го контура
    double j = fabs(contourArea(Mat(contour2))); //площадь 2-го контура
    return (i > j);
}

//Получение координат центра контура
Point getContourCentre(CONT& vec)
{
    double tempx = 0.0, tempy = 0.0;
    for (int i = 0; i < vec.size(); i++) {
        tempx += vec[i].x;
        tempy += vec[i].y;
    }
    return Point(tempx / (double)vec.size(), tempy / (double)vec.size());
}

//Проверка находится ли один контур в другом
bool isContourInsideContour(CONT& in, CONT& out)
{
	//каждую точку внутреннего контура проверяем на нахождение во внешнем контуре
    for(int i = 0; i < in.size(); i++){
        if(pointPolygonTest(out, in[i], false) <= 0) return false; //проверка, находится ли точка внутри контура
    }
    return true;
}

//Удаление ненужных контуров (слишком больших или слишком маленьких)
vector<CONT > findLimitedConturs(Mat contour, float minPix, float maxPix)
{
    vector<CONT > contours;
    vector<Vec4i> hierarchy;
    findContours(contour, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); //нахождение всех контуров изображения
    cout << "contours.size = " << contours.size() << endl;
    int m = 0;
    //проверяем все контуры
    while (m < contours.size()) {
    	//если контур меньше или равен указанному минимуму или больше указанного максимума, удаляем контур
        if (contourArea(contours[m]) <= minPix) {
            contours.erase(contours.begin() + m);
        } else if (contourArea(contours[m]) > maxPix){
            contours.erase(contours.begin() + m);
        } else ++m;
    }
    //cout << "contours.size = " << contours.size() << endl; - вывод количества оставшихся контуров
    return contours;
}

//Получаем массив пар контуров для дальнейшей обработки
//частей QR-кода
vector<vector<CONT > > getContourPair(vector<CONT > &contours)
{
    vector<vector<CONT > > vecpair;
    vector<bool> bflag(contours.size(), false);

    //по всем контурам
    for (int i = 0; i < contours.size() - 1; i++) {
        if(bflag[i]) continue;
        vector<CONT > temp; //создаем массив контуров, этот массив и есть потенциальный QR-код
        temp.push_back(contours[i]); //добавляем контур в массив
        for (int j = i + 1; j < contours.size(); j++) {
        	//если контур находится внутри рассматриваемого контура,
        	//добавляем его в массив
            if (isContourInsideContour(contours[j], contours[i])) {
                temp.push_back(contours[j]);
                bflag[j] = true;
            }
        }
        if (temp.size() > 1) {
            vecpair.push_back(temp);
        }
    }
    bflag.clear();
    //сортируем каждый потенциальный QR-код по площади контуров
    for (int i = 0; i < vecpair.size(); i++) {
        sort(vecpair[i].begin(), vecpair[i].end(), compareContourAreas);
    }
    return vecpair;
}

//Удаление неподходящих пар векторов
void eliminatePairs(vector<vector<CONT > >& vecpair, double minRatio, double maxRatio)
{
    cout << "maxRatio = " << maxRatio << endl;
    int m = 0;
    bool flag = false;
    //по всем потенциальным QR-кодам
    while (m < vecpair.size()) {
        flag = false;
        //если количество контуров у потенциального QR-кода меньше трёх
        if (vecpair[m].size() < 3) {
            vecpair.erase(vecpair.begin() + m); //удаляем потенциальный QR-код
            continue;
        }
        //по всем контурам потенциального QR-кода
        for(int i = 0; i < vecpair[m].size() - 1; i++){
            double area1 = contourArea(vecpair[m][i]); //площадь первого контура
            double area2 = contourArea(vecpair[m][i + 1]); //площадь второго контура
            //если отношение их площадей не подходит для QR-кода, удаляем эту пару контуров
            if(area1 / area2 < minRatio || area1 / area2 > maxRatio){
                vecpair.erase(vecpair.begin() + m);
                flag = true;
                break;
            }
        }
        if(!flag){
            ++ m;
        }
    }
    //если количество пар контуров больше трёх, применяем эту же функцию, но уже с меньшим
    //максимальным отношением площадей контуров
    if (vecpair.size() > 3) {
        eliminatePairs(vecpair, minRatio, maxRatio * 0.9);
    }
}

//Получение дистанции между точками
double getDistance(Point a, Point b)
{
    return sqrt(pow((a.x - b.x), 2) + pow((a.y - b.y), 2));
}

//Нахождение шаблона QR-кода на изображении
FinderPattern getFinderPattern(vector<vector<CONT > > &vecpair)
{
	//центры трёх контуров
    Point pt1 = getContourCentre(vecpair[0][vecpair[0].size() - 1]);
    Point pt2 = getContourCentre(vecpair[1][vecpair[1].size() - 1]);
    Point pt3 = getContourCentre(vecpair[2][vecpair[2].size() - 1]);
    //дистанции между центрами трёх контуров
    double d12 = getDistance(pt1, pt2);
    double d13 = getDistance(pt1, pt3);
    double d23 = getDistance(pt2, pt3);
    double x1, y1, x2, y2, x3, y3;
    double Max = max(d12, max(d13, d23)); //максимальная дистанция из трёх
    Point p1, p2, p3;
    //устанавливаем ориентацию QR-кода
    if (Max == d12) {
        p1 = pt1;
        p2 = pt2;
        p3 = pt3;
    } else if (Max == d13) {
        p1 = pt1;
        p2 = pt3;
        p3 = pt2;
    } else if (Max == d23) {
        p1 = pt2;
        p2 = pt3;
        p3 = pt1;
    }
    //координаты точек трёх квадратов QR-кода
    x1 = p1.x;
    y1 = p1.y;
    x2 = p2.x;
    y2 = p2.y;
    x3 = p3.x;
    y3 = p3.y;
    //перебор условий для нахождения правильного положения QR-кода
    if (x1 == x2) {
        if (y1 > y2) {
            if (x3 < x1) {
                return FinderPattern(p3, p2, p1);
            } else {
                return FinderPattern(p3, p1, p2);
            }
        } else {
            if (x3 < x1) {
                return FinderPattern(p3, p1, p2);
            } else {
                return FinderPattern(p3, p2, p1);
            }
        }
    } else {
        double newy = (y2 - y1) / (x2 - x1) * x3 + y1 - (y2 - y1) / (x2 - x1) * x1;
        if (x1 > x2) {
            if (newy < y3) {
                return FinderPattern(p3, p2, p1);
            } else {
                return FinderPattern(p3, p1, p2);
            }
        } else {
            if (newy < y3) {
                return FinderPattern(p3, p1, p2);
            } else {
                return FinderPattern(p3, p2, p1);
            }
        }
    }
}

//Эрозия, размытие для превращения QR-кода в черный квадрат
//Таким образом он лучше виден на изображениях, содержащих
//множество мешающих детектированию объектов
void erosion(Mat &src, Mat &erosion_dst)
{
    int erosion_type = MORPH_RECT;
    int erosion_size = 5;

    Mat element = getStructuringElement(erosion_type,
                       Size(2*erosion_size + 1, 2*erosion_size+1),
                       Point(erosion_size, erosion_size));

    erode(src, erosion_dst, element); //эрозия

    //imshow("Erosion Demo", erosion_dst); //вывод на экран изображения после эрозии
}

//Нахождение чёрного квадрата на изображении
void findBlackSquare(Mat &src, Mat &erosion_src, int mode)
{
	vector<Vec4i> hierarchy;
	vector<CONT > contours;
	Mat gray;
	cvtColor(erosion_src, gray, CV_BGR2GRAY); //цветное -> оттенки серого
	inRange(gray, (0), (10), gray); //остаётся только чёрнаю часть изображения
	//imshow("gray1", gray);
	findContours(gray, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE); //нахождение контуров

	sort(contours.begin(), contours.end(), compareContourAreas); //сортировка контуров по площади, охватываемой ими

	CONT contour = contours[0]; //наибольший контур
  	double eps = 0.016 * arcLength(contour, true); //точность, с которой рисуется контур
	approxPolyDP(contour, contour, eps, true); //многоугольник, охватывающий черную часть изображения
	Rect rect = boundingRect(contour); //прямоугольник
	double k = (rect.height+0.0)/rect.width;
	contours[0] = contour;

	Mat crop(src.rows, src.cols, CV_8UC3);
	//если черная часть близка по форме к квадрату
	if (0.8<k && k<1.3 && rect.area()>100)
	{
		Mat mask = Mat::zeros(gray.rows, gray.cols, CV_8UC1);
		drawContours(mask, contours, 0, Scalar(255), CV_FILLED); //рисуются контуры

		crop.setTo(Scalar(mode,mode,mode)); //цвет фона
		src.copyTo(crop, mask); //копируется только часть изображения, ограниченная контуром
		normalize(mask.clone(), mask, 0.0, 255.0, CV_MINMAX, CV_8UC1); //нормализация изображения

	}

	//imshow("result", src);
	//imshow("cropped", crop);
	src = crop.clone();
	//waitKey();
}

//Нахождение QR-кода в случае, когда геометрическое нахождение
//не сработало, то есть, на изображениях с искажением QR-кода,
//с наличием множества мешающих предметов, с QR-кодом, изображённым
//не на плоскости, а, например, на футболке или на столбе
void findFailedQR(Mat &src, int mode)
{
	Mat erosion_dst;
	erosion(src, erosion_dst);
	findBlackSquare(src, erosion_dst, mode);
}

//Применение всех вышеописанных этапов детектирования QR-кода
//и вывод QR-кода во фронтальной проекции
void invoke(Mat ori, Mat gray)
{
	Mat tempOri = ori.clone(), tempGray = gray.clone();
	Mat pcanny;
	gray.copyTo(pcanny);

	Canny(pcanny, pcanny, 50, 150, 3); //детектор краёв

	Mat bin;
	threshold(gray, bin, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU); //бинаризация изображения

	Mat contour;
	bin.copyTo(contour);

	vector<CONT > contours;
	contours = findLimitedConturs(contour, 8.00, 0.2 * ori.cols * ori.rows); //нахождение подходящих контуров

    if(!contours.empty()) sort(contours.begin(), contours.end(), compareContourAreas);
    vector<vector<CONT > > vecpair;
    vecpair = getContourPair(contours);
    eliminatePairs(vecpair, 1.0, 10.0);

    //если алгоритм геометрического нахождения не справился с изображением,
    //применятся эрозия и обнаружение чёрной части изображения,
    //создание изображение с интересующей частью и ЧЁРНЫМ фоном
    //затем алгоритм геометрического нахождения проверяет, является ли
    //найденная часть QR-кодом
    if (vecpair.size() < 3) {
    	findFailedQR(tempOri, 0); // чёрный
    	cvtColor(tempOri, tempGray, CV_BGR2GRAY);

    	tempGray.copyTo(pcanny);

    	Canny( pcanny, pcanny, 50, 150, 3 );

    	threshold(tempGray, bin, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

    	bin.copyTo(contour);
    	contours = findLimitedConturs(contour, 8.00, 0.2 * tempOri.cols * tempOri.rows);

    	if(!contours.empty()) sort(contours.begin(), contours.end(), compareContourAreas);
    	vecpair = getContourPair(contours);
    	eliminatePairs(vecpair, 1.0, 10.0);
    }
    //если алгоритм геометрического нахождения не справился с изображением,
    //применятся эрозия и обнаружение чёрной части изображения,
    //создание изображение с интересующей частью и БЕЛЫМ фоном
    //затем алгоритм геометрического нахождения проверяет, является ли
    //найденная часть QR-кодом
	if (vecpair.size() < 3) {
		tempOri = ori.clone();
		tempGray = gray.clone();
		findFailedQR(tempOri, 255); // белый
		cvtColor(tempOri, tempGray, CV_BGR2GRAY);

		tempGray.copyTo(pcanny);

		Canny( pcanny, pcanny, 50, 150, 3 );

		threshold(tempGray, bin, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

		bin.copyTo(contour);
		contours = findLimitedConturs(contour, 8.00, 0.2 * tempOri.cols * tempOri.rows);

		if(!contours.empty()) sort(contours.begin(), contours.end(), compareContourAreas);
		vecpair = getContourPair(contours);
		eliminatePairs(vecpair, 1.0, 10.0);
	}

	ori = tempOri.clone();
	gray = tempGray.clone();

	cout<<"there are "<<vecpair.size()<<" pairs left!!"<<endl;

	//нахождение квадратов в QR-коде
	FinderPattern fPattern = getFinderPattern(vecpair);
	cout<<"topleft = "<<fPattern.topleft.x<<", "<<fPattern.topleft.y<<endl
	     <<"topright = "<<fPattern.topright.x<<", "<<fPattern.topright.y<<endl
	     <<"bottomleft = "<<fPattern.bottomleft.x<<", "<<fPattern.bottomleft.y<<endl;
	Mat drawing;
	ori.copyTo(drawing);

	//пометка квадратов цветными кругами
	circle(drawing, fPattern.topleft, 3, CV_RGB(255,0,0), 2, 8, 0);
	circle(drawing, fPattern.topright, 3, CV_RGB(0,255,0), 2, 8, 0);
	circle(drawing, fPattern.bottomleft, 3, CV_RGB(0,0,255), 2, 8, 0);

	vector<Point2f> vecsrc;
	vector<Point2f> vecdst;
	//добавление координат центров квадратов
	vecsrc.push_back(fPattern.topleft);
	vecsrc.push_back(fPattern.topright);
	vecsrc.push_back(fPattern.bottomleft);
	vecdst.push_back(Point2f(20, 20));
	vecdst.push_back(Point2f(120, 20));
	vecdst.push_back(Point2f(20, 120));
	Mat affineTrans = getAffineTransform(vecsrc, vecdst); //аффинное преобразование по координатам центров квадратов
	Mat warped;
	warpAffine(ori, warped, affineTrans, ori.size()); //фронтальная проекция QR-кода
	Mat qrcode_color = warped(Rect(0, 0, 140, 140));
	Mat qrcode_gray;
	cvtColor (qrcode_color,qrcode_gray,CV_BGR2GRAY);
	Mat qrcode_bin;
	threshold(qrcode_gray, qrcode_bin, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU); //бинаризация QR-кода

	//imshow("binary", bin);
	//imshow("canny", pcanny);
	//imshow("finder patterns", drawing);
	imshow("binaried qr code", qrcode_bin);
	//imshow("gray qr code", qrcode_gray);
}


int main( int argc, char **argv )
{
    Mat ori = imread(argv[1]); //считывание цветного изображения
    Mat gray = imread(argv[1], 0); //считывание серого изображения
    imshow("ori", ori);
    //waitKey();
    invoke(ori, gray);
    waitKey();
    return 0;
}
