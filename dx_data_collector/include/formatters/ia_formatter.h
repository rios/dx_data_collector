/**
 * @file ia_formatter.h
 * @author Leo Keselman (github.com/rios-ai)
 * @brief Intelligent Agent formatter implementation
 * @version 0.1
 * @date 2024-07-26
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef __IA_FORMATTER_H__
#define __IA_FORMATTER_H__

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>

#include <dx_rios_yaml/yaml.h>
#include <dx_rios_utils/json/nlohmann/json.hpp>

#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgcodecs.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/hwcontext_qsv.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
}

#include "interfaces/data_formatter.h"


namespace rios { namespace data_collector {

/**
 * @brief Encapsulates a formatted data
 * 
 */
class IAData : public FormattedData
{
public:

  DECLARE_SMART_PTR(IAData)

public:

  /**
   * @brief Construct a new IA Data object
   * 
   * @param config The configuration for the formatter
   * @param segment_name The name of the segment
   * @param msg_jsons Map of jsons describing each message
   * @param video_msgs A map of images to be saved as videos keyed by their names 
   * @param params_json The json of parameters for the segment
   */  
  IAData(const rios::cfg& config,
         const std::string& segment_name,
         std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons,  
         std::shared_ptr<std::unordered_map<std::string, std::list<DxRosMsg::Ptr>>> video_msgs,
         std::shared_ptr<nlohmann::json> params_json);

  /**
   * @brief Store formatted data as files at a path
   * 
   * @param path The path to store the files 
   * @return true Successfully stored
   * @return false Failed to store 
   */
  virtual bool asFiles(std::filesystem::path path) override;

private:

  class FormatContext
  {
    public:
      FormatContext() { context_ = NULL; context_ = avformat_alloc_context(); }
      ~FormatContext() { if(context_->pb) avio_close(context_->pb); avformat_free_context(context_); }
      AVFormatContext* operator->() { return context_; }
      AVFormatContext* get() { return context_; }

    private:
      AVFormatContext* context_;
  };

  class DeviceContext
  {
    public:
      DeviceContext() { context_ = NULL; if (av_hwdevice_ctx_create(&context_, AV_HWDEVICE_TYPE_QSV, NULL, NULL, 0) < 0){ context_ = NULL; } }
      ~DeviceContext() { av_buffer_unref(&context_); }
      AVBufferRef* operator->() { return context_; }
      AVBufferRef* get() { return context_; }

    private:
      AVBufferRef* context_;
  };

  class HWFramesRef
  {
    public:
      HWFramesRef(DeviceContext& hw_device_ctx) 
      { 
        context_ = NULL; 
        context_ = av_hwframe_ctx_alloc(hw_device_ctx.get()); 
      }
      ~HWFramesRef() { av_buffer_unref(&context_); }
      AVBufferRef* operator->() { return context_; }
      AVBufferRef* get() { return context_; }

    private:
      AVBufferRef* context_;
  };

  class OutputFormat
  {
    public:
      OutputFormat(std::filesystem::path video_path) { output_format_ = NULL; output_format_ = av_guess_format(NULL, video_path.c_str(), NULL); }
      ~OutputFormat() { }
      const AVOutputFormat* operator->() { return output_format_; }
      const AVOutputFormat* get() { return output_format_; }

    private:
      const AVOutputFormat* output_format_;
  };

  class Stream
  {
    public:
      Stream(FormatContext& format_context) { stream_ = NULL; stream_ = avformat_new_stream(format_context.get(), NULL); }
      ~Stream() { }
      AVStream* operator->() { return stream_; }
      AVStream* get() { return stream_; }

    private:
      AVStream* stream_;
  };

  class VideoFrame
  {
    public:
      VideoFrame() { frame_ = av_frame_alloc(); }
      ~VideoFrame() { av_frame_unref(frame_); av_frame_free(&frame_); }
      AVFrame* operator->() { return frame_; }
      AVFrame* get() { return frame_; }

    private:
      AVFrame* frame_;
  };

  class VideoPacket
  {
    public:
      VideoPacket() { packet_ = av_packet_alloc(); }
      ~VideoPacket() { av_packet_free(&packet_); }
      AVPacket* operator->() { return packet_; }
      AVPacket* get() { return packet_; }

    private:
      AVPacket* packet_;
  };

  class VideoCodec
  {
    public:
      VideoCodec(AVCodecID codec_id) 
      {
        codec_ = avcodec_find_encoder(codec_id); 
        codec_context_ = avcodec_alloc_context3(codec_);
      }
      VideoCodec(const std::string& codec_name)
      {
        codec_ = avcodec_find_encoder_by_name(codec_name.c_str());
        codec_context_ = avcodec_alloc_context3(codec_);
      }
      ~VideoCodec() { avcodec_free_context(&codec_context_); }
      const AVCodec* getCodec() { return codec_; }
      AVCodecContext* getCodecContext() { return codec_context_; }

    private:
      const AVCodec* codec_;
      AVCodecContext* codec_context_;
  };

  class ScaleContext
  {
    public:
      ScaleContext(VideoFrame& input_ros_frame, VideoFrame& video_frame) 
      { 
        scale_context_ = NULL; 
        scale_context_ = sws_getContext(input_ros_frame->width, input_ros_frame->height, (AVPixelFormat)input_ros_frame->format,
                                        video_frame->width, video_frame->height, (AVPixelFormat)video_frame->format,
                                        SWS_BILINEAR, NULL, NULL, NULL); 
      }
      ~ScaleContext() { sws_freeContext(scale_context_); }
      struct SwsContext* operator->() { return scale_context_; }
      struct SwsContext* get() { return scale_context_; }

    private:
      struct SwsContext* scale_context_;
  };

  /**
   * @brief The configuration for the formatter
   */
  const rios::cfg& config_;

  /**
   * @brief The name of the segment
   * 
   */
  std::string segment_name_;

  /**
   * @brief A map of jsons describing the messages by their uuid
   * 
   */
  std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons_;

  /**
   * @brief Map of video messages by their topic
   * 
   */
  std::shared_ptr<std::unordered_map<std::string, std::list<DxRosMsg::Ptr>>> video_msgs_;

  /**
   * @brief The json of parameters for the segment
   * 
   */
  std::shared_ptr<nlohmann::json> params_json_;

  /**
   * @brief Store the videos
   * 
   * @param path The path to store the videos
   */
  bool storeVideos(std::filesystem::path path);

  /**
   * @brief Encode and store a video based on ROS messages
   * 
   * @param video_path The path to store the video
   * @param msg_queue The list of messages to encode
   * @return true Encoding succeeded
   * @return false Encoding failed
   */
  bool encodeVideo(std::filesystem::path video_path, std::list<DxRosMsg::Ptr>& msg_queue);

};

/**
 * @brief Encapsulates a data formatter
 * 
 */
class IAFormatter : public DataFormatter
{
public:

  DECLARE_SMART_PTR(IAFormatter)

public:

  /**
   * @brief Construct a new IA Formatter object
   * 
   * @param ia_formatter_config The configuration for the formatter
   */ 
  IAFormatter(const rios::cfg& ia_formatter_config);

  /**
   * @brief Format the data as desired
   *
   * @param data_queue The queue of messages
   * @param params The parameters coming from ROS
   * @param snapshot_name The name of the snapshot to format
   * @return true Formatting successful
   * @return false Formatting failed
   */
  virtual FormattedData::Ptr formatData(
    std::deque<DxRosMsg::Ptr> & data_queue,
    std::unordered_map<std::string, std::shared_ptr<std::string>> params, std::string snapshot_name,
    const std::shared_ptr<const rios::data_collector::IngestorMap> topic_ingestors) override;

private:

  /**
   * @brief The configuration for the formatter
   */
  const rios::cfg& config_;

  /**
   * @brief Store a json value from FieldData
   * 
   * @tparam DataType The type of the data
   * @param json The json to store to
   * @param field_data The data
   * @return true Data was this type and was stored
   * @return false Data was not this type and was not stored
   */
  template <typename DataType>
  bool storeFromVariant(nlohmann::json* json, const FieldData& field_data)
  {
    if (std::holds_alternative<DataType>(field_data))
    {
      *json = std::get<DataType>(field_data);
      return true;
    }

    return false;
  }

};


} /* data_collector */ } /* rios */


#endif /* __IA_FORMATTER_H__ */
