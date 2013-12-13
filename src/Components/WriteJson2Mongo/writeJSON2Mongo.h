
#include "Component_Aux.hpp"
#include "Component.hpp"
#include "DataStream.hpp"
#include "Property.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <cstdlib>
#include <iostream>
#include "mongo/client/dbclient.h"

namespace Processors {
namespace TestDB {

using namespace cv;

class ThresholdTranslator {
public:
        static int fromStr(const std::string & s)
        {
                if (s == "BINARY")
                        return THRESH_BINARY;
                else if (s == "BINARY_INV")
                        return THRESH_BINARY_INV;
                else if (s == "TRUNC")
                        return THRESH_TRUNC;
                else if (s == "TOZERO")
                        return THRESH_TOZERO;
                else if (s == "TOZERO_INV")
                        return THRESH_TOZERO_INV;
                else
                        return THRESH_BINARY;
        }

        static std::string toStr(int t)
        {
                switch (t)
                {
                        case THRESH_BINARY:
                                return "BINARY";
                        case THRESH_BINARY_INV:
                                return "BINARY_INV";
                        case THRESH_TRUNC:
                                return "TRUNC";
                        case THRESH_TOZERO:
                                return "TOZERO";
                        case THRESH_TOZERO_INV:
                                return "TOZERO_INV";
                        default:
                                return "BINARY";
                }
        }
};

class TestDB_Processor: public Base::Component
{
public:
        /*!
         * Constructor.
         */
	TestDB_Processor(const std::string & name = "");

        /*!
         * Destructor
         */
        virtual ~TestDB_Processor();

        /*!
         * Prepares communication interface.
         */
        virtual void prepareInterface();

protected:

        /*!
         * Connects source to given device.
         */
        bool onInit();

        /*!
         * Disconnect source from device, closes streams, etc.
         */
        bool onFinish();

        /*!
         * Retrieves data from device.
         */
        bool onStep();

        /*!
         * Start component
         */
        bool onStart();

        /*!
         * Stop component
         */
        bool onStop();


        /*!
         * Event handler function.
         */
        void onNewImage();

        /// Event handler.
        Base::EventHandler <TestDB_Processor> h_onNewImage;

        /// Input data stream
        Base::DataStreamIn <cv::Mat> in_img;

        /// Output data stream - processed image
        Base::DataStreamOut <Mat> out_img;

private:
        /// Type of the performed thresholding operation.
        Base::Property<int, ThresholdTranslator> m_type;

        Base::Property<double> m_thresh;
        Base::Property<double> m_maxval;

        void run();
};
}//: namespace TestDB
}//: namespace Processors

REGISTER_COMPONENT("WriteJSON2Mongo", Processors::TestDB::TestDB_Processor)


