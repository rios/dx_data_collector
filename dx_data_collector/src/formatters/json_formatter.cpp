#include "formatters/json_formatter.h"

rios::data_collector::JsonFormatter::JsonFormatter(const rios::cfg& json_formatter_config)
: config_(json_formatter_config)
{

}

rios::data_collector::FormattedData::Ptr rios::data_collector::JsonFormatter::formatData(std::deque<DxRosMsg::Ptr>& data_queue, std::string snapshot_name)
{
  std::string episode_uuid = boost::uuids::to_string(boost::uuids::random_generator()());

  // Json containing keys for data
  std::shared_ptr<nlohmann::json> key_json = std::make_shared<nlohmann::json>();
  key_json->operator[]("TimeRange.begin") = nlohmann::json();
  key_json->operator[]("TimeRange.begin")["secs"] = data_queue.front()->timeRecvd().sec;
  key_json->operator[]("TimeRange.begin")["nsecs"] = data_queue.front()->timeRecvd().nsec;
  key_json->operator[]("TimeRange.end") = nlohmann::json();
  key_json->operator[]("TimeRange.end")["secs"] = data_queue.back()->timeRecvd().sec;
  key_json->operator[]("TimeRange.end")["nsecs"] = data_queue.back()->timeRecvd().nsec;
  key_json->operator[]("uuid") = boost::uuids::to_string(boost::uuids::random_generator()());
  key_json->operator[]("components") = nlohmann::json::array();
  key_json->operator[]("ts") = ros::Time::now().sec;

  std::time_t t = std::time(0); 
  std::tm* now = std::localtime(&t);
  std::string year = std::to_string(now->tm_year + 1900);
  std::string month = std::to_string(now->tm_mon + 1);
  if (now->tm_mon < 9) month = "0" + month;
  std::string day = std::to_string(now->tm_mday);
  if (now->tm_mday < 9) day = "0" + day;
  key_json->operator[]("dt") = year + "-" + month + "-" + day;
  std::string workcell_serial_number = "undefined";
  ros::param::get("workcell_serial_number", workcell_serial_number);
  key_json->operator[]("workcell_serial_number") = workcell_serial_number;

  std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons = std::make_shared<std::unordered_map<std::string, nlohmann::json>>();
  std::shared_ptr<std::unordered_map<std::string, cv_bridge::CvImageConstPtr>> images = std::make_shared<std::unordered_map<std::string, cv_bridge::CvImageConstPtr>>();
  
  for (auto & msg : data_queue)
  {
    nlohmann::json key_obj = nlohmann::json();
    std::string msg_uuid = boost::uuids::to_string(boost::uuids::random_generator()());
    key_obj["topic"] = msg->topicName();
    key_obj["component_uuid"] = msg_uuid;
    key_obj["type"] = msg->msgType() + "/" + msg->msgHash();

    msg_jsons->operator[](msg_uuid) = nlohmann::json();
    nlohmann::json& msg_obj = msg_jsons->operator[](msg_uuid);

    msg_obj["topic"] = msg->topicName();
    msg_obj["uuid"] = msg_uuid;
    msg_obj["episode_key_type"] = "rios_ros_data_forwarder_msgs/TimeRange/b341004f74e15bf5e1b2053a9183bdc7";
    msg_obj["episode_uuid"] = episode_uuid;
    msg_obj["type"] = msg->msgType() + "/" + msg->msgHash();

    for (const auto &[field_name, field_data] : msg->parsedMsgData())
    {
      nlohmann::json* cur_obj = &msg_jsons->operator[](msg_uuid);
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
      else if (std::holds_alternative<nonstd::span<const uint8_t>>(field_data))
      {
        if (msg->msgType() == "sensor_msgs/Image")
        {
          std::string img_uuid = msg_uuid + ".png";

          // create Image msg to pass to cv_bridge
          sensor_msgs::ImagePtr img_msg(new sensor_msgs::Image());
          nonstd::span<const uint8_t> binary_span = std::get<nonstd::span<const uint8_t>>(field_data);
          img_msg->data = std::vector<unsigned char>((unsigned char *)binary_span.data(), (unsigned char *)(binary_span.data() + binary_span.size()));
          img_msg->encoding = std::get<std::string>(msg->parsedMsgData().at("/encoding"));
          img_msg->height = std::get<uint32_t>(msg->parsedMsgData().at("/height"));
          img_msg->width = std::get<uint32_t>(msg->parsedMsgData().at("/width"));
          img_msg->step = std::get<uint32_t>(msg->parsedMsgData().at("/step"));
          img_msg->is_bigendian  = std::get<uint8_t>(msg->parsedMsgData().at("/is_bigendian"));
          img_msg->header.frame_id = std::get<std::string>(msg->parsedMsgData().at("/header/frame_id"));

          // Create and store the CV image
          cv_bridge::CvImageConstPtr cv_image = cv_bridge::toCvShare(img_msg);

          images->operator[](img_uuid) = cv_image;  
          *cur_obj = img_uuid;
        }
        else if (msg->msgType() == "sensor_msgs/CompressedImage")
        {
          std::string img_uuid = msg_uuid + ".png";

          // create Image msg to pass to cv_bridge
          sensor_msgs::CompressedImagePtr img_msg(new sensor_msgs::CompressedImage());
          nonstd::span<const uint8_t> binary_span = std::get<nonstd::span<const uint8_t>>(field_data);
          img_msg->data = std::vector<unsigned char>((unsigned char *)binary_span.data(), (unsigned char *)(binary_span.data() + binary_span.size()));
          img_msg->format = std::get<std::string>(msg->parsedMsgData().at("/format"));
          img_msg->header.frame_id = std::get<std::string>(msg->parsedMsgData().at("/header/frame_id"));

          // Create and store the CV image
          cv_bridge::CvImageConstPtr cv_image = cv_bridge::toCvCopy(img_msg);

          images->operator[](img_uuid) = cv_image;  
          *cur_obj = img_uuid;
        }
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

    key_json->operator[]("components").emplace_back(key_obj);
  }
  
  return std::make_shared<JsonData>(key_json, msg_jsons, images);
}

rios::data_collector::JsonData::JsonData(std::shared_ptr<nlohmann::json> key_json_data,
                                         std::shared_ptr<std::unordered_map<std::string, nlohmann::json>> msg_jsons,  
                                         std::shared_ptr<std::unordered_map<std::string, cv_bridge::CvImageConstPtr>> images)
: key_json_data_(key_json_data)
, msg_jsons_(msg_jsons)
, images_(images)
{

}

bool rios::data_collector::JsonData::asFiles(std::filesystem::path path)
{
  // Create folder structure as per original standard
  std::filesystem::path current_path = path / "key";
  if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
  {
    ROS_ERROR_STREAM("Json formatter could not create folder " << current_path.string());
    return false;
  }
  
  current_path = current_path / "rios_ros_data_forwarder_msgs";
  if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
  {
    ROS_ERROR_STREAM("Json formatter could not create folder " << current_path.string());
    return false;
  }

  current_path = current_path / "TimeRange";
  if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
  {
    ROS_ERROR_STREAM("Json formatter could not create folder " << current_path.string());
    return false;
  }

  current_path = current_path / "b341004f74e15bf5e1b2053a9183bdc7";
  if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
  {
    ROS_ERROR_STREAM("Json formatter could not create folder " << current_path.string());
    return false;
  }

  std::time_t t = std::time(0); 
  std::tm* now = std::localtime(&t);
  std::string year = std::to_string(now->tm_year + 1900);
  std::string month = std::to_string(now->tm_mon + 1);
  if (now->tm_mon < 9) month = "0" + month;
  std::string day = std::to_string(now->tm_mday);
  if (now->tm_mday < 9) day = "0" + day;
  current_path = current_path / ("dt=" + year + "-" + month + "-" + day);
  if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
  {
    ROS_ERROR_STREAM("Json formatter could not create folder " << current_path.string());
    return false;
  }

  // Create key json
  std::filesystem::path filename = current_path / (key_json_data_->operator[]("uuid").get<std::string>() + ".json");
  std::ofstream file(filename);
  if (!file.is_open())
  {
    return false;
  }
  
  file << key_json_data_->dump();
  file.close(); 

  // Create the message jsons

  for (auto &[msg_uuid, msg_json] : *msg_jsons_)
  {
    std::filesystem::path current_path = path;
    std::string msg_type = msg_json["type"].get<std::string>();
    for (auto & folder_name : rios::utils::getTokens(msg_type, "/"))
    {
      current_path = current_path / folder_name;
      if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
      {
        ROS_ERROR_STREAM("Json formatter could not create folder " << current_path.string());
        return false;
      }
    }

    current_path = current_path / ("dt=" + year + "-" + month + "-" + day);
    if (!std::filesystem::exists(current_path) && !std::filesystem::create_directory(current_path))
    {
      ROS_ERROR_STREAM("Json formatter could not create folder " << current_path.string());
      return false;
    }

    std::filesystem::path filename = current_path / (msg_json["uuid"].get<std::string>() + ".json");
    std::ofstream file(filename);
    if (!file.is_open())
    {
      return false;
    }
    
    file << msg_json.dump();
    file.close(); 

    std::string image_name = msg_json["uuid"].get<std::string>() + ".png";
    if (images_->count(image_name))
    {
      // There is an image associated with this data - write it now
      std::filesystem::path image_filename = current_path / image_name;
      cv::imwrite(image_filename, images_->operator[](image_name)->image);
    }
  }

  return true;
}
