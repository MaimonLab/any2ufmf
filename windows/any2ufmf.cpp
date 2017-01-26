#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <objbase.h>      // For COM headers

#include "cv.h"
#include "highgui.h"
#include "ufmfWriter.h"
#include "previewVideo.h"

#include "pgTimeStamp.h"
#include "fmfreader.h"
#include <boost\filesystem.hpp>

typedef enum {
	DialogTypeInput,
	DialogTypeOutput
} DialogType;

bool ChooseFile(char fileName[], const char dialogTitle[], const COMDLG_FILTERSPEC filterSpec[], int nFilters, DialogType dialogType);
bool ChooseFile(char fileName[], const char dialogTitle[], const COMDLG_FILTERSPEC filterSpec[], int nFilters, DialogType dialogType, char defaultFileName[]);
bool ChooseMultiFile(std::vector<std::string>* fileName, const char dialogTitle[], const COMDLG_FILTERSPEC filterSpec[], int nFilters, char defaultFileName[]);


int main(int argc, char * argv[])
{
	bool interactiveMode = argc <= 3;
	bool fileChoiceSuccess = true;

	char aviFileName[512];  //maintained to reduced changes
	std::vector<std::string> aviFiles;  //holds (potentially) multiple input avi's

	//Get input avi file
	if (argc > 1) {
		// first argument is the input AVI

		//strcpy(aviFileName,argv[1]);
		aviFiles.push_back(argv[1]);
		//fprintf(stdout,"Input AVI file = %s\n",aviFileName);
		fprintf(stdout, "Input AVI file = %s\n", aviFiles[0]);

	}
	else {
		// if not run from cmd line, create dialog to select avi
		const COMDLG_FILTERSPEC aviTypes[] =
		{
			{L"Audio-video Interleave Files (*.avi)",   L"*.avi"},
			{L"Fly Moive Format Files (*.fmf)", L"*.fmf"},
			{L"All Files (*.*)",    					L"*.*"}
		};
		//fileChoiceSuccess = ChooseFile(aviFileName, "Choose AVI file", aviTypes, ARRAYSIZE(aviTypes), DialogTypeInput);
		fileChoiceSuccess = ChooseMultiFile(&aviFiles, "Choose AVI file", aviTypes, ARRAYSIZE(aviTypes), NULL);

	}

	//Get output ufmf file
	char ufmfFileName[512];
	if (argc > 2) {
		strcpy(ufmfFileName, argv[2]);
		fprintf(stdout, "Output UFMF file = %s\n", ufmfFileName);
	}
	else {
		const COMDLG_FILTERSPEC ufmfTypes[] =
		{
			{ L"Micro Fly Movie Format Files (*.ufmf)",  L"*.ufmf" },
			{ L"All Files (*.*)",    					L"*.*" }
		};

		// choose default filename: (AVI filename substring between last backslash and last dot) + ".ufmf"
		char defaultFileName[512];
		strcpy(aviFileName, (aviFiles[0]).c_str());
		strcpy(defaultFileName, aviFileName);
		char* strLastDot = strrchr(defaultFileName, '.');
		if (strLastDot != NULL) {
			strcpy(strLastDot, ".ufmf");
			char *strLastBackslash = strrchr(defaultFileName, '\\');
			if (strLastBackslash != NULL) {
				char tmp[512];
				strcpy(tmp, &defaultFileName[strLastBackslash + 1 - defaultFileName]);
				strcpy(defaultFileName, tmp);
			}

			fileChoiceSuccess = ChooseFile(ufmfFileName, "Choose output file", ufmfTypes, ARRAYSIZE(ufmfTypes), DialogTypeOutput, defaultFileName);
		}
		else {
			fileChoiceSuccess = ChooseFile(ufmfFileName, "Choose output file", ufmfTypes, ARRAYSIZE(ufmfTypes), DialogTypeOutput);
		}
	}

	// test output file
	if (!fileChoiceSuccess || strlen(ufmfFileName) == 0) {
		if (!interactiveMode)
			fprintf(stderr, "Empty output filename specified. Aborting.\n");
		return 1;
	}

	// attempt to open output ufmf file
	FILE *fp = fopen(ufmfFileName, "w");
	if (fp == NULL) {
		if (interactiveMode) {
			MessageBox(NULL, "Error opening output file. Exiting.", NULL, MB_OK);
		}
		else {
			fprintf(stderr, "Error opening output file\n");
		}
		return 1;
	}
	fclose(fp);

	//Get parameters file
	char ufmfParamsFileName[512];
	if (argc > 3) {
		strcpy(ufmfParamsFileName, argv[3]);
		fprintf(stdout, "UFMF Compression Parameters file = %s\n", ufmfParamsFileName);
	}
	else {
		int choice = MessageBox(NULL, "Select a custom parameters file?", "Specify parameters?", MB_YESNO);
		if (choice == IDYES) {
			const COMDLG_FILTERSPEC ufmfParamTypes[] =
			{
				{ L"Text Files (*.txt)", L"*.txt" },
				{ L"All Files (*.*)",    L"*.*" }
			};
			ChooseFile(ufmfParamsFileName, "Choose parameters file", ufmfParamTypes, ARRAYSIZE(ufmfParamTypes), DialogTypeInput);
		}
		else
			ufmfParamsFileName[0] = '\0';
	}

	double old_ts = 0.0;
	double offset = 0.0;
	ufmfWriter * writer;
	std::vector<std::string>::const_iterator i;

	//Loop over each of the selected videos
	for (i = aviFiles.begin(); i != aviFiles.end(); i++)
	{
		strcpy(aviFileName, (*i).c_str());  //copy current avi file name into char array 
											//(makes things back compatible with existing code)

		// test input file
		if (!fileChoiceSuccess || strlen(aviFileName) == 0) {
			if (!interactiveMode)
				fprintf(stderr, "Empty input filename specified. Aborting.\n");
			return 1;
		}

		// open input avi or fmf
		CvCapture* capture;
		fmfreader fmfcapture;
		bool isfmf = false;
		if (boost::filesystem::path(aviFileName).extension() == ".fmf") {
			fmfcapture.fmf_open(aviFileName);
			isfmf = true;
		}
		else if(boost::filesystem::path(aviFileName).extension() == ".avi") {
			capture = cvCaptureFromAVI(aviFileName);
			if (capture == NULL) {
				if (interactiveMode) {
					MessageBox(NULL, "Error reading AVI. Exiting.", NULL, MB_OK);
				}
				else {
					fprintf(stderr, "Error reading AVI %s. Exiting.\n", aviFileName);
				}
				return 1;
			}
		}
		else {
			if (interactiveMode) {
				MessageBox(NULL, "Invalid file type selected.  Exiting.", NULL, MB_OK);
			}
			else {
				fprintf(stderr, "Invalid file type selected.  Exiting.\n");
			}
		}

		// declare variable to hold frame data
		IplImage * frame = NULL;

		// Get movie info (frame size, nframes, etc)
		unsigned __int32 frameH;
		unsigned __int32 frameW;
		double nFrames;
		if (isfmf){
			// get fmf frame info
			frameH = fmfcapture.fmf_get_fheight();
			frameW = fmfcapture.fmf_get_fwidth();
			nFrames = fmfcapture.fmf_get_nframes();

			//frame = fmfcapture.fmf_queryframe();
			fmfcapture.fmf_pointff();
			frame = fmfcapture.fmf_nextframe();
			capture = NULL;
		}
		else {
			// get avi frame info
			frame = cvQueryFrame(capture); // this call is necessary to get correct capture properties
			frameH = (unsigned __int32)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
			frameW = (unsigned __int32)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
			nFrames = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
		}
		fprintf(stderr, "Number of frames in the video: %f\n", nFrames);

		// log file
		//FILE * logFID = fopen("C:\\Code\\imaq\\any2ufmf\\out\\log.txt","w");  //output errors to file
		FILE * logFID = stderr;  //output errors to console

		// create output ufmf writer if first video in series
		if (i == aviFiles.begin())
		{
			writer = new ufmfWriter(ufmfFileName, frameW, frameH, logFID, ufmfParamsFileName);
			if (!writer->startWrite()) {
				if (interactiveMode) {
					MessageBox(NULL, "Error initializing uFMF writer. Exiting.", NULL, MB_OK);
				}
				else {
					fprintf(stderr, "Error starting write\n");
				}
				return 1;
			}
		}

		// start preview thread
		HANDLE lock = CreateSemaphore(NULL, 1, 1, NULL);
		previewVideo * preview = new previewVideo(lock);

		unsigned __int64 frameNumber;
		IplImage * frameWrite = NULL;
		IplImage * grayFrame = cvCreateImage(cvSize(frameW, frameH), IPL_DEPTH_8U, 1);

		double timestamp;
		double frameRate = 1.0 / 30.0;

		fprintf(stderr, "Hit esc to stop playing\n");
		bool DEBUGFAST = false;

		//Loop over each frame of an individual video
		for (frameNumber = 0, timestamp = 0.; ; frameNumber++, timestamp += frameRate) {

			if (DEBUGFAST && frameNumber >= 3000)
			{
				break;
			}
			if ((frameNumber % 100) == 0) {
				fprintf(stderr, "** frame %lu\n", frameNumber);
			}

			if (!DEBUGFAST && (WaitForSingleObject(lock, MAXWAITTIMEMS) != WAIT_OBJECT_0)) {
				fprintf(stderr, "Error waiting for preview thread to unlock\n");
				break;
			}

			//Read in frame data from avi
			if (frameNumber != 0) {
				if (!DEBUGFAST || (frame == NULL))
				{
					if (isfmf) {
						frame = fmfcapture.fmf_nextframe();
						//frame = fmfcapture.fmf_queryframe();
					}
					else {
						frame = cvQueryFrame(capture);
					}
				}
			}

			if (!DEBUGFAST) ReleaseSemaphore(lock, 1, NULL);
			if (!frame) {
				fprintf(stderr, "Last frame read = %d\n", frameNumber);
				break;
			}

			// Send the frame to the preview window
			if (!DEBUGFAST && !preview->setFrame(frame, frameNumber)) {
				break;
			}

			// Convert frame if necessary
			if (!DEBUGFAST || frameWrite == NULL) {
				if (frame->nChannels > 1) {
					cvCvtColor(frame, grayFrame, CV_RGB2GRAY);
					frameWrite = grayFrame;
				}
				else {
					frameWrite = frame;
				}
			}

			//Unwrap time embedded time stamps
			timestamp = pg_timestamp(frame) + offset;
			if (timestamp - old_ts < 0)
			{
				offset += 128.0;
				timestamp = pg_timestamp(frame) + offset;
			}
			old_ts = timestamp;
			//printf("%f\n", timestamp);

			// Write the frame to the ufmf checking for error
			if (!writer->addFrame((unsigned char*)frameWrite->imageData, timestamp)) {
				fprintf(stderr, "Error adding frame %d\n", frameNumber);
				break;
			}

		}

		if (!preview->stop()) {
			fprintf(stderr, "Error waiting for preview thread to unlock\n");
		}

		// clean up
		if (preview != NULL) {
			delete preview;
			preview = NULL;
		}
		if (lock) {
			CloseHandle(lock);
			lock = NULL;
		}
		if (capture != NULL) {
			cvReleaseCapture(&capture);
			capture = NULL;
			frame = NULL;
		}
		if (fmfcapture.fmf_fp_isopen()) {
			fmfcapture.fmf_close();
		}
		if (grayFrame != NULL) {
			cvReleaseImage(&grayFrame);
			grayFrame = NULL;
		}

	}
	if (!writer->stopWrite()) {
		fprintf(stderr, "Error stopping writing\n");
		if (interactiveMode) {
			fprintf(stderr, "Hit enter to exit\n");
			getc(stdin);
		}
		return 1;
	}
	if (writer != NULL) {
		delete writer;
	}
	
	if (interactiveMode) {
		fprintf(stderr, "Hit enter to exit\n");
		getc(stdin);
	}
	return 0;
}


bool ChooseFile(char fileName[], const char dialogTitle[], const COMDLG_FILTERSPEC filterSpec[], int nFilters, DialogType dialogType)
{
	return ChooseFile(fileName, dialogTitle, filterSpec, nFilters, dialogType, NULL);
}

bool ChooseFile(char fileName[], const char dialogTitle[], const COMDLG_FILTERSPEC filterSpec[], int nFilters, DialogType dialogType, char defaultFileName[])
{
	HRESULT hr;

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileDialog *fileDialog = NULL;
		if (dialogType == DialogTypeInput) {
			hr = CoCreateInstance(CLSID_FileOpenDialog,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&fileDialog));
		}
		else {
			hr = CoCreateInstance(CLSID_FileSaveDialog,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_PPV_ARGS(&fileDialog));
		}
		if (SUCCEEDED(hr))
		{
			// Set the options on the dialog.
			DWORD dwFlags;

			// Before setting, always get the options first in order 
			// not to override existing options.
			hr = fileDialog->GetOptions(&dwFlags);
			if (SUCCEEDED(hr))
			{
				// In this case, get shell items only for file system items.
				hr = fileDialog->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
				if (SUCCEEDED(hr))
				{
					// set dialog title
					wchar_t wideTitle[512];
					MultiByteToWideChar(CP_ACP,
						MB_COMPOSITE,
						dialogTitle,
						-1,
						wideTitle,
						512);
					hr = fileDialog->SetTitle(wideTitle);
					if (SUCCEEDED(hr))
					{
						// Set the file types to display only. 
						// Notice that this is a 1-based array.
						hr = fileDialog->SetFileTypes(nFilters, filterSpec);
						if (SUCCEEDED(hr))
						{
							// Set the selected file type index to the first filter
							hr = fileDialog->SetFileTypeIndex(1);
							if (SUCCEEDED(hr))
							{
								// set default file extension
								hr = fileDialog->SetDefaultExtension((*filterSpec).pszSpec);
								if (SUCCEEDED(hr))
								{
									// set default file name
									if (defaultFileName != NULL) {
										wchar_t wideDefaultFileName[512];
										MultiByteToWideChar(CP_ACP,
											MB_COMPOSITE,
											defaultFileName,
											-1,
											wideDefaultFileName,
											512);
										hr = fileDialog->SetFileName(wideDefaultFileName);
									}
									// Show the dialog
									hr = fileDialog->Show(NULL);
									if (SUCCEEDED(hr))
									{
										// Obtain the result once the user clicks 
										// the 'Open' button.
										// The result is an IShellItem object.
										IShellItem *psiResult;
										hr = fileDialog->GetResult(&psiResult);
										if (SUCCEEDED(hr))
										{
											PWSTR pszFilePath = NULL;
											hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

											if (SUCCEEDED(hr))
											{
												WideCharToMultiByte(CP_ACP,
													WC_COMPOSITECHECK,
													pszFilePath,
													-1,
													fileName,
													512,
													NULL,
													NULL);
												CoTaskMemFree(pszFilePath);
											}
											psiResult->Release();
										}
									}
								}
							}
						}
					}
				}
			}
			fileDialog->Release();
		}
		CoUninitialize();
	}

	if (FAILED(hr)) {
		*fileName = 0;
	}

	return SUCCEEDED(hr);
}

bool ChooseMultiFile(std::vector<std::string>* fileName, const char dialogTitle[], const COMDLG_FILTERSPEC filterSpec[], int nFilters, char defaultFileName[])
{
	HRESULT hr;

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr))
	{
		IFileOpenDialog *fileDialog = NULL;
		hr = CoCreateInstance(CLSID_FileOpenDialog,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&fileDialog));
		if (SUCCEEDED(hr))
		{
			// Set the options on the dialog.
			DWORD dwFlags;

			// Before setting, always get the options first in order 
			// not to override existing options.
			hr = fileDialog->GetOptions(&dwFlags);
			if (SUCCEEDED(hr))
			{
				// In this case, get shell items only for file system items.
				//Add multiselect functionality only if opening a file

				hr = fileDialog->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);

				if (SUCCEEDED(hr))
				{
					// set dialog title
					wchar_t wideTitle[512];
					MultiByteToWideChar(CP_ACP,
						MB_COMPOSITE,
						dialogTitle,
						-1,
						wideTitle,
						512);
					hr = fileDialog->SetTitle(wideTitle);
					if (SUCCEEDED(hr))
					{
						// Set the file types to display only. 
						// Notice that this is a 1-based array.
						hr = fileDialog->SetFileTypes(nFilters, filterSpec);
						if (SUCCEEDED(hr))
						{
							// Set the selected file type index to the first filter
							hr = fileDialog->SetFileTypeIndex(1);
							if (SUCCEEDED(hr))
							{
								// set default file extension
								hr = fileDialog->SetDefaultExtension((*filterSpec).pszSpec);
								if (SUCCEEDED(hr))
								{
									// set default file name
									if (defaultFileName != NULL) {
										wchar_t wideDefaultFileName[512];
										MultiByteToWideChar(CP_ACP,
											MB_COMPOSITE,
											defaultFileName,
											-1,
											wideDefaultFileName,
											512);
										hr = fileDialog->SetFileName(wideDefaultFileName);
									}
									// Show the dialog
									hr = fileDialog->Show(NULL);
									if (SUCCEEDED(hr))
									{
										// Obtain the result once the user clicks 
										// the 'Open' button.
										// The result is an IShellItemArray object.
										IShellItemArray *psiResult;
										hr = fileDialog->GetResults(&psiResult);
										if (SUCCEEDED(hr))
										{
											DWORD dwItemCount = 0;
											hr = psiResult->GetCount(&dwItemCount);  //find how many items selected
											if (SUCCEEDED(hr))
											{
												IShellItem *psiResultItem;  //holds the individual item in the array
												PWSTR pszFilePath = NULL;  //holds the actual name
												char path[512];

												for (DWORD dwItem = 0; dwItem < dwItemCount; dwItem++)
												{
													psiResult->GetItemAt(dwItem, &psiResultItem);  //get an item
													hr = psiResultItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
													if (SUCCEEDED(hr))
													{
														////DO thing with string
														WideCharToMultiByte(CP_ACP,
															WC_COMPOSITECHECK,
															pszFilePath,
															-1,
															path,
															512,
															NULL,
															NULL);
														fileName->push_back(path);
													}
												}
												CoTaskMemFree(pszFilePath);
											}
										}
										psiResult->Release();
									}
								}
							}
						}
					}
				}
			}
			fileDialog->Release();
		}
		CoUninitialize();
	}
	return SUCCEEDED(hr);
}
