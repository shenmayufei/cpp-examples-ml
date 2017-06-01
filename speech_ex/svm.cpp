// Example : Support Vector Machine (SVM) learning
// usage: prog training_data_file testing_data_file

// For use with test / training datasets : speech_ex

// Author : Toby Breckon, toby.breckon@cranfield.ac.uk
// Version : 0.2

// Copyright (c) 2011 School of Engineering, Cranfield University
// License : LGPL - http://www.gnu.org/licenses/lgpl.html


#include <cv.h>       // opencv general include file
#include <ml.h>		  // opencv machine learning include file

using namespace cv; // OpenCV API is in the C++ "cv" namespace

#include <stdio.h>

/******************************************************************************/

// use SVM "grid search" for kernel parameters

#define USE_OPENCV_GRID_SEARCH_AUTOTRAIN 1  // set to 0 to set SVM parameters manually

/******************************************************************************/

#define NUMBER_OF_TRAINING_SAMPLES 6238
#define ATTRIBUTES_PER_SAMPLE 617
#define NUMBER_OF_TESTING_SAMPLES 1559

#define NUMBER_OF_CLASSES 26

// N.B. classes are spoken alphabetric letters A-Z labelled 1 -> 26

/******************************************************************************/

// loads the sample database from file (which is a CSV text file)


int read_data_from_csv(const char* filename, Mat data, Mat classes, int n_samples )
{
    float tmp;

    // if we can't read the input file then return 0
    FILE* f = fopen( filename, "r" );
    if( !f )
    {
        printf("ERROR: cannot read file %s\n",  filename);
        return 0; // all not OK
    }

    // for each sample in the file

    for(int line = 0; line < n_samples; line++)
    {

        // for each attribute on the line in the file

        for(int attribute = 0; attribute < (ATTRIBUTES_PER_SAMPLE + 1); attribute++)
        {
            if (attribute < ATTRIBUTES_PER_SAMPLE)
            {

                // first 617 elements (0-616) in each line are the attributes

                fscanf(f, "%f,", &tmp);
                data.at<float>(line, attribute) = tmp;


            }
            else if (attribute == ATTRIBUTES_PER_SAMPLE)
            {

                // attribute 617 is the class label {1 ... 26} == {A-Z}

                fscanf(f, "%f,", &tmp);
                classes.at<float>(line, 0) = tmp;
            }
        }
    }

    fclose(f);

    return 1; // all OK
}

/******************************************************************************/

int main( int argc, char** argv )
{
    // lets just check the version first

    printf ("OpenCV version %s (%d.%d.%d)\n",
            CV_VERSION,
            CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION);

    // define training data storage matrices (one for attribute examples, one
    // for classifications)

    Mat training_data = Mat(NUMBER_OF_TRAINING_SAMPLES, ATTRIBUTES_PER_SAMPLE, CV_32FC1);
    Mat training_classifications = Mat(NUMBER_OF_TRAINING_SAMPLES, 1, CV_32FC1);

    //define testing data storage matrices

    Mat testing_data = Mat(NUMBER_OF_TESTING_SAMPLES, ATTRIBUTES_PER_SAMPLE, CV_32FC1);
    Mat testing_classifications = Mat(NUMBER_OF_TESTING_SAMPLES, 1, CV_32FC1);

    // load training and testing data sets

    if (read_data_from_csv(argv[1], training_data, training_classifications, NUMBER_OF_TRAINING_SAMPLES) &&
            read_data_from_csv(argv[2], testing_data, testing_classifications, NUMBER_OF_TESTING_SAMPLES))
    {
        // define the parameters for training the SVM (kernel + SVMtype type used for auto-training,
        // other parameters for manual only)

        CvSVMParams params = CvSVMParams(
                                 CvSVM::C_SVC,   // Type of SVM, here N classes (see manual)
                                 CvSVM::LINEAR,  // kernel type (see manual)
                                 0.0,			// kernel parameter (degree) for poly kernel only
                                 0.0,			// kernel parameter (gamma) for poly/rbf kernel only
                                 0.0,			// kernel parameter (coef0) for poly/sigmoid kernel only
                                 10,				// SVM optimization parameter C
                                 0,				// SVM optimization parameter nu (not used for N classe SVM)
                                 0,				// SVM optimization parameter p (not used for N classe SVM)
                                 NULL,			// class wieghts (or priors)
                                 // Optional weights, assigned to particular classes.
                                 // They are multiplied by C and thus affect the misclassification
                                 // penalty for different classes. The larger weight, the larger penalty
                                 // on misclassification of data from the corresponding class.

                                 // termination criteria for learning algorithm

                                 cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 1000, 0.000001)

                             );

        // train SVM classifier (using training data)

        printf( "\nUsing training database: %s\n\n", argv[1]);
        CvSVM* svm = new CvSVM;

        printf( "\nTraining the SVM (in progress) ..... ");
        fflush(NULL);

#if (USE_OPENCV_GRID_SEARCH_AUTOTRAIN)

        printf( "(SVM 'grid search' => may take some time!)");
        fflush(NULL);

        // train using auto training parameter grid search if it is available
        // (i.e. OpenCV 2.x) with 10 fold cross valdiation
        // N.B. this does not search kernel choice

        svm->train_auto(training_data, training_classifications,
                        Mat(), Mat(), params, 10);
        params = svm->get_params();
        printf( "\nUsing optimal parameters degree %f, gamma %f, ceof0 %f\n\t C %f, nu %f, p %f\n Training ..",
                params.degree, params.gamma, params.coef0, params.C, params.nu, params.p);
#else
        // otherwise use regular training and use parameters manually specified above

        svm->train(training_data, training_classifications, Mat(), Mat(), params);

#endif

        printf( ".... Done\n");

        // get the number of support vectors used to define the SVM decision boundary

        printf("Number of support vectors for trained SVM = %i\n", svm->get_support_vector_count());

        // perform classifier testing and report results

        Mat test_sample;
        int correct_class = 0;
        int wrong_class = 0;
        int false_positives [NUMBER_OF_CLASSES];
        char class_labels[NUMBER_OF_CLASSES];
        float result;

        // zero the false positive counters in a simple loop

        for (int i = 0; i < NUMBER_OF_CLASSES; i++)
        {
            false_positives[i] = 0;
            class_labels[i] = (char) 65 + i; // ASCII 65 = A
        }

        printf( "\nUsing testing database: %s\n\n", argv[2]);

        for (int tsample = 0; tsample < NUMBER_OF_TESTING_SAMPLES; tsample++)
        {

            // extract a row from the testing matrix

            test_sample = testing_data.row(tsample);

            // run SVM classifier

            result = svm->predict(test_sample);

            // printf("Testing Sample %i -> class result (character %c)\n", tsample, class_labels[((int) result) - 1]);

            // if the prediction and the (true) testing classification are the same
            // (N.B. openCV uses a floating point decision tree implementation!)

            if (fabs(result - testing_classifications.at<float>(tsample, 0))
                    >= FLT_EPSILON)
            {
                // if they differ more than floating point error => wrong class

                wrong_class++;

                false_positives[(int) (testing_classifications.at<float>(tsample, 0) - 1)]++;

            }
            else
            {

                // otherwise correct

                correct_class++;
            }
        }

        printf( "\nResults on the testing database: %s\n"
                "\tCorrect classification: %d (%g%%)\n"
                "\tWrong classifications: %d (%g%%)\n",
                argv[2],
                correct_class, (double) correct_class*100/NUMBER_OF_TESTING_SAMPLES,
                wrong_class, (double) wrong_class*100/NUMBER_OF_TESTING_SAMPLES);

        for (unsigned char i = 0; i < NUMBER_OF_CLASSES; i++)
        {
            printf( "\tClass (character %c) false postives 	%d (%g%%)\n",class_labels[(int) i],
                    false_positives[(int) i],
                    (double) false_positives[i]*100/NUMBER_OF_TESTING_SAMPLES);
        }

        // all matrix memory free by destructors

        // all OK : main returns 0

        return 0;
    }

    // not OK : main returns -1

    return -1;
}
/******************************************************************************/
