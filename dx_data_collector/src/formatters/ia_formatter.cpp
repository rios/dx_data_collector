#include "formatters/ia_formatter.h"

rios::data_collector::IAFormatter::IAFormatter(const rios::cfg& ia_formatter_config)
: config_(ia_formatter_config)
{

}

rios::data_collector::FormattedData::Ptr rios::data_collector::IAFormatter::formatData(
  std::deque<DxRosMsg::Ptr> & data_queue,
  std::unordered_map<std::string, std::shared_ptr<std::string>> params, std::string snapshot_name,
  const std::shared_ptr<const rios::data_collector::IngestorMap> topic_ingestors)
{
  if (data_queue.empty())
  {
    ROS_ERROR_STREAM("No data to format");
    return nullptr;
  }

  std::unordered_map<std::string, std::list<DxRosMsg::Ptr>> messages_by_topic;

  // For each image topic we want to generate a video, for each other topic we want to generate a json file
  std::shared_ptr<std::unordered_map<std::string, std::list<DxRosMsg::Ptr>>> video_msgs_by_topic = std::make_shared<std::unordered_map<std::string, std::list<DxRosMsg::Ptr>>>();

  // Iterate through the messages in the data queue and organize the messages by topic
  for (auto & msg : data_queue)
  {
    if (msg->msgType() == "sensor_msgs/Image" || msg->msgType() == "sensor_msgs/CompressedImage")
    {
      if (!video_msgs_by_topic->count(msg->topicName()))
      {
        video_msgs_by_topic->operator[](msg->topicName()) = std::list<DxRosMsg::Ptr>();
      }

      video_msgs_by_topic->operator[](msg->topicName()).push_back(msg);
    }
    else
    {
      if (!messages_by_topic.count(msg->topicName()))
      {
        messages_by_topic[msg->topicName()] = std::list<DxRosMsg::Ptr>();
      }

      messages_by_topic[msg->topicName()].push_back(msg);
    }
  }

  // Iterate through normal messages and make a json for each one
  std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons = std::make_shared<std::unordered_map<std::string, nlohmann::json>>();
  for (auto & [topic_name, msg_list] : messages_by_topic)
  {
    msg_jsons->operator[](topic_name) = nlohmann::json();
    nlohmann::json& topic_obj = msg_jsons->operator[](topic_name);

    topic_obj["topic"] = topic_name;
    topic_obj["type"] = msg_list.front()->msgType() + "/" + msg_list.front()->msgHash();

    // Make the messages field as a list
    topic_obj["messages"] = nlohmann::json::array();

    for (const auto & msg : msg_list)
    {
      // Append an empty object to the messages list
      topic_obj["messages"].push_back(nlohmann::json());

      for (const auto &[field_name, field_data] : msg->parsedMsgData())
      {
        // Get a reference to the last object in the messages list
        nlohmann::json* cur_obj = &topic_obj["messages"].back();
        std::vector<std::string> field_name_as_tokens = rios::data_collector::fieldNameAsTokens(field_name);
        for (auto & token : field_name_as_tokens)
        {
          if (!cur_obj->contains(token))
          {
            // We need to create this level of the json
            cur_obj->operator[](token) = nlohmann::json();
          }
          else
          {
            // We need to get the value of this level of the json 
          }
          cur_obj = &(cur_obj->operator[](token));
        }

        // Now we set the final value

        bool stored = false;
        // Check for special data types
        if (std::holds_alternative<ros::Time>(field_data))
        {
          ros::Time time = std::get<ros::Time>(field_data);
          cur_obj->operator[]("secs") = time.sec;
          cur_obj->operator[]("nsecs") = time.nsec;
          stored = true;    
        }
        else if (std::holds_alternative<ros::Duration>(field_data))
        {
          ros::Duration duration = std::get<ros::Duration>(field_data);
          cur_obj->operator[]("secs") = duration.sec;
          cur_obj->operator[]("nsecs") = duration.nsec;
          stored = true;
        }
        else
        {
          stored = storeFromVariant<std::string>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<double>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<float>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<bool>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<char>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<uint8_t>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<uint16_t>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<uint32_t>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<uint64_t>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<int8_t>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<int16_t>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<int32_t>(cur_obj, field_data);
          if (!stored) stored = storeFromVariant<int64_t>(cur_obj, field_data);
        }

        if (!stored)
        {
          ROS_ERROR_STREAM("Could not store data for field " << field_name);
        } 
      }
    }
  }

  // Make a json file to store all params for this segment
  std::shared_ptr<nlohmann::json> params_json = std::make_shared<nlohmann::json>();

  // If there are no params, make an empty json object
  if (params.empty())
  {
    *params_json = nlohmann::json::object();
  }

  // Iterate through params to fill out the json
  for (auto & [param_name, param_value] : params)
  {
    params_json->operator[](param_name) = *param_value;
  }

  // Create a name for the segment based on start time and length
  ros::Time start_time = data_queue.front()->timeRecvd();
  ros::Duration seg_time = data_queue.back()->timeRecvd() - start_time;

  // Create a time zone for Pacific Time (PST/PDT)
  boost::local_time::time_zone_ptr pst_tz(new boost::local_time::posix_time_zone("PST-08PDT,M3.2.0,M11.1.0"));

  // Convert the UTC time to local time in the PST time zone
  boost::local_time::local_date_time start_pst_time(start_time.toBoost(), pst_tz);

  std::stringstream start_time_stream = std::stringstream();
  start_time_stream << std::setfill('0'); // Set the fill character to '0'

  start_time_stream << std::setw(4) << start_pst_time.local_time().date().year() << "_"; // Pad the year with leading 0s
  start_time_stream << std::setw(2) << start_pst_time.local_time().date().month().as_number() << "_"; // Pad the month with leading 0s
  start_time_stream << std::setw(2) << start_pst_time.local_time().date().day().as_number() << "_T_"; // Pad the day with leading 0s
  start_time_stream << std::setw(2) << start_pst_time.local_time().time_of_day().hours() << "_"; // Pad the hours with leading 0s
  start_time_stream << std::setw(2) << start_pst_time.local_time().time_of_day().minutes() << "_"; // Pad the minutes with leading 0s
  start_time_stream << std::setw(2) << start_pst_time.local_time().time_of_day().seconds() << "PST_D_"; // Pad the seconds with leading 0s
  start_time_stream << std::setw(2) << static_cast<int>(seg_time.toSec()) << "s"; // Pad the segment time with leading 0s

  return std::make_shared<IAData>(config_, start_time_stream.str(), msg_jsons, video_msgs_by_topic, params_json);
}

rios::data_collector::IAData::IAData(const rios::cfg& config,
                                     const std::string& segment_name,
                                     std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons,  
                                     std::shared_ptr<std::unordered_map<std::string, std::list<DxRosMsg::Ptr>>> video_msgs,
                                     std::shared_ptr<nlohmann::json> params_json)
: config_(config)
, msg_jsons_(msg_jsons)
, video_msgs_(video_msgs)
, segment_name_(segment_name)
, params_json_(params_json)
{

}

bool rios::data_collector::IAData::asFiles(std::filesystem::path path)
{
  // Create folder structure as per original standard
  std::filesystem::path current_path = path / segment_name_;
  if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
  {
    ROS_ERROR_STREAM("IA formatter could not create folder " << current_path.string());
    return false;
  }

  if (!storeVideos(current_path))
  {
    return false;
  }

  // Iterate through the jsons and dump them to file as well
  for (auto &[topic, msg_json] : *msg_jsons_)
  {
    std::string topic_name = topic;
    std::replace(topic_name.begin(), topic_name.end(), '/', '_');

    std::filesystem::path json_folder = current_path / topic_name;
    std::filesystem::create_directory(json_folder);
    std::filesystem::path filename = json_folder / "messages.json";
    std::ofstream file(filename);
    if (!file.is_open())
    {
      ROS_ERROR_STREAM("Could not open file " << filename.string());
      return false;
    }

    file << msg_json.dump();
    file.close();
  }

  // Dump the params json to file
  std::filesystem::path params_folder = current_path / "params";
  std::filesystem::create_directory(params_folder);
  std::filesystem::path params_filename = params_folder / "params.json";
  std::ofstream params_file(params_filename);
  if (!params_file.is_open())
  {
    ROS_ERROR_STREAM("Could not open file " << params_filename.string());
    return false;
  }

  params_file << params_json_->dump();
  params_file.close();

  return true;
}

bool rios::data_collector::IAData::storeVideos(std::filesystem::path path)
{
  std::vector<std::thread> encoding_threads;

  // Iterate through each video topics and encode a video
  for (auto & [topic, msg_queue] : *video_msgs_)
  {
    // Change slashes to underscores in the topic name
    std::string topic_name = topic;
    std::replace(topic_name.begin(), topic_name.end(), '/', '_');

    // Create the video folder
    std::filesystem::path video_path = path / topic_name;
    std::filesystem::create_directory(video_path);

    // Start a thread for the encoding
    encoding_threads.push_back(std::thread(
      [this, &msg_queue, video_path]()
      {
        try
        {
          bool success = encodeVideo(video_path, msg_queue);
          if (!success)
          {
            ROS_ERROR_STREAM("Could not encode video for " << video_path);
          }
          else
          {
            ROS_INFO_STREAM("Encoded video for " << video_path);
          }
        }
        catch(const std::exception& e)
        {
          ROS_ERROR_STREAM("Could not encode video for " << video_path << ": " << e.what());
        }
        
        
      }));
  }

  // Wait for all threads to finish
  for (auto & thread : encoding_threads)
  {
    thread.join();
  }

  ROS_INFO_STREAM("All videos encoded");

  return true;
}

bool rios::data_collector::IAData::encodeVideo(std::filesystem::path video_path, std::list<DxRosMsg::Ptr>& msg_queue)
{
  ROS_INFO_STREAM("Encoding video to " << video_path);

  std::filesystem::path video_file = video_path / "video.mp4";
  std::filesystem::path metadata_file = video_path / "metadata.json";

  nlohmann::json metadata;

  int width;
  int height;

  // Based on the type of message, determine the width and height in pixels
  if (msg_queue.front()->msgType() == "sensor_msgs/CompressedImage")
  {
    // Need to decompress the image first

    // create Image msg to pass to cv_bridge
    sensor_msgs::CompressedImagePtr img_msg(new sensor_msgs::CompressedImage());
    nonstd::span<const uint8_t> binary_span = std::get<nonstd::span<const uint8_t>>(msg_queue.front()->parsedMsgData().at("/data/0"));
    img_msg->data = std::vector<unsigned char>((unsigned char *)binary_span.data(), (unsigned char *)(binary_span.data() + binary_span.size()));
    img_msg->format = std::get<std::string>(msg_queue.front()->parsedMsgData().at("/format"));
    img_msg->header.frame_id = std::get<std::string>(msg_queue.front()->parsedMsgData().at("/header/frame_id"));

    // Create the CV image
    cv_bridge::CvImageConstPtr cv_image = cv_bridge::toCvCopy(img_msg, "rgb8");

    width = cv_image->image.cols;
    height = cv_image->image.rows;
  }
  else if (msg_queue.front()->msgType() == "sensor_msgs/Image")
  {
    try
    {
      width = std::get<uint32_t>(msg_queue.front()->parsedMsgData().at("/width"));
      height = std::get<uint32_t>(msg_queue.front()->parsedMsgData().at("/height"));
    }
    catch(const std::exception& e)
    {
      ROS_ERROR_STREAM("Could not get width and height from image message");
      return false;
    }
  }
  else
  {
    ROS_ERROR_STREAM("Unsupported ROS message type for video " << msg_queue.front()->msgType());
    return false;
  }

  // Create the Format Context
  FormatContext format_context;

  if (!format_context.get()) 
  {
    ROS_ERROR_STREAM("Could not allocate format context");
    return false;
  }

  // Set the output format
  OutputFormat output_format(video_file);
  if (!output_format.get()) 
  {
    ROS_ERROR_STREAM("Could not guess output format");
    return false;
  }
  format_context->oformat = output_format.get();

  // Open the output file
  if (!(output_format->flags & AVFMT_NOFILE)) 
  {
      int ret = avio_open(&format_context->pb, video_file.c_str(), AVIO_FLAG_WRITE);
      if (ret < 0) 
      {
          ROS_ERROR_STREAM("Could not open output file");
          return false;
      }
  }

  // Create a new video stream in the AVFormatContext
  Stream video_stream(format_context);
  if (!video_stream.get()) 
  {
    ROS_ERROR_STREAM("Could not create video stream");
    return false;
  }

  // Set the time base for the video stream
  int fps = 60;
  if (config_["out_fps"])
  {
    fps = config_["out_fps"].as<int>();
  }
  else
  {
    ROS_WARN_STREAM("No output fps specified, defaulting to 60");
  }

  video_stream->time_base = (AVRational){1, fps};
  video_stream->avg_frame_rate = (AVRational){video_stream->time_base.den, video_stream->time_base.num};

  // Find and set up the codec
  std::string codec_name = "libx264";
  if (config_["codec"]) codec_name = config_["codec"].as<std::string>();
  else ROS_WARN_STREAM("No codec specified, defaulting to libx264");
  VideoCodec video_codec(codec_name);

  // Find the desired pixel format
  std::string pixel_format_name = "nv12";
  if (config_["pixel_format"]) pixel_format_name = config_["pixel_format"].as<std::string>();
  else ROS_WARN_STREAM("No pixel format specified, defaulting to nv12");

  AVPixelFormat sw_pix_fmt = av_get_pix_fmt(pixel_format_name.c_str());

  if (sw_pix_fmt == AV_PIX_FMT_NONE)
  {
    ROS_ERROR_STREAM("Could not find pixel format " << pixel_format_name);
    return false;
  }

  bool hardware_encode = false;
  if (codec_name == "h264_qsv" || codec_name == "hevc_qsv")
  {
    ROS_INFO_STREAM("Using hardware encoding for " << codec_name);
    video_codec.getCodecContext()->pix_fmt = AV_PIX_FMT_QSV;
    hardware_encode = true;
  }
  else
  {
    ROS_INFO_STREAM("Using software encoding for " << codec_name);
    video_codec.getCodecContext()->pix_fmt = sw_pix_fmt;
  }
  
  if (!video_codec.getCodec()) 
  {
    ROS_ERROR_STREAM("Could not find codec of name " << codec_name);
    return false;
  }

  if (config_["bitrate"])
  {
    int bitrate = config_["bitrate"].as<int>();
    video_codec.getCodecContext()->bit_rate = bitrate;
    metadata["bitrate"] = bitrate;
  }

  video_codec.getCodecContext()->width = width;
  video_codec.getCodecContext()->height = height;
  video_codec.getCodecContext()->time_base = video_stream->time_base;
  video_codec.getCodecContext()->framerate = video_stream->avg_frame_rate;

  // Set the codec parameters for the video stream
  AVCodecParameters* video_codec_parameters = video_stream->codecpar;
  video_codec_parameters->codec_id = video_codec.getCodec()->id;
  video_codec_parameters->codec_type = AVMEDIA_TYPE_VIDEO;
  video_codec_parameters->width = video_codec.getCodecContext()->width;
  video_codec_parameters->height = video_codec.getCodecContext()->height;
  video_codec_parameters->format = video_codec.getCodecContext()->pix_fmt;
  video_codec_parameters->bit_rate = video_codec.getCodecContext()->bit_rate;

  // Fill out the encoding metadata
  metadata["codec_name"] = codec_name;
  metadata["pixel_format"] = pixel_format_name;
  metadata["encoding_fps"] = fps;
  metadata["num_frames"] = msg_queue.size();
  double duration = msg_queue.back()->msgTime().toSec() - msg_queue.front()->msgTime().toSec();
  metadata["duration_s"] = duration;
  metadata["observed_fps"] = msg_queue.size() / duration;
  metadata["width"] = width;
  metadata["height"] = height;
  metadata["ros_topic"] = msg_queue.front()->topicName();
  metadata["ros_start_time"] = msg_queue.front()->msgTime().toSec();
  metadata["ros_topic_type"] = msg_queue.front()->msgType();
  metadata["ros_topic_hash"] = msg_queue.front()->msgHash();
  try
  {
    metadata["ros_header_frame"] = std::get<std::string>(msg_queue.front()->parsedMsgData().at("/header/frame_id"));
  }
  catch(const std::exception& e){}
  
  try
  {
    if (msg_queue.front()->msgType() == "sensor_msgs/Image")
    {
      metadata["encoding"] = std::get<std::string>(msg_queue.front()->parsedMsgData().at("/encoding"));
    }
    else if (msg_queue.front()->msgType() == "sensor_msgs/CompressedImage")
    {
      metadata["format"] = std::get<std::string>(msg_queue.front()->parsedMsgData().at("/format"));
    }
  }
  catch(const std::exception& e){}

  // Write the file header
  if (avformat_write_header(format_context.get(), NULL) < 0) 
  {
    ROS_ERROR_STREAM("Error writing file header");
    return false;
  }

  // If there are any codec options, set them now
  if (config_["codec_options"])
  {
    metadata["codec_options"] = nlohmann::json::object();
    for (YAML::const_iterator it = config_["codec_options"].begin(); it != config_["codec_options"].end(); ++it) 
    {
      std::string option_name = it->first.as<std::string>();
      std::string option_value = it->second.as<std::string>();

      if (av_opt_set(video_codec.getCodecContext()->priv_data, option_name.c_str(), option_value.c_str(), 0) < 0)
      {
        ROS_WARN_STREAM("Could not set codec option " << option_name);
      }

      metadata["codec_options"][option_name] = option_value;
    }
  }

  // Create the hw device context
  DeviceContext hw_device_ctx;
  if (hardware_encode && !hw_device_ctx.get()) 
  {
    ROS_ERROR_STREAM("Could not create hardware device context");
    return false;
  }

  AVHWFramesContext* frames_ctx = NULL;
  if (hardware_encode)
  {
    HWFramesRef hw_frames_ref(hw_device_ctx);
    if (!hw_frames_ref.get()) 
    {
      ROS_ERROR_STREAM("Could not allocate hardware frame context");
      return false;
    }

    frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
    frames_ctx->format = AV_PIX_FMT_QSV;
    frames_ctx->sw_format = sw_pix_fmt;
    frames_ctx->width = video_codec_parameters->width;
    frames_ctx->height = video_codec_parameters->height;
    frames_ctx->initial_pool_size = 20;

    int err = av_hwframe_ctx_init(hw_frames_ref.get());
    if (err < 0) 
    {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, err);
      ROS_ERROR_STREAM("Error init hw frame context: " << errbuf);
      return false;
    }

    video_codec.getCodecContext()->hw_device_ctx = av_buffer_ref(hw_device_ctx.get());

    video_codec.getCodecContext()->hw_frames_ctx = av_buffer_ref(hw_frames_ref.get());
  }

  int ret = avcodec_open2(video_codec.getCodecContext(), video_codec.getCodec(), NULL);

  if (ret < 0)
  {
    // Grab the error and its meaning
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    ROS_ERROR_STREAM("Error opening the codec: " << errbuf << ". This might be caused by sw pixel format [" << pixel_format_name << "] not being supported by codec [" << codec_name << "]");
    return false;
  }

  // Iterate through the images on the topic to make video and encode frames
  int frame_size = video_codec.getCodecContext()->width * video_codec.getCodecContext()->height;
  ros::Time start_video_time = msg_queue.front()->msgTime();

  // Create a json metadata array for mapping video frames to ROS timestamps
  metadata["frames"] = nlohmann::json::array();

  for (auto it = msg_queue.begin(); it != msg_queue.end() ; )
  {
    DxRosMsg::Ptr msg = *it;

    // Input ROS frame
    VideoFrame input_ros_frame;
    input_ros_frame->width = video_codec.getCodecContext()->width;
    input_ros_frame->height = video_codec.getCodecContext()->height;
    input_ros_frame->format = AV_PIX_FMT_RGB24;

    // In case we need to decompress
    cv_bridge::CvImageConstPtr cv_image = nullptr;

    if (av_frame_get_buffer(input_ros_frame.get(), 32) < 0)
    {
      ROS_ERROR_STREAM("Could not allocate input ros frame data");
      return false;
    }

    if (msg->msgType() == "sensor_msgs/Image")
    {
      // Check the image encoding
      std::string encoding = "bgr8";
      try
      {
        encoding = std::get<std::string>(msg->parsedMsgData().at("/encoding"));
      }
      catch(const std::exception& e)
      {
        ROS_WARN_STREAM_THROTTLE(60, "Could not get encoding for image, defaulting to " << encoding);
      }
      
      
      if (encoding == "bgr8")
      {
        input_ros_frame->format = AV_PIX_FMT_BGR24;
      }
      else if (encoding == "rgb8")
      {
        input_ros_frame->format = AV_PIX_FMT_RGB24;
      }
      else
      {
        ROS_ERROR_STREAM("Unsupported ROS image encoding " << encoding);
        return false;
      }

      nonstd::span<const uint8_t> binary_span = std::get<nonstd::span<const uint8_t>>(msg->parsedMsgData().at("/data/0"));
      input_ros_frame->data[0] = (unsigned char *)binary_span.data();
      input_ros_frame->linesize[0] = 3 * video_codec.getCodecContext()->width;
    }
    else if (msg->msgType() == "sensor_msgs/CompressedImage")
    {
      // Need to decompress the image first

      // create Image msg to pass to cv_bridge
      try
      {
        sensor_msgs::CompressedImagePtr img_msg(new sensor_msgs::CompressedImage());
        nonstd::span<const uint8_t> binary_span = std::get<nonstd::span<const uint8_t>>(msg->parsedMsgData().at("/data/0"));
        img_msg->data = std::vector<unsigned char>((unsigned char *)binary_span.data(), (unsigned char *)(binary_span.data() + binary_span.size()));
        img_msg->format = std::get<std::string>(msg->parsedMsgData().at("/format"));
        img_msg->header.frame_id = std::get<std::string>(msg->parsedMsgData().at("/header/frame_id"));

        // Create the CV image
        cv_image = cv_bridge::toCvCopy(img_msg, "rgb8");

        // Create the input frame
        input_ros_frame->format = AV_PIX_FMT_RGB24;
        input_ros_frame->data[0] = (unsigned char *)cv_image->image.data;
        av_image_fill_linesizes(input_ros_frame->linesize, AV_PIX_FMT_RGB24, video_codec.getCodecContext()->width);
      }
      catch(const std::exception& e)
      {
        ROS_ERROR_STREAM_THROTTLE(60.0, "Could not decompress image: " << e.what());
        return false;
      }
    }
    else
    {
      ROS_ERROR_STREAM("Unsupported ROS message type " << msg->msgType());
      return false;
    }

    // Output frame
    VideoFrame video_frame;
    video_frame->format = hardware_encode ? frames_ctx->sw_format : video_codec.getCodecContext()->pix_fmt;
    video_frame->width = video_codec.getCodecContext()->width;
    video_frame->height = video_codec.getCodecContext()->height;
    double video_time_s = (msg->msgTime() - start_video_time).toSec();
    video_frame->pts = video_time_s*video_stream->time_base.den/video_stream->time_base.num;

    // Store the frame timestamp metadata
    metadata["frames"].push_back({{"video_time_s", video_time_s}, {"ros_time", msg->msgTime().toSec()}});

    if (av_frame_get_buffer(video_frame.get(), 32) < 0)
    {
      ROS_ERROR_STREAM("Could not allocate output frame data");
      return false;
    }

    // Create scaling context
    ScaleContext scale_context(input_ros_frame, video_frame);

    if (!scale_context.get()) 
    {
      ROS_ERROR_STREAM("Could not create scaling context");
      return false;
    }

    // Perform the conversion
    sws_scale(scale_context.get(), input_ros_frame->data, input_ros_frame->linesize, 0, input_ros_frame->height, video_frame->data, video_frame->linesize);

    VideoPacket packet;

    if (av_frame_make_writable(video_frame.get()) < 0)
    {
      ROS_ERROR_STREAM("Could not make frame writable");
      return false;
    }

    VideoFrame hw_frame;
    if (hardware_encode)
    {
      hw_frame->format = frames_ctx->format;
      hw_frame->width = video_frame->width;
      hw_frame->height = video_frame->height;
      hw_frame->pts = video_frame->pts;

      // Allocate the hardware frame.
      int err = av_hwframe_get_buffer(video_codec.getCodecContext()->hw_frames_ctx, hw_frame.get(), 0);
      if (err < 0) 
      {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, err);
        ROS_ERROR_STREAM("Could not get hw frame buffer: " << errbuf);
        return false;
      }

      // Transfer the data from system memory to hardware memory.
      err = av_hwframe_transfer_data(hw_frame.get(), video_frame.get(), 0);
      if (err < 0) 
      {
        char errbuf[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, err);
        ROS_ERROR_STREAM("Could not transfer frame to hw: " << errbuf);
        return false;
      }
    }

    // Encode the frame
    ret = hardware_encode ? avcodec_send_frame(video_codec.getCodecContext(), hw_frame.get()) :
                            avcodec_send_frame(video_codec.getCodecContext(), video_frame.get());
    if (ret < 0)
    {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
      ROS_ERROR_STREAM("Could not send frame for encoding: " << errbuf);
      return false;
    }

    ret = avcodec_receive_packet(video_codec.getCodecContext(), packet.get());

    // Write the packet to the video file
    if (ret >= 0 && av_interleaved_write_frame(format_context.get(), packet.get()) < 0) 
    {
      ROS_ERROR_STREAM("Error writing packet");
      return false;
    }

    av_packet_unref(packet.get());

    // We are now done with the ROS message - deallocate it to free memory sooner
    it = msg_queue.erase(it);
  }

  // Signal the end of the frames
  ret = avcodec_send_frame(video_codec.getCodecContext(), NULL);
  if (ret < 0) 
  {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
    ROS_ERROR_STREAM("Could not send frame for encoding: " << errbuf);
    return false;
  }

  // Flush the buffer
  VideoPacket packet;
  while (true) 
  {
    int ret = avcodec_receive_packet(video_codec.getCodecContext(), packet.get());
    if (ret == AVERROR_EOF) 
    {
      av_packet_unref(packet.get());
      break;
    } 
    else if (ret < 0) 
    {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
      ROS_ERROR_STREAM("Error flushing the buffer: " << errbuf);
      av_packet_unref(packet.get());
      break;
    }

    // Write the packet to the file
    if (av_interleaved_write_frame(format_context.get(), packet.get()) < 0)
    {
      ROS_ERROR_STREAM("Error writing packet");
      av_packet_unref(packet.get());
      return false;
    }

    av_packet_unref(packet.get());
  }

  // Write the file trailer
  if (av_write_trailer(format_context.get()) < 0) 
  {
      ROS_ERROR_STREAM("Error writing file trailer");
      return false;
  }

  // Output the metadata
  std::ofstream metadata_file_stream(metadata_file);
  metadata_file_stream << metadata.dump();
  metadata_file_stream.close();

  return true;
}
