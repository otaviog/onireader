#include <memory>

#include <OpenNI.h>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <iostream>
using namespace std;

namespace py = pybind11;

static bool g_openni_init = false;

namespace {
py::array_t<uint8_t>
convert_rgb_image_to_array(openni::VideoFrameRef *frame_ref) {

  std::vector<size_t> shape;
  std::vector<size_t> stride;

  const size_t width = frame_ref->getWidth();
  const size_t height = frame_ref->getHeight();
  shape = {height, width, 3};
  stride = {width * 3, 3, 1};

  auto format = py::format_descriptor<uint8_t>::format();
  size_t data_size = frame_ref->getDataSize();

  uint8_t *data = new uint8_t[data_size];
  memcpy(data, frame_ref->getData(), data_size);
  return py::array_t<uint8_t>(py::buffer_info(data, sizeof(uint8_t), format,
                                              shape.size(), shape, stride));
}

py::array_t<uint16_t>
convert_depth_image_to_array(openni::VideoFrameRef *frame_ref) {

  std::vector<size_t> shape;
  std::vector<size_t> stride;

  const size_t width = frame_ref->getWidth();
  const size_t height = frame_ref->getHeight();

  shape = {height, width};
  stride = {width * 2, 2};

  auto format = py::format_descriptor<int16_t>::format();
  size_t data_size = frame_ref->getDataSize();

  uint16_t *data = reinterpret_cast<uint16_t *>(new uint8_t[data_size]);
  memcpy(data, frame_ref->getData(), data_size);
  return py::array_t<uint16_t>(py::buffer_info(data, sizeof(uint16_t), format,
                                               shape.size(), shape, stride));
}

} // namespace

class Device {
public:
  Device() {
    if (!g_openni_init) {
      openni::OpenNI::initialize();
      g_openni_init = true;
    }

    _frame_count = 0;
  }

  ~Device() {
    _depth_stream.stop();
    _rgb_stream.stop();
    _device.close();
  }

  bool open(const std::string &str) {
    _device.open(str.c_str());
    _device.setImageRegistrationMode(
        openni::ImageRegistrationMode::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

    _playback_ctrl = _device.getPlaybackControl();

    _depth_stream.create(_device, openni::SENSOR_DEPTH);
    _depth_stream.start();

    _rgb_stream.create(_device, openni::SENSOR_COLOR);
    _rgb_stream.start();

    int max_depth =
        _device.getPlaybackControl()->getNumberOfFrames(_depth_stream);
    int max_rgb = _device.getPlaybackControl()->getNumberOfFrames(_rgb_stream);

    _frame_count = std::max(max_depth, max_rgb);
  }

  py::tuple readDepth() {
    openni::VideoFrameRef frame_ref;

    _depth_stream.readFrame(&frame_ref);
    return py::make_tuple(convert_depth_image_to_array(&frame_ref),
                          frame_ref.getTimestamp(), frame_ref.getFrameIndex());
  }

  py::tuple readColor() {
    openni::VideoFrameRef frame_ref;
    _rgb_stream.readFrame(&frame_ref);

    return py::make_tuple(convert_rgb_image_to_array(&frame_ref),
                          frame_ref.getTimestamp(), frame_ref.getFrameIndex());
  }

  bool seek(int frame_id) {
    openni::Status rc = _playback_ctrl->seek(_rgb_stream, frame_id);

    if (rc != openni::STATUS_OK) {
      return false;
    }

    rc = _playback_ctrl->seek(_rgb_stream, frame_id);
    if (rc != openni::STATUS_OK) {
      return false;
    }
    return true;
  }

  int get_count() const { return _frame_count; }

private:
  openni::PlaybackControl *_playback_ctrl;
  openni::Device _device;
  openni::VideoStream _depth_stream, _rgb_stream;
  int _frame_count;
};

PYBIND11_MODULE(_onireader, m) {
  m.doc() = ".oni reader module";

  py::class_<Device>(m, "Device")
      .def(py::init<>())
      .def("open", &Device::open)
      .def("readDepth", &Device::readDepth)
      .def("readColor", &Device::readColor)
      .def("seek", &Device::seek)
      .def("__len__", &Device::get_count);
}
