//
// Created by Thomas Lamy on 26.08.25.
//

#ifndef USB_METER_OSD_JSONPARSER_H
#define USB_METER_OSD_JSONPARSER_H
#include <sstream>
#include <string>
#include <unordered_map>

class JsonParser {
public:
  std::unordered_map<std::string, std::string>
  parseObject(const std::string &jsonStr);
  void skipWhitespace();
  std::string parseString();
  double parseNumber();

private:
  std::string json;
  size_t pos = 0;
};

#endif // USB_METER_OSD_JSONPARSER_H
