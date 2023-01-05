#define _WINSOCKAPI_
#include "ba2tk/src/ba2archive.h"
#include "string_cast.h"
#include "napi_helpers.h"
#include <vector>
#include <thread>
#include <napi.h>

#include <iostream>

const char *convertErrorCode(BA2::EErrorCode code) {
  switch (code) {
    case BA2::ERROR_ACCESSFAILED: return "access failed";
    case BA2::ERROR_CANCELED: return "canceled";
    case BA2::ERROR_FILENOTFOUND: return "file not found";
    case BA2::ERROR_INVALIDDATA: return "invalid data";
    case BA2::ERROR_INVALIDHASHES: return "invalid hashes";
    case BA2::ERROR_SOURCEFILEMISSING: return "source file missing";
    case BA2::ERROR_ZLIBINITFAILED: return "zlib init failed";
    case BA2::ERROR_NONE: return nullptr;
    default: return "unknown";
  }
}

class ExtractAllWorker : public Napi::AsyncWorker {
public:
  ExtractAllWorker(std::shared_ptr<BA2::Archive> archive,
    const char *outputDirectory,
    bool overwrite,
    const Napi::Function &appCallback)
    : Napi::AsyncWorker(appCallback)
    , m_Archive(archive)
    , m_FilePath(outputDirectory)
    , m_Overwrite(overwrite)
  {}

  void Execute() {
    BA2::EErrorCode code;
    code = m_Archive->extractAll(m_FilePath.c_str(),
      [](int value, std::string fileName) { return true; }, true);
    if (code != BA2::ERROR_NONE) {
      SetError(convertErrorCode(code));
    }
  }

  virtual void OnOK() override {
    Callback().Call(Receiver().Value(), std::initializer_list<napi_value>{ Env().Null() });
  }
private:
  std::shared_ptr<BA2::Archive> m_Archive;
  std::string m_FilePath;
  bool m_Overwrite;
};


class BA2Archive : public Napi::ObjectWrap<BA2Archive> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports) {
    Napi::Function func = DefineClass(env, "BA2Archive", {
      InstanceMethod("getType", &BA2Archive::getType),
      InstanceMethod("getFileList", &BA2Archive::getFileList),
      InstanceMethod("extractAll", &BA2Archive::extractAll),
    });
    Napi::FunctionReference *constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    exports.Set("BA2Archive", func);

    env.SetInstanceData<Napi::FunctionReference>(constructor);
    return exports;
  }

  BA2Archive(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<BA2Archive>(info)
    , m_Wrapped(new BA2::Archive())
  {
  }

  void readAsync(const Napi::CallbackInfo& info, const std::string& filePath, const Napi::Function& cb) {
    const Napi::Env env = info.Env();
    std::thread* loadThread;

    m_ThreadCB = Napi::ThreadSafeFunction::New(info.Env(), cb,
      "AsyncLoadCB", 0, 1, [loadThread](Napi::Env) {
      loadThread->join();
      delete loadThread;
    });


    loadThread = new std::thread{ [this, env, filePath]() {
      auto callback = [](Napi::Env env, Napi::Function jsCallback, BA2Archive* result) {
        jsCallback.Call({ env.Null(), result->Value() });
      };

      auto callbackError = [](Napi::Env env, Napi::Function jsCallback, std::string* err = nullptr) {
        jsCallback.Call({Napi::String::New(env, err->c_str())});
      };

      try {
        read(filePath);
        m_ThreadCB.Acquire();
        m_ThreadCB.BlockingCall(this, callback);
      }
      catch (const std::exception& e) {
        std::string* err = new std::string{ e.what() };
        m_ThreadCB.Acquire();
        m_ThreadCB.BlockingCall(err, callbackError);
      }

      m_ThreadCB.Release();
    } };
  }

  static Napi::Object CreateNewItem(Napi::Env env) {
    Napi::FunctionReference* constructor = env.GetInstanceData<Napi::FunctionReference>();
    return constructor->New({ });
  }


  ~BA2Archive() {
  }

private:

  void read(const std::string &filePath) {
    BA2::EErrorCode err = m_Wrapped->read(toWC(filePath.c_str(), CodePage::UTF8, filePath.length()).c_str());
    if (err != BA2::ERROR_NONE) {
      throw std::runtime_error(convertErrorCode(err));
    }
  }

  Napi::Value getType(const Napi::CallbackInfo &info) {
    switch (m_Wrapped->getType()) {
      case BA2::TYPE_GENERAL: return Napi::String::From(info.Env(), "general");
      case BA2::TYPE_DX10: return Napi::String::From(info.Env(), "dx10");
      default: return info.Env().Null();
    }
  }

  Napi::Value getFileList(const Napi::CallbackInfo& info) {
    return toNAPI(info.Env(), m_Wrapped->getFileList());
  }
    
  Napi::Value extractAll(const Napi::CallbackInfo& info) {
    Napi::String outputDirectory = info[0].ToString();
    Napi::Boolean overwrite = info[1].ToBoolean();
    Napi::Function callback = info[2].As<Napi::Function>();
    auto worker = new ExtractAllWorker(m_Wrapped,
                                       outputDirectory.Utf8Value().c_str(),
                                       overwrite,
                                       callback);
    worker->Queue();
    return info.Env().Undefined();
  }

private:
  std::shared_ptr<BA2::Archive> m_Wrapped;

  Napi::ThreadSafeFunction m_ThreadCB;
};

Napi::Value loadBA2(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  Napi::String filePath = info[0].ToString();
  Napi::Function cb = info[1].As<Napi::Function>();

  Napi::Object result = BA2Archive::CreateNewItem(env);
  BA2Archive* resultObj = BA2Archive::Unwrap(result);

  resultObj->readAsync(info, filePath, cb);

  return info.Env().Undefined();
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  exports.Set("loadBA2", Napi::Function::New(env, loadBA2));
  BA2Archive::Init(env, exports);
  return exports;
}

NODE_API_MODULE(BA2Archive, InitAll)
