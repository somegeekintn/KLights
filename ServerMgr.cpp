//
//  ServerMgr.cpp
//  KLights
//
//  Created by Casey Fleser on 04/22/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "ServerMgr.h"

#include <LittleFS.h>

static const char notFoundContent[] PROGMEM = 
R"==(<!DOCTYPE html><html lang='en'>
<head><title>Resource not found</title></head>
<body>
  <p>The resource was not found.</p>
  <p><a href="/">Start again</a></p>
</body>
)==";

static const char uploadContent[] PROGMEM =
R"==(<!doctype html><html lang='en'>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>File Upload</title>
</head>

<body style="width:300px">
  <h1>File Upload</h1>
  <div><a href="/">Home</a></div>
  <hr>
  <div id='zone' style='width:16em;height:12em;padding:10px;background-color:#ddd'>Drop files here...</div>

  <script>
    // allow drag&drop of file objects 
    function dragHelper(e) {
      e.stopPropagation();
      e.preventDefault();
    }

    // allow drag&drop of file objects 
    function dropped(e) {
      dragHelper(e);
      var fls = e.dataTransfer.files;
      var formData = new FormData();
      for (var i = 0; i < fls.length; i++) {
        formData.append('file', fls[i], '/' + fls[i].name);
      }
      fetch('/', { method: 'POST', body: formData }).then(function () {
        window.alert('done.');
      });
    }
    var z = document.getElementById('zone');
    z.addEventListener('dragenter', dragHelper, false);
    z.addEventListener('dragover', dragHelper, false);
    z.addEventListener('drop', dropped, false);
  </script>
</body>
)==";

#define UNUSED __attribute__((unused))

class FileServerHandler : public RequestHandler {
public:
    // @brief Construct a new File Server Handler object
    // @param fs The file system to be used.
    // @param path Path to the root folder in the file system that is used for
    // serving static data down and upload.
    // @param cache_header Cache Header to be used in replies.
    FileServerHandler() { }

    // @brief check incoming request. Can handle POST for uploads and DELETE.
    // @param requestMethod method of the http request line.
    // @param requestUri request ressource from the http request line.
    // @return true when method can be handled.
    bool canHandle(HTTPMethod requestMethod, const String UNUSED& _uri) override {
        return ((requestMethod == HTTP_POST) || (requestMethod == HTTP_DELETE));
    } // canHandle()

    bool canUpload(const String& uri) override {
        // only allow upload on root fs level.
        return (uri == "/");
    } // canUpload()

    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, const String& requestUri) override {
        // ensure that filename starts with '/'
        String fName = requestUri;
        if (!fName.startsWith("/")) {
            fName = "/" + fName;
        }

        // HTTP_POST done in upload. no other forms.
        if (requestMethod == HTTP_DELETE) {
            if (LittleFS.exists(fName)) {
                LittleFS.remove(fName);
            }
        }

        server.send(200); // all done.
        return (true);
    }

    void upload(ESP8266WebServer UNUSED& server, const String UNUSED& _requestUri, HTTPUpload& upload) override {
        // ensure that filename starts with '/'
        String fName = upload.filename;
        if (!fName.startsWith("/")) {
            fName = "/" + fName;
        }

        if (upload.status == UPLOAD_FILE_START) {
            if (LittleFS.exists(fName)) {
                LittleFS.remove(fName);
            } // if
            _fsUploadFile = LittleFS.open(fName, "w");
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (_fsUploadFile) {
                _fsUploadFile.write(upload.buf, upload.currentSize);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (_fsUploadFile) {
                _fsUploadFile.close();
            }
        }
    }

protected:
    File _fsUploadFile;
};

ServerMgr::ServerMgr() {
}

void ServerMgr::setup() {
    server.on("/minup.html", [this]() { this->handleBasicUpload(); });
    server.on("/", HTTP_GET, [this]() { this->handleRedirect(); });

    // register some REST services
    server.on("/$fs", HTTP_GET, [this]() { this->handleFileList(); });
    server.on("/$sysinfo", HTTP_GET, [this]() { this->handleSysInfo(); });

    server.addHandler(new FileServerHandler());

    server.serveStatic("/", LittleFS, "/");
    server.onNotFound([this]() { this->handleNotFound(); });

    server.begin(80);
    Serial.println(F("Web server listening on port 80"));
}

void ServerMgr::loop() {
    server.handleClient();
}

void ServerMgr::handleFileList() {
    Dir     dir = LittleFS.openDir("/");
    String  result;

    result += "[\n";
    while (dir.next()) {
        if (result.length() > 4) {
            result += ",";
        }
        result += "  {";
        result += " \"name\": \"" + dir.fileName() + "\", ";
        result += " \"size\": " + String(dir.fileSize()) + ", ";
        result += " \"time\": " + String(dir.fileTime());
        result += " }\n";
    }
    result += "]";
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, F("application/json; charset=utf-8"), result);
}

void ServerMgr::handleSysInfo() {
    String result;

    FSInfo fs_info;
    LittleFS.info(fs_info);

    result += "{\n";
    result += "  \"flashSize\": " + String(ESP.getFlashChipSize()) + ",\n";
    result += "  \"freeHeap\": " + String(ESP.getFreeHeap()) + ",\n";
    result += "  \"fsTotalBytes\": " + String(fs_info.totalBytes) + ",\n";
    result += "  \"fsUsedBytes\": " + String(fs_info.usedBytes) + ",\n";
    result += "  \"upTime\": " + String(millis() / 1000) + "\n";
    result += "}";

    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, F("application/json; charset=utf-8"), result);
}

void ServerMgr::handleBasicUpload() {
    server.send(200, "text/html", FPSTR(uploadContent));
}

// Called on request without filename. This will redirect to the file 
// index.html if it exists, otherwise to the built-in minup.html page

void ServerMgr::handleRedirect() {
    String url(F("/index.html"));

    if (!LittleFS.exists(url)) {
        url = F("/minup.html");
    }

    server.sendHeader("Location", url, true);
    server.send(302);
}

void ServerMgr::handleNotFound() {
    server.send(404, "text/html", FPSTR(notFoundContent));
}
