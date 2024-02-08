#include <dx_rios_utils/fmt.h>

#include <dx_rios_pybind/ros.hpp>
#include <dx_rios_pybind/yaml.hpp>

#include "dx_data_collector_commander/data_collector_commander.h"


namespace py = pybind11;


namespace rios { namespace data_collection {


PYBIND11_MODULE(data_collector_commander, module) {
    
    /* commander (not intended for use) */
    
    py::class_ <DataCollectorCommander, DataCollectorCommander::Ptr> (module, "DataCollectorCommander")

        .def(py::init (

            [](const std::string & collector_id)
            {
                /* nicely start an anonymous cpp node if the user does not  */

                if (not rios::pybind::is_roscpp_initialized())
                {
                    if (not rios::pybind::init_roscpp_anonymous_node())
                    {
                        throw std::runtime_error("ROS node failed to start (roscpp)");
                    }
                }

                return std::make_shared <DataCollectorCommander> (collector_id);
            }

        ), "construct a new data collector commander", py::arg("collector_id") = "dx_data_collector")

        .def("take_snapshot", 
             &DataCollectorCommander::takeSnapshot, "Take a snapshot of the data buffer", 
             py::arg("time_s") = py::none(), py::arg("snapshot_name") = py::none())

        .def("start_recording", 
             &DataCollectorCommander::startRecording, "Start recording data")

        .def("stop_recording",
             &DataCollectorCommander::stopRecording, "Stop recording and take a snapshot of the data",
             py::arg("snapshot_name") = py::none())

    ;

}

} /* ns data_collection */ } /* ns rios */
