#include "circleDetect.h"

// adjust array values to be in the range 0-255 by spreading equally into bins
//TODO:NOTE: will spread into as many bins as possible up to 255, if < 255 unique values in the array, then the number of unique values will be used. Thus, if a binary image array is passed, a binary spread will be returned
void circleDetect::normalizeArray(float **buffer,int nRows,int nCols){
//BEGIN normalizeArray
	//get boundary values to adjust range of values to 0-255
	int nPix = nRows*nCols;	//count of pixels
	int pixPerBin = round((float)nPix/256.0f);//don't worry about preciseness here, bins will still fill up acceptably
	float sorted[nPix];	//create sorted array of all values
	//copy values into sortArray
	for(int i = 0; i < nRows; i++){//for each pixel
		for(int j = 0; j<nCols; j++){
			sorted[i*nRows+j] = buffer[i][j];
		}
	}
	float* first(&sorted[0]);
	float* last(first + 4);
	sort(first,last, std::greater<float>());//sort the array
	int bins[256] = {0};	//create array of bin edges (top of each bin)
	bins[0] = sorted[pixPerBin];
	int pixDone = 0;	//pixel offset used to shift bin count start location
	for(int b = 1; b<256; b++){	//for each bin
		//binMax = sortedArry[ bin# * numpixPerBin]
		bins[b] = sorted[pixPerBin+pixDone];
		pixDone += pixPerBin;
//		cout<<pixDone<<'\n';
		/*
		while(bins[b]==bins[b-1]&&pixDone<nPix){//check for bin redundancy
			int pixLeft = nPix-pixDone;
			int binsLeft = 256-b;
			pixPerBin = round((float)pixLeft/(float)binsLeft)+1;//change pixPerBin for future bins (+1 to ensure that always at least 1 pixel)
			bins[b] = (int)round(sorted[pixDone+pixPerBin]); //get new bin max
			pixDone += pixPerBin;
//			cout<<pixLeft<<'\n';
		}
		*/





	}
	for(int i = 0; i < nRows; i++){//for each pixel 
		for(int j = 0; j<nCols; j++){
			//find pixel bin (compare to bin maxes until <=)
			for(int b = 0; b<256; b++){	//for each bin
				//set pixel value to corresponding bin
				if(buffer[i][j]<=bins[b]){
					buffer[i][j]=(float)b;
					break;
				}
			}
		}
	}
	//END normalizeArray
}



/*-----------------------------------------------------------------------**/
// rad = desired radius to detect TODO:make this a range
void circleDetect::hough(image &edgeImg, image &tgt, ROI roi, int rad){
	// parameter space for circle: x,y,R
	int rMin = 1;	//radius (in pix?)
	int rMax = min(edgeImg.getNumberOfRows(),edgeImg.getNumberOfColumns())/2;
	int rDelta = 1;
	int rSize = rMax-rMin+1;
	//NOTE: this next parts assumes that circle center is not on image edges to decrease search space
	int aMin = 0+edgeImg.getNumberOfRows()*.1;	//possible x
	int aMax = edgeImg.getNumberOfRows()*.9;
	int aDelta = 1;
	int aSize = aMax-aMin+1;
	int bMin = 0+edgeImg.getNumberOfColumns()*.1;	//possible y
	int bMax = edgeImg.getNumberOfColumns()*.9;
	int bDelta = 1;
	int bSize = bMax-bMin+1;
	cout<<"filling search space, computation is "<<aSize<<'x'<<bSize<<" for each edge image pixel. Be patient. Currently on pixel #:\n";
	float ***acc = new float**[rSize+1];// = {0};	//accumulator array
	for(int r = 0; r < rSize; ++r) {
		acc[r] = new float*[aSize+1];
		for(int a = 0; a < aSize; ++a){
			acc[r][a] = new float[bSize+1];
		}
	}
//	int acc[rMax-rMin][aMax-aMin][bMax-bMin];	// set up accumulator array
//	memset(acc,0,sizeof(acc));	//initialize to 0

	for (int i=0; i<edgeImg.getNumberOfRows(); i++){	// for each edge point
		for (int j=0; j<edgeImg.getNumberOfColumns(); j++){
			if(j%100==0&&i%100==0) cout<<"("<<i<<","<<j<<")\n";
			int G = edgeImg.getPixel(i,j);// get gradient magnitude
			if (roi.InROI(i,j)&&G>100){//skip pixels not in roi & if G!>0
				// calculate the possible circle equations (cone?)
				for(int a=aMin; a<aMax; a+=aDelta){ // for all x
					for(int b=bMin; b<bMax; b+=bDelta){ // for all x// for all y
						int r = sqrt(pow((a-i),2)+pow((b-j),2));
						if(r<=rMax&&r>=rMin)
							acc[r-rMin][a-aMin][b-bMin]+=1;
/*						for(int r=rMin; r<rMax; r+=rDelta){
//							if(j%50==0&&i%50==0) cout<<"["<<r<<"]["<<a<<"]["<<b<<"]\n";
							//acc[r-rMin][a-aMin][b-bMin] += G; // increment accumulator array
							acc[r-rMin][a-aMin][b-bMin]+=1;
						}
*/
					}
				}
			//}else{	
				//ignore the pixel
			}
		}
	}
	cout<<"...done. Generating Display(s) of Hough-space\n";
	system("mkdir radii");
	int nRows = aSize/aDelta;
	int nCols = bSize/bDelta;
	int nImages = rSize/rDelta;
	tgt.resize(edgeImg);
//	normalizeArray(acc[desiredR-rMin],nRows,nCols);//adjust range of accumulator to 0-255 for display
	for(int r = 0; r<nImages; r+=rDelta){	//for each radius
		int desiredR = r+rMin;
		for(int i = 0; i<nRows; i++){//for each pixel in hough space
			for(int j = 0; j<nCols; j++){
//				cout<<"["<<r<<"]["<<i<<"]["<<j<<"]\n";
				tgt.setPixel(i+aMin,j+bMin,acc[r][i][j]);
				//NOTE: tgt is used as buffer here
			}
		}
		tgt.save(("radii/circleR"+utility::intToString(desiredR)+".pgm").c_str());
	}
	int frameRate = 24;//frame/sec
	int bitRate = edgeImg.getNumberOfRows()*edgeImg.getNumberOfColumns()*frameRate;//calc lossless bitrate
	system(("avconv -i radii/circleR%d.pgm -qscale 1 houghSpace.mp4"));//make Hough-space video
//TODO: note: decreasing the frame rate does not extend video length (don't do it)
//	system(("avconv -i radii/circleR%d.pgm -qscale 2 -r "+utility::intToString(frameRate)+" houghSpace.mp4").c_str());//make Hough-space video
//	system("rm -r radii");	//cleanup

	cout<<"finding local maxima in hough space...\n";
	int thresh = 250;
	int nCircles=0;
	image localMax;
	localMax.resize(tgt);
	tgt.deleteImage();	//clear previous image
	tgt.resize(localMax);
	while(nCircles<1&&thresh>0){	//detect at least 1 circle
		//for each radius within range of acceptable radii
		int plusMinusR = 1;//rad/4;
		for(int r = rad-plusMinusR; r<rad+plusMinusR; r++){
			for(int i = 1; i<nRows-1; i++){//for each pixel in hough space
				for(int j = 1; j<nCols-1; j++){
					// (pseudo)threshold hough space (drop off anything below thresh, keep values above thresh)
					if(acc[r][i][j]>thresh //if above thresh
					&& acc[r][i][j]>acc[r-1][i][j]//AND if Local Max
					&& acc[r][i][j]>acc[r+1][i][j]
					&& acc[r][i][j]>acc[r][i-1][j]
					&& acc[r][i][j]>acc[r][i+1][j]
					&& acc[r][i][j]>acc[r][i][j-1]
					&& acc[r][i][j]>acc[r][i][j+1]){
						acc[r][i][j] = 255;
						nCircles++;
						int a = i+aMin, b = j+bMin;
						localMax.setPixel(a,b,acc[r][i][j]);// generate resulting binary image
						//draw circle at desired radius for each white pixel
						for(int th = 0; th<360; th++){
							int x = round(a + r*cos(((float)th)*2.0f*3.1415f/180.0f));
							int y = round(b + r*sin(((float)th)*2.0f*3.1415f/180.0f));
							tgt.setPixel(x,y,255);
						}
					}//else{//not local max. do nothing
				}
			}
		}
		thresh -=20;
	}
	localMax.save(("circleDetect_"+utility::intToString(nCircles)+"LocalMaxAtR_"+utility::intToString(rad)+".pgm").c_str());
	//BEGIN cleanupAccumulator
	for(int r = 0; r < rSize; ++r) {
		for(int a=0; a<aSize; ++a)
			delete [] acc[r][a];	//deletes all 'b's
		delete [] acc[r];		//deletes all 'a's
	}
	delete [] acc;				//deletes all 'r's
	//END cleanupAccumulator
}
