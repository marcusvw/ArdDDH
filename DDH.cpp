
#include <FS.h>
#include <SPIFFS.h>
#include <SD.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include "DDH.h"

bool DDH__UseSDCard = true;
bool DDH__forceDownload = false;
String DDH__ImageServer("");
String DDH__OTAHostName("");
FS *DDH__FSHandler = NULL;
void DDH_Init(String paramImgServer, bool paramForceDownload, bool paramUseSD, uint32_t checkSWVersion, String paramOTAHostName)
{
    DDH__UseSDCard = paramUseSD;
    DDH__forceDownload = paramForceDownload;
    DDH__ImageServer = paramImgServer;
    DDH__OTAHostName = paramOTAHostName;
    ArduinoOTA.onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
            type = "sketch";
        else // U_SPIFFS
            type = "filesystem";
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
        Serial.println("DDH INF Start updating " + type);
    });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("DDH INF Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("DD ERR Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    Serial.printf("DDH INF Hostname:%s\r\n", DDH__OTAHostName.c_str());
    if (!DDH__UseSDCard)
    {
        DDH__InitalizeSPIFileSystem(DDH__forceDownload);
        DDH__FSHandler = &SPIFFS;
    }
    else
    {
        DDH__FSHandler = &SD;
    }

    ArduinoOTA.setHostname(DDH__OTAHostName.c_str());
    ArduinoOTA.begin();
}

bool DDH__InitalizeSPIFileSystem(bool forceFormat)
{
    bool initok = false;
    initok = SPIFFS.begin();
    if ((!initok)||forceFormat) // Format SPIFS, of not formatted or format requested. - Try 1
        {
            Serial.println("DDH ERR SPIFFS Dateisystem wird formatiert.");
            
            SPIFFS.format();
            ESP.restart();
            
        }
   
    if (initok)
    {
        Serial.println("DDH INF SPIFFS ist  OK");
    }
    else
    {
        Serial.println("DDH ERR SPIFFS ist nicht OK");
    }
    return initok;
}
/**
 * Function checks if image is available and updtodate
 * if not it will download the image file from imgServer
 ***/
bool DDH_CheckImage(String path)
{
    bool retVal = true;
    if ((!DDH__FSHandler->exists(path)) || (DDH__forceDownload))
    {
        if (DDH__FSHandler->exists(path))
        {
            DDH__FSHandler->remove(path);
        }
        //M5.Lcd.print("Downloading Image\r\n");
        Serial.printf("GUI INF File: %s not found, downloading from %s", path.c_str(), DDH__ImageServer.c_str());
        File file = DDH__FSHandler->open(path, "a");
        HTTPClient http;

        http.begin(DDH__ImageServer + path); //Specify the URL and certificate
        int httpCode = http.GET();           //Make the request

        if (httpCode == HTTP_CODE_OK)
        { //Check for the returning code
            http.writeToStream(&file);
        }
        else
        {
            retVal = false;
            Serial.printf("GUI ERR File: %s failed to download", path.c_str());
            //M5.Lcd.print("Downloading Image failed\r\n");
        }

        file.close();
        if(retVal==false)
        {
            DDH__FSHandler->remove(path);
        }
        Serial.printf("GUI INF File: %s downloaded and stored", path.c_str());
        http.end(); //Free the resources
    }
}

void DDH__Loop()
{
    ArduinoOTA.handle();
    yield();
}
