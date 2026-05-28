#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <vector>

class Preferences {
public:
    bool begin(const char* name, bool readOnly = false) {
        _name = name;
        _filename = std::string("save_") + name + ".bin";
        _load();
        return true;
    }
    
    void end() {
        _save();
    }
    
    bool isKey(const char* key) { return _data.find(key) != _data.end(); }
    void remove(const char* key) { _data.erase(key); }
    void clear() { _data.clear(); _strData.clear(); _blobData.clear(); }
    
    size_t putUInt(const char* key, uint32_t value) { _data[key] = value; return 4; }
    size_t putInt(const char* key, int32_t value) { _data[key] = *(uint32_t*)&value; return 4; }
    size_t putFloat(const char* key, float value) { _data[key] = *(uint32_t*)&value; return 4; }
    size_t putBool(const char* key, bool value) { _data[key] = value; return 1; }
    size_t putByte(const char* key, uint8_t value) { _data[key] = value; return 1; }
    size_t putUChar(const char* key, uint8_t value) { return putByte(key, value); }
    size_t putBytes(const char* key, const void* value, size_t len) {
        std::vector<uint8_t> vec((const uint8_t*)value, (const uint8_t*)value + len);
        _blobData[key] = vec;
        return len;
    }
    size_t putString(const char* key, const std::string& value) { _strData[key] = value; return value.length(); }

    uint32_t getUInt(const char* key, uint32_t defaultValue = 0) {
        if (!isKey(key)) return defaultValue;
        return _data[key];
    }
    int32_t getInt(const char* key, int32_t defaultValue = 0) {
        if (!isKey(key)) return defaultValue;
        uint32_t val = _data[key];
        return *(int32_t*)&val;
    }
    float getFloat(const char* key, float defaultValue = 0.0f) {
        if (!isKey(key)) return defaultValue;
        uint32_t val = _data[key];
        return *(float*)&val;
    }
    bool getBool(const char* key, bool defaultValue = false) {
        if (!isKey(key)) return defaultValue;
        return _data[key] != 0;
    }
    uint8_t getByte(const char* key, uint8_t defaultValue = 0) {
        if (!isKey(key)) return defaultValue;
        return _data[key] & 0xFF;
    }
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0) {
        return getByte(key, defaultValue);
    }
    size_t getBytes(const char* key, void* buf, size_t maxLen) {
        if (_blobData.find(key) == _blobData.end()) return 0;
        const auto& vec = _blobData[key];
        size_t copyLen = (vec.size() < maxLen) ? vec.size() : maxLen;
        memcpy(buf, vec.data(), copyLen);
        return copyLen;
    }
    std::string getString(const char* key, const std::string& defaultValue = "") {
        if (_strData.find(key) == _strData.end()) return defaultValue;
        return _strData[key];
    }

private:
    std::string _name;
    std::string _filename;
    std::map<std::string, uint32_t> _data;
    std::map<std::string, std::string> _strData;
    std::map<std::string, std::vector<uint8_t>> _blobData;
    
    void _load() {
        _data.clear();
        _strData.clear();
        _blobData.clear();
        std::ifstream file(_filename, std::ios::binary);
        if (file.is_open()) {
            size_t count;
            file.read((char*)&count, sizeof(count));
            for (size_t i = 0; i < count; i++) {
                size_t keyLen;
                file.read((char*)&keyLen, sizeof(keyLen));
                char keyBuf[64] = {0};
                file.read(keyBuf, keyLen);
                uint32_t val;
                file.read((char*)&val, sizeof(val));
                _data[keyBuf] = val;
            }
        }
        
        std::ifstream strFile(_filename + ".str", std::ios::binary);
        if (strFile.is_open()) {
            size_t count;
            strFile.read((char*)&count, sizeof(count));
            for (size_t i = 0; i < count; i++) {
                size_t keyLen;
                strFile.read((char*)&keyLen, sizeof(keyLen));
                char keyBuf[64] = {0};
                strFile.read(keyBuf, keyLen);
                
                size_t valLen;
                strFile.read((char*)&valLen, sizeof(valLen));
                char valBuf[128] = {0};
                if (valLen < 128) {
                    strFile.read(valBuf, valLen);
                    _strData[keyBuf] = valBuf;
                }
            }
        }
        
        std::ifstream blobFile(_filename + ".blob", std::ios::binary);
        if (blobFile.is_open()) {
            size_t count;
            blobFile.read((char*)&count, sizeof(count));
            for (size_t i = 0; i < count; i++) {
                size_t keyLen;
                blobFile.read((char*)&keyLen, sizeof(keyLen));
                char keyBuf[64] = {0};
                blobFile.read(keyBuf, keyLen);
                
                size_t valLen;
                blobFile.read((char*)&valLen, sizeof(valLen));
                std::vector<uint8_t> valBuf(valLen);
                blobFile.read((char*)valBuf.data(), valLen);
                _blobData[keyBuf] = valBuf;
            }
        }
    }
    
    void _save() {
        std::ofstream file(_filename, std::ios::binary);
        if (file.is_open()) {
            size_t count = _data.size();
            file.write((char*)&count, sizeof(count));
            for (auto& pair : _data) {
                size_t keyLen = pair.first.length();
                file.write((char*)&keyLen, sizeof(keyLen));
                file.write(pair.first.c_str(), keyLen);
                file.write((char*)&pair.second, sizeof(pair.second));
            }
        }
        
        std::ofstream strFile(_filename + ".str", std::ios::binary);
        if (strFile.is_open()) {
            size_t count = _strData.size();
            strFile.write((char*)&count, sizeof(count));
            for (auto& pair : _strData) {
                size_t keyLen = pair.first.length();
                strFile.write((char*)&keyLen, sizeof(keyLen));
                strFile.write(pair.first.c_str(), keyLen);
                
                size_t valLen = pair.second.length();
                strFile.write((char*)&valLen, sizeof(valLen));
                strFile.write(pair.second.c_str(), valLen);
            }
        }
        
        std::ofstream blobFile(_filename + ".blob", std::ios::binary);
        if (blobFile.is_open()) {
            size_t count = _blobData.size();
            blobFile.write((char*)&count, sizeof(count));
            for (auto& pair : _blobData) {
                size_t keyLen = pair.first.length();
                blobFile.write((char*)&keyLen, sizeof(keyLen));
                blobFile.write(pair.first.c_str(), keyLen);
                
                size_t valLen = pair.second.size();
                blobFile.write((char*)&valLen, sizeof(valLen));
                blobFile.write((const char*)pair.second.data(), valLen);
            }
        }
    }
};
