//
//  ServerMgr.cpp
//  KLights
//
//  Created by Casey Fleser on 04/22/2022.
//  Copyright © 2022 Casey Fleser. All rights reserved.
//

#include "ServerMgr.h"
#include "PixelController.h"
#include "config.h"
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
  <hr><div id='zone' style='width:16em;height:12em;padding:10px;background-color:#ddd'>Drop files here...</div>

  <script>
    function dragHelper(e) {
        e.stopPropagation();
        e.preventDefault();
    }

    function dropped(e) {
        dragHelper(e);
        var fls = e.dataTransfer.files;
        var formData = new FormData();
        for (var i = 0; i < fls.length; i++) {
            formData.append('file', fls[i], '/' + fls[i].name);
        }
        fetch('/', { method: 'POST', body: formData }).then(function () { window.alert('done.'); });
    }
    var z = document.getElementById('zone');
    z.addEventListener('dragenter', dragHelper, false);
    z.addEventListener('dragover', dragHelper, false);
    z.addEventListener('drop', dropped, false);
  </script>
</body>
)==";

#define BUILD_TIME      __DATE__ " " __TIME__

class FileServerHandler : public RequestHandler {
public:
    FileServerHandler() { }

    bool canHandle(HTTPMethod requestMethod, const String &uri) override {
        return (requestMethod == HTTP_POST) || (requestMethod == HTTP_DELETE);
    }

    bool canUpload(const String &uri) override {
        return uri == "/";     // currently only allow upload on root fs level.
    }

    bool handle(ESP8266WebServer &server, HTTPMethod requestMethod, const String &requestUri) override {
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

        server.send(200);

        return true;
    }

    void upload(ESP8266WebServer &server, const String &requestUri, HTTPUpload &upload) override {
        String fName = upload.filename;
        if (!fName.startsWith("/")) {
            fName = "/" + fName;
        }

        if (upload.status == UPLOAD_FILE_START) {
            if (LittleFS.exists(fName)) {
                LittleFS.remove(fName);
            } // if
            _fsUploadFile = LittleFS.open(fName, "w");
        } 
        else if (upload.status == UPLOAD_FILE_WRITE) {
            if (_fsUploadFile) {
                _fsUploadFile.write(upload.buf, upload.currentSize);
            }
        } 
        else if (upload.status == UPLOAD_FILE_END) {
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
    bootTime = time(NULL);

    server.on(F("/minup.html"), [this]() { this->handleBasicUpload(); });
    server.on(F("/"), HTTP_GET, [this]() { this->handleRedirect(); });

    // OTA Update handler
    httpUpdater.setup(&server, "/otaupdate");
    // Note: this required a change to Updater.cpp as writeStream calls the progress handler
    // but not write. Consider creating an HTTP updater that contains its own progress handler
    Update.onProgress([](size_t progress, size_t len) {
        // progress from Updater is currently broken as it gives the size of the destination
        // insteaed of the source. we'll just blink the LEDs on each update for now.
        static bool pToggle = true;
        SHSVRec color = ColorUtils::purple.withVal(pToggle ? 0.5 : 0.1);
        
        gPixels->setAreaColor(area_status_1, color);
        gPixels->setAreaColor(area_status_2, color);
        gPixels->show();
        pToggle = !pToggle;
    });

    // register some REST services
    server.on(F("/$fs"), HTTP_GET, [this]() { this->handleFileList(); });
    server.on(F("/$sysinfo"), HTTP_GET, [this]() { this->handleSysInfo(); });
    server.on(F("/$effect"), HTTP_GET, [this]() { this->handleEffect(); });
    server.addHandler(new FileServerHandler());

    server.serveStatic("/", LittleFS, "/");
    server.onNotFound([this]() { this->handleNotFound(); });

    server.begin(80);
    Serial.println(F("Web server listening on port 80"));
}

void ServerMgr::loop() {
    // Note: Made changes to streamFile in ESP8266WebServer.h to use file.sendSize instead of
    // file.sendAll which can delay up to 1 second waiting for Stream (aka File) to make more
    // data available to send. Probably I should figure out how to send a PR to the good folks
    // at https://github.com/esp8266/Arduino

    server.handleClient();
}

void ServerMgr::handleFileList() {
    StaticJsonDocument<1024> jsonDoc;
    Dir         dir = LittleFS.openDir("/");
    String      result;
    int16_t     idx = 0;

    while (dir.next()) {
        jsonDoc[idx][F("name")] = dir.fileName();
        jsonDoc[idx][F("size")] = dir.fileSize();
        jsonDoc[idx][F("time")] = dir.fileTime();
        idx++;
    }

    serializeJson(jsonDoc, result);
    server.sendHeader(F("Cache-Control"), F("no-cache"));
    server.send(200, F("application/json; charset=utf-8"), result);
}

void ServerMgr::handleSysInfo() {
    StaticJsonDocument<512> jsonDoc;
    String      result;
    FSInfo      fs_info;
    time_t      now = time(NULL);

    LittleFS.info(fs_info);

    jsonDoc[F("project")] = kPROJ_TITLE;
    jsonDoc[F("buildTime")] = BUILD_TIME;
    jsonDoc[F("versionSDK")] = ESP.getSdkVersion();
    jsonDoc[F("versionCore")] = ESP.getCoreVersion();
    jsonDoc[F("versionFull")] = ESP.getFullVersion();
    jsonDoc[F("versionBoot")] = ESP.getBootVersion();
    jsonDoc[F("flashSize")] = ESP.getFlashChipSize();
    jsonDoc[F("freeHeap")] = ESP.getFreeHeap();
    jsonDoc[F("heapFrag")] = ESP.getHeapFragmentation();
    jsonDoc[F("sketchSize")] = ESP.getSketchSize();
    jsonDoc[F("sketchSpace")] = ESP.getFreeSketchSpace();
    jsonDoc[F("fsTotalBytes")] = fs_info.totalBytes;
    jsonDoc[F("fsUsedBytes")] = fs_info.usedBytes;
    jsonDoc[F("bootTime")] = bootTime;
    jsonDoc[F("curTime")] = now;
    serializeJson(jsonDoc, result);

    server.sendHeader(F("Cache-Control"), F("no-cache"));
    server.send(200, F("application/json; charset=utf-8"), result);
}

void ServerMgr::handleEffect() {
    StaticJsonDocument<256> jsonDoc;
    int                     argCount = server.args();

    for (int i=0; i<argCount; i++) {
        jsonDoc[server.argName(i)] = server.arg(i);
    }
    gPixels->handleWebCommand(jsonDoc);
    server.send(200, F("application/json; charset=utf-8"), F("{ \"result\": \"ok\" }"));
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
