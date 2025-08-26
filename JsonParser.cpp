#include "JsonParser.h"
#include <sstream>
#include <string>
#include <unordered_map>

void JsonParser::skipWhitespace() {
  while (pos < json.length() && std::isspace(json[pos])) {
    pos++;
  }
}

std::string JsonParser::parseString() {
  if (json[pos] != '"')
    return "";
  pos++; // Skip opening quote

  std::string result;
  while (pos < json.length() && json[pos] != '"') {
    if (json[pos] == '\\' && pos + 1 < json.length()) {
      pos++; // Skip escape character
      switch (json[pos]) {
      case 'n':
        result += '\n';
        break;
      case 't':
        result += '\t';
        break;
      case 'r':
        result += '\r';
        break;
      case '\\':
        result += '\\';
        break;
      case '"':
        result += '"';
        break;
      default:
        result += json[pos];
        break;
      }
    } else {
      result += json[pos];
    }
    pos++;
  }

  if (pos < json.length())
    pos++; // Skip closing quote
  return result;
}

double JsonParser::parseNumber() {
  size_t start = pos;
  if (json[pos] == '-')
    pos++;

  while (pos < json.length() && (std::isdigit(json[pos]) || json[pos] == '.')) {
    pos++;
  }

  return std::stod(json.substr(start, pos - start));
}

std::unordered_map<std::string, std::string>
JsonParser::parseObject(const std::string &jsonStr) {
  json = jsonStr;
  pos = 0;
  std::unordered_map<std::string, std::string> result;

  skipWhitespace();
  if (pos >= json.length() || json[pos] != '{') {
    return result; // Not a valid JSON object
  }
  pos++; // Skip opening brace

  while (pos < json.length()) {
    skipWhitespace();

    if (json[pos] == '}')
      break; // End of object

    // Parse key
    std::string key = parseString();
    if (key.empty())
      break;

    skipWhitespace();
    if (pos >= json.length() || json[pos] != ':')
      break;
    pos++; // Skip colon

    skipWhitespace();

    // Parse value
    std::string value;
    if (json[pos] == '"') {
      value = parseString();
    } else if (json[pos] == '-' || std::isdigit(json[pos])) {
      double num = parseNumber();
      value = std::to_string(num);
    } else if (json.substr(pos, 4) == "true") {
      value = "true";
      pos += 4;
    } else if (json.substr(pos, 5) == "false") {
      value = "false";
      pos += 5;
    } else if (json.substr(pos, 4) == "null") {
      value = "null";
      pos += 4;
    }

    result[key] = value;

    skipWhitespace();
    if (pos < json.length() && json[pos] == ',') {
      pos++; // Skip comma
    }
  }

  return result;
}
