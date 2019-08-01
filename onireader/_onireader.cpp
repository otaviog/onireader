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

class VideoMode {
public:
  VideoMode(int width = -1, int height = -1, float fps = 0.0f,
            openni::PixelFormat format = openni::PIXEL_FORMAT_RGB888)
      : width(width), height(height), fps(fps), format(format) {}
  VideoMode(openni::VideoMode vmode)
      : width(vmode.getResolutionX()), height(vmode.getResolutionY()),
        fps(vmode.getFps()), format(vmode.getPixelFormat()) {}

  int width, height;
  float fps;
  openni::PixelFormat format;
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

  ~Device() { Close(); }

  bool Open(const string &uri) {
    if (uri.empty()) {
      if (device_.open(openni::ANY_DEVICE) != openni::Status::STATUS_OK)
        return false;

    } else {
      if (device_.open(uri.c_str()) != openni::Status::STATUS_OK)
        return false;
    }

    return true;
  }

  py::list get_video_modes(openni::SensorType sensor) {
    const openni::SensorInfo *info = device_.getSensorInfo(sensor);
    const openni::Array<openni::VideoMode> &modes =
        info->getSupportedVideoModes();
    py::list result;

    for (int i = 0; i < modes.getSize(); i++) {
      const auto mode = modes[i];
      result.append(VideoMode(mode));
    }

    return result;
  }

  py::list get_depth_video_modes() {
    return get_video_modes(openni::SENSOR_DEPTH);
  }

  py::list get_color_video_modes() {
    return get_video_modes(openni::SENSOR_COLOR);
  }

  void Start(int depth_mode, int color_mode) {
    playback_ctrl_ = device_.getPlaybackControl();
    if (playback_ctrl_)
      playback_ctrl_->setSpeed(-1);

    depth_stream_.create(device_, openni::SENSOR_DEPTH);
    color_stream_.create(device_, openni::SENSOR_COLOR);

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
    if (color_mode >= 0) {
      const openni::SensorInfo *info =
          device_.getSensorInfo(openni::SENSOR_COLOR);
      const openni::Array<openni::VideoMode> &modes =
          info->getSupportedVideoModes();
      color_stream_.setVideoMode(modes[color_mode]);
    }

    color_stream_.start();
    if (playback_ctrl_) {
      int max_depth = playback_ctrl_->getNumberOfFrames(depth_stream_);
      int max_color =
          device_.getPlaybackControl()->getNumberOfFrames(color_stream_);

      _frame_count = max(max_depth, max_color);
    } else {
      _frame_count = -1;
    }
  }

  void Close() {
    depth_stream_.stop();
    color_stream_.stop();
    device_.close();
  }

  py::tuple ReadDepth() {
    openni::VideoFrameRef frame_ref;
    depth_stream_.readFrame(&frame_ref);

    if (!frame_ref.isValid()) {
      return py::make_tuple(nullptr, -1, -1);
    }

    return py::make_tuple(convert_image_to_array<uint16_t>(&frame_ref, false),
                          frame_ref.getTimestamp(), frame_ref.getFrameIndex());
  }

  float get_horizontal_fov() const {
    return depth_stream_.getHorizontalFieldOfView();
  }

  float get_vertical_fov() const {
    return depth_stream_.getVerticalFieldOfView();
  }

  VideoMode get_depth_video_mode() {
    return VideoMode(depth_stream_.getVideoMode());
  }

  VideoMode get_color_video_mode() {
    return VideoMode(color_stream_.getVideoMode());
  }

  py::tuple ReadColor() {
    openni::VideoFrameRef frame_ref;
    color_stream_.readFrame(&frame_ref);

    if (!frame_ref.isValid()) {
      return py::make_tuple(nullptr, -1, -1);
    }
    return py::make_tuple(convert_image_to_array<uint8_t>(&frame_ref, true),
                          frame_ref.getTimestamp(), frame_ref.getFrameIndex());
  }

  bool Seek(int frame_id) {
    openni::Status rc = playback_ctrl_->seek(color_stream_, frame_id);

    if (rc != openni::STATUS_OK) {
      return false;
    }

    rc = playback_ctrl_->seek(color_stream_, frame_id);
    if (rc != openni::STATUS_OK) {
      return false;
    }
    return true;
  }

  int get_count() const { return _frame_count; }

  int get_max_depth_value() const { return depth_stream_.getMaxPixelValue(); }

private:
  openni::PlaybackControl *playback_ctrl_;
  openni::Device device_;
  openni::VideoStream depth_stream_, color_stream_;
  int _frame_count;
};

PYBIND11_MODULE(_onireader, m) {
  m.doc() = ".oni reader module";

  m.attr("ANY_DEVICE") = py::str("");
  py::enum_<openni::PixelFormat>(m, "PixelFormat")
      .value("DEPTH_1_MM", openni::PIXEL_FORMAT_DEPTH_1_MM)
      .value("DEPTH_100_UM", openni::PIXEL_FORMAT_DEPTH_100_UM)
      .value("SHIFT_9_2", openni::PIXEL_FORMAT_SHIFT_9_2)
      .value("SHIFT_9_3", openni::PIXEL_FORMAT_SHIFT_9_3)
      .value("RGB888", openni::PIXEL_FORMAT_RGB888)
      .value("YUV422", openni::PIXEL_FORMAT_YUV422)
      .value("GRAY8", openni::PIXEL_FORMAT_GRAY8)
      .value("GRAY16", openni::PIXEL_FORMAT_GRAY16)
      .value("JPEG", openni::PIXEL_FORMAT_JPEG)
      .value("YUYV", openni::PIXEL_FORMAT_YUYV);

  py::class_<VideoMode>(m, "VideoMode")
      .def_readonly("width", &VideoMode::width)
      .def_readonly("height", &VideoMode::height)
      .def_readonly("fps", &VideoMode::fps)
      .def_readonly("format", &VideoMode::format);

  py::class_<Device>(m, "Device")
      .def(py::init<>())
      .def("open", &Device::Open)
      .def("start", &Device::Start)
      .def("get_depth_video_modes", &Device::get_depth_video_modes)
      .def("get_color_video_modes", &Device::get_color_video_modes)
      .def("read_depth", &Device::ReadDepth)
      .def("read_color", &Device::ReadColor)
      .def("get_depth_video_mode", &Device::get_depth_video_mode)
      .def("get_color_video_mode", &Device::get_color_video_mode)
      .def("get_horizontal_fov", &Device::get_horizontal_fov)
      .def("get_max_depth_value", &Device::get_max_depth_value)
      .def("get_vertical_fov", &Device::get_vertical_fov)
      .def("seek", &Device::Seek)
      .def("__len__", &Device::get_count);
}
