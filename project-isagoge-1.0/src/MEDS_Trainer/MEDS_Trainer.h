/**************************************************************************
 * Project Isagoge: Enhancing the OCR Quality of Printed Scientific Documents
 * File name:   MEDS_Trainer.h
 * Written by:  Jake Bruce, Copyright (C) 2013
 * History:     Created Oct 27, 2013 1:15:36 AM
 * ------------------------------------------------------------------------
 * Description: High level class for initiating Tesseract to run the MEDS
 *              module in training mode for several training pages,
 *              converting their grid information into
 *              feature vectors (based upon which feature extractor is
 *              in use), aggregating the vectors into a single vector,
 *              and then sending that vector into the Detector/TrainerPredictor
 *              so it may be used for training purposes.
 * ------------------------------------------------------------------------
 * This file is part of Project Isagoge.
 *
 * Project Isagoge is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Project Isagoge is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Project Isagoge.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/
#ifndef MEDS_TRAINER_H
#define MEDS_TRAINER_H

#include <Detection.h>
#include <TessInterface.h>
#include <Sample.h>
#include <iostream>
#include <string>
#include <fstream>
using namespace std;

//#define JUST_GET_SAMPLES

//#define DBG_MEDS_TRAINER // comment this out to not display grid
//#define DBG_MEDS_TRAINER_SHOW_TRAINDATA
//#define DBG_DISPLAY

// This class carry out any training needed for both the detector and segmenter
template <typename DetectorType>
class MEDS_Trainer {
 public:
  // The first three arguments are in regards to the detector:
  // 1. train_detector - If true then train the detector even if
  //                     it has already been trained. Otherwise
  //                     only train it if necessary
  // 2. detector_path  - This is the path to the training images to be used. It is required to be
  //                     a subdirectory of test_sets/training/MainTrainingDir/training_sets
  // 3. training_set   - The name of the dataset on which training is carried out. The
  //                     dataset is located in [detector_path]/training_sets/[training_set].
  //                     Included in this directory are all the images used for training.
  // 4. get_new_samples - If true will get new samples during training
  //                      even if samples were previously written to a file,
  //                      otherwise only gets samples if file hasn't already
  //                      been written. If not doing training this argument
  //                      has no effect.
  // 5. api_init_params - Parameters specifying how Tesseract will be initialized for training purposes.
  // 6. ext (optional)  - The extension expected for all document images
  //                     (.png by default)
  MEDS_Trainer(bool always_train_, const string& detector_path, string training_set_,
      bool get_new_samples_, vector<string> api_init_params_,
      const string& ext_=".png") :
        tess_interface(NULL), samples_read(NULL), samples_extracted(NULL),
        detector(NULL) {
    get_new_samples = get_new_samples_;
    training_set = training_set_;
    training_set_ = Basic_Utils::checkTrailingSlash(training_set_);
    top_path = Basic_Utils::checkTrailingSlash(detector_path) + (string)"../../"; // path the MainTrainingDir/
    groundtruth_path = top_path + (string)"groundtruths/" + training_set + (string)".dat";

    // the following is the directory in which the predictor will be
    // contained. the full path to the predictor is determined by
    // the classifier during its initialization during trainDetector()
    // function which must be called in order to fully initialize the detector.
    predictor_path = top_path + (string)"predictor/" + training_set_;
    sample_path = top_path + (string)"samples/" + training_set_;
    training_set_path = Basic_Utils::checkTrailingSlash(detector_path);
    always_train = always_train_;
    api_init_params = api_init_params_;
    ext = ext_;
    const int init_res = api.Init(api_init_params[0].c_str(), api_init_params[1].c_str());
    if(init_res != 0) {
      cout << "ERROR: Tesseract was not initialized properly.\n";
      assert(false);
    }
    char* page_seg_mode = (char*)"tessedit_pageseg_mode";
    if (!api.SetVariable(page_seg_mode, "3")) {
      cout << "ERROR: Could not set tesseract's api corectly during training!\n";
      assert(false);
    }
    // make sure that we're in the right document layout analysis mode
    int psm = 0;
    api.GetIntVariable(page_seg_mode, &psm);
    assert(psm == tesseract::PSM_AUTO);
    // turn on equation detection
    if (!api.SetVariable("textord_equation_detect", "true")) {
      cout << "Could not turn on Tesseract's equation detection during training!\n";
      assert(false);
    }
    // initialize the interface used to grab the custom BlobInfoGrid from
    // tesseract's api
    tesseract::EquationDetectBase* ti = new tesseract::TessInterface();
    tess_interface = (tesseract::TessInterface*)ti;
  }

  ~MEDS_Trainer() {
    if(tess_interface != NULL) {
      delete (TessInterface*)tess_interface;
      tess_interface = NULL;
    }
    api.setEquationDetector(NULL); // avoids seg fault
    samples_read = samples_extracted = NULL; // owned by detector
    detector = NULL; // owned by MEDS class
  }

  // Here is where training of the detection component
  // is carried out. Compile-time polymorphism is used so that
  // different types of detection components can be easily
  // interchanged. If the train_detector argument is true, then
  // training will be carried out even if the type of classifier
  // being used has already been trained using the chosen
  // classifier/extractor/trainer/dataset combination. If it is false, then
  // training will only be carried out if the classifier hasn't been
  // trained on the chosen classifier/extractor/trainer/dataset combination
  // -- If desired training was already carried out then the purpose of this
  //    function is only to read in and initialize the predictor
  void trainDetector() {
    if(detector != NULL) {
      cout << "ERROR: Detector expected to be uninitialized prior to training, however "
           << "was already initialized. Make sure it has been deleted.\n";
      assert(false); // probably lame programming style.. but this at least tells where the line number is so I don't have to hunt for it
    }
    detector = new DetectorType;
    if(tess_interface == NULL) {
      cout << "ERROR: trainDetector() called with a NULL TessInterface module.\n";
      assert(false);
    }
    // make sure the directories are there!
    if(!Basic_Utils::existsDirectory(predictor_path))
      Basic_Utils::exec((string)"mkdir " + predictor_path);
    if(!Basic_Utils::existsDirectory(training_set_path)) {
      cout << "ERROR: " << training_set_path << " is expected "
           << "to exist and contain the images that can be used "
           << "to train the detector if necessary. The directory does not exist!\n";
      assert(false);
    }
    if(Basic_Utils::fileCount((string)training_set_path) == 0) {
      cout << "ERROR: " << training_set_path << " is expected "
           << "to contain training images but is empty!\n";
      assert(false);
    }

    // assumes all n files in the training_ are images
    // named 1.png, 2.png, 3.png, .... n.png
    // do training only if necessary (i.e. if always_train is turned on
    // or it's not turned on and there is no trained predictor available
    // in the predictor_path)
    // this should determine the full predictor path as well as where the samples
    // are stored for the given classifier
    detector->initClassifier(predictor_path, sample_path);
    predictor_path = detector->getPredictorPath(); // change it to the full path
    sample_path = detector->getSamplePath();
    // tell the detector where it can find the groundtruth.dat file
    // so it can determine the label of each sample
    detector->initTrainingPaths(groundtruth_path, training_set_path, ext);
    if(always_train || !Basic_Utils::existsFile(predictor_path)) {
      cout << "predictor at " << predictor_path << " doesn't exist. Training is about to be carried out.\n";
      vector<vector<BLSample*> >* samples = getSamples();
      cout << "finished calling detector's getSamples() method\n";
#ifdef JUST_GET_SAMPLES
      cout << "Finished getting samples. To continue with training, comment out "
           << "JUST_GET_SAMPLES in MEDS_Trainer.h.\n";
      exit(EXIT_SUCCESS);
#endif
      detector->initTraining(samples); // detector owns the samples
      cout << "finished calling inittraining\n";
      if(always_train)
        detector->setAlwaysTrain();
      detector->train_();
      cout << "Finished calling the detector's train_() method\n";
    }
    else {
      cout << "Predictor found at " << predictor_path
           << ". Training was not carried out.\n";
    }
    cout << "Initializing the Detector to use the predictor at the "
         << "following location: " << detector->getPredictorPath() << endl;
    detector->initPrediction(api_init_params);
  }

  // Its much harder to do supervised training on this part. Much of the computations
  // at this stage will either be purely heuristic in nature or unsupervised for now.
  // This is kept as a place holder for the case where supervised learning may be
  // applicable in future work.
  void trainSegmentor() {

  }

  // verifies that the samples extracted are the same as the samples being read
  void sampleReadVerify() {
    cout << "Verifying that the samples read in from a previous run and the samples "
         << "extracted on the current run are the same.\n";
    cout << "-- Reading in the old samples.\n";
    readOldSamples(top_path + (string)"samples");
    cout << "-- Extracting features from training set to create new samples.\n";
    getNewSamples(false);
    cout << "-- Comparing the read samples to the extracted ones.\n";
    assert(samples_extracted->size() == samples_read->size());
    for(int i = 0; i < samples_extracted->size(); ++i) {
      assert(samples_extracted[i].size() == samples_read[i].size());
      for(int j = 0; j < samples_extracted[i].size(); ++j) {
        BLSample* newsample = samples_extracted[i][j];
        BLSample* oldsample = samples_read[i][j];
        assert(*newsample == *oldsample);
      }
    }
    cout << "Success!\n";
    detector->destroySamples(samples_extracted);
    detector->destroySamples(samples_read);
  }

  inline DetectorType* getDetector() {
    return detector;
  }

  inline string getPredictorPath() {
    return predictor_path;
  }

 private:

  // Will call either getNewSamples() or readOldSamples() depending on
  // the get_new_samples flag as well as whether or not samples already exist
  // returns a pointer to the resulting vector of samples (each entry of the
  // vector holds the samples extracted from one image).
  vector<vector<BLSample*> >* getSamples() {
    if(samples_extracted != NULL || samples_read != NULL) {
      cout << "ERROR: samples have already been extracted but not deleted and "
           << "nullified prior to calling getSamples(). Make sure samples are "
           << "deleted prior to calling this function.\n";
      assert(false);
    }
    // get the samples
    string sample_path = detector->getSamplePath();
    if(get_new_samples || !Basic_Utils::existsFile(sample_path))
      return getNewSamples(true);
    else
      return readOldSamples(sample_path);
  }

  // does feature extraction on the detector to get all of the samples,
  // also optionally writes them to a file
  vector<vector<BLSample*> >* getNewSamples(bool write_to_file) {
    samples_extracted = new vector<vector<BLSample*> >;
    // set the tesseract api's equation detector to the one being used
    api.setEquationDetector(tess_interface);
    // count the number of training images in the training_set_path
    int img_num = Basic_Utils::fileCount(training_set_path);

    // the newapi is owned by BlobInfoGrid which is instantiated from findEquationParts
    // of the tess_interface. findEquationParts is called from within the "api"
    // (i.e. the stack-allocated TessBaseAPI instantiated as part of this class).
    // This newapi is initialized during the preparation of the BlobInfoGrid owned
    // by tess_interface. The newapi is also used by the feature extractor.
    // when reset is called on tess_interface, the BlobInfoGrid is deleted and the
    // newapi deleted along with it after each iteration of sample extraction (each
    // iteration corresponds to a single image from which samples are extracted).
    tesseract::TessBaseAPI* newapi = new tesseract::TessBaseAPI;

    // assumes all n files in the training dir are images
    // named 1.png, 2.png, 3.png, .... n.png
    // iterate the images in the dataset, getting the features
    // from each and appending them to the samples vector

    // initialize any feature extraction parameters which need to do
    // precomputations on the entire training set prior to any feature
    // extraction. this may or may not be applicable depending on the
    // feature extraction implementation being used by the detector
    // the tesseract api initialization params are passed in incase the
    // feature extractor initialization requires the api. It is expected
    // that the api is ended if it is used upon completion of this method
    detector->initFeatExtFull(newapi, false, api_init_params);
    for(int i = 1; i <= img_num; ++i) {
      ((TessInterface*)tess_interface)->setTessAPI(newapi);
      string img_name = Basic_Utils::intToString(i) + ext;
      string img_filepath = training_set_path + img_name;
      Pix* curimg = Basic_Utils::leptReadImg(img_filepath);
      api.SetImage(curimg); // SetImage SHOULD deallocate everything from the last page
      // including my MEDS module, the BlobInfoGrid, etc!!!!
      api.AnalyseLayout(); // Run Tesseract's layout analysis
      cout << "ran layout analysis!\n";
      tesseract::BlobInfoGrid* grid = ((TessInterface*)tess_interface)->getGrid();
#ifdef DBG_MEDS_TRAINER
      string winname = "BlobInfoGrid for Image " + Basic_Utils::intToString(i);
      ScrollView* gridviewer = grid->MakeWindow(100, 100, winname.c_str());
      grid->DisplayBoxes(gridviewer);
#endif

      detector->setImage(curimg);
      detector->setAPI(newapi);
      // now to get the features from the grid and append them to the
      // samples vector.
      vector<BLSample*> img_samples = detector->getAllSamples(grid, i);

#ifdef DBG_MEDS_TRAINER
      cout << "Finished grabbing samples.\n";
      M_Utils::waitForInput();
      delete gridviewer;
      gridviewer = NULL;
#endif

#ifdef DBG_MEDS_TRAINER_SHOW_TRAINDATA
      // to debug I'll color all the blobs that are labeled as math
      // red and all the other ones as blue
      Pix* colorimg = Basic_Utils::leptReadImg(img_filepath);
      colorimg = pixConvertTo32(colorimg);
      for(int j = 0; j < img_samples.size(); j++) {
        BLSample* sample = img_samples[j];
        GroundTruthEntry* entry = sample->entry;
        if(sample->label)
          Lept_Utils::fillBoxForeground(colorimg, sample->blobbox, LayoutEval::RED);
        else
          Lept_Utils::fillBoxForeground(colorimg, sample->blobbox, LayoutEval::BLUE);
      }
      string dbgname = "dbg_training_im" + Basic_Utils::intToString(i) + ".png";
      pixWrite(dbgname.c_str(), colorimg, IFF_PNG);
#ifdef DBG_DISPLAY
      pixDisplay(colorimg, 100, 100);
      M_Utils::waitForInput();
#endif
      pixDestroy(&colorimg);
#endif


      // now append the samples found for the current image to the
      // the vector which holds all of them. For now I have this organized
      // as a vector for each image
      samples_extracted->push_back(img_samples);
      pixDestroy(&curimg); // destroy finished image
      // clear the memory used by the current MEDS module and the feature extractor
      ((TessInterface*)tess_interface)->reset();
      newapi = new tesseract::TessBaseAPI;
      detector->reset();
#ifdef DBG_MEDS_TRAINER
      delete gridviewer;
      gridviewer = NULL;
#endif
      cout << "Finished acquiring " << (*samples_extracted)[i-1].size()
           << " samples for image " << i << endl;
    }
    cout << "about to delete newapi\n";
    delete newapi;
    newapi = NULL;
    cout << "done deleting newapi\n";
    cout << "about to write samples to file\n";
    if(write_to_file)
      writeSamples(sample_path);
    cout << "done writing samples to file\n";
    return samples_extracted;
  }

  vector<vector<BLSample*> >* readOldSamples(const string& sample_path) {
    samples_read = new vector<vector<BLSample*> >;
    ifstream s(sample_path.c_str());
    if(!s.is_open()) {
      cout << "ERROR: Couldn't open sample file for reading at " << sample_path << endl;
      assert(false);
    }
    int maxlen = 1500;
    char line[maxlen];
    int nullcount = 0;
    int curimg = 0;
    while(!s.eof()) {
      s.getline(line, maxlen);
      if(s == NULL) {
        if(nullcount++ > 1) {
          cout << "ERROR: Invalid sample file at " << sample_path << endl;
          assert(false);
        }
        continue;
      }
      BLSample* sample = readSample((string)line);
      if(sample->image_index != curimg) {
        assert(sample->image_index == (curimg + 1)); // should start at 1 and go up by 1
        ++curimg;
        vector<BLSample*> imgsample_vec;
        samples_read->push_back(imgsample_vec);
      }
      (*samples_read)[curimg - 1].push_back(sample);
    }
    return samples_read;
  }

  // reads the sample in the following space-delimited format:
  // label featurevec imgindex blobbox groundtruth_entry
  // label: 0 or 1
  // featurevec: comma delimited list of doubles
  // imgindex: gives the image number for the blob
  // blobbox: comma delimited coordinates as follows x,y,w,h
  // groundtruth_entry: info on the matching groundtruth entry if label is 1
  //                    otherwise the following comma delimited list is all 0's
  //                    type,x,y,w,h <- type is E/D/L (embedded displayed or labeled)
  //                     and x,y,w,h is the bounding box of the entry
  // Thus the entire space-delimited format is as follows:
  // label f1,f2,etc., imgindex x1,y1,w1,h1 entrytype x2,y2,w2,h2
  // f is the feature, x1-h1 are the coords for the blob, and x2-h2 are the coords
  // for the groundtruth entry if applicable (otherwise all 0's)
  BLSample* readSample(const string& line) {
    vector<string> spacesplit = Basic_Utils::stringSplit(line, ' ');
    // get the label
    int labelint = atoi(spacesplit[0].c_str());
    assert(labelint == 0 || labelint == 1);
    bool label = (labelint == 1) ? true : false;
    // get the feature vec
    string fvecstring = spacesplit[1];
    vector<string> featstrvec = Basic_Utils::stringSplit(fvecstring, ',');
    vector<double> featurevec;
    assert(featstrvec.size() == detector->numFeatures());
    for(int i = 0; i < featstrvec.size(); i++) {
      double feat = atof(featstrvec[i].c_str());
      featurevec.push_back(feat);
    }
    // get the imgindex
    int imgindex = atoi(spacesplit[2].c_str());
    assert(spacesplit[2].length() < 3); // only expecting about 15 samples for now
    // get blobbox
    string blobboxstr = spacesplit[3];
    vector<string> blobstrvec = Basic_Utils::stringSplit(blobboxstr, ',');
    assert(blobstrvec.size() == 4);
    int x = atoi(blobstrvec[0].c_str());
    int y = atoi(blobstrvec[1].c_str());
    int w = atoi(blobstrvec[2].c_str());
    int h = atoi(blobstrvec[3].c_str());
    BOX* blobbox = boxCreate(x, y, w, h);
    //get the groundtruth type if applicable
    GroundTruthEntry* gtentry;
    string gtentrystr = spacesplit[4];
    vector<string> gtentrystrvec =  Basic_Utils::stringSplit(gtentrystr, ',');
    assert(gtentrystrvec.size() == 5);
    const char* gttype = gtentrystrvec[0].c_str();
    assert(strlen(gttype) == 1);
    char typchar = gttype[0];
    assert(typchar == 'D' || typchar == 'E' || typchar == 'L' || typchar == '0');
    if(typchar == '0')
      gtentry = NULL;
    else if(typchar == 'D' || typchar == 'E' || typchar == 'L') {
      gtentry = new GroundTruthEntry;
      if(typchar == 'D')
        gtentry->entry = GT_Entry::DISPLAYED;
      else if(typchar == 'E')
        gtentry->entry = GT_Entry::EMBEDDED;
      else if(typchar == 'L')
        gtentry->entry = GT_Entry::LABEL;
      else
        error();
      // get the groundtruth entry's box
      int gtx = atoi(gtentrystrvec[1].c_str());
      int gty = atoi(gtentrystrvec[2].c_str());
      int gtw = atoi(gtentrystrvec[3].c_str());
      int gth = atoi(gtentrystrvec[4].c_str());
      BOX* gtrect = boxCreate(gtx, gty, gtw, gth);
      gtentry->rect = gtrect;
      gtentry->image_index = imgindex;
    }
    else
      error();
    // now everything's parsed and loaded, set up the sample
    BLSample* sample = new BLSample;
    sample->label = label;
    sample->features = featurevec;
    sample->image_index = imgindex;
    sample->blobbox = blobbox;
    sample->entry = gtentry;
    return sample;
  }

  inline void error() {
    cout << "ERROR: Invalid sample file!\n";
    assert(false);
  }

  void writeSamples(const string& sample_path) {
    cout << "Writing the samples to " << sample_path << endl;
    int samplecount = 0;
    ofstream s(sample_path.c_str());
    if(!s.is_open()) {
      cout << "ERROR: Couldn't open " << sample_path << " for writing\n";
      assert(false);
    }
    for(int i = 0; i < samples_extracted->size(); i++) {
      for(int j = 0; j < (*samples_extracted)[i].size(); j++) {
        writeSample((*samples_extracted)[i][j], s);
        samplecount++;
      }
    }
    cout << "Total of " << samplecount << " samples written.\n";
  }

  // writes the sample in the following space-delimited format:
  // label featurevec imgindex blobbox groundtruth_entry
  // label: 0 or 1
  // featurevec: comma delimited list of doubles
  // imgindex: gives the image number for the blob
  // blobbox: comma delimited coordinates as follows x,y,w,h
  // groundtruth_entry: info on the matching groundtruth entry if label is 1
  //                    otherwise the following comma delimited list is all 0's
  //                    type,x,y,w,h <- type is E/D/L (embedded displayed or labeled)
  //                     and x,y,w,h is the bounding box of the entry
  // Thus the entire space-delimited format is as follows:
  // label f1,f2,etc., imgindex x1,y1,w1,h1 entrytype x2,y2,w2,h2
  // f is the feature, x1-h1 are the coords for the blob, and x2-h2 are the coords
  // for the groundtruth entry if applicable (otherwise all 0's)
  void writeSample(BLSample* sample, ofstream& fs) {
    int imgindex = sample->image_index;
    if(sample->label)
      fs << 1 << " ";
    else
      fs << 0 << " ";
    int numfeat = (sample->features).size();
    for(int i = 0; i < numfeat; i++) {
      fs << setprecision(20) << sample->features[i];
      fs << (((i + 1) < numfeat) ? "," : " ");
    }
    fs << imgindex << " ";
    BOX* box = sample->blobbox;
    fs << box->x << "," << box->y << "," << box->w << "," << box->h << " ";
    if(sample->entry) {
      GroundTruthEntry* entry = sample->entry;
      GT_Entry::GTEntryType type = entry->entry;
      if(type == GT_Entry::DISPLAYED)
        fs << "D,";
      else if(type == GT_Entry::EMBEDDED)
        fs << "E,";
      else if(type == GT_Entry::LABEL)
        fs << "L,";
      else {
        cout << "ERROR: Unexpected groundtruth entry type\n";
        assert(false);
      }
      BOX* gtbox = entry->rect;
      fs << gtbox->x << "," << gtbox->y << "," << gtbox->w
         << "," << gtbox->h << "\n";
    }
    else
      fs << "0,0,0,0,0\n";
  }

  // Tesseract framework
  tesseract::TessBaseAPI api;

  // This is module essentially the MEDS module without prediction
  // The MEDS module which does prediction needs to be templated so that
  // it can utilize compile-time polymorphism to choose its detector and
  // segmenter (??? will this still work in the Tesseract framework or will I need to
  // make modifications???.. if it needs to be templated then I will template it.....)
  tesseract::EquationDetectBase* tess_interface; // overrides Tesseract's EquationDetectBase class

  // here different trainer_predictors in Detection/Detection.h can be chosen from
  // and experimented with through compile-time polymorphism
  // --owned by MEDS class
  DetectorType* detector; // see Detection.h for details on this class

  // only one of the following is used unless sampleReadVerify is being used to test
  // that the samples extracted are the same as the ones read from the file. if training
  // is carried out, however, only one is used and whichever one is used will be owned
  // by the detector, not by this class.
  vector<vector<BLSample*> >* samples_extracted;
  vector<vector<BLSample*> >* samples_read; // should be the same as samples_extracted
                                            // can be verified by sampleReadVerify

  // some paths and such
  string top_path; // this is the root of the detector directory
  string training_set; // name of the directory which contains the images used for training and also the name of the corresponding groundtruth file
  string predictor_path;  // top_path/predictor
  string sample_path; // top_path/samples
  string training_set_path; // top_path/training_set
  string groundtruth_path; // top_path/[groundtruthname].dat
  string ext; // image extension (i.e. png, jpg, etc.)
  vector<string> api_init_params;
  bool always_train; // if false only train if no trained predictor
                     // is available in top_path/predictor
  bool get_new_samples; // if false reads in samples from a file if they've
                        // already been written, otherwise always gets new samples
};


#endif

