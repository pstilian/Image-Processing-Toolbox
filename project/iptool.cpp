#include "../iptools/core.h"

#include <string.h>

#include <fstream>
#include <iomanip> // for setprecision()

#define MAXLEN 256//max length of function name string or filename

//local methods:
int executeFunction(image src,char tgtLoc[], ROI roi, int ac, char *av[]);
void usageErr();

//global vars (b/c I'm being lazy)
bool ISCOLOR = true;	//default true, set false if it is grey

int main (int argc, char** argv){
	//BEGIN checkInput
	if(argc < 4){
		usageErr();
		return 0;
	} //implied else
	//END checkInput
	char imageFile[MAXLEN];	//name of image file
	cout<<argv[1]<<" is ";
	//BEGIN IMAGE PREPPER
	//check input file format
	if(strstr(argv[1],".pgm")!='\0'){
		cout<<"ideal greyScale format. swell!\n";
		strcpy(imageFile,argv[1]);
		ISCOLOR = false;	//else this is TRUE
	} else if(strstr(argv[1],".ppm")!='\0'){//if pgm or ppm format
		cout<<"ideal color format. swell!\n";
		strcpy(imageFile,argv[1]);
	}else if(utility::isImage(argv[1])){	//convert if common format image
		//BEGIN convertFromImage
		cout<<"recognized image format, creating ";
		strcpy(imageFile,argv[1]);
		strcat(imageFile,".ppm");
		cout<<imageFile<<" for processing";
		char c[MAXLEN] = "convert ";	//command for system
		strcat(c,argv[1]);
		strcat(c," ");
		strcat(c,imageFile);
		system(c);	//convert using imageMagick
		//END convertFromImage
	} else if(utility::isVideo(argv[1])){	//convert if common video formats
		cout<<"video.\n";
		//BEGIN	vidProcSetup
		string srcDir = "iptoolVidDecomp/";
		string tgtDir = "iptoolVidRecomp/";
		system(("mkdir "+srcDir).c_str());	//create directory for frames
		system(("mkdir "+tgtDir).c_str());
		cout<<"temp directory created for frames.\n";
		//convert to images using avconv
		char c[MAXLEN] = "avconv -i ";
		strcat(c,argv[1]);
		strcat(c," iptoolVidDecomp/%d.ppm");
		cout<<"extracting frames...\n";
		system(c);
		//END vidProcSetup
		//BEGIN getROIs
		ROI foi;
		foi.CreateFOIList(argv[3]);//get Frames of Interest from FOI file
		cout<<"region of interest read from file\n";
		//END getROIs
		//check for any across-frame value functions
		if (!strncasecmp(argv[4],"equalizeVideo",MAXLEN)) {
			//BEGIN equalizeFrames
			//BEGIN computeMappings
			int map[foi.numBaseROI+1][256];	//value mappings for equalizeVideo
			cout<<"equalizing video from unified mapping taken over all frames...\n";
			for(int r=0; r<foi.numBaseROI; r++){	//for each sub-ROI in list (BaseROI)
				//compute across-frame mapping
				equalize::getMap(srcDir,foi.list[r],map[r]);
				cout<<"FOI #"<<r<<" mapping created.\n";
			}
			//END computeMappings
			//BEGIN processFrames
			cout<<"processing image files...\n";
			int frame = 1;
			while(true){	//for all frames
				cout<<"frame "<<frame;
				//BEGIN imageExists
				string filename = srcDir+utility::intToString(frame)+".ppm";
				std::ifstream ifile(filename.c_str());
				if(!ifile) break;	//exit when out of files
				ifile.close();
				//END imageExists
				if(foi.InROI(frame)){	//if frame is in at least 1 ROI
					//BEGIN setupSrcImage
					image source;
					char fname[MAXLEN];
					strcpy(fname, filename.c_str());
					source.read(fname);	//load src image
//
					image HSIsrc;	//convert src to HSI
					colorSpace::RGBtoHSI(source,HSIsrc);
					image HSItgt;
					HSItgt.copyImage(HSIsrc);
//
					//END setupSrcImage
					//BEGIN setupTgtImage
					image tgt;
					tgt.resize(source);
					//END setupTgtImage
					//NOTE: warning! this implementation will not behave properly on overlapping ROIs!
					for(int rr=0; rr<foi.numBaseROI; rr++){	//for each sub-ROI in list (BaseROI)
						if (foi.list[rr].InROI(frame)) {//if in this FOI
							
							equalize::fromMap(HSIsrc, HSItgt, foi.list[rr], map[rr],BLUE);//equalize the frame using map
							// cout<<".";	//dots to show progress, also # of dots indicate # of ROIs frame is in
						}
					}
//
					colorSpace::HSItoRGB(HSItgt,tgt);	//convert back to RGB for finished image
//
					tgt.save((tgtDir+utility::intToString(frame)+".ppm").c_str());	//save tgt image
					 cout<<" equalized\n";

				}else{	//frame in no ROIs
					//copy image to output dir
					system(("mv "+filename+" "+tgtDir+utility::intToString(frame)+".ppm").c_str());
					 cout<<" unchanged\n";
					

					
				}
				frame++;
			}
			//END PROCESS FRAMES
		} else {	//apply image processing to all frames separately
			//BEGIN PROCESS FRAMES
			int frame = 1;
			cout<<"processing image files...\n";
			while(true){	//for all frames
				string filename = srcDir+utility::intToString(frame)+".ppm";
				std::ifstream ifile(filename.c_str());
				if(!ifile) break;
				ifile.close();
				if (foi.InROI(frame)) {//call function if in FOI
					image source;
					char *fname = new char[filename.length() + 1];
					strcpy(fname, filename.c_str());
					source.read(fname);	//load src image
					string t = tgtDir+utility::intToString(frame)+".ppm"; 
					char *targ = new char[t.length() + 1];
					strcpy(targ, t.c_str());
					int argN = 4; //number of args before function name
					int FargLen = argc - argN;	//length of function args
					char *fArgs[FargLen];	//get function Args from argv
					for(int i = 0; i<FargLen; i++){
						fArgs[i] = argv[argN+i];
					}
					//TODO: this function call causes processing to disregard frame selection!
					executeFunction(source, targ, foi, FargLen, fArgs);
				}else{
					//copy image to output dir
					system(("mv "+filename+" "+tgtDir+utility::intToString(frame)+".ppm").c_str());
				}
				frame++;
			}
			//END PROCESS FRAMES
		}
		cout<<"done.\n";
		//BEGIN imagesToVideo
		char cc[MAXLEN] = "avconv -r 25 -i iptoolVidRecomp/%d.ppm ";
		strcat(cc,argv[2]);
		system(cc);//remux video with avconv
		system(("rm -r "+ srcDir+" "+tgtDir).c_str());//cleanup
		//END imagesToVideo
		cout<<"SUCCESS! video is in your current directory.\n";
		return 0;
	} else {
		cout<<"unrecognize file type\n"; 
		usageErr();
		return 0;
	}
	//END FILE PREPPER
	if(argc==4){	//if script
		cout<<"running script "<<argv[3]<<"...\n";
		cout<<"don't do that; it doesn't work.\n";	//TODO: fix & remove this line
/*=== DON'T USE THIS SCRIPTING, IT DOES NOT WORK RIGHT NOW ===================================
### Scripting ###
A list of commands (script) containing a list of functions to perform and a set of parameters for each function can be passed with the input image name and output image name in order to run the script.
syntax:
	"iptool <inputFile> <outputFile> <scriptName>"
example:
	"iptool myInput.pgm myOutput.pgm myScript.txt"

This allows for the easy re-use of a coplex series of commands to be performed on one image (or video) file.

		//open script file
		while(!EOF){
			//TODO: read line from script
			int nArgs = 0;	//TODO: this should be len(baseROI)?
			char** argList;	//TODO: this shoudl be ROIlist[i]?
			if(!executeFunction(nArgs, argList)){	//execute commands
				return 0;
			}else{
				usageErr();
				return 0;
			}
			//TODO:buffer = tgt;
			//TODO:close original src file
			//TODO:src = buffer;
		}
		//TODO:copy buffertgt to tgt destination, delete buffertgt

=======================================================================*/
	}else{	//attempt to execute assuming all is specified properly
		cout<<"loading file "<<imageFile<<".\n";
		image source;	
		source.read(imageFile);	//load src image
		int argN = 0; //number of args before function name
		//BEGIN getROI
		ROI roi;
		cout<<"region of interest is ";
		if(!strncasecmp(argv[3],"all\0",MAXLEN)){	//if ROI is whole image
			cout<<"whole image\n";
			roi.CreateList(0,0,source.getNumberOfRows(),source.getNumberOfColumns());
			argN = 4;
		}else if(atoi(argv[3]) && atoi(argv[4]) && atoi(argv[5]) && atoi(argv[6])){	//if ROI is x,y,sx,sy notation
			cout<<"x="<<argv[3]<<" y="<<argv[4]<<" sx="<<argv[5]<<" sy="<<argv[6]<<'\n';
			argN = 7;
			roi.CreateList(atoi(argv[3]),atoi(argv[4]),atoi(argv[5]),atoi(argv[6]));
		}else{	//assume ROI filename is passed
			cout<<"being read from file "<<argv[3]<<"...\n";
			roi.CreateList(argv[3]);	//construct from filename
			argN = 4;
		}
		//END getROI
		int FargLen = argc - argN;	//length of function args
		char *fArgs[FargLen];	//get function Args from argv
		for(int i = 0; i<FargLen; i++){
			fArgs[i] = argv[argN+i];
		} 
		if(!executeFunction(source,argv[2],roi,FargLen,fArgs)){
			return 0;
		}else{
			cout<<"ERROR executing function "<<argv[argN+1]<<'\n';
			return 0;
		}
	}
	//TODO: convert & save to desired format (right now all image outputs are .ppm or .pgm)
}

void usageErr(){
	cout<<"ERROR: usage.\n"
			"See readme for detailed usage info.\n";
}

//this function accepts a list of parameters from a script or the command line and executes the desired function the first parameter is the name of the software (iptool) and is not used by this function
//params:
//	ac = number of arguments
//	ag = list of arguments
int executeFunction(image src,char tgtLoc[], ROI roi, int ac, char *av[]){
	cout<<"running function "<<av[0]<<"...\n";
	image tgt;
        if (!strncasecmp(av[0],"add",MAXLEN)) {
		//int value = atoi(av[1]);
        	add::addGrey(src,tgt,roi,atoi(av[1]));
/* ======= ======= ======= ======= PROJECT 1 ======= ======= ======= ======= */
        }else if (!strncasecmp(av[0],"binarize",MAXLEN) 
		|| !strncasecmp(av[0],"grayThresh",MAXLEN)
		|| !strncasecmp(av[0],"greyThresh",MAXLEN)) {
		//int thresh = atoi(av[1]);
		threshold::thresh(src,tgt,roi,atoi(av[1]));
 	}else if (!strncasecmp(av[0],"scale",MAXLEN)) {
		//int s = atoi(av[1]);
		if(scale::scale2or4(src,tgt,roi,atoi(av[1])))
			cout<<"error in function"<<av[0]<<".\n";
	}else if (!strncasecmp(av[0],"colorThresh",MAXLEN)) { 
		//int thresh = atoi(av[1]);
		//int R = atoi(av[2]);
		//int G = atoi(av[3]);
		//int B = atoi(av[4]);
		threshold::thresh(src,tgt,roi,atoi(av[1]),atoi(av[2]),atoi(av[3]),atoi(av[4]));
	}else if (!strncasecmp(av[0],"smooth2d",MAXLEN)) {
		//int winS = atoi(av[1]);
		if(smooth::smooth2d(src,tgt,roi,atoi(av[1])))
			cout<<"error in function "<<av[0]<<".\n";
	}else if (!strncasecmp(av[0],"smooth1d",MAXLEN)) {
		//int winS = atoi(av[1]);
		if(smooth::smooth1d(src,tgt,roi,atoi(av[1])))
			cout<<"error in function "<<av[0]<<".\n";
	}else if (strncasecmp(av[0],"weightedThreshold",MAXLEN)==0
		|| strncasecmp(av[0],"weightedThresh",MAXLEN)==0
		|| strncasecmp(av[0],"adaptiveThreshold",MAXLEN)==0
		|| strncasecmp(av[0],"adaptiveThresh",MAXLEN)==0){
		//int wS = atoi(av[1]);	//window size
		//int w  = atoi(av[2]);	//amount above mean
		threshold::adapThresh(src,tgt,roi,atoi(av[1]),atoi(av[2]));
/* ======= ======= ======= ======= PROJECT 2 ======= ======= ======= ======= */
        }else if (!strncasecmp(av[0],"equalizeGrey",MAXLEN)
		|| !strncasecmp(av[0],"equalizeGray",MAXLEN)){
		cout<<"performing greyscale histogram equalization.\n";
		equalize::grey(src,tgt,roi);
	}else if (!strncasecmp(av[0],"equalizeColors",MAXLEN)){
		cout<<"performing histogram equalization on colors separately.\n";
		equalize::colors(src,tgt,roi);
	}else if (!strncasecmp(av[0],"equalizeAvg",MAXLEN)){
		cout<<"performing histogram equalization using avg of colors.\n";
		equalize::colorAvg(src,tgt,roi);
	}else if (!strncasecmp(av[0],"equalizeIntensity",MAXLEN)){
		cout<<"performing equalization using HSI color space.\n";
		equalize::intensity(src,tgt,roi);
	}else if (!strncasecmp(av[0],"equalizeChannel",MAXLEN)){
		int channel = atoi(av[1]);
		cout<<"equalizing RGB color channel #"<<channel<<'\n';
		tgt.resize(src.getNumberOfRows(),src.getNumberOfColumns());
		tgt.copyImage(src);
		equalize::channel(src,tgt,roi,channel);
	}else if (!strncasecmp(av[0],"getHistogram",MAXLEN)){
		cout<<"working on this...\n";
		int *histP =  equalize::getHist(src, roi.list[0], GREY);
		equalize::drawHist( histP , tgt );
/* ======= ======= ======= ======= PROJECT 3 ======= ======= ======= ======= */
	}else if (!strncasecmp(av[0],"edgeDetect",MAXLEN)){
		cout<<"applying sobel edge detection";
		tgt.resize(src);
		if(ISCOLOR){
			cout<<" to intensity of color image...\n";
			image HSIsrc, HSItgt;
			HSIsrc.resize(src);
			HSItgt.resize(src);
			colorSpace::RGBtoHSI(src,HSIsrc);
			// edgeDetect on I
			if(ac == 1) edgeDetect::sobel(HSIsrc,tgt,roi,BLUE);//Intensity=BLUE
			else if(ac == 2){
				int T = atoi(av[1]);	//threshold
				cout<<"threshold = "<<T<<"\n";
				edgeDetect::sobel(HSIsrc,HSItgt,roi,BLUE,T);//Intensity=BLUE
			}
			tgt.resize(HSItgt);
			colorSpace::HSItoRGB(HSItgt,tgt);
		}else{//is grey
			cout<<" to greyscale image...\n";
			if(ac == 1) edgeDetect::sobel(src,tgt,roi,GREY);
			else if(ac == 2){
				int T = atoi(av[1]);	//threshold
				cout<<"threshold = "<<T<<"\n";
				edgeDetect::sobel(src,tgt,roi,GREY,T);
			}
		}
	}else if (!strncasecmp(av[0],"detectEdgeDirection",MAXLEN)){
		if(atoi(av[2])<0||atoi(av[3])<0) cout<<"ERR?: angles should be >0... I think...\n";
		tgt.resize(src);
		int T  = atoi(av[1]);	//threshold
		float degToRad = 3.141592653589793238462643383279502884f/180.0f;
		float A1 = degToRad * atof(av[2]);	//angle start point (convert from degrees)
		float A2 = degToRad * atof(av[3]);	//angle end point
		cout << setprecision(4);
		cout<<"detecting edges with gradient magnitude >"<<T<<" between the angles "<<A1<<" and "<< A2 <<"(radians)...\n";
		if(ISCOLOR){
			image HSIsrc, HSItgt;
			HSIsrc.resize(src);
			HSItgt.resize(src);
			colorSpace::RGBtoHSI(src,HSIsrc);
			edgeDetect::directional(HSIsrc,HSItgt,roi,BLUE,T,A1,A2);// edgeDetect on I (Intensity = BLUE)
			tgt.resize(HSItgt);
			colorSpace::HSItoRGB(HSItgt,tgt);
		}else{//is grey
			edgeDetect::directional(src,tgt,roi,GREY,T,A1,A2);
		}
	}else if (!strncasecmp(av[0],"circleDetect",MAXLEN)){
		//BEGIN getEdgeImage
		char edgeImgLoc[] = "circleDetect_EdgeImg.pgm";
		static int acc = 2;
		char *avv[acc];
		char fname[] = "edgeDetect"; 
		avv[0] = fname;
		//char thresh[] = av[1];
		avv[1] = av[1];
		executeFunction(src,edgeImgLoc,roi,acc,avv);
		image edgeImg;
		edgeImg.read(edgeImgLoc);
		//END getEdgeImage
		//TODO:note: is current magnitude image format good enough? 
		cout<<"attempting to detect circles...\n";
		image detectedCircles;
		circleDetect::hough(edgeImg,detectedCircles,roi,atoi(av[2]));
		detectedCircles.save("circleDetect_circlesDetected.pgm");
		add::addGreyImage(src,detectedCircles,tgt,roi,GREEN);	//draw circles on orignal image
/* ======= ======= ======= ======= PROJECT 4 ======= ======= ======= ======= */
	}else 	if (strncasecmp(av[0],"Filtering",MAXLEN)==0){
		fourier::filter();


	}else{	//return err: command not recognized
		cout<<"ERR: function "<<av[0]<<" not recognized\n";
		return 1;
	}
	cout<<"saving output to "<<tgtLoc<<"\n";
	tgt.save(tgtLoc);
	return 0;
}


