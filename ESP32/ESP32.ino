/*
   Dummy Distributed File System implementation over TCP on ULP microcontroller.
   Had a lot of fun programming this, basic operations implemented, zero security
*/
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true
////////////////////////////////
// Your WiFi Credentials here //
////////////////////////////////
#define WIFI_SSID ""
#define WIFI_PWD  ""
////////////////////////////////////
// Your DFServer Credentials here //
////////////////////////////////////
#define DFS_SERVER "192.168.1.99"
#define DFS_PORT 8888
////////////////////////////////////
WiFiClient m_Client;
String m_strFilePrefix = "/";
String m_strFilePostfix = ".dfs";
uint32_t uiLastPing = 0;
////////////////////////////////////
String GetMD5(const String& rstrFile)
{
  File fFile = SPIFFS.open(rstrFile, "r");
  if (!fFile)return String();

  if (fFile.seek(0, SeekSet)) {
    MD5Builder md5;
    md5.begin();
    md5.addStream(fFile, fFile.size());
    md5.calculate();
    return md5.toString();
  }
  return String();
}
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.print(file.size());
      Serial.print("\tMD5:");
      String strChk = "/" + String(file.name());
      Serial.println(GetMD5(strChk));
    }
    file = root.openNextFile();
  }
}
///////////////////////////////////
// Sketch setup                  //
///////////////////////////////////
void setup()
{
  //
  Serial.begin(115200);
  Serial.println("Mounting FS...");
  while (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
    SPIFFS.format();
    Serial.println("Failed to mount file system");
    delay(1000);
  }

  // Carte Blanche on every start up
  Serial.println("Formatting flash");
  SPIFFS.format();

  // Debug
  listDir(SPIFFS, "/", 0);

  //
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  // Give it 10 seconds to connect, otherwise rebootW
  uint8_t iRetries = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(".");
    delay(1000);
    iRetries += 1;

    if (iRetries >= 10)
      ESP.restart();
  }

  Serial.println("Connected " + WiFi.localIP().toString());

  m_Client.stop();
  m_Client.flush();

}
void Send(const String& rstrData)
{
  Serial.println(">" + rstrData);
  m_Client.write(rstrData.c_str(), rstrData.length());
}
///////////////////////////////////
// Sketch loop                   //
///////////////////////////////////
void loop()
{
  // Check WiFi status
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[ERROR] WiFi disconnected, rebooting");
  }

  // Check if server connection up
  if (m_Client.connected() == false)
  {
    Serial.println("[ERROR] Server disconnected, reconnecting");
    if (m_Client.connect(DFS_SERVER, DFS_PORT))
    {
      Serial.println("[DFS] DFS Server connected");
    }
    else delay(1000);
  }
  else
  {
    if (m_Client.available())
    {
      String strJobType = m_Client.readStringUntil(',');
      Serial.println("[DFS] Command " + strJobType);

      if (strJobType == "WRITE")
      {
        String strFileName =  m_strFilePrefix + m_Client.readStringUntil(',') + m_strFilePostfix;
        String strFileData = m_Client.readStringUntil('\n');
        Serial.println("[DFS] File write request '" + strFileName + "'");
        File fFile = SPIFFS.open(strFileName, "w");
        if (!fFile)
          Send("WRITE,FAILED\n");
        else
        {
          fFile.print(strFileData);
          fFile.close();
          Send("WRITE,OK\n");
        }
      }

      else if (strJobType == "APPEND")
      {
        String strFileName = m_strFilePrefix + m_Client.readStringUntil(',') + m_strFilePostfix;
        String strFileData = m_Client.readStringUntil('\n');
        Serial.println("[DFS] File append request '" + strFileName + "'");
        File fFile = SPIFFS.open(strFileName, "a");
        if (!fFile)
          Send("APPEND,FAILED\n");
        else
        {
          fFile.print(strFileData);
          fFile.close();
          Send("APPEND,OK\n");
        }
      }
      else if (strJobType == "SIZE")
      {
        String strFileName = m_strFilePrefix + m_Client.readStringUntil('\n') + m_strFilePostfix;
        Serial.println("[DFS] File size request '" + strFileName + "'");
        File fFile = SPIFFS.open(strFileName, "r");
        Serial.println(fFile.size());
        if (!fFile)
          Send("SIZE,FAILED\n");
        else
        {
          Send("SIZE,OK," + String(fFile.size()) + "\n");
          fFile.close();
        }
      }
      else if (strJobType == "DELETE")
      {
        String strFileName = m_strFilePrefix + m_Client.readStringUntil('\n') + m_strFilePostfix;
        Serial.println("[DFS] File delete request '" + strFileName + "'");
        SPIFFS.remove(strFileName);
        Send("DELETE,OK\n");
      }
      else if (strJobType == "MD5")
      {
        String strFileName = m_strFilePrefix + m_Client.readStringUntil('\n') + m_strFilePostfix;
        Serial.println("[DFS] File MD5 request '" + strFileName + "'");
        Send("MD5," + GetMD5(strFileName) + "\n");
      }
      else if (strJobType == "FORMAT")
      {
        Serial.println("[DFS] Format requested");
        bool formatted = SPIFFS.format();
        Send("FORMAT,OK\n");
      }
      else if (strJobType == "READ")
      {
        String strFileName = m_strFilePrefix + m_Client.readStringUntil('\n') + m_strFilePostfix;
        Serial.println("[DFS] File read request '" + strFileName + "'");
        File fFile = SPIFFS.open(strFileName);
        if (!fFile)
          Send("READ,FAILED\n");
        else
        {
          // Read the file until end
          String strData = "";
          while (fFile.available())
          {
            strData += String((char)fFile.read());
          }
          Send("READ,OK," + strData + "\n");
          fFile.close();
        }
      }
      else if (strJobType == "READB")
      {
        // All checks - server side
        int iLocation       = m_Client.readStringUntil(',').toInt();
        int iSize           = m_Client.readStringUntil(',').toInt();
        String strFileName  = m_strFilePrefix + m_Client.readStringUntil('\n') + m_strFilePostfix;

        Serial.println("[DFS] File readb request '" + strFileName + "' at " + String(iLocation) + " of " + String(iSize) + " bytes");

        File fFile = SPIFFS.open(strFileName);
        if (!fFile)
        {
          Send("READ,FAILED\n");
          return;
        }


        fFile.seek(iLocation, SeekSet);
        byte bufferInfo[iSize];
        uint32_t nBytes = fFile.readBytes((char*)bufferInfo, iSize);
        fFile.close();
        for (int z = 0; z < nBytes; z++)
        {
          printf("%02X ", bufferInfo[z]);
        }
        printf("\n");
        byte fullBuffer[iSize + 8];
        memcpy(fullBuffer, "READ,OK,", 8);
        memcpy(fullBuffer + 8, bufferInfo, nBytes);
        m_Client.write(fullBuffer, sizeof(fullBuffer));
      }




      else if (strJobType == "PING")
      {

      }
      //listDir(SPIFFS, "/", 0);
    }
  }
}
