bin_PROGRAMS = isagoge

SUBDIRS = Tesseract Groundtruth_Generation/select_random \
Groundtruth_Generation/select_training \
Groundtruth_Generation dlib-18.4 Utils Evaluation Grid \
Detection Segmentation MEDS MEDS_Trainer 

isagoge_CPPFLAGS = -ITesseract/api \
-ITesseract/ccmain -ITesseract/ccstruct \
-ITesseract/ccutil -ITesseract/classify \
-ITesseract/config -ITesseract/contrib \
-ITesseract/cube -ITesseract/cutil \
-ITesseract/dict -ITesseract/doc \
-ITesseract/image -ITesseract/java \
-ITesseract/m4 -ITesseract/neural_networks \
-ITesseract/po -ITesseract/tessdata \
-ITesseract/testing -ITesseract/textord \
-ITesseract/training -ITesseract/viewer \
-ITesseract/wordrec -ITesseract -IEvaluation \
-IGrid \
-IMEDS/ -IUtils/ -Idlib-18.4 -IMEDS_Trainer \
-IDetection -IDetection/BinaryClassifier \
-IDetection/FeatureExtractor \
-IDetection/FeatureExtractor/Implementations \
-IDetection/FeatureExtractor/Implementations/F_Ext1 \
-IDetection/FeatureExtractor/Implementations/F_Ext2 \
-IDetection/FeatureExtractor/Implementations/F_Ext3 \
-IDetection/FeatureExtractor/Implementations/F_Ext4 \
-IDetection/FeatureExtractor/NGrams \
-IDetection/Trainer -IDetection/Sample -ISegmentation \
-IDetection/TessInterface -IDetection/FeatureExtractor/Sentence \
-IDetection/FeatureExtractor/NGrams \
-IDetection/FeatureExtractor/NGrams/NGProfileGen

isagoge_LDADD = /usr/local/lib/liblept.so \
MEDS/libMEDS.la \
Tesseract/api/libtesseract.la \
Grid/libGrid.la \
MEDS_Trainer/libMEDS_Trainer.la \
MEDS/libMEDS.la \
Utils/libUtils.la \
Evaluation/libevaluation.la \
Segmentation/libSegmentation.la \
dlib-18.4/dlib/libdlib.la \
Detection/FeatureExtractor/Implementations/F_Ext1/libF_Ext1.la \
Detection/FeatureExtractor/Implementations/F_Ext2/libF_Ext2.la \
Detection/FeatureExtractor/Implementations/F_Ext3/libF_Ext3.la \
Detection/FeatureExtractor/Implementations/F_Ext4/libF_Ext4.la \
Detection/BinaryClassifier/libBinaryClassifier.la \
Detection/libDetection.la \
Detection/TessInterface/libTessInterface.la

isagoge_SOURCES = IsagogeMain.cpp
