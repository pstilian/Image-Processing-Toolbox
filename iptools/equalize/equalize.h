#ifndef EQUALIZE_H
#define EQUALIZE_H

#include "../image/image.h"
#include "../roi/roi.h"

#include <math.h>
#include "../utility/utility.h"
#include "../colorSpace/colorSpace.h"
#include <fstream>

class equalize{
	public:	
		equalize();
		virtual ~equalize();
		//type of equalization:
		static void channel(image &src, image &tgt, ROI roi, int chan);
		static void grey(image &src, image &tgt, ROI roi);
		static void colors(image &src, image &tgt, ROI roi);
		static void colorAvg(image &src, image &tgt, ROI roi);
		static void intensity(image &src, image &tgt, ROI roi);
		static void videoFrame(image &src, image &tgt, BaseROI roi, int map[]);
		//other functions:
		static void getMap(string srcDir, BaseROI foi, int map[]);
	private:
		static void box(image &im);
		static void drawHist(int histH[256], image &tgt);
		static void drawColorHist(int histH[256][3], image &tgt);
};

#endif

