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

class SensorInfo {
public:
  SensorInfo(int width = -1, int height = -1, float fps = 0.0f)
      : width(width), height(height), fps(fps) {}
  int width, height;
  float fps;
};
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
      if (device_.open(openni::ANY_DEVICE) != openni::Status::STATUS_OK)
        return false;

    } else {
      if (device_.open(uri.c_str()) != openni::Status::STATUS_OK)
        return false;
    }

    return true;
  }

  py::list get_sensor_infos(openni::SensorType sensor) {
    const openni::SensorInfo *info = device_.getSensorInfo(sensor);
    const openni::Array<openni::VideoMode> &modes =
        info->getSupportedVideoModes();
    py::list result;
    for (int i = 0; i < modes.getSize(); i++) {
      const auto mode = modes[i];
      result.append(SensorInfo(mode.getResolutionX(), mode.getResolutionY(),
                               mode.getFps()));
    }

    return result;
  }

  py::list get_depth_sensor_infos() {
    return get_sensor_infos(openni::SENSOR_DEPTH);
  }

  py::list get_rgb_sensor_infos() {
    return get_sensor_infos(openni::SENSOR_COLOR);
  }

  void start(int depth_mode, int rgb_mode) {
    playback_ctrl_ = device_.getPlaybackControl();
    if (playback_ctrl_)
      playback_ctrl_->setSpeed(-1);

    depth_stream_.create(device_, openni::SENSOR_DEPTH);
    rgb_stream_.create(device_, openni::SENSOR_COLOR);

    device_.setImageRegistrationMode(
        openni::ImageRegistrationMode::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

    if (depth_mode >= 0) {
      const openni::SensorInfo *info =
          device_.getSensorInfo(openni::SENSOR_DEPTH);
      const openni::Array<openni::VideoMode> &modes =
          info->getSupportedVideoModes();

      depth_stream_.setVideoMode(modes[depth_mode]);
    }

    depth_stream_.start();

    if (rgb_mode >= 0) {
      const openni::SensorInfo *info =
          device_.getSensorInfo(openni::SENSOR_COLOR);
      const openni::Array<openni::VideoMode> &modes =
          info->getSupportedVideoModes();
      rgb_stream_.setVideoMode(modes[rgb_mode]);
    }

    rgb_stream_.start();
    if (playback_ctrl_) {
      int max_depth = playback_ctrl_->getNumberOfFrames(depth_stream_);
      int max_rgb =
          device_.getPlaybackControl()->getNumberOfFrames(rgb_stream_);

      _frame_count = max(max_depth, max_rgb);
    } else {
      _frame_count = -1;
    }
  }

  void close() {
    depth_stream_.stop();
    rgb_stream_.stop();
    device_.close();
  }

  py::tuple readDepth() {
    openni::VideoFrameRef frame_ref;
    depth_stream_.readFrame(&frame_ref);

    if (!frame_ref.isValid()) {
      return py::make_tuple(nullptr, -1, -1);
    }

    return py::make_tuple(convert_image_to_array<uint16_t>(&frame_ref, false),
                          frame_ref.getTimestamp(), frame_ref.getFrameIndex());
  }

  py::tuple readColor() {
    openni::VideoFrameRef frame_ref;
    rgb_stream_.readFrame(&frame_ref);

    if (!frame_ref.isValid()) {
      return py::make_tuple(nullptr, -1, -1);
    }
    return py::make_tuple(convert_image_to_array<uint8_t>(&frame_ref, true),
                          frame_ref.getTimestamp(), frame_ref.getFrameIndex());
  }

  bool seek(int frame_id) {
    openni::Status rc = playback_ctrl_->seek(rgb_stream_, frame_id);

    if (rc != openni::STATUS_OK) {
      return false;
    }

    rc = playback_ctrl_->seek(rgb_stream_, frame_id);
    if (rc != openni::STATUS_OK) {
      return false;
    }
    return true;
  }

  int get_count() const { return _frame_count; }

private:
  openni::PlaybackControl *playback_ctrl_;
  openni::Device device_;
  openni::VideoStream depth_stream_, rgb_stream_;
  int _frame_count;
};

PYBIND11_MODULE(_onireader, m) {
  m.doc() = ".oni reader module";

  m.attr("ANY_DEVICE") = py::str("");
  py::class_<SensorInfo>(m, "SensorInfo")
      .def_readonly("width", &SensorInfo::width)
      .def_readonly("height", &SensorInfo::height)
      .def_readonly("fps", &SensorInfo::fps);

  py::class_<Device>(m, "Device")
      .def(py::init<>())
      .def("open", &Device::open)
      .def("start", &Device::start)
      .def("get_depth_sensor_infos", &Device::get_depth_sensor_infos)
      .def("get_rgb_sensor_infos", &Device::get_rgb_sensor_infos)
      .def("readDepth", &Device::readDepth)
      .def("readColor", &Device::readColor)
      .def("seek", &Device::seek)
      .def("__len__", &Device::get_count);
}
