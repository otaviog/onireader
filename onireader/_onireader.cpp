#include <memory>

#include <OpenNI.h>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

using namespace std;

namespace py = pybind11;

static bool g_openni_init = false;

namespace {

template <typename img_type>
py::array_t<img_type> convert_image_to_array(openni::VideoFrameRef *frame_ref,
                                             bool is_rgb) {
  vector<size_t> shape;
  vector<size_t> stride;

  const size_t width = frame_ref->getWidth();
  const size_t height = frame_ref->getHeight();

  if (is_rgb) {
    shape = {height, width, 3};
    stride = {width * 3, 3, 1};
  } else {
    shape = {height, width};
    stride = {width * 2, 2};
  }

  size_t data_size = frame_ref->getDataSize();

  uint8_t *data = new uint8_t[data_size];
  memcpy(data, frame_ref->getData(), data_size);

  py::capsule free_data(data, [](void *data) { delete[](uint8_t *) data; });

  return py::array_t<img_type>(shape, stride,
                               reinterpret_cast<img_type *>(data), free_data);
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

  ~Device() { close(); }

  bool open(const string &uri) {
    if (uri.empty()) {
      _device.open(openni::ANY_DEVICE);
    } else {
      _device.open(uri.c_str());
    }

    _playback_ctrl = _device.getPlaybackControl();
    if (_playback_ctrl)
      _playback_ctrl->setSpeed(-1);

    _depth_stream.create(_device, openni::SENSOR_DEPTH);

    _rgb_stream.create(_device, openni::SENSOR_COLOR);

    _device.setImageRegistrationMode(
        openni::ImageRegistrationMode::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

    _depth_stream.start();
    _rgb_stream.start();
    if (_playback_ctrl) {
      int max_depth = _playback_ctrl->getNumberOfFrames(_depth_stream);
      int max_rgb =
          _device.getPlaybackControl()->getNumberOfFrames(_rgb_stream);

      _frame_count = max(max_depth, max_rgb);
    } else {
      _frame_count = -1;
    }
  }

  void close() {
    _depth_stream.stop();
    _rgb_stream.stop();
    _device.close();
  }

  py::tuple readDepth() {
    openni::VideoFrameRef frame_ref;
    _depth_stream.readFrame(&frame_ref);

    if (!frame_ref.isValid()) {
      return py::make_tuple(nullptr, -1, -1);
    }

    return py::make_tuple(convert_image_to_array<uint16_t>(&frame_ref, false),
                          frame_ref.getTimestamp(), frame_ref.getFrameIndex());
  }

  py::tuple readColor() {
    openni::VideoFrameRef frame_ref;
    _rgb_stream.readFrame(&frame_ref);

    if (!frame_ref.isValid()) {
      return py::make_tuple(nullptr, -1, -1);
    }
    return py::make_tuple(convert_image_to_array<uint8_t>(&frame_ref, true),
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

  m.attr("ANY_DEVICE") = py::str("");
  py::class_<Device>(m, "Device")
      .def(py::init<>())
      .def("open", &Device::open)
      .def("readDepth", &Device::readDepth)
      .def("readColor", &Device::readColor)
      .def("seek", &Device::seek)
      .def("__len__", &Device::get_count);
}
