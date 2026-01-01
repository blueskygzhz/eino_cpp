#ifndef NLOHMANN_JSON_HPP
#define NLOHMANN_JSON_HPP

#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <sstream>

namespace nlohmann {

// Forward declaration
class json;

// Minimal JSON implementation for compatibility
class json {
public:
    enum value_t {
        null,
        object,
        array,
        string,
        boolean,
        number_integer,
        number_unsigned,
        number_float,
        binary,
        discarded
    };

    json() : type_(null) {}
    
    json(const std::string& value) : type_(string), string_value_(value) {}
    
    json(const char* value) : type_(string), string_value_(value ? value : "") {}
    
    json(double value) : type_(number_float), double_value_(value) {}
    
    json(float value) : type_(number_float), double_value_(value) {}
    
    json(int value) : type_(number_integer), int_value_(value) {}
    
    json(unsigned int value) : type_(number_unsigned), int_value_(value) {}
    
    json(bool value) : type_(boolean), bool_value_(value) {}
    
    json(const std::map<std::string, json>& value) 
        : type_(object), object_value_(value) {}
    
    json(const std::vector<json>& value) 
        : type_(array), array_value_(value) {}
    
    // Assignment operators for easier API
    json& operator=(const std::string& value) {
        type_ = string;
        string_value_ = value;
        return *this;
    }
    
    json& operator=(const char* value) {
        type_ = string;
        string_value_ = value ? value : "";
        return *this;
    }
    
    json& operator=(double value) {
        type_ = number_float;
        double_value_ = value;
        return *this;
    }
    
    json& operator=(int value) {
        type_ = number_integer;
        int_value_ = value;
        return *this;
    }
    
    json& operator=(bool value) {
        type_ = boolean;
        bool_value_ = value;
        return *this;
    }
    
    // operator[]
    json& operator[](const std::string& key) {
        if (type_ == null) {
            type_ = object;
        }
        if (type_ != object) {
            throw std::runtime_error("json type is not object");
        }
        return object_value_[key];
    }
    
    const json& operator[](const std::string& key) const {
        if (type_ != object) {
            throw std::runtime_error("json type is not object");
        }
        auto it = object_value_.find(key);
        if (it != object_value_.end()) {
            return it->second;
        }
        static json null_json;
        return null_json;
    }
    
    json& operator[](size_t index) {
        if (type_ == null) {
            type_ = array;
            array_value_.resize(index + 1);
        }
        if (type_ != array) {
            throw std::runtime_error("json type is not array");
        }
        if (index >= array_value_.size()) {
            array_value_.resize(index + 1);
        }
        return array_value_[index];
    }
    
    // Type checking
    bool is_null() const { return type_ == null; }
    bool is_object() const { return type_ == object; }
    bool is_array() const { return type_ == array; }
    bool is_string() const { return type_ == string; }
    bool is_boolean() const { return type_ == boolean; }
    bool is_number() const { 
        return type_ == number_integer || type_ == number_unsigned || type_ == number_float;
    }
    
    // Value access
    std::string get_string() const {
        if (type_ != string) throw std::runtime_error("not a string");
        return string_value_;
    }
    
    bool get_boolean() const {
        if (type_ != boolean) throw std::runtime_error("not a boolean");
        return bool_value_;
    }
    
    int get_integer() const {
        if (type_ != number_integer) throw std::runtime_error("not an integer");
        return int_value_;
    }
    
    double get_double() const {
        if (type_ != number_float) throw std::runtime_error("not a double");
        return double_value_;
    }
    
    // Conversions
    operator std::string() const { return get_string(); }
    operator bool() const { return get_boolean(); }
    operator int() const { return get_integer(); }
    operator double() const { return get_double(); }
    
    // Size
    size_t size() const {
        if (type_ == object) return object_value_.size();
        if (type_ == array) return array_value_.size();
        return 0;
    }
    
    // Contains key
    bool contains(const std::string& key) const {
        if (type_ != object) return false;
        return object_value_.find(key) != object_value_.end();
    }
    
    // Dump to string
    std::string dump(int indent = -1) const {
        std::ostringstream oss;
        dump_impl(oss, indent, 0);
        return oss.str();
    }
    
    // Type
    value_t type() const { return type_; }
    
    // Iterators
    auto begin() { 
        if (type_ == object) {
            // Return object iterator
        } else if (type_ == array) {
            return array_value_.begin();
        }
        return array_value_.end();
    }
    
    auto end() {
        if (type_ == array) {
            return array_value_.end();
        }
        return array_value_.end();
    }

private:
    void dump_impl(std::ostringstream& oss, int indent, int current_indent) const {
        switch (type_) {
            case null:
                oss << "null";
                break;
            case string:
                oss << "\"" << string_value_ << "\"";
                break;
            case number_integer:
                oss << int_value_;
                break;
            case number_float:
                oss << double_value_;
                break;
            case boolean:
                oss << (bool_value_ ? "true" : "false");
                break;
            case object:
                oss << "{";
                for (auto it = object_value_.begin(); it != object_value_.end(); ++it) {
                    if (it != object_value_.begin()) oss << ",";
                    oss << "\"" << it->first << "\":";
                    it->second.dump_impl(oss, indent, current_indent);
                }
                oss << "}";
                break;
            case array:
                oss << "[";
                for (size_t i = 0; i < array_value_.size(); ++i) {
                    if (i > 0) oss << ",";
                    array_value_[i].dump_impl(oss, indent, current_indent);
                }
                oss << "]";
                break;
            default:
                oss << "null";
        }
    }
    
    value_t type_;
    std::string string_value_;
    int int_value_ = 0;
    double double_value_ = 0.0;
    bool bool_value_ = false;
    std::map<std::string, json> object_value_;
    std::vector<json> array_value_;
};

} // namespace nlohmann

#endif // NLOHMANN_JSON_HPP
